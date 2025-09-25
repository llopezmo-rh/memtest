# memtest

## Description

Small C program for GNU/Linux to test memory allocation. Useful to test container performance and Kubernetes settings like limits.

An **intiger** amount of MiB must be provided as an argument and it tries to allocate an amount of memory as near the one provided as possible. According to the tests, it uses to allocate a bit more, but it has always been less than 1% above the value requested.

~~~
$ memtest
Use: memtest <amount_of_MiB>
~~~

It executes the command `pause()` at the end, which means that the process has to be killed manually. That can be done by pressing Control+C if it is executed interactively. If it is wanted to execute it for a limited time period, [timeout](https://ss64.com/bash/timeout.html) can be used for example.

## Compilation

~~~
$ gcc memtest.c -o memtest
~~~

## Output example

~~~
PID: 332379

Initial process memory: 1.54 MiB
Target process memory:  20.00 MiB
---------------
Allocating memory until at least 20.00 MiB...
Approximate memory consumed by the process in total: 20.04 MiB
~~~
