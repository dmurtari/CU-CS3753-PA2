CSCI 3753: Programming Assignment 2
===================================

Spring 2014
Originally by Andy Sayler
Completed by Domenic Murtari

Adopted from previous code by:
 Chris Wailes <chris.wailes@gmail.com> - 2010
 Wei-Te Chen <weite.chen@colorado.edu> - 2011
 Blaise Barney - pthread-hello.c

Folders
-------
`input`: `names*.txt` input files
`handout`: Assignment description and documentation

Executables
-----------
`lookup`: A basic non-threaded DNS query-er
`multi-lookup`: A multi-threaded version of the DNS query-er
`queueTest`: Unit test program for queue
`pthread-hello`: A simple threaded "Hello World" program

Usage
-----
Building:
    make

Clean:
	make clean

Lookup DNS info for all names in `input` folder:
	./multi-lookup input/names*.txt results.txt

Check for memory leaks with Valgrind
	valgrind ./multi-lookup input/names*.txt results.txt

Benchmarking
------------
On a CU-CS-VM running Ubuntu 12.04 64bit, 4 CPUs and 4GB of memory, the best
performance seemed to be achieved with running 5 resolver threads. Data from
the benchmarks can be seen in [`banchmarks.md`](benchmarks.md).