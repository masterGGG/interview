#include <vector>
#include <queue>
#include <iostream>
#include <unordered_map>

template <typename T>
struct Cmp {
  bool operator()(T a, T b) {
    return a < b;
  }
};

//g++ comp.cpp -o xxx -std=c++11
int main() {
  std::priority_queue<int, std::vector<int>, Cmp<int>> max_heap;

  max_heap.emplace(4);
  max_heap.emplace(3);
  max_heap.emplace(7);
  max_heap.emplace(1);
  max_heap.emplace(6);

  while (!max_heap.empty()) {
    std::cout << "Top : " << max_heap.top() << std::endl;
    max_heap.pop(); 
  }

  std::unordered_map<int, int> _map;

  _map.emplace(1, 11);
  _map.emplace(2, 19);

  if (_map.count(1)) {
    std::cout << "1 exist" << std::endl;
  }

  return 0;
}