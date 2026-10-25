#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
extern struct proc proc[]; // Add this line to declare proc array
extern uint64 map_shared_pages(struct proc*, struct proc*, uint64, uint64);
extern uint64 unmap_shared_pages(struct proc*, uint64, uint64);

uint64
sys_map_shared_pages(void)
{
  int src_pid, dst_pid;
  uint64 src_va, size;
  argint(0, &src_pid);
  argint(1, &dst_pid);
  argaddr(2, &src_va);
  argaddr(3, &size);
  struct proc *dst = myproc();
  // A process may replace only its own mappings. No two-proc lock cycle.
  if(dst_pid != dst->pid || src_pid == dst_pid)
    return -1;
  for(struct proc *src = proc; src < &proc[NPROC]; src++){
    acquire(&src->lock);
    if(src->pid == src_pid &&
       (src->state == RUNNING || src->state == RUNNABLE || src->state == SLEEPING)){
      vmspace_acquire();
      uint64 result = map_shared_pages(src, dst, src_va, size);
      vmspace_release();
      release(&src->lock);
      return result;
    }
    release(&src->lock);
  }
  return -1;
}

uint64
sys_unmap_shared_pages(void)
{
  uint64 addr, size;
  argaddr(0, &addr);
  argaddr(1, &size);
  vmspace_acquire();
  uint64 result = unmap_shared_pages(myproc(), addr, size);
  vmspace_release();
  return result;
}

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}





