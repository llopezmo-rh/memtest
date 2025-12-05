# memtest

## Description

Small C program for GNU/Linux to test memory allocation. Useful to test container performance and Kubernetes settings like limits.

## Compilation

As it is a single file, this is the easiest way:

~~~
$ gcc memtest.c -Wall -Wextra -o memtest
~~~

Feel free to add as many compilation options as you prefer, but those ones are OK for most of the cases.

Adding GCC compilation optimizers does not seem to change the behaviour according to the tests carried out.

## Execution instructions

Both [a binary file for GNU/Linux and a container image](https://github.com/llopezmo-rh/memtest/releases) are provided.

### Binary file

Options to select the amount of memory, which can be both an integer or a decimal value:
- Write it as an argument.
- Use the environment variable `TARGET_MIB`.

It is mandatory to provide at least one, otherwise the program will show the following error message:
~~~
$ memtest
No argument provided.
Usage: memtest [<amount_of_MiB>]
Without argument, use the environment variable TARGET_MIB
~~~

If a different environment variable name is preferred, the following constant in `memtest.c` has to be edited (and the file recompiled):
~~~
#define TARGET_MIB_ENV_VARIABLE "TARGET_MIB"
~~~

In case both are provided, the interactive argument has a higher order of precedence.

The command `pause()` is executed at the end, which means that the process has to be killed manually. That can be done by pressing Control+C if it is executed interactively. If it is wanted to execute it for a limited time period, [timeout](https://ss64.com/bash/timeout.html) can be used for example.
~~~
$ memtest 15.5
PID: 526492

Initial process memory: 1.46 MiB
Target process memory:  15.50 MiB
---------------
Allocating memory until at least 15.50 MiB...
Approximate memory consumed by the process in total: 15.46 MiB
Waiting for signal SIGINT or SIGTERM...
^CSignal SIGINT received. Exiting...
~~~

### Container image

You can download [the container image](https://github.com/llopezmo-rh/memtest/releases) and use, for example, [`podman load`](https://docs.podman.io/en/latest/markdown/podman-load.1.html) or you can pull the image directly like this:
~~~
$ podman pull quay.io/llopezmo/memtest:1.0
Trying to pull quay.io/llopezmo/memtest:1.0...
Getting image source signatures
Copying blob 72c3419fff1e skipped: already exists  
Copying config d23d07261f done   | 
Writing manifest to image destination
d23d07261fba0c9ec1e29e6ef73e006add51e6585da5004b61d84b4f2742169b
~~~

The environment variable is required unless it is wanted to execute the binary file interactively from inside the container.

Execution example:
~~~
$ export TARGET_MIB=14
$ podman run --name memtest -d --replace --rm --env TARGET_MIB quay.io/llopezmo/memtest:1.0 
a0b837a4bebf51054c0bf5922b55637a08beb53d227463cd1979d4f56f761e97
~~~
