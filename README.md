# xv6 shared-memory kernel extension

A C/RISC-V extension of MIT xv6 with shared-page syscalls, reference-counted
physical frames, and QEMU kernel regression tests. The original xv6 source is
under `assighnment 3/assighnment 3/xv6-riscv-riscv/`; its existing MIT license
remains there. The restored `mkfs/mkfs.c` is attributed to upstream MIT xv6.

## Contract

```c
uint64 map_shared_pages(int source_pid, int destination_pid, void *address, uint64 size);
int unmap_shared_pages(void *address, uint64 size);
```

Destination must be the calling process and must differ from source. The source
must be a live process. Both address spaces must already reserve the requested
range at the same virtual address. The result preserves the byte offset; failure
returns `-1`. The whole covered page range is shared, so callers should use
page-aligned dedicated allocations rather than allocator metadata.

This is an educational API. Source-process lookup does not implement a
production permission or shared-memory-handle system.

## Memory safety and concurrency

- Check zero size, address overflow, process bounds, user permissions, and all
  source/destination page-table paths before changing a mapping.
- Commit makes no allocations: replacing the validated leaf PTEs cannot fail
  halfway through and never needs to unmap uninstalled pages during rollback.
- Each alias retains a physical-frame reference before releasing its old
  destination mapping. Owner exit/reaping therefore cannot free a live alias.
- Unmap validates the entire shared range, releases its references, and flushes
  local translations. Repeated unmap is rejected.
- User ELF segments explicitly separate executable/read-only text from writable
  data; the linker-script dependency is tracked during rebuilds.
- Exit and fork tolerate unmapped holes. Fork copies remaining pages privately
  and clears the shared marker on those new allocations.
- A coarse VM spinlock serializes published page-table changes in map/unmap,
  grow, fork, exec publication, and process reaping. Source process lock precedes
  VM lock; mapping no longer takes two process locks in arbitrary order.
- After unmap, `sz` remains the address-space upper bound. Accessing a hole faults
  until an explicit mapping fills it. No demand paging or copy-on-write is added.

## Tests

```bash
sudo apt-get install gcc-riscv64-linux-gnu qemu-system-misc
python3 scripts/test_qemu.py --cpus 1 --usertests
python3 scripts/test_qemu.py --cpus 3
```

The runner builds a clean image, boots the real kernel under QEMU system
emulation, checks serial output, and fails on a panic, failed assertion, early
exit, or timeout. The tests cover bidirectional visibility, invalid/overflowing
ranges, preservation after rejected mapping, caller-only destinations, repeated
alias replacement, double unmap, fork/exit with holes, private fork semantics,
and a borrower that continues using a frame after its owner is reaped. The
four-process CAS log asserts all four bounded records are present.

GitHub Actions runs the 1/3-vCPU matrix and the baseline `usertests -q` suite.

## Scope

This demonstrates page-table manipulation, system-call integration, memory
ownership, process lifecycle, lock ordering, and atomic operations. The VM lock
is intentionally coarse; it favors a small auditable educational implementation
over production scalability. This project does not add a network driver.
