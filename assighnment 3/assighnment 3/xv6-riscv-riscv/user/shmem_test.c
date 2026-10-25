#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define PAGE 4096
#define FAIL ((uint64)-1)
static void check(int ok, char *message) {
  if(!ok){ printf("SHMEM FAIL: %s\n", message); exit(1); }
}
static char *pages(int n) {
  char *base = sbrk((n+1)*PAGE);
  check(base != (char *)-1, "allocate pages");
  return (char *)(((uint64)base + PAGE - 1) & ~(uint64)(PAGE-1));
}
static void wait_ok(void) {
  int status = -1;
  check(wait(&status) >= 0 && status == 0, "child exit status");
}
static void visibility_and_validation(void) {
  char *p = pages(2);
  p[0] = 'A'; p[PAGE] = 'B';
  int owner = getpid();
  int pid = fork();
  check(pid >= 0, "fork visibility");
  if(pid == 0){
    check(map_shared_pages(owner,getpid(),p,0) == FAIL, "zero size");
    check(map_shared_pages(getpid(),getpid(),p,PAGE) == FAIL, "self mapping");
    check(map_shared_pages(owner,owner,p,PAGE) == FAIL, "remote destination");
    check(map_shared_pages(-1,getpid(),p,PAGE) == FAIL, "unknown owner");
    check(map_shared_pages(owner,getpid(),(void *)(FAIL-16),64) == FAIL, "overflow");
    char *last = sbrk(0)-PAGE;
    last[0] = 'Z';
    check(map_shared_pages(owner,getpid(),last,2*PAGE) == FAIL, "invalid range end");
    check(last[0] == 'Z', "invalid range is atomic");
    check(map_shared_pages(owner,getpid(),p,2*PAGE) == (uint64)p, "map two pages");
    check(p[0] == 'A' && p[PAGE] == 'B', "owner data visible");
    p[0] = 'C';
    check(unmap_shared_pages(p,PAGE) == 0, "unmap first page");
    check(unmap_shared_pages(p,PAGE) == -1, "reject double unmap");
    int grandchild = fork();
    check(grandchild >= 0, "fork with a hole");
    if(grandchild == 0){
      check(p[PAGE] == 'B', "fork copied remaining page");
      p[PAGE] = 'D';
      check(unmap_shared_pages(p+PAGE,PAGE) == -1, "fork copy is private");
      exit(0);
    }
    wait_ok();
    check(p[PAGE] == 'B', "fork isolation");
    check(map_shared_pages(owner,getpid(),p,PAGE) == (uint64)p, "remap hole");
    check(p[0] == 'C', "remapped frame retained");
    for(int i=0;i<200;i++)
      check(map_shared_pages(owner,getpid(),p,PAGE) == (uint64)p, "repeat alias replacement");
    check(unmap_shared_pages(p,2*PAGE) == 0, "release both references");
    exit(0); // kernel cleanup must tolerate the holes
  }
  wait_ok();
  check(p[0] == 'C' && p[PAGE] == 'B', "cross-process visibility and owner survival");
  printf("SHMEM validation/visibility/fork/repeat/unmap: PASS\n");
}
static void owner_lifetime(void) {
  int report[2], gate[2];
  check(pipe(report) == 0 && pipe(gate) == 0, "lifetime pipes");
  int owner = fork();
  check(owner >= 0, "fork owner");
  if(owner == 0){
    char *p = pages(1); p[0] = 'Q';
    int ready[2]; check(pipe(ready) == 0, "owner ready pipe");
    int source = getpid(), alias = fork();
    check(alias >= 0, "fork borrower");
    if(alias == 0){
      check(map_shared_pages(source,getpid(),p,PAGE) == (uint64)p, "borrow before owner exit");
      check(write(ready[1],"M",1) == 1, "ready signal");
      check(write(report[1],"M",1) == 1, "mapped report");
      char c; check(read(gate[0],&c,1) == 1, "wait for owner reap");
      check(p[0] == 'Q', "frame survives owner reap");
      p[0] = 'R';
      check(unmap_shared_pages(p,PAGE) == 0, "release final reference");
      check(write(report[1],"P",1) == 1, "lifetime pass report");
      exit(0);
    }
    char c; check(read(ready[0],&c,1) == 1, "borrower ready");
    exit(0);
  }
  char c; check(read(report[0],&c,1) == 1 && c == 'M', "borrower mapped");
  wait_ok(); // owner page table is now freed
  char *pressure = pages(8);
  memset(pressure,0x5a,8*PAGE);
  check(write(gate[1],"G",1) == 1, "release borrower");
  check(read(report[0],&c,1) == 1 && c == 'P', "owner lifetime result");
  close(report[0]); close(report[1]); close(gate[0]); close(gate[1]);
  printf("SHMEM owner-exit lifetime: PASS\n");
}
int main(void) {
  visibility_and_validation();
  owner_lifetime();
  printf("SHMEM_REGRESS: PASS\n");
  exit(0);
}
