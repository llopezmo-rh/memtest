#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

#define ANY_CHAR 'x'
#define MIB_TO_BYTES 1048576


// Return value unit: bytes
unsigned long long int get_current_rss() 
	{
	// Open the virtual file which hosts the information
    FILE* file = fopen("/proc/self/statm", "r");
    if (!file) 
		{
        perror("Could not open /proc/self/statm");
        return 0;
    	}

    unsigned long long int rss_pages = 0;
    // The second value in the file is the RSS in pages.
    // "%*ld" used to read and discard the first value (total size).
    if (fscanf(file, "%*ld %ld", &rss_pages) != 1) 
		{
        perror("Could not parse /proc/self/statm");
		assert(fclose(file) == 0);
        return 0;
    	}

    // Close file
	assert(fclose(file) == 0);
	
    // Get the system's page size and convert pages to bytes.
    unsigned long long int page_size = sysconf(_SC_PAGESIZE);
	return rss_pages * page_size;
}

int main(int argc, char *argv[]) 
	{
	pid_t pid;
	unsigned long long int target_bytes, current_bytes, page_size;
	double target_mib, current_mib;
	char* memory_hog;
	// Check arguments
	if (argc != 2)
		{
		fprintf(stderr, "Use: %s <amount_of_MiB>\n", argv[0]);
		return 1;
		}
	// Convert to bytes
	target_bytes = strtoull(argv[1], NULL, 10) * MIB_TO_BYTES;
	target_mib = (double)target_bytes / MIB_TO_BYTES;
	if (target_bytes == 0 || target_bytes == ULLONG_MAX)
		{
		fprintf(stderr, "Argument \"%s\" not valid\n", argv[1]);
		fprintf(stderr, "Use: %s <amount_of_MiB>\n", argv[0]);
		return 1;
		}
	
	// Print process ID
	pid = getpid();
	printf("PID: %llu\n\n", pid);

	// Calculate memmory to allocate
	current_bytes = get_current_rss();
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
	// rough approximation
	while(get_current_rss() < (target_bytes - (page_size * 10)))
		{
		// This memory will be freed automatically by the operating system
		// once the process finished. In case the code is edited and freeing
		// the memory manually becomes a requirement for any reason, the
		// solution would be to create a memory data structure like a list
		// to store the memory_hog pointers and free them one by one at the end.
		memory_hog = (char*)malloc(page_size);
		assert(memory_hog != NULL);
		// The memory is not allocated by malloc. memset required
		memset(memory_hog, ANY_CHAR, page_size);
		}

	// Print information
	current_bytes = get_current_rss();
	current_mib = (double)current_bytes / MIB_TO_BYTES;
	printf("Approximate memory consumed by the process in total: %.2f MiB\n",
		current_mib);
	
	// Pause the process
	pause();

	return 0;
	}
