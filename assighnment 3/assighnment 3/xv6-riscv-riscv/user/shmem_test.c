#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define TEST_STRING "Hello daddy"
#define SHARED_SIZE 4096  // One page
#define DISABLE_UNMAP 0   // Set to 1 to test cleanup on exit

void print_proc_size(char* label, int pid) {
    printf("%s: Process %d size information would be shown here\n", label, pid);
    // Note: In real implementation, you'd need a way to get process size
    // This might require another system call or debugging interface
}

int main(int argc, char *argv[]) {
    char *shared_mem;
    char *local_buffer;
    int child_pid, parent_pid;
    uint64 shared_addr;
    int disable_unmap = DISABLE_UNMAP;
    
    printf("=== Shared Memory Test ===\n");
    
    parent_pid = getpid();
    printf("Parent PID: %d\n", parent_pid);
    
    // Allocate some local memory to share
    local_buffer = malloc(SHARED_SIZE);
    if (local_buffer == 0) {
        printf("Failed to allocate memory\n");
        exit(1);
    }
    
    printf("Allocated buffer at: %p\n", local_buffer);
    strcpy(local_buffer, "Initial data from parent");
    
    print_proc_size("Before fork", parent_pid);
    
    child_pid = fork();
    if (child_pid < 0) {
        printf("Fork failed\n");
        exit(1);
    }
    
    if (child_pid == 0) {
        // Child process
        printf("\n--- Child Process (PID: %d) ---\n", getpid());
        
        print_proc_size("Child before mapping", getpid());
        
        // Map shared memory from parent to child
        shared_addr = map_shared_pages(parent_pid, getpid(), local_buffer, SHARED_SIZE);
        if (shared_addr == -1) {
            printf("Child: Failed to map shared memory\n");
            exit(1);
        }
        
        printf("Child: Mapped shared memory at address: %p\n", (void*)shared_addr);
        shared_mem = (char*)shared_addr;
        
        print_proc_size("Child after mapping", getpid());
        
        // Read initial data
        printf("Child: Read from shared memory: '%s'\n", shared_mem);
        
        // Write new data
        strcpy(shared_mem, TEST_STRING);
        printf("Child: Wrote to shared memory: '%s'\n", TEST_STRING);
        
        if (!disable_unmap) {
            // Unmap shared memory
            if (unmap_shared_pages((void*)shared_addr, SHARED_SIZE) == 0) {
                printf("Child: Successfully unmapped shared memory\n");
            } else {
                printf("Child: Failed to unmap shared memory\n");
            }
            
            print_proc_size("Child after unmapping", getpid());
        }
        
        // Test malloc after unmapping
        char *test_malloc = malloc(1024);
        if (test_malloc != 0) {
            printf("Child: malloc() works after unmapping\n");
            strcpy(test_malloc, "Child malloc test");
            printf("Child: malloc content: '%s'\n", test_malloc);
            free(test_malloc);
        } else {
            printf("Child: malloc() failed after unmapping\n");
        }
        
        print_proc_size("Child after malloc test", getpid());
        
        printf("Child: Exiting\n");
        exit(0);
    } else {
        // Parent process
        // Wait for child to complete first to avoid race condition
        int child_status;
        wait(&child_status);
        
        printf("\n--- Parent Process ---\n");
        printf("Parent: Child PID is %d\n", child_pid);
        printf("Parent: Child exited with status %d\n", child_status);
        
        // Read what child wrote
        printf("Parent: Reading from local buffer: '%s'\n", local_buffer);
        
        // Verify we can still access the memory
        printf("Parent: Final content: '%s'\n", local_buffer);
        
        // Test cleanup
        printf("\n--- Testing Cleanup ---\n");
        
        // Create another child to test the mapping again
        int child2_pid = fork();
        if (child2_pid == 0) {
            // Second child - test that we can map again
            uint64 addr2 = map_shared_pages(parent_pid, getpid(), local_buffer, SHARED_SIZE);
            if (addr2 != -1) {
                printf("Child2: Successfully mapped memory at %p\n", (void*)addr2);
                printf("Child2: Read: '%s'\n", (char*)addr2);
                
                // Clean unmap
                unmap_shared_pages((void*)addr2, SHARED_SIZE);
                printf("Child2: Cleaned up mapping\n");
            } else {
                printf("Child2: Failed to map memory\n");
            }
            exit(0);
        } else {
            wait(&child_status);
            printf("Parent: Second child completed\n");
        }
        
        print_proc_size("Parent final", parent_pid);
        
        free(local_buffer);
        printf("=== Test Complete ===\n");
    }
    
    return 0;
}