## summary

This is a C++ port of my ultra low-latency [keydb](https://github.com/robaho/leveldb) key/value database.

## background

The main motivation for this project was to familiarize myself with current C++ development, specifically C++20.

The project is a bit of an "inception", in that, my original keydb was written in Go, then I did a Java port, then I discovered the Google [leveldb](https://github.com/google/leveldb/tree/068d5ee1a3ac40dabd00d211d5013af44be55bea) project - which is written in C++. I changed the keydb api to mimic the Google leveldb api - and renamed them leveldb and jleveldb.

Now with this C++ port, I have written the same library from scratch in three languages - along with an independent version developed by Google. My library focus was on api simplicity, maintainability and performance. The three versions are written using the same design, but are idiomatic within the language.

leveldb only supports multi-threaded access. There is a client/server module to expose cpp_leveldb for multi-process access available at [cpp_leveldbr](https://github.com/robaho/cpp_leveldbr)

## performance

Google's leveldb includes a benchmarking tool [dbbench](https://github.com/google/leveldb/blob/068d5ee1a3ac40dabd00d211d5013af44be55bea/benchmarks/db_bench.cc). I have replicated this in each of the libraries. All tests are run on the same Intel iMac under OSX. All timings are in microseconds / operation.

| Test | Go     | Java | C++ | C++ PGO<sup>1</sup> | Google |
| ---  | ---: | ---: | ---: | ---: | ---: |
| write no-sync | 4.10 | 4.86 | 5.34 | 5.18 | 4.60 |
| write sync<sup>2</sup> | 49 | 47 | 10281 | 10560 | 10023 |
| write batch | 1.02 | 1.08 | 1.20 | 1.03 | 1.94 |
| write overwrite | 4.21 | 4.70 | 6.10 | 5.58 | 8.20 |
| read random | 2.42 | 5.65 | 5.36 | 4.15 | 4.74 |
| read sequential | 0.45 | 0.35 | 0.47 | 0.41 | 0.15 |
| read random compact | 2.30 | 5.20 | 4.50 | 3.29 | 2.13 |
| read sequential compact | 0.11 | 0.10 | 0.05 | 0.04 | 0.12 |

<sup>1</sup> Compiled with profile guided optimizations.

<sup>2</sup> Both C++ and Google versions use `FULLFSYNC` on OSX for improved safety.

## learnings

Modern C++ is a joy in many ways but it isn't perfect.

### pros

1. Approaches garbage collection memory safety by using `shared_ptr`, `unique_ptr`, etc.
1. Tools like ASan make tracking down memory issues/leaks fairly straightforward.
1. Many easy to use profilers available. I used [Samply](https://github.com/mstange/samply) during development.
1. Many modern debuggers and IDEs available - supporting syntax highlighting, code completion and popup api documentation.
1. Excellent support for unit tests using facilities like Boost.
1. Performance can be outstanding.

### cons

1. `shared_ptr` is slow, making it only suitable for top-level objects, then you must use raw pointers internally.
1. Dynamic memory management (e.g. `malloc/free`) is very slow. This necessitates "manual" memory management, e.g. buffer re-use, arena allocators, etc. The initial "clean" C++ version was nearly 3x slower than the final - much, much slower than the Go or Java versions. In order to achieve the desired performance, you end up creating "restrictive/bug-prone" apis. For instance, the Google API docs state:

    > Caller should delete the iterator when it is no longer needed.
    The returned iterator should be deleted before this db is deleted.

    and

    > Return the value for the current entry.  The underlying storage for
    the returned slice is valid only until the next modification of the iterator.

    Which works, but it does make the api more fragile (i.e. error prone). Often the inefficiency is simply pushed farther (e.g. you can't store the returned iterator value in a map without a copy).

1. Compilation times are extremely slow, especially when using higher optimization levels. Since C++ does not use a context-free grammar everything must be recompiled almost always.
1. Writing lock-free data structures is much easier in a GC language due to the [ABA problem](https://en.wikipedia.org/wiki/ABA_problem). `std::atomic` helps but using a well-tested library like Boost is probably safer. Still, I was able to implement the concurrent skip list in a lock-free manner.
1. Cryptic error messages. Missing a single character can lead to a 100 line error across multiple files. Same underlying cause as the slow compile times.
1. Having to work with both header and implementation files is a pain.

## next steps

I am going to test using profile guided optimization for the Go and C++ versions. I am also going to test a statically compiled Java version using GraalVM.
