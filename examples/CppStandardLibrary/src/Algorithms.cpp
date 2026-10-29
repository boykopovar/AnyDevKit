#include "Checks.hpp"
#include <algorithm>
#include <array>
#include <iterator>
#include <numeric>
#include <ranges>
#include <vector>

void CheckAlgorithms() {
    std::vector<int> values{5, 1, 3, 2, 4};
    std::ranges::sort(values);
    Require(std::ranges::is_sorted(values) && std::binary_search(values.begin(), values.end(), 3), "Sort and search");
    auto view = values | std::views::filter([](int x) { return x % 2 != 0; }) | std::views::transform([](int x) { return x * x; });
    Require(std::accumulate(view.begin(), view.end(), 0) == 35, "Range adaptors");
    std::array<int, 5> sums{}; std::partial_sum(values.begin(), values.end(), sums.begin());
    Require(sums.back() == 15 && std::reduce(values.begin(), values.end()) == 15, "Numeric algorithms");
    std::vector<int> copied; std::ranges::copy(values, std::back_inserter(copied));
    std::ranges::reverse(copied);
    Require(std::ranges::equal(values, copied | std::views::reverse), "Iterator adaptors");
    std::make_heap(copied.begin(), copied.end()); Require(copied.front() == 5, "Heap algorithms");
    std::sort_heap(copied.begin(), copied.end()); Require(copied == values, "Heap sort");
    Require(std::inner_product(values.begin(), values.end(), values.begin(), 0) == 55, "Inner product");
    std::array<int, 3> permutation{1, 2, 3}; unsigned count = 1;
    while (std::next_permutation(permutation.begin(), permutation.end())) ++count;
    Require(count == 6, "Permutations");
}
