// #include "mityaeva_radix/omp/include/sorter_omp.hpp"

// #include <omp.h>

// #include <algorithm>
// #include <cstddef>
// #include <vector>

// namespace mityaeva_radix {
// void SorterOmp::CountingSortAsc(std::vector<double> &source, std::vector<double> &destination, int byte) {
//   auto *mas = reinterpret_cast<unsigned char *>(source.data());
//   std::vector<int> counter(256, 0);
//   auto size = source.size();
//   for (std::size_t i = 0; i < size; i++) {
//     counter[mas[(8 * i) + byte]]++;
//   }
//   int j = 0;
//   for (; j < 256; j++) {
//     if (counter[j] != 0) {
//       break;
//     }
//   }
//   int tem = counter[j];
//   counter[j] = 0;
//   j++;
//   for (; j < 256; j++) {
//     int b = counter[j];
//     counter[j] = tem;
//     tem += b;
//   }
//   for (std::size_t i = 0; i < size; i++) {
//     destination[counter[mas[(8 * i) + byte]]] = source[i];
//     counter[mas[(8 * i) + byte]]++;
//   }
// }

// void SorterOmp::CountingSortDesc(std::vector<double> &source, std::vector<double> &destination, int byte) {
//   auto *mas = reinterpret_cast<unsigned char *>(source.data());
//   std::vector<int> count(256, 0);
//   auto size = source.size();
//   for (std::size_t i = 0; i < size; i++) {
//     count[mas[(8 * i) + byte]]++;
//   }
//   int sum = 0;
//   std::vector<int> pos(256, 0);
//   for (int i = 255; i >= 0; i--) {
//     sum += count[i];
//     pos[i] = sum - count[i];
//   }
//   for (std::size_t i = 0; i < size; i++) {
//     int byte_val = mas[(8 * i) + byte];
//     destination[pos[byte_val]] = source[i];
//     pos[byte_val]++;
//   }
// }

// void SorterOmp::LSDSortDouble(std::vector<double> &inp) {
//   if (inp.size() <= 1) {
//     return;
//   }
//   auto count_negative = std::ranges::count_if(inp, [](auto x) { return x < 0; });
//   std::vector<double> negative;
//   negative.reserve(count_negative);
//   std::vector<double> positive;
//   positive.reserve(inp.size() - count_negative);
//   for (auto x : inp) {
//     if (x < 0) {
//       negative.push_back(x);
//     } else {
//       positive.push_back(x);
//     }
//   }
//   std::vector<double> out_n(negative.size());
//   std::vector<double> out_p(positive.size());
//   for (int i = 0; i < 8; i++) {
//     if (i % 2 == 0) {
//       CountingSortDesc(negative, out_n, i);
//       CountingSortAsc(positive, out_p, i);
//     } else {
//       CountingSortDesc(out_n, negative, i);
//       CountingSortAsc(out_p, positive, i);
//     }
//   }
//   inp.clear();
//   inp.insert(inp.begin(), negative.begin(), negative.end());
//   inp.insert(inp.end(), positive.begin(), positive.end());
// };

// void SorterOmp::sort(std::vector<double> &mas, std::vector<double> &tmp, int nThreads, int byteNum) {
//   int size = mas.size();

//   // Устанавливаем количество потоков
//   omp_set_num_threads(nThreads);

//   // Вектор счетчиков для каждого потока (2D вектор)
//   std::vector<std::vector<int>> counters(nThreads, std::vector<int>(256, 0));

// // Этап 1: Подсчет элементов по байтам
// #pragma omp parallel
//   {
//     int threadNum = omp_get_thread_num();
//     int threadSize = size / nThreads;
//     int start = threadNum * threadSize;
//     int end = (threadNum == nThreads - 1) ? size : start + threadSize;

//     // Получаем ссылку на счетчики текущего потока
//     auto &localCounters = counters[threadNum];

//     // Подсчет элементов
//     unsigned char *masUC = reinterpret_cast<unsigned char *>(mas.data());
//     for (int i = start; i < end; i++) {
//       localCounters[masUC[8 * i + byteNum]]++;
//     }
//   }

//   // Этап 2: Вычисление префиксных сумм
//   int totalSum = 0;
//   for (int j = 0; j < 256; j++) {
//     for (int i = 0; i < nThreads; i++) {
//       int current = counters[i][j];
//       counters[i][j] = totalSum;
//       totalSum += current;
//     }
//   }

//   // Этап 3: Размещение элементов в tmp
//   std::vector<int> threadOffsets(nThreads, 0);

// #pragma omp parallel
//   {
//     int threadNum = omp_get_thread_num();
//     int threadSize = size / nThreads;
//     int start = threadNum * threadSize;
//     int end = (threadNum == nThreads - 1) ? size : start + threadSize;

//     auto &localCounters = counters[threadNum];
//     unsigned char *masUC = reinterpret_cast<unsigned char *>(mas.data());

//     for (int i = start; i < end; i++) {
//       int byteValue = masUC[8 * i + byteNum];
//       int position = localCounters[byteValue] + threadOffsets[threadNum];
//       tmp[position] = mas[i];
//       threadOffsets[threadNum]++;
//     }
//   }

//   // Этап 4: Копирование обратно в mas
//   mas = tmp;
// }

// }  // namespace mityaeva_radix
