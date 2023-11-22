# Making-Free-Joins-Scalable

A parallel C++ implementation of Free Joins using Intel OneTBB.

To run Free Joins, download the JOB benchmark and run the following in your terminal:

```
mkdir -p build/release
cd build/release
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

Afterwards, execute any of the JOB queries by passing it as a cmdline parameter to main. For example, to run query 12a, use:
```
./main 12a
```
