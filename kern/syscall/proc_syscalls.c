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

  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */
/*
#if OPT_A2
int curproc_childcheck(pid_t pid) {
  int result = 0;
  
  struct proc *proc = curproc;
  spinlock_acquire(&proc->p_lock);
  unsigned num = proc_infoarray_num(&proc->p_children);
  for (int i = 0; i < num; i++) {
    if (proc_infoarray_get(&proc->p_children, i)->pid == pid) {
      result = 1;
      break;
    }
  }	
  spinlock_release(&proc->p_lock);
  return result;
}


struct addrspace *proc_setas(struct proc *p, struct addrspace *as) {
	struct addrspace *tem;
	spinlock_acquire(&p->p_lock);
	tem = p->p_addrspace;
        p->p_addrspace = as;
 	spinlock_release(&p->p_lock);
 	return tem;
 }

int proc_exist(pid_t pid) {
  int r = 1;
  P(proc_count_mutex);
  if (proc_ids[pid] == 'c') r = 0;
       
  V(proc_count_mutex);
        
  return r;
}


int curproc_childexitcode(pid_t pid) {
	int r;
	struct proc *proc = curproc;
	struct proc_info *cpinfo;
	
	spinlock_acquire(&proc->p_lock);
	unsigned num = proc_infoarray_num(&proc->p_children);
	for (unsigned i = 0; i < num; i++) {
		cpinfo = proc_infoarray_get(&proc->p_children, i); 
                if (cpinfo->pid == pid) {
                        break;
                }
        }
	spinlock_release(&proc->p_lock);
		
	P(cpinfo->waitsem);
	r = cpinfo->exitcode;
	return r;
}


void curproc_removechild(pid_t pid) {
        struct proc *proc = curproc;

	spinlock_acquire(&proc->p_lock);
        unsigned num = proc_infoarray_num(&proc->p_children);
      
	for (unsigned i = 0; i < num; i++) {
                if (proc_infoarray_get(&proc->p_children, i)->pid == pid) {
			proc_infoarray_remove(&proc->p_children, i);		
                        break;
                }
        }
        spinlock_release(&proc->p_lock);
	P(proc_count_mutex);
	if (proc_ids[pid] == 'z') {
		proc_ids[pid] = 'c';
                if (proc_min_id > pid) proc_min_id = pid;
        }
	V(proc_count_mutex);
}


void curproc_updateexitcode(int code) {
	P(proc_count_mutex);
        
	struct proc *p = curproc;
	struct proc *proc = p->p_parent;
	if (proc != NULL && proc->p_id >= PID_MIN && proc_ids[proc->p_id] == 'u') {
		pid_t pid = p->p_id;
        	spinlock_acquire(&proc->p_lock);
        	unsigned num = proc_infoarray_num(&proc->p_children);
        	for (unsigned i = 0; i < num; i++) {
                	if (proc_infoarray_get(&proc->p_children, i)->pid == pid) {
                        	proc_infoarray_get(&proc->p_children, i)->exitcode = code;
                        	break;
                	}
        	}
		spinlock_release(&proc->p_lock);
	}
	V(proc_count_mutex);
}


int curproc_add(pid_t pid) {
	int result = 0;
	struct proc_info *child = kmalloc(sizeof(struct proc_info));
	if (child == NULL) return(ENOMEM);

	child->pid = pid;
	child->exitcode = 0;
	child->waitsem = sem_create("waitsem", 0);
        if (child->waitsem == NULL) return(ENOMEM);

 
	struct proc *proc = curproc;
	
	spinlock_acquire(&proc->p_lock);
	result = proc_infoarray_add(&proc->p_children, child, NULL);
	spinlock_release(&proc->p_lock);
	return result;
}

void curproc_changeparent(struct proc *child) {
	spinlock_acquire(&child->p_lock);
	child->p_parent = curproc;
	spinlock_release(&child->p_lock);
}


#endif
*/
void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;

  #if OPT_A2
  int encode = _MKWAIT_EXIT(exitcode);
  curproc_updateexitcode(encode);
  #else

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

  #if OPT_A2
  KASSERT(retval);
  *retval = curproc->p_id;
  #else

  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  *retval = 1;
  #endif
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

  #if OPT_A2
  if ( !proc_exist(pid) ) {
    return(ESRCH);
  }

  if (!curproc_childcheck(pid)) {
    return(ECHILD);
  }
  
  exitstatus = curproc_childexitcode(pid);
  curproc_removechild(pid);
  #else

  /* for now, just pretend the exitstatus is 0 */
  exitstatus = 0;

  #endif

  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

#if OPT_A2
int sys_fork(struct trapframe *tf, pid_t *retval) {
  KASSERT(tf);

  if(proc_nospace()) {
    return(ENPROC);
  }

  struct addrspace *caddr = as_create();
  if (!caddr) {
    return(ENOMEM);
  }

  if (as_copy(curproc_getas(), &caddr)){
    return(ENOMEM);
  }

  struct proc *cproc = proc_create_runprogram(curproc->p_name); 
  if (!cproc) {
    return(ENOMEM);
  }

  spinlock_acquire(&cproc->p_lock);
  *retval = cproc->p_id;
  spinlock_release(&cproc->p_lock);

  proc_setas(cproc, caddr);
  
  curproc_add(cproc->p_id);
  curproc_changeparent(cproc);

  struct trapframe *tem = kmalloc(sizeof(*tem));;
  memcpy(tem, tf, sizeof(struct trapframe));
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
    ptrs[counter] = buf;
    counter++;


    while(true) {
        result = copyinstr(args + sizeof(userptr_t) * counter, buf+readin, PATH_MAX - readin, &len);
        if(result) return result;
        if(len == 0) break;
        readin += len;
        ptrs[counter] = buf+readin;
        counter++;
    }

    result = runprogram(buf, ptrs, counter);
    return result;


}

#endif



