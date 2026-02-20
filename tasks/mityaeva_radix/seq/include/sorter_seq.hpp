#pragma once

#include <vector>

namespace mityaeva_radix {
class SorterSeq {
 public:
  static void CountingSort(std::vector<double> &inp, std::vector<double> &out, int byte);
  static void LSDSortDouble(std::vector<double> &inp);
};
}  // namespace mityaeva_radix
