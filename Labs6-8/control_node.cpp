#include <unistd.h>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <zmq.hpp>
#include "my_zmq.h"
#include "topology.h"
 
std::vector<long long> ping_storage(0);
topology_t<long long> control_node; /*хранение и управление топологией сети*/
std::vector<std::pair<void*, void*>> children; /*для хранения контекстов и сокетов для дочерних процессов*/
 
void pinger(int wait) { /*пингер, он же heart beat time*/
    while (true) {
        for (size_t i = 0; i < ping_storage.size();i++) {
            int value = ping_storage[i];
            int ind = control_node.find(value);
            auto* token = new node_token_t({ ping, value, value });
            node_token_t reply({ fail, value, value });
            if (ind != -1 and my_zmq::send_receive_wait(token, reply, children[ind].second) and reply.action == success) {
                std::cout << "OK" << std::endl;
                continue;
            }
            else {
                std::cout << "Heartbit: node " << value << " is unavailable now" << std::endl;
                auto iterator = std::find(ping_storage.begin(), ping_storage.end(), value);
                if (iterator != ping_storage.end()) {
                    ping_storage.erase(iterator);
                }
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(wait));
    }
}
 
void delete_control_node(long long id) { /*удаляем узел из сети*/
    int ind = control_node.find(id);
    int rc;
    bool ok;
    if (ind != -1) {
        auto* token = new node_token_t({ destroy, id, id });
        node_token_t reply({ fail, id, id });
        ok = my_zmq::send_receive_wait(token, reply, children[ind].second);
        if (reply.action == destroy and reply.parent_id == id) {
            rc = zmq_close(children[ind].second);
            assert(rc == 0);
            rc = zmq_ctx_destroy(children[ind].first);
            assert(rc == 0);
            auto it = children.begin();
            while (ind--) {
                ++it;
            }
            children.erase(it);
        }
        else if (reply.action == bind and reply.parent_id == id) { /*происходит перезапуск сокета, который ранее использовался для связи с 
        соответствующим потомком*/
            rc = zmq_close(children[ind].second);
            assert(rc == 0);
            rc = zmq_ctx_term(children[ind].first);
            assert(rc == 0);
            my_zmq::init_pair_socket(children[ind].first, children[ind].second);
            rc = zmq_bind(children[ind].second, ("tcp://*:" + std::to_string(PORT_BASE + id)).c_str());
            assert(rc == 0);
        }
        if (ok) {
            control_node.erase(id);
            std::cout << "OK: " << id << std::endl;
        }
        else {
            std::cout << "Error: Node " << id << " is unavailable" << std::endl;
        }
    }
    else {
        std::cout << "Error: Not found" << std::endl;
    }
}
 
int main() { 
    int rc;
    bool ok;
    std::string s;
    std::thread new_thread;
    long long id;
    std::cout << "Create id parent: create calculation node (use parent = -1 if parent is control node)" << std::endl;
    std::cout << "Heartbeat milliseconds: ping calculation node with id $id" << std::endl;
    std::cout << "Remove id: delete calculation node with id " << std::endl;
    std::cout << "Exec id key val: add [key, val] add local dictionary" << std::endl;
    std::cout << "Exec id key: check local dictionary" << std::endl;
    while (std::cin >> s >> id) {
        if (s == "create") {
            long long parent_id;
            std::cin >> parent_id;
            int ind;
            if (parent_id == -1) {
                void* new_context = nullptr;
                void* new_socket = nullptr;
                my_zmq::init_pair_socket(new_context, new_socket);
                rc = zmq_bind(new_socket, ("tcp://*:" + std::to_string(PORT_BASE + id)).c_str());
                assert(rc == 0);
 
                int fork_id = fork(); /*создание новой вычислительной ноды*/
                if (fork_id == 0) {
                    rc = execl(NODE_EXECUTABLE_NAME, NODE_EXECUTABLE_NAME, std::to_string(id).c_str(), nullptr);
                    assert(rc != -1);
                    return 0;
                }
                else {
                    auto* token = new node_token_t({ ping, id, id });
                    node_token_t reply({ fail, id, id });
                    if (my_zmq::send_receive_wait(token, reply, new_socket) and reply.action == success) {//проверка создания нового сокета(выч ноды)
                        children.emplace_back(std::make_pair(new_context, new_socket));//добавляем в вектор новый сокет ребёнка тип н дентей у контрол ноды
                        control_node.insert(id);//вставляем ид в топологию
                    }
                    else {
                        rc = zmq_close(new_socket);
                        assert(rc == 0);
                        rc = zmq_ctx_destroy(new_context);
                        assert(rc == 0);
                    }
                }
                ping_storage.push_back(id);
            }
            else if ((ind = control_node.find(parent_id)) == -1) { /*обработке команды "create", которая используется для 
            создания вычислительной ноды в топологии*/
                std::cout << "Error: Not found" << std::endl;
                continue;
            }
            else {
                if (control_node.find(id) != -1) {
                    std::cout << "Error: Already exists" << std::endl;
                    continue;
                }
                auto* token = new node_token_t({ create, parent_id, id });
                node_token_t reply({ fail, id, id });
                if (my_zmq::send_receive_wait(token, reply, children[ind].second) and reply.action == success) {
                    control_node.insert(parent_id, id);
                    ping_storage.push_back(id);
                }
                else {
                    std::cout << "Error: Parent is unavailable" << std::endl;
                    continue;
                }
            }
        }


        else if (s == "remove") {
            delete_control_node(id);
        }

        
        else if (s == "heartbeat") {
            if (ping_storage.empty()) {
                std::cout << "Error: there are no calculation nodes at all" << std::endl;
                continue;
            }
            new_thread = std::thread(pinger, id);
        }

        
        else if (s == "exec") { /*для команды exec*/
            ok = true;
            std::string key;
            char c;
            int val = -1;
            bool add = false;
            std::cin >> key;
            if ((c = getchar()) == ' ') {
                add = true;
                std::cin >> val;
            }
            int ind = control_node.find(id);
            if (ind == -1) {
                std::cout << "Error: Not found" << std::endl;
                continue;
            }
            key += SENTINEL;

            if (add) { /*добавление пары [key, val] в локальный словарь вычислительной ноды с заданным id*/
                for (auto i : key) {
                    auto* token = new node_token_t({ exec_add, i, id });
                    node_token_t reply({ fail, id, id });
                    if (!my_zmq::send_receive_wait(token, reply, children[ind].second) or reply.action != success) {
                        std::cout << "Fail: " << i << std::endl;
                        ok = false;
                        break;
                    }
                }
                auto* token = new node_token_t({ exec_add, val, id });
                node_token_t reply({ fail, id, id });
                if (!my_zmq::send_receive_wait(token, reply, children[ind].second) or reply.action != success) {
                    std::cout << "Fail: " << val << std::endl;
                    ok = false;
                }
            }
            else { /*обработка ключей*/
                for (auto i : key) {
                    auto* token = new node_token_t({ exec_check, i, id });
                    node_token_t reply({ fail, i, id });
                    if (!my_zmq::send_receive_wait(token, reply, children[ind].second) or reply.action != success) {
                        ok = false;
                        std::cout << "Fail: " << i << std::endl;
                        break;
                    }
                }
            }
            if (!ok) {
                std::cout << "Error: Node is unavailable" << std::endl;
            }
        }
    }
    new_thread.detach();
    return 0;
}
 