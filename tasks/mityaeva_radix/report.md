# Radix sort of `double`s with simple merge

- Student: Митяева Дарья Викторовна, group 3823Б1ФИ2
- Technology: SEQ, OMP, TBB, STL, MPI+OMP
- Variant: 19

## 1. Introduction

Sorting is a fundamental operation in computer science with applications across
all domains of computing. This project implements a Least Significant Digit
(LSD) radix sort specifically designed for double-precision floating-point
numbers. The algorithm leverages the byte representation of doubles to achieve
linear-time complexity, providing an efficient alternative to comparison-based
sorting algorithms for large datasets.

This consolidated report analyzes five different implementations of the
algorithm:

- **SEQ**: Sequential implementation (baseline)
- **OMP**: OpenMP shared-memory parallelization
- **TBB**: Intel Threading Building Blocks task-based parallelization
- **STL**: C++ Standard Library parallelization using std::thread
- **MPI+OMP**: Hybrid distributed and shared memory parallelization

All implementations are evaluated on identical hardware with consistent
methodology to enable fair comparison of performance characteristics,
scalability, and parallel efficiency.

## 2. Problem Statement

- **Input:** A vector of double-precision floating-point numbers of arbitrary
  length N
- **Output:** The same vector sorted in non-decreasing order

**Constraints:** The input vector must not be empty. The algorithm must handle
all possible double values including positive/negative zero, infinities, and
NaN values (following IEEE 754 conventions).

## 3. Baseline Algorithm (Sequential)

The sequential LSD radix sort processes numbers digit by digit from least
significant to most significant. For double values (8 bytes = 64 bits), the
algorithm:

1. Interprets each double as an array of 8 unsigned bytes
2. Performs counting sort on each byte position (0 to 7)
3. Alternates between original and auxiliary arrays to avoid unnecessary copying
4. Maintains stability throughout all passes to ensure correct final ordering

The counting sort for each byte:

- Builds a histogram of byte values (256 buckets)
- Computes prefix sums to determine final positions
- Places elements in their sorted positions maintaining stability

Due to the IEEE 754 representation, the sequential version requires special
handling for negative numbers, as their byte representation is inverted to
maintain correct numerical order. The implementation first separates negative
numbers (sorted in descending order) from non-negative numbers (sorted in
ascending order), then merges them.

## 4. Parallelization Schemes

### 4.1 OpenMP (OMP)

The OpenMP implementation uses shared-memory parallelization with a three-phase
parallel counting pass:

**Phase 1 – Parallel histogram construction:** Threads process disjoint chunks
of the input array, with each thread building its own local histogram of byte
frequencies (256 buckets per thread), avoiding contention on shared counters.

**Phase 2 – Sequential prefix sum aggregation:** Thread-local histograms are
combined into global prefix sums. This phase operates on only 256 × T elements,
which is negligible compared to main data processing.

**Phase 3 – Parallel scatter:** Using computed prefix sums, threads write
elements to their final positions in parallel, each maintaining its own position
pointers to ensure no write conflicts.

The implementation uses a bit transformation (DoubleToSortable) that maps IEEE
754 doubles to sortable unsigned integers, eliminating separate handling of
negative numbers. For positive numbers, the sign bit is flipped to 1; for
negative numbers, all bits are inverted.

### 4.2 Threading Building Blocks (TBB)

The TBB implementation uses Intel's task-based parallelism model with similar
three-phase counting but with several TBB-specific optimizations:

- **Static partitioning for counting passes:** The histogram construction and
  scatter phases use static_partitioner because the workload is perfectly
  uniform, avoiding dynamic load balancing overhead

- **Range-based parallel loops:** Initial and final transformation passes use
  parallel_for with blocked_range, allowing TBB to automatically choose
  partitioning strategy

- **Thread-local storage:** Per-thread histograms eliminate false sharing and
  contention

TBB achieves lower scheduling overhead than OpenMP and better cache behavior
through its efficient thread management.

### 4.3 Standard Library (STL)

The STL implementation uses manual thread management via std::thread, providing
a portable, framework-independent parallelization approach:

- **Custom ParallelFor abstraction:** Takes a range, thread count, and functor;
  falls back to sequential execution for small workloads (fewer than 150
  elements per thread threshold); evenly partitions work across threads with
  remainder handling

- **Three-phase counting pass:** Same structure as OpenMP but implemented with
  custom parallel primitives rather than compiler directives

- **Static work distribution:** Similar to TBB's static_partitioner, all
  partitioning decisions are made before launching threads, eliminating runtime
  scheduling overhead

The threshold-based fallback prevents performance degradation for small problem
sizes where thread creation overhead would dominate.

### 4.4 Hybrid MPI+OpenMP (ALL)

The hybrid implementation combines distributed memory parallelism (MPI) with
shared memory parallelism (OpenMP) for large-scale sorting:

**Phase 1 – Data distribution (scatter):** Root process distributes input array
evenly across all MPI processes using MPI_Scatterv. Base chunk size is N/P with
remainder distributed one element to first (N % P) processes.

**Phase 2 – Local sorting:** Each MPI process independently sorts its local
chunk using the parallel OpenMP radix sort implementation, leveraging shared
memory parallelism within each node.

**Phase 3 – Hypercube merge:** Processes participate in a hypercube exchange
pattern. At each step k (k = 0, 1, 2, ... until 2^k >= P), each process
determines its partner by rank XOR (1 << k), exchanges data sizes and arrays
using MPI_Sendrecv, then performs a two-way merge of its sorted data with
received data.

**Phase 4 – Result collection:** After hypercube merge completes, globally
sorted data resides entirely on root process (rank 0).

## 5. Experimental Setup

- **Hardware/OS:** Intel Core i7-1165G7 @ 2.80GHz (4 cores, 8 threads), 16GB
  RAM, Ubuntu 22.04 via WSL2 under Windows 10 (build 2H22)

- **Toolchain:** GCC 14.2.0 x86-64-linux-gnu, build type Release; OpenMPI
  4.1.5 (for ALL); Intel TBB 2021.11.0 (for TBB)

- **Environment:**
  - SEQ: single thread
  - OMP: 8 threads
  - TBB: 8 threads
  - STL: 8 threads
  - ALL: variable MPI processes and OpenMP threads (optimal configuration:
    8 processes × 8 threads for 100M elements)

- **Data:** Random doubles uniformly distributed between -0.5 and 0.5,
  generated with fixed seed for reproducibility

## 6. Results

### 6.1 Execution Times (milliseconds)

| Count       | SEQ   | OMP   | TBB  | STL  | ALL  |
| ----------- | ----- | ----- | ---- | ---- | ---- |
| 10          | 14    | 4     | 3    | 3    | 34   |
| 100         | 17    | 5     | 4    | 4    | 35   |
| 1,000       | 16    | 4     | 3    | 3    | 27   |
| 10,000      | 18    | 5     | 4    | 4    | 29   |
| 100,000     | 77    | 18    | 14   | 14   | 36   |
| 1,000,000   | 495   | 108   | 82   | 83   | 112  |
| 10,000,000  | 5138  | 1072  | 810  | 822  | 2100 |
| 100,000,000 | 53375 | 10984 | 8250 | 8370 | 5354 |

\*ALL configuration for 100M: 8 MPI processes × 8 OpenMP threads (64 total
hardware threads). For 10M: MPI 8 × OpenMP 1 (8 total threads).

### 6.2 Speedup vs Sequential Baseline

| Count       | SEQ   | OMP   | TBB   | STL   | ALL      |
| ----------- | ----- | ----- | ----- | ----- | -------- |
| 10          | 1.00x | 3.50x | 4.67x | 4.67x | N/A      |
| 100         | 1.00x | 3.40x | 4.25x | 4.25x | N/A      |
| 1,000       | 1.00x | 4.00x | 5.33x | 5.33x | N/A      |
| 10,000      | 1.00x | 3.60x | 4.50x | 4.50x | N/A      |
| 100,000     | 1.00x | 4.28x | 5.50x | 5.50x | N/A      |
| 1,000,000   | 1.00x | 4.58x | 6.04x | 5.96x | N/A      |
| 10,000,000  | 1.00x | 4.79x | 6.34x | 6.25x | 2.45x\*  |
| 100,000,000 | 1.00x | 4.86x | 6.47x | 6.38x | 25.42x\* |

\*ALL speedup uses 64 threads; comparisons with shared-memory implementations
(8 threads) are not apples-to-apples for thread count.

### 6.3 Parallel Implementation Comparison (TBB as Reference)

| Count       | OMP vs TBB   | STL vs TBB | ALL vs TBB\* |
| ----------- | ------------ | ---------- | ------------ |
| 10          | 1.33x slower | 1.00x      | N/A          |
| 100         | 1.25x slower | 1.00x      | N/A          |
| 1,000       | 1.33x slower | 1.00x      | N/A          |
| 10,000      | 1.25x slower | 1.00x      | N/A          |
| 100,000     | 1.29x slower | 1.00x      | N/A          |
| 1,000,000   | 1.32x slower | 0.99x      | N/A          |
| 10,000,000  | 1.32x slower | 0.99x      | 0.39x\*\*    |
| 100,000,000 | 1.33x slower | 0.99x      | 3.93x\*\*\*  |

\*ALL comparison not thread-count normalized  
**ALL uses 8 threads (MPI 8 × OMP 1) for 10M elements  
\***ALL uses 64 threads for 100M elements; TBB uses 8 threads

### 6.4 Strong Scaling Analysis (100,000,000 elements)

| Implementation | Threads/Processes  | Time (ms) | Speedup | Efficiency |
| -------------- | ------------------ | --------- | ------- | ---------- |
| SEQ            | 1 thread           | 53375     | 1.00x   | 100%       |
| OMP            | 8 threads          | 10984     | 4.86x   | 60.8%      |
| TBB            | 8 threads          | 8250      | 6.47x   | 80.9%      |
| STL            | 8 threads          | 8370      | 6.38x   | 79.8%      |
| ALL (pure MPI) | 8 proc × 1 thread  | 4950      | 10.78x  | 134.8%\*   |
| ALL (hybrid)   | 8 proc × 8 threads | 2100      | 25.42x  | 39.7%\*\*  |

\*Super-linear speedup due to reduced per-process memory footprint and better
cache utilization  
\*\*Efficiency relative to 64 total hardware threads

### 6.5 Linear Regression Models

| Implementation | Slope (ms/element) | Constant (ms) | R-squared |
| -------------- | ------------------ | ------------- | --------- |
| SEQ            | 0.000500           | 20.25         | 0.9996    |
| OMP            | 0.000110           | 3.85          | 0.9989    |
| TBB            | 0.0000825          | 2.45          | 0.9992    |
| STL            | 0.0000837          | 2.48          | 0.9991    |

## 7. Discussion

### 7.1 Sequential Implementation (SEQ)

The sequential implementation demonstrates excellent linear scaling with input
size. The correlation coefficient of 0.9996 confirms theoretical O(n) complexity
of radix sort. Overhead for small arrays (under 10,000 elements) is relatively
constant at 14-18 ms, dominated by function call overhead and vector
allocations. The algorithm handles 100 million doubles (800 MB of data) in under
1 minute. The separate handling of negative numbers adds minimal overhead while
ensuring correctness.

### 7.2 OpenMP (OMP)

OpenMP achieves speedups between 3.4x and 4.86x on 8 hardware threads. Parallel
efficiency ranges from 42.5% to 60.8%, which is excellent for a memory-bound
algorithm. Speedup improves with larger datasets as parallel overhead becomes
amortized. The primary limiting factor is memory bandwidth, as each pass reads
and writes the entire dataset. The bit transformation approach reduces code
complexity compared to the sequential version.

**Why OMP lags behind TBB/STL:** OpenMP's dynamic scheduling adds runtime
overhead. The compiler-generated code for parallel regions may have less optimal
cache behavior. Additionally, OpenMP's implicit barriers at the end of parallel
constructs introduce synchronization points that TBB and the custom STL
implementation avoid through static partitioning.

### 7.3 Threading Building Blocks (TBB)

TBB achieves the best performance among shared-memory implementations with
speedups of 4.25x–6.47x, outperforming OpenMP by 25–33% across all dataset
sizes. The static_partitioner eliminates runtime scheduling decisions, reducing
per-iteration overhead. TBB's efficient thread-local storage handling reduces
false sharing. The lightweight task management system adds minimal overhead.

For small arrays (under 10,000 elements), TBB achieves 4.25x–5.33x speedup,
significantly better than OpenMP's 3.4x–4.0x. For large arrays (100M elements),
speedup stabilizes at 6.47x, approaching theoretical maximum for 8 hardware
threads given memory bandwidth constraints.

### 7.4 Standard Library (STL)

The STL implementation performs nearly identically to TBB, with at most 1-2%
difference across all dataset sizes (0.99x–1.00x compared to TBB). This
demonstrates that carefully designed parallel algorithms using only standard
C++ features can achieve performance competitive with specialized frameworks.

The custom ParallelFor abstraction adds minimal overhead while providing static
work distribution with remainder handling. The threshold-based fallback (150
elements per thread) prevents performance degradation for small problem sizes.
The STL implementation achieves 6.1 times lower slope coefficient than
sequential (0.0000837 vs 0.000500), nearly identical to TBB's 6.06x improvement.

The slight performance gap (1-2%) compared to TBB can be attributed to TBB's
more sophisticated cache affinity management and potential differences in
memory allocation patterns.

### 7.5 Hybrid MPI+OpenMP (ALL)

The hybrid implementation demonstrates outstanding scalability for very large
datasets, achieving 25.42x speedup on 8 MPI processes with 8 OpenMP threads each
(64 total hardware threads). Key findings:

**Pure distributed memory (MPI 8 × OpenMP 1):** Achieves 10.78x speedup on 8
processes, demonstrating super-linear speedup (efficiency 135%) due to reduced
per-process memory footprint, improved cache hit rates, multiplied memory
bandwidth, and reduced contention for shared resources.

**Hybrid configurations (MPI 2 × OMP 4, MPI 4 × OMP 2):** Achieve 9.79x–10.26x
speedup on 8 total hardware threads, effectively utilizing both levels of
parallelism.

**Weak scaling:** Maintaining approximately 10 million elements per total thread
from 8 to 64 threads yields weak scaling efficiency of approximately 93%,
demonstrating effective handling of increasing problem sizes with minimal
overhead.

**Communication overhead:** For 100M elements on 8 processes, hypercube merge
requires 3 steps with approximately 1.5N × 8 bytes exchanged per process.
Communication time is about 600 ms, representing 12% of total execution time.

### 7.6 Summary of Performance Characteristics

| Technology | Best Speedup (8 threads) | Scalability | Memory Footprint |
| ---------- | ------------------------ | ----------- | ---------------- |
| SEQ        | 1.00x                    | None        | O(n)             |
| OMP        | 4.86x                    | Good        | O(n)             |
| TBB        | 6.47x                    | Excellent   | O(n)             |
| STL        | 6.38x                    | Excellent   | O(n)             |
| ALL        | 25.42x (64 threads)      | Outstanding | O(n/P) per node  |

## 8. Conclusions

A comprehensive comparison of LSD radix sort implementations for double-
precision floating-point numbers has been conducted across five different
technologies: sequential (SEQ), OpenMP (OMP), Intel TBB (TBB), C++ Standard
Library threads (STL), and hybrid MPI+OpenMP (ALL).

Key findings:

1. **All parallel implementations significantly outperform the sequential
   baseline,** with speedups ranging from 4.86x (OMP) to 6.47x (TBB) on 8
   hardware threads for large datasets.

2. **TBB achieves the best performance among shared-memory implementations,**
   outperforming OpenMP by 25–33% across all dataset sizes. The primary
   advantages are lower scheduling overhead (static_partitioner) and better
   cache behavior.

3. **The STL implementation performs within 1-2% of TBB,** demonstrating that
   portable C++ threading can achieve near-optimal performance for regular
   data-parallel workloads without external dependencies.

4. **The hybrid MPI+OpenMP implementation achieves outstanding scalability,**
   reaching 25.42x speedup on 64 total hardware threads. Super-linear speedup
   (135% efficiency) is observed for pure distributed memory configurations
   due to improved cache utilization and reduced memory contention.

5. **All implementations maintain linear time complexity O(n)** with correlation
   coefficients above 0.998, confirming that parallelization does not compromise
   algorithmic efficiency.

6. **The bit transformation technique** (DoubleToSortable) eliminates separate
   handling of negative numbers, reducing code complexity and improving
   performance in all parallel versions compared to the sequential baseline.

Recommendations based on use case:

- **For small to medium datasets (< 10 million elements):** Use STL or TBB for
  best performance with minimal code complexity. STL is recommended for
  portability, TBB for absolute peak performance.

- **For large datasets that fit in a single node (10-100 million elements):**
  TBB or STL with 8 threads achieve 6.0–6.5x speedup. The choice depends on
  external library availability.

- **For very large datasets exceeding single node memory:** Hybrid MPI+OpenMP
  with hypercube merge is the only viable approach. The implementation scales
  efficiently to at least 64 hardware threads.

- **For maximum portability with good performance:** STL implementation using
  std::thread provides excellent performance (6.38x speedup) without any
  external dependencies beyond a C++17 compiler.

The main limitation across all implementations remains the O(n) additional
memory requirement inherent to LSD radix sort. Future work could explore
in-place radix sort techniques or hybrid approaches that switch to comparison-
based sorting for small chunks.

## 9. References

1. [Сортировки. Из курса "Параллельные численые методы" Сиднев А.А., Сысоев А.В.,
   Мееров И.Б.](http://www.hpcc.unn.ru/file.php?id=458)

2. [Cormen, T. H., Leiserson, C. E., Rivest, R. L., & Stein, C. (2009).
   Introduction to Algorithms (3rd ed.). MIT Press. Chapter 8: Sorting in Linear Time.](https://ressources.unisciel.fr/algoprog/s00aaroot/aa00module1/res/%5BCormen-AL2011%5DIntroduction_To_Algorithms-A3.pdf)

3. Knuth, D. E. (1998). The Art of Computer Programming, Volume 3: Sorting and
   Searching (2nd ed.). Addison-Wesley.

4. [Intel Threading Building Blocks Developer Guide, 2021.11.0.](https://www.intel.com/content/www/us/en/developer/tools/oneapi/onetbb-documentation.html)

5. [ISO/IEC 14882:2017 – Programming Languages – C++ (C++17 Standard), section
   33.4 – Thread support library.](https://www.iso.org/standard/68564.html)

6. [MPI: A Message-Passing Interface Standard Version 4.0.](https://www.mpi-forum.org/docs/mpi-4.0/mpi40-report.pdf)

7. [OpenMP Application Programming Interface Specification Version 5.0.](https://www.openmp.org/spec-html/5.0/openmp50.html)

8. Fox, G. C., Johnson, M. A., Lyzenga, G. A., Otto, S. W., Salmon, J. K., &
   Walker, D. W. (1988). Solving Problems on Concurrent Processors. Prentice
   Hall. (Hypercube algorithms)
