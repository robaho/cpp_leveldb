## summary

This is a C++ port of my ultra low-latency [keydb](https://github.com/robaho/leveldb) key/value database.

## background

The main motivation for this project was to familiarize myself with current C++ development, specifically C++20.

The project is a bit of an "inception", in that, my original keydb was written in Go, then I did a Java port, then I discovered the Google [leveldb](https://github.com/google/leveldb/tree/068d5ee1a3ac40dabd00d211d5013af44be55bea) project - which is written in C++. I changed the keydb api to mimic the Google leveldb api - and renamed them leveldb and jleveldb.

Now with this C++ port, I have written the same library from scratch in three languages ([Java code](https://github.com/robaho/jleveldb),[Go code](https://github.com/robaho/leveldb))- along with an independent version developed by Google. My library focus was on api simplicity, maintainability and performance. The three versions are written using the same design, but are idiomatic within the language.

leveldb only supports multi-threaded access. There is a client/server module to expose cpp_leveldb for multi-process access available at [cpp_leveldbr](https://github.com/robaho/cpp_leveldbr)

## performance

Google's leveldb includes a benchmarking tool [dbbench](https://github.com/google/leveldb/blob/068d5ee1a3ac40dabd00d211d5013af44be55bea/benchmarks/db_bench.cc). I have replicated this in each of the libraries. All tests are run on the same Intel iMac under OSX. All timings are in microseconds / operation. The methodology is that for each platform, db_bench was run multiple times, and the lowest timing for each task was recorded. Because of the short duration of the test, and the tasks themselves, they are subject to OS background interference.

| Test | [Go](https://github.com/robaho/leveldb)     | Go PGO<sup>2</sup> | [Java](https://github.com/robaho/jleveldb) | Java GraalVM<sup>1</sup> | C++ | C++ PGO<sup>2</sup> | [Google](https://github.com/google/leveldb/tree/068d5ee1a3ac40dabd00d211d5013af44be55bea) |
| --- | ---: | ---: |---: | ---: | ---: | ---: | ---: |
| write no-sync | 4.25 | 4.23 |4.86 | 5.51 | 4.65 | 4.36 | 4.60 |
| write sync<sup>3</sup> | 45 | 46 | 46 | 50 | 10822 | 10888 | 10023 |
| write batch | 1.07 | 1.06 | 1.06 | 1.72 | 1.19 | 1.02 | 1.94 |
| write overwrite | 4.37 | 4.30 | 4.65 | 5.60 | 4.65 | 4.43 | 8.20 |
| read random | 2.44 | 2.49 | 3.79 | 8.07 | 5.93 | 4.61 | 4.74 |
| read sequential | 0.36 | 0.31 | 0.35 | 0.51 | 0.53 | 0.42 | 0.15 |
| read random compact | 2.44 | 2.40 | 3.62 | 7.55 | 3.71 | 2.68 | 2.13 |
| read sequential compact | 0.09 | 0.09 | 0.07 | 0.14 | 0.06 | 0.04 | 0.12 |

<sup>1</sup> Profile guided optimizations are not enabled since they require the enterprise version.

<sup>2</sup> Compiled with profile guided optimizations.

<sup>3</sup> Both C++ and Google versions use `FULLFSYNC` on OSX for improved safety.

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
