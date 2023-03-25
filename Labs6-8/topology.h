#ifndef INC_6_8_LAB__TOPOLOGY_H_
#define INC_6_8_LAB__TOPOLOGY_H_

#include <iostream>
#include <list>
#include <map>

template<typename T>
class topology_t { /*шаблон*/
 public:
  using list_type = std::list<std::list<T>>;
  using iterator = typename std::list<T>::iterator;
  using list_iterator = typename list_type::iterator;

  list_type container;
  size_t container_size;
  topology_t() : container(), container_size(0){}; /*конструкторк класс topology_t*/
  ~topology_t() = default;

  void insert(const T &elem) { /*добавляет элемент в структуру topology_t*/
	std::list<T> new_list;
	new_list.emplace_back(elem);
	++container_size;
	container.emplace_back(new_list);
  }

  bool insert(const T &parent, const T &elem) { /*Функция insert принимает на вход два аргумента: parent и elem. 
  Она ищет элемент parent в контейнере и добавляет элемент elem после него в ту же подсписок, если parent был найден. 
  Если parent не найден, функция возвращает false, иначе - true*/
	for (list_iterator external_it = container.begin(); external_it != container.end(); ++external_it) {
	  for (iterator internal_it = external_it->begin(); internal_it != external_it->end(); ++internal_it) {
		if (*internal_it == parent) {
		  external_it->insert(++internal_it, elem);
		  ++container_size;
		  return true;
		}
	  }
	}
	return false;
  }

  bool erase(const T &elem) { /*функция удаляет элемент типа T из контейнера container*/
	for (list_iterator external_it = container.begin(); external_it != container.end(); ++external_it) {
	  for (iterator internal_it = external_it->begin(); internal_it != external_it->end(); ++internal_it) {
		if (*internal_it == elem) {
		  if (external_it->size() > 1) {
			external_it->erase(internal_it);
		  } else {
			container.erase(external_it);
		  }
		  --container_size;
		  return true;
		}
	  }
	}
	return false;
  }

  size_t size() {
	return container_size;
  }
  int find(const T &elem) {// в каком списке существует (или нет) элемент с идентификатором $id
	int ind = 0;
	for (auto &external : container) {
	  for (auto &internal : external) {
		if (internal == elem) {
		  return ind;
		}
	  }
	  ++ind;
	}
	return -1;
  }
  
  template<typename S> /*оператор вывода в поток*/
  friend std::ostream &operator<<(std::ostream &os, const topology_t<S> &topology) {
	for (auto &external : topology.container) {
	  os << "{";
	  for (auto &internal : external) {
		os << internal << " ";
	  }
	  os << "}" << std::endl;
	}
	return os;
  }
};

#endif//INC_6_8_LAB__TOPOLOGY_H_