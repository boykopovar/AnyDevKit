#include "Checks.hpp"
#include <algorithm>
#include <array>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <span>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

void CheckContainers() {
    std::vector<int> values;
    for (int i = 0; i < 4096; ++i) values.push_back(i);
    auto copy = values;
    values.erase(values.begin(), values.begin() + 1024);
    Require(values.front() == 1024 && copy.size() == 4096, "Vector ownership");
    ExpectException<std::out_of_range>([&] { static_cast<void>(values.at(values.size())); });
    std::deque<int> deque(values.begin(), values.end());
    deque.push_front(-1); deque.push_back(4096);
    Require(deque.front() == -1 && deque.back() == 4096, "Deque growth");
    std::list<int> list{3, 1, 2, 2}; list.sort(); list.unique();
    std::list<int> other{4, 5}; list.splice(list.end(), other);
    Require(other.empty() && list.size() == 5 && list.back() == 5, "List splice");
    std::forward_list<int> forward{3, 1, 2}; forward.sort(); forward.reverse();
    Require(forward.front() == 3, "Forward list");
    std::map<int, int> map; std::unordered_map<int, int> unordered;
    for (int i = 0; i < 1024; ++i) { map.emplace(i, i * 2); unordered.emplace(i, i * 2); }
    unordered.rehash(4096);
    for (const auto& [key, value] : map) Require(unordered.at(key) == value, "Map rehash");
    auto node = map.extract(1); node.key() = 2048; map.insert(std::move(node));
    Require(map.at(2048) == 2 && !map.contains(1), "Map node ownership");
    std::multimap<int, int> multi{{1, 2}, {1, 3}};
    std::unordered_multimap<int, int> hashMulti{{1, 2}, {1, 3}};
    std::set<int> set{1, 1, 2}; std::multiset<int> multiset{1, 1, 2};
    std::unordered_set<int> hashSet{1, 1, 2}; std::unordered_multiset<int> hashMultiset{1, 1, 2};
    Require(multi.count(1) == 2 && hashMulti.count(1) == 2 && set.size() == 2 && multiset.count(1) == 2 && hashSet.size() == 2 && hashMultiset.count(1) == 2, "Associative multiplicity");
    std::stack<int> stack; std::queue<int> queue; std::priority_queue<int> priority;
    for (int value : {2, 1, 3}) { stack.push(value); queue.push(value); priority.push(value); }
    Require(stack.top() == 3 && queue.front() == 2 && priority.top() == 3, "Container adaptors");
    std::array<int, 3> array{1, 2, 3}; std::span<int> span(array); span[1] = 7;
    Require(array[1] == 7, "Span storage");
    std::vector<bool> bits(100, false); bits[65] = true;
    Require(bits[65] && !bits[64], "Vector bool");
}
