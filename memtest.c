// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define ANY_CHAR 'x'
#define MIB_TO_BYTES 1048576
// The main use case for this piece of software is to execute it from inside a
// container in order to test settings like memory limits for Kubernetes,
// Podman, Docker... Therefore, the MiB to be consumed can be provided through
// an argument or an environment variable, having the earlier a higher order
// of precedence.
//
// The following constant stores the name of the environment variable used,
// if any.
#define TARGET_MIB_ENV_VARIABLE "TARGET_MIB"


// Global variable to detect whether a signal is received and also which one
volatile sig_atomic_t received_signal = 0;


static void print_use(const char* executable_file_name)
	{
	fprintf
		(
		stderr,
		"Usage: %s [<amount_of_MiB>]\n"
		"Without argument, use the environment variable %s\n",
		executable_file_name,
		TARGET_MIB_ENV_VARIABLE
		);
	}

// This program executes pause() at the end and it is designed to be finished
// by sending a SIGTERM (command "kill", Kubernetes...) or a SIGINT (Control+C) 
// signal.
//
// The process will NOT be directly terminated if those signals are received.
// Instead, this is what will happen:
// 1. pause() will be interrupted, which will make possible the
// process termination.
// 2. Due to a condition in the while loop, the memory reservation will be
// interrupted. This will accelerate the process termination, although not
// directly because of the signal, but because it will reach its own end.
//
// The function handle_signal simply keeps the signal received into the 
// global sig_atomic_t variable. Then the process will continue.
//
// The purposes of this approach are the following:
// 1. Return an exit code instead of the default error code if the signals
// are received.
// 2. Report the signal received.
static void handle_signal(int sig)
    {
    received_signal = sig;
    }

// Helper function to safely parse the input argument
static int parse_arguments(double* input_mib, const char *arg)
	{
	char *endptr;

	// Check whether arg is NULL
	if (arg == NULL)
		{
		fprintf(stderr, "No argument provided.\n");
		return -1;
		}

	// Reset errno before calling strtol
	errno = 0;
	*input_mib = strtod(arg, &endptr);

	// If the number is out of range, strtol returns either LONG_MAX or
	// LONG_MIN and sets errno to ERANGE
	if (errno == ERANGE)
		{
		perror("strtod: number out of double type range\n");
		return -1;
		}
	
	// Other strtol errors 
	if (errno != 0 && *input_mib == 0)
		{
		perror("strtod error\n");
		return -1;
		}
	
	// No number
	if (endptr == arg) 
		{
		fprintf(stderr, "Error: no number in argument.\n");
		return -1;
		}

	// Non-numerical characters
	if (*endptr != '\0')
		{
		fprintf(stderr, "Invalid characters in argument: \"%s\"\n", endptr);
		return -1;
		}

	// Negative value
	if (*input_mib <= 0.0)
		{
		fprintf(stderr, "Memory amount must be a positive integer.\n");
		return -1;
		}

	return 0;
	}

// Return value unit: bytes
static size_t get_current_rss() 
	{
	int ok;
	// Open the virtual file which hosts the information
    FILE* file = fopen("/proc/self/statm", "r");
    if (!file) 
		{
        perror("Could not open /proc/self/statm");
        return 0;
    	}

    long int rss_pages = 0;
    // The second value in the file is the RSS in pages.
    // "%*ld" used to read and discard the first value (total size).
	ok = fscanf(file, "%*d %ld", &rss_pages);
	if (ok != 1)
		{
        perror("Could not parse /proc/self/statm");
		fclose(file);
        return 0;
    	}

    // Close file
	ok = fclose(file);
	if (ok != 0)
		{
		fprintf(stderr, "Error closing file\n");
		return 0;
		}
	
    // Get the system's page size and convert pages to bytes.
    long int page_size = sysconf(_SC_PAGESIZE);
	if (page_size == -1)
		{
		fprintf(stderr, "Error accessing the system configuration\n");
		return 0;
		}

	// Return the number of pages multiplied by the page size in bytes
	return rss_pages * page_size;
	}

int main(int argc, char *argv[]) 
	{
	struct sigaction sa;
	int ok;
	size_t target_bytes, current_bytes, page_size;
	double target_mib, current_mib;
	char *memory_hog, *input_arg;

	// Signal handling setup
	// Clear structure
	memset(&sa, 0, sizeof(sa));
	// Assign handling function
	sa.sa_handler = handle_signal;
	// Apply to SIGINT (Ctrl+C) and SIGTERM (Kubernetes termination)
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	// Check and parse target MiB
	// An eventual argument has a higher priority than the environment variable
	input_arg = NULL;
	if (argc == 2)
		{
		input_arg = argv[1];
		}
	else if (argc == 1)
		{
		input_arg = getenv(TARGET_MIB_ENV_VARIABLE);
		}
	else
		{
		print_use(argv[0]);
		return 1;
		}
	// Although parse_arguments already checks whether input_arg is NULL, doing
	// it here to in order to make it more robust, and also to make easier the
	// use of the argument of print_use
	if (input_arg == NULL)
		{
		fprintf(stderr, "No argument provided.\n");
		print_use(argv[0]);
		return 1;
		}
	ok = parse_arguments(&target_mib, input_arg);
	if (ok != 0) return 1;
	
	// Convert to bytes
	target_bytes = (size_t)(target_mib * MIB_TO_BYTES);
	
	// Print process ID
	printf("PID: %d\n\n", getpid());

	// Calculate memmory to allocate
	current_bytes = get_current_rss();
	if (current_bytes == 0) return 1;
	current_mib = (double)current_bytes / MIB_TO_BYTES;
	printf("Initial process memory: %.2f MiB\n", current_mib);
	printf("Target process memory:  %.2f MiB\n", target_mib);
	printf("---------------\n");
	if (target_bytes < current_bytes)
		{
		fprintf(stderr,
			"The memory requested (%.2f MiB) is less than the base memory "
			"used by the process (%.2f MiB). Exiting...\n",
			target_mib, current_mib);
		return 1;
		}

	// Allocate memory
	printf("Allocating memory until at least %.2f MiB...\n", target_mib);
	page_size = sysconf(_SC_PAGESIZE);
	// The memory is allocated page by page instead of executing malloc only
	// once, which would make the code simpler. The reason is to reduce the
	// memory overhead created by malloc and therefore reduce the memory
	// excess over the target.
	// 
	// According to the tests, the total memory allocated is always greater
	// than the target. Therefore, page_size is substracted from the target
	// in order to reduce the memory excess over the target wanted.
	// page_size is also multiplied by 10 to make this reduction even greater
	// and get as near the target value as possible, although it is just a
	// rough approximation.
	//
	// If one of the configured signals is received, the process will continue
	// because our handling function does not terminate it. However, it is not
	// wanted that this memory reservation loop continues. Therefore, if a
	// signal is received, it will be finished. If the signal is received 
	// previously, the loop will not even start.
	while(received_signal == 0 &&
		get_current_rss() < (target_bytes - (page_size * 10)))
		{
		// If current_bytes == 0, it is because get_current_rss failed
		if (current_bytes == 0) return 1;
		// This memory will be freed automatically by the operating system
		// once the process finished. In case the code is edited and freeing
		// the memory manually becomes a requirement for any reason, the
		// solution would be to create a memory data structure like a list
		// to store the memory_hog pointers and free them one by one at the end.
		memory_hog = (char*)malloc((size_t)page_size);
		if (memory_hog == NULL)
			{
			fprintf(stderr, "Error allocating memory with malloc\n");
			return 1;
			}
		// The memory is not allocated by malloc. memset required
		memset(memory_hog, ANY_CHAR, (size_t)page_size);
		}

	// Print information
	current_bytes = get_current_rss();
	if (current_bytes == 0) return 1;
	current_mib = (double)current_bytes / MIB_TO_BYTES;
	printf("Approximate memory consumed by the process in total: %.2f MiB\n",
		current_mib);
	printf("Waiting for signal SIGINT or SIGTERM...\n");
	
	// Pause the process
	// It is a loop instead to protect the process in case other signals are 
	// handled in the future and in a different way. Also, to prevent the
	// process from executing the loop if the signal is received previously
	while (received_signal == 0)
		{
		pause();
		}

	// Final message reporting the signal received
	if (received_signal == SIGINT)
		fprintf(stderr, "Signal SIGINT received. Exiting...\n");
	else if (received_signal == SIGTERM)
		fprintf(stderr, "Signal SIGTERM received. Exiting...\n");

	return 0;
	}
