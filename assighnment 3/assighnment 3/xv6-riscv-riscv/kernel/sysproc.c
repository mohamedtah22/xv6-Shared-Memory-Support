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
  struct proc *src_proc, *dst_proc;
  
  // Get arguments from user space
  argint(0, &src_pid);
  argint(1, &dst_pid);
  argaddr(2, &src_va); 
  argaddr(3, &size);
   
  
  // Find source process
  src_proc = 0;
  for(struct proc *p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->state != UNUSED && p->pid == src_pid){
      src_proc = p;
      break;
    }
    release(&p->lock);
  }
  
  if(src_proc == 0)
    return -1;
  
  // Find destination process
  dst_proc = 0;
  for(struct proc *p = proc; p < &proc[NPROC]; p++){
    if(p != src_proc) acquire(&p->lock);
    if(p->state != UNUSED && p->pid == dst_pid){
      dst_proc = p;
      break;
    }
    if(p != src_proc) release(&p->lock);
  }
  
  if(dst_proc == 0){
    release(&src_proc->lock);
    return -1;
  }
  
  // Call kernel function
  uint64 result = map_shared_pages(src_proc, dst_proc, src_va, size);
  
  // Release locks
  release(&src_proc->lock);
  if(dst_proc != src_proc)
    release(&dst_proc->lock);
  
  return result;
}

uint64
sys_unmap_shared_pages(void)
{
  uint64 addr, size;
  
  // Get arguments from user space
  argaddr(0, &addr);  
  argaddr(1, &size);
    
  
  // Call kernel function for current process
  return unmap_shared_pages(myproc(), addr, size);
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




