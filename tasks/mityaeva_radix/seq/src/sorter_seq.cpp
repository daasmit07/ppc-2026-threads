#include "mityaeva_radix/seq/include/sorter_seq.hpp"

#include <cstddef>
#include <vector>

namespace mityaeva_radix {
void SorterSeq::CountingSort(std::vector<double> &inp, std::vector<double> &out, int byte) {
  auto *mas = reinterpret_cast<unsigned char *>(inp.data());
  std::vector<int> counter(256, 0);
  auto size = inp.size();

  // Подсчет количества элементов
  for (std::size_t i = 0; i < size; i++) {
    counter[mas[(8 * i) + byte]]++;
  }

  // Поиск первого ненулевого элемента
  int j = 0;
  for (; j < 256; j++) {
    if (counter[j] != 0) {
      break;
    }
  }

  // Преобразование счетчиков в позиции
  int tem = counter[j];
  counter[j] = 0;
  j++;

  for (; j < 256; j++) {
    int b = counter[j];
    counter[j] = tem;
    tem += b;
  }

  // Сортировка
  for (std::size_t i = 0; i < size; i++) {
    out[counter[mas[(8 * i) + byte]]] = inp[i];
    counter[mas[(8 * i) + byte]]++;
  }
}

void SorterSeq::LSDSortDouble(std::vector<double> &inp) {
  auto size = inp.size();
  std::vector<double> out(size);
  CountingSort(inp, out, 0);
  CountingSort(out, inp, 1);
  CountingSort(inp, out, 2);
  CountingSort(out, inp, 3);
  CountingSort(inp, out, 4);
  CountingSort(out, inp, 5);
  CountingSort(inp, out, 6);
  CountingSort(out, inp, 7);
};

}  // namespace mityaeva_radix
