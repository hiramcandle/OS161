#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>

#include "opt-A2.h"
#if OPT_A2
#include <kern/limits.h>
#include <vfs.h>
#include <kern/fcntl.h>
#include <mips/trapframe.h>
#include <copyinout.h>
#include <limits.h>
#include <test.h>

#endif

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
/*
  #if OPT_A2
  int encode = _MKWAIT_EXIT(exitcode);
  curproc_updateexitcode(encode);
  #else
*/
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  (void)exitcode;

  #endif

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{


  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  *retval = 1;
  return(0);
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */

  if (options != 0) {
     return(EINVAL);
  }


  /* for now, just pretend the exitstatus is 0 */
  exitstatus = 0;


  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

#if OPT_A2
int sys_fork(struct trapframe *tf, pid_t *retval) {
//    KASSERT(tf);
    struct trapframe *tem = kmalloc(sizeof(*tem));
    memcpy(tem, tf, sizeof(struct trapframe));
//  if(proc_nospace()) {
//    return(ENPROC);
//  }

    struct addrspace *caddr = as_create();
//  if (!caddr) {
//    return(ENOMEM);
//  }
//    struct addrspace *tem;

    struct proc *cproc = proc_create_runprogram(curproc->p_name);
    if (cproc == NULL) {
        return(ENOMEM);
    }
    as_copy(curproc_getas(), &caddr);

    spinlock_acquire(&cproc->p_lock);
    *retval = cproc->p_id;
    cproc->p_addrspace = caddr;
    cproc->p_parent = curproc;
    spinlock_release(&cproc->p_lock);


  //proc_setas(cproc, caddr);
  
  curproc_add(cproc->p_id);
  //curproc_changeparent(cproc);


  thread_fork(cproc->p_name, cproc, thread_entrypoint, tem, 0);

  return(0);
}


int sys_execv(const userptr_t program,userptr_t args) {

    size_t len = 0;
    char buf[PATH_MAX];
    char * ptrs[64];

    int readin = 0;
    int counter = 0;
    int result;

    result = copyinstr(program, buf, PATH_MAX , &len);
    if(result) return result;
    readin += len;


    while(true) {
	userptr_t tem;
	result = copyin(args + sizeof(userptr_t) * counter, &tem, sizeof(userptr_t));
	if(result) return result;
 
	if(tem == NULL) break;
        result = copyinstr(tem , buf + readin, PATH_MAX - readin, &len);
        if(result) return result;
        
        ptrs[counter] = buf+readin;
        readin += len;
	counter++;
    }

    result = runprogram(buf, ptrs, counter);
    return result;

}

#endif



