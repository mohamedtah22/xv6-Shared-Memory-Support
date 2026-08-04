# xv6 Shared-Memory Support

A kernel-level extension to MIT xv6-riscv that allows processes to map the same physical user pages into their address spaces.

The implementation adds two system calls, a shared-page PTE flag, lifecycle-aware unmapping, and user-space programs that verify cross-process visibility and exercise a concurrent shared log.

## Project Scope

This repository focuses on:

- mapping an existing user-memory range from one process into another;
- preserving the source virtual address in the destination process;
- ensuring shared pages are not freed by the destination during unmapping;
- exposing the functionality through xv6 system calls;
- validating the implementation with **shmem_test** and **log_test**.

## How the Mapping Works

~~~mermaid
flowchart TD
    A["Source process virtual page"] --> B["Physical page frame"]
    C["Destination process same virtual address"] --> B
    D["Destination PTE marked PTE_S"] --> C
    E["Unmap removes PTE without freeing frame"] --> D
~~~

The destination receives page-table entries that reference the source process's existing physical frames. The mapping keeps the original page offset, rounds the covered range to page boundaries internally, and copies the original access flags while adding **PTE_S**.

## User API

The following declarations are exposed in **user/user.h**:

~~~c
int map_shared_pages(int src_pid, int dst_pid, void *src_va, uint64 size);
int unmap_shared_pages(void *addr, uint64 size);
~~~

### map_shared_pages

Maps the source process range **[src_va, src_va + size)** into the destination process at the same virtual address.

- Returns the mapped virtual address on success.
- Returns **-1** when the process or mapping request is invalid.
- Replaces destination mappings in the target range before installing the shared mappings.
- Rolls back pages installed during the current operation if a later page cannot be mapped.

### unmap_shared_pages

Removes a shared mapping from the calling process.

- Verifies that every covered page is valid and marked shared.
- Removes the page-table entries without freeing the underlying physical frames.
- Returns **0** on success and **-1** for an invalid range.

## Kernel Changes

| File | Responsibility |
|---|---|
| **kernel/riscv.h** | Defines **PTE_S**, the software-managed shared-page flag |
| **kernel/vm.c** | Implements page mapping/unmapping and shared-page-aware cleanup |
| **kernel/sysproc.c** | Resolves source/destination processes and handles syscall arguments |
| **kernel/syscall.c** | Connects syscall numbers to kernel handlers |
| **kernel/syscall.h** | Assigns syscall numbers 23 and 24 |
| **kernel/defs.h** | Publishes kernel function declarations |
| **kernel/proc.c** | Includes process lookup support |
| **user/user.h** | Publishes the user-facing API |
| **user/usys.pl** | Generates the user-space syscall stubs |
| **user/shmem_test.c** | Functional shared-memory test |
| **user/log_test.c** | Multi-process concurrent-log test |
| **Makefile** | Builds the two user test programs into the xv6 image |

All xv6 source files are located under:

~~~text
assighnment 3/assighnment 3/xv6-riscv-riscv/
~~~

The directory spelling above matches the repository exactly.

## Tests

### shmem_test

The shared-memory test exercises the core contract:

1. A parent allocates and initializes a page.
2. A child maps the parent's page.
3. The child reads the parent's data and writes a new value.
4. The parent observes the child's write through its original mapping.
5. The child unmaps the shared page and continues allocating memory.
6. Another child maps the page again to verify reuse.

This checks visibility in both directions, explicit unmapping, and repeat mapping across processes.

### log_test

The concurrent-log test creates four child processes and maps one parent-owned page into all of them. Each child:

- searches the page for a free log slot;
- reserves a 32-bit record header with an atomic compare-and-swap;
- stores its child index and message length in the header;
- writes an aligned record into the shared page.

After all children exit, the parent reads the records from its own mapping. This test combines shared-memory visibility with a small lock-free reservation protocol.

## Build and Run

### Requirements

You need a standard xv6-riscv development environment:

- a RISC-V cross compiler and binutils;
- QEMU with RISC-V system emulation;
- GNU Make;
- a Unix-like host.

### Start xv6

~~~console
$ cd "assighnment 3/assighnment 3/xv6-riscv-riscv"
$ make clean
$ make qemu
~~~

At the xv6 shell, run:

~~~console
$ shmem_test
$ log_test
~~~

To exit QEMU, press **Ctrl-a**, then **x**.

## Implementation Details

### Shared-Page Ownership

Standard xv6 unmapping may release the physical page. That behavior is unsafe for an alias owned by another process. The modified **uvmunmap** checks **PTE_S** and removes a shared destination mapping without calling **kfree** for that frame.

### Address and Flag Preservation

**map_shared_pages**:

- rounds the start address down and the end address up to page boundaries;
- walks the source page table to obtain each physical frame;
- preserves the source PTE permissions;
- adds the **PTE_S** marker;
- maps the pages at the corresponding destination virtual addresses;
- returns the original unrounded source address so the caller retains its byte offset.

### Failure Handling

The implementation rejects missing processes, invalid or unmapped source pages, zero-sized requests, and invalid unmap ranges. If a multi-page mapping fails partway through, pages already installed by that call are removed.

## What This Project Demonstrates

- xv6 kernel development;
- RISC-V Sv39 page-table manipulation;
- system-call design from kernel handler to user stub;
- virtual-to-physical mapping and PTE flags;
- memory ownership and page lifecycle reasoning;
- process-table synchronization;
- atomic compare-and-swap in a shared-memory workload;
- kernel/user integration testing.

## Limitations

This is a focused educational extension, not a general-purpose shared-memory subsystem. Mappings use the same virtual address in both processes, and the source process remains the owner of the underlying allocation. Applications must coordinate access to shared data and respect the mapped range's lifetime.
