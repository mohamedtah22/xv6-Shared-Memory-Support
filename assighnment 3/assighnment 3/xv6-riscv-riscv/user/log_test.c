// user/log_test.c

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGSIZE       4096
#define HEADER_SIZE  4

int
main(void) {
    const int num_children = 4;
    char *raw = sbrk(2*PGSIZE);
    char *shared_buf = raw == (char*)-1 ? 0 : (char*)(((uint64)raw+PGSIZE-1)&~(uint64)(PGSIZE-1));
    if (shared_buf == 0) {
        printf("malloc failed\n");
        exit(1);
    }
    memset(shared_buf, 0, PGSIZE);
    int parent_pid = getpid();
    printf("Parent: allocated shared buffer at %p\n", shared_buf);

    for (int i = 0; i < num_children; i++) {
        int pid = fork();
        if (pid < 0) {
            printf("fork failed\n");
            exit(1);
        }
        if (pid == 0) {
            if(map_shared_pages(parent_pid, getpid(), shared_buf, PGSIZE) == (uint64)-1){
                printf("child map_shared_pages failed\n");
                exit(1);
            }
            // Child: write its log entry
            int idx = i + 1;
            sleep(idx);  // stagger output

            char msg[32] = "Hello from child ";
            int len = strlen(msg);
            msg[len++] = '0' + idx;
            msg[len] = '\0';

            // Scan for a free slot in the shared buffer
            char *addr = shared_buf;
            while ((uint64)(addr + HEADER_SIZE + len)
                   <= (uint64)(shared_buf + PGSIZE)) {
                uint32 old = *(uint32*)addr;
                if (old == 0) {
                    uint32 hdr = (idx << 16) | len;
                    if (__sync_val_compare_and_swap((uint32*)addr, 0, hdr) == 0) {
                        memmove(addr + HEADER_SIZE, msg, len);
                        printf("Child %d wrote: %s at %p\n",
                               idx, msg, addr);
                        break;
                    }
                }
                // advance to next aligned slot
                uint64 next = ((uint64)addr + HEADER_SIZE + len + 3) & ~3ULL;
                addr = (char*)next;
            }
            exit(0);
        }
    }

    // Parent waits for all children
    for (int i = 0; i < num_children; i++) {
        wait(0);
    }

    // small delay to ensure all writes are visible
    sleep(2);

    int seen = 0;
    // Parent reads back the log entries
    printf("\nParent reading messages:\n");
    char *addr = shared_buf;
    while ((uint64)(addr + HEADER_SIZE) <= (uint64)(shared_buf + PGSIZE)) {
        uint32 hdr = *(uint32*)addr;
        if (hdr == 0)
            break;
        int idx = hdr >> 16;
        int mlen = hdr & 0xFFFF;
        char buf[64];
        if(idx < 1 || idx > num_children || mlen >= sizeof(buf) ||
           (uint64)(addr + HEADER_SIZE + mlen) > (uint64)(shared_buf + PGSIZE) ||
           (seen & (1 << idx))){
            printf("LOG_TEST: FAIL (invalid record)\n");
            exit(1);
        }
        seen |= 1 << idx;
        memmove(buf, addr + HEADER_SIZE, mlen);
        buf[mlen] = '\0';
        printf("Message from child %d: %s\n", idx, buf);
        uint64 next = ((uint64)addr + HEADER_SIZE + mlen + 3) & ~3ULL;
        addr = (char*)next;
    }

    if(seen != 0x1e){ printf("LOG_TEST: FAIL (missing records)\n"); exit(1); }
    printf("LOG_TEST: PASS\n");
    exit(0);
}

