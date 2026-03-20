#pragma once

#include <vector>

#include "kazennova_a_fox_algorithm/common/include/common.hpp"
#include "task/include/task.hpp"

namespace kazennova_a_fox_algorithm {

class KazennovaATestTaskOMP : public BaseTask {
 public:
  static constexpr ppc::task::TypeOfTask GetStaticTypeOfTask() {
    return ppc::task::TypeOfTask::kOMP;
  }
  explicit KazennovaATestTaskOMP(const InType &in);

 private:
  bool ValidationImpl() override;
  bool PreProcessingImpl() override;
  bool RunImpl() override;
  bool PostProcessingImpl() override;

  void DecomposeMatrix(const std::vector<double>& src,
                       std::vector<double>& dst,
                       int n, int bs, int q);
  void AssembleMatrix(const std::vector<double>& src,
                      std::vector<double>& dst,
                      int n, int bs, int q);
  void MultiplyBlock(int a_idx, int b_idx, int c_idx, int bs);

  int matrix_size_{0};
  int block_size_{0};
  int block_count_{0};
  std::vector<double> a_blocks_;
  std::vector<double> b_blocks_;
  std::vector<double> c_blocks_;
};

}  // namespace kazennova_a_fox_algorithm