إليك ملف README.md احترافي ومختصر، مصمم خصيصاً لمشروعك بناءً على متطلبات الوظيفة التي عملت عليها. هذا الملف سيجعل المستودع (Repository) الخاص بك يبدو ممتازاً لأي صاحب عمل أو مبرمج يزور صفحتك.

يمكنك نسخ النص التالي ووضعه داخل ملف جديد باسم README.md في مجلد المشروع:

xv6 Shared Memory & Multi-process Logging System
This project implements Shared Memory capabilities within the xv6 operating system and utilizes them to build a synchronized multi-process logging system.

Overview
The goal of this project was to extend the xv6 kernel to allow multiple processes to share the same physical memory pages and coordinate access to a shared buffer using atomic operations.


Features Implemented
1. Memory Sharing API 



map_shared_pages: A kernel function that maps physical pages from a source process to a destination process's virtual address space.


Virtual Memory Management: Used walk() and mappages() to handle page table entries (PTEs) and managed page alignment using PGROUNDDOWN and PGROUNDUP.




Memory Safety: Introduced a custom PTE_S flag (shared page) to ensure physical pages are only freed by their original owners during process exit.


2. Multi-process Logging 


Shared Logging Buffer: A shared memory region accessible by multiple child processes for writing log messages.



Atomic Operations: Implemented a synchronization protocol using __sync_val_compare_and_swap to ensure atomic header writes and prevent race conditions.



Memory Alignment: Managed a 32-bit header (child index and message length) and enforced 4-byte alignment for all buffer entries.


Project Structure

kernel/vm.c: Core memory mapping and unmapping logic.



shmem_test.c: User-space program to verify shared memory mapping and process size (sz) management.



log_test.c: Demonstration of concurrent logging where multiple children write to a shared buffer and the parent reads them.


How to Run
Clean the build: make clean.

Compile and run xv6: make qemu.

Run tests inside xv6:

shmem_test

log_test
