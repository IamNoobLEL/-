#include "my_zmq.h"
#include <iostream>
#include <map>
#include <unistd.h>
 
long long node_id; /*считываем идентификатор узла*/
 
int main(int argc, char** argv) {
    std::string key;
    int val;
    std::map<std::string, int> dict; /*создаем пустой словарь для хранения пар ключ-значение типа std::string-int*/
    int rc;
    assert(argc == 2); 
    node_id = std::stoll(std::string(argv[1]));/*преобраз с помощью std::stoll строчный индентифик в целое число с плавающей т */
 
    /*создается ZeroMQ сокет для соединения с родительским процессом в дереве процессов. 
    Создается контекст ZeroMQ и парный сокет, который подключается к адресу "tcp://localhost:PORT_BASE + node_id". 
    PORT_BASE - это константа, задающая базовый порт для соединений. Значение node_id добавляется к порту, чтобы создать 
    уникальный адрес для каждого процесса в дереве*/
    void* node_parent_context = zmq_ctx_new();
    void* node_parent_socket = zmq_socket(node_parent_context, ZMQ_PAIR);
    rc = zmq_connect(node_parent_socket, ("tcp://localhost:" + std::to_string(PORT_BASE + node_id)).c_str());
    assert(rc == 0);
 
    long long child_id = -1; /*инициализируем переменные*/
    void* node_context = nullptr;
    void* node_socket = nullptr;
    std::cout << "OK: " << getpid() << std::endl; /*выводим ОК с номером процесса*/
 
    /*Внутри цикла происходит прием сообщения через ZeroMQ-сокет node_parent_socket и создание нового экземпляра node_token_t со 
    значением { fail, node_id, node_id }. После этого происходит проверка условия, которое возвращает значение true, если на 
    данном узле есть дочерние узлы, значение true, если узел был разбужен, и значение false, если никаких новых дочерних узлов 
    не было добавлено. Если условие истинно, происходит переход на следующую итерацию цикла. Если условие ложно, то выполняется 
    код внутри блока else*/
    bool has_child = false, awake = true, add = false;
    while (awake) {
        node_token_t token({ fail, 0, 0 });
        my_zmq::receive_msg(token, node_parent_socket);
        auto* reply = new node_token_t({ fail, node_id, node_id });
 

        if (token.action == bind and token.parent_id == node_id) { /*проверяем пришло ли сообщение от род процесса, содержащее
        запрос на связывание с дочерним процессом*/
            my_zmq::init_pair_socket(node_context, node_socket);
            rc = zmq_bind(node_socket, ("tcp://*:" + std::to_string(PORT_BASE + token.id)).c_str());
            assert(rc == 0);
            has_child = true;
            child_id = token.id;
            auto* token_ping = new node_token_t({ ping, child_id, child_id });
            node_token_t reply_ping({ fail, child_id, child_id });
            if (my_zmq::send_receive_wait(token_ping, reply_ping, node_socket) and reply_ping.action == success) {
                reply->action = success;
            }
        }


        else if (token.action == create) { /*обрабатываем сообщение, которое указывает на необходимость создания нового
        узла в дереве*/
            if (token.parent_id == node_id) {
                if (has_child) {
                    rc = zmq_close(node_socket);
                    assert(rc == 0);
                    rc = zmq_ctx_term(node_context);
                    assert(rc == 0);
                }
                my_zmq::init_pair_socket(node_context, node_socket);
                rc = zmq_bind(node_socket, ("tcp://*:" + std::to_string(PORT_BASE + token.id)).c_str());
                assert(rc == 0);
                int fork_id = fork();
                if (fork_id == 0) {
                    rc = execl(NODE_EXECUTABLE_NAME, NODE_EXECUTABLE_NAME, std::to_string(token.id).c_str(), nullptr);
                    assert(rc != -1);
                    return 0;
                }
                else { /*проверяем есть ли дочерний процесс*/
                    bool ok = true;
                    if (has_child) {
                        auto* token_bind = new node_token_t({ bind, token.id, child_id });
                        node_token_t reply_bind({ fail, token.id, token.id });
                        ok = my_zmq::send_receive_wait(token_bind, reply_bind, node_socket);
                        ok = ok and (reply_bind.action == success);
                    }
                    if (ok) { /*есть ли у текущего узла (node) уже ребенок, и если есть, то отправляет ему запрос на связывание 
                    с новым узлом, указанным в токене*/
                        auto* token_ping = new node_token_t({ ping, token.id, token.id });
                        node_token_t reply_ping({ fail, token.id, token.id });
                        ok = my_zmq::send_receive_wait(token_ping, reply_ping, node_socket);
                        ok = ok and (reply_ping.action == success);
                        if (ok) {
                            reply->action = success;
                            child_id = token.id;
                            has_child = true;
                        }
                        else { /*если не удалось связать узел-родитель с новым узлом-потомком или не удалось установить соединение 
                        с новым узлом-потомком. Текущий узел закрывает свой сокет и завершает свою работу, уничтожая свой контекст 
                        ZeroMQ*/
                            rc = zmq_close(node_socket);
                            assert(rc == 0);
                            rc = zmq_ctx_term(node_context);
                            assert(rc == 0);
                        }
                    }
                }
            }
            else if (has_child) { /*передаем сообщение от текущего узла родительскому. Если передача произошла успешно,
            то ответ может содержать какую-то полезную информацию для дальнейшей обработки*/
                auto* token_down = new node_token_t(token);
                node_token_t reply_down(token);
                reply_down.action = fail;
                if (my_zmq::send_receive_wait(token_down, reply_down, node_socket) and reply_down.action == success) {
                    *reply = reply_down;
                }
            }
        }


        else if (token.action == ping) { /*обрабатываем сообщения типа ping*/
            if (token.id == node_id) {
                reply->action = success;
            }
            else if (has_child) {
                auto* token_down = new node_token_t(token);
                node_token_t reply_down(token);
                reply_down.action = fail;
                if (my_zmq::send_receive_wait(token_down, reply_down, node_socket) and reply_down.action == success) {
                    *reply = reply_down;
                }
            }
        }


        else if (token.action == destroy) { /*проверяем действие указанное в token. Если дестрой, то оффаем сокет, контекст.
        И убираем ребенка*/
            if (has_child) {
                if (token.id == child_id) {
                    bool ok;
                    auto* token_down = new node_token_t({ destroy, node_id, child_id });
                    node_token_t reply_down = { fail, child_id, child_id };
                    ok = my_zmq::send_receive_wait(token_down, reply_down, node_socket);
                    if (reply_down.action == destroy) {
                        rc = zmq_close(node_socket);
                        assert(rc == 0);
                        rc = zmq_ctx_destroy(node_context);
                        assert(rc == 0);
                        has_child = false;
                        child_id = -1;
                    }
                    else if (reply_down.action == bind) { /*Если в ответе bind, то текущий узел переустанавливает связь с 
                    новым портом, который указан в ответе от дочернего узла. Далее происходит проверка, что связь с дочерним 
                    узлом установлена успешно*/
                        rc = zmq_close(node_socket);
                        assert(rc == 0);
                        rc = zmq_ctx_destroy(node_context);
                        assert(rc == 0);
                        my_zmq::init_pair_socket(node_context, node_socket);
                        rc = zmq_bind(node_socket, ("tcp://*:" + std::to_string(PORT_BASE + reply_down.id)).c_str());
                        assert(rc == 0);
                        child_id = reply_down.id;
                        auto* token_ping = new node_token_t({ ping, child_id, child_id });
                        node_token_t reply_ping({ fail, child_id, child_id });
                        ok = my_zmq::send_receive_wait(token_ping, reply_ping, node_socket) and (reply_ping.action == success);
                    }
                    if (ok) {
                        reply->action = success;
                    }
                }
                else if (token.id == node_id) { /*проверяет, что действие, указанное в токене, равно "bind" и идентификатор 
                узла в токене равен идентификатору этого узла*/
                    rc = zmq_close(node_socket);
                    assert(rc == 0);
                    rc = zmq_ctx_destroy(node_context);
                    assert(rc == 0);
                    awake = false;
                    reply->action = bind;
                    reply->id = child_id;
                    reply->parent_id = token.parent_id;
                }
                else { /*если нет потомков или ID удаляемого узла не соответствует ID потомка, создаётся новый токен с 
                теми же полями, что и полученный токен, и отправляется потомку*/
                    auto* token_down = new node_token_t(token);
                    node_token_t reply_down = token;
                    reply_down.action = fail;
                    if (my_zmq::send_receive_wait(token_down, reply_down, node_socket) and (reply_down.action == success)) {
                        *reply = reply_down;
                    }
                }
            }
            else if (token.id == node_id) { /*к обработке сообщений типа destroy*/
                reply->action = destroy;
                awake = false;
            }
        }


        else if (token.action == exec_check) { /*обрабатывает запрос типа exec_check, который посылается узлу 
        для выполнения операции проверки наличия определенного ключа в его словаре dict*/
            if (token.id == node_id) {
                char c = token.parent_id;
                if (c == SENTINEL) {
                    if (dict.find(key) != dict.end()) {
                        std::cout << "OK:" << node_id << ":" << dict[key] << std::endl;
                    }
                    else {
                        std::cout << "OK:" << node_id << ":'" << key << "' not found" << std::endl;
                    }
                    reply->action = success;
                    key = "";
                }
                else {
                    key += c;
                    reply->action = success;
                }
            }
            else if (has_child) { /*если у текущего узла есть дочерний*/
                auto* token_down = new node_token_t(token);
                node_token_t reply_down(token);
                reply_down.action = fail;
                if (my_zmq::send_receive_wait(token_down, reply_down, node_socket) and reply_down.action == success) {
                    *reply = reply_down;
                }
            }
        }

        
        else if (token.action == exec_add) { /*действие токена является "exec_add", и если идентификатор токена 
        соответствует идентификатору узла*/
            if (token.id == node_id) {
                char c = token.parent_id;
                if (c == SENTINEL) {
                    add = true;
                    reply->action = success;
                }
                else if (add) {
                    val = token.parent_id;
                    dict[key] = val;
                    std::cout << "OK:" << node_id << std::endl;
                    add = false;
                    key = "";
                    reply->action = success;
                }
                else {
                    key += c;
                    reply->action = success;
                }
            }
            else if (has_child) { /*Если у узла есть потомок и поступил токен с действием, отличным от exec_add, exec_check, ping и 
            destroy, то создается новый токен, который отправляется потомку. После получения ответа от потомка, если действие в 
            ответе равно success, то ответ присваивается указателю reply*/
                auto* token_down = new node_token_t(token);
                node_token_t reply_down(token);
                reply_down.action = fail;
                if (my_zmq::send_receive_wait(token_down, reply_down, node_socket) and reply_down.action == success) {
                    *reply = reply_down;
                }
            }
        }
        /*Этот код закрывает сокет и контекст ZMQ для родительского узла после того, как все операции на текущем узле были 
        завершены и все ответы на запросы были отправлены родительскому узлу. Затем код удаляет текущий узел из глобального 
        словаря узлов и завершает функцию*/
        my_zmq::send_msg_no_wait(reply, node_parent_socket);
    }
    rc = zmq_close(node_parent_socket);
    assert(rc == 0);
    rc = zmq_ctx_destroy(node_parent_context);
    assert(rc == 0);
}