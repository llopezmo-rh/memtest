# memtest

## Description

Small C program for GNU/Linux to test memory allocation. Useful to test container performance and Kubernetes settings like limits.

An **intiger** amount of MiB must be provided as an argument and it tries to allocate an amount of memory as near the one provided as possible. According to the tests, it uses to allocate a bit more, but rarely above 5% over the value requested.

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
$ memtest.py 20
The base memory used by this script is 13.57 MiB.
Reserving additional memory until approximately 20 MiB are used..

Process ID: 275742
Approximate memory used by this process in total: 20.70 MiB

^C
Finishing because the process was manually interrupted from the keyboard
~~~
