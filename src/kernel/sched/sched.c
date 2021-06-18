#include "msched.h"
#include "heap.h"
#include "debug.h"

struct task_struct *running_proc_head = NULL;
struct task_struct *wait_proc_head = NULL;
struct task_struct *current = NULL;

void init_sched() {
  /**
   * 为当前执行流创建进程描述符，该结构位于当前执行流的栈的最底端
   * 第一个线程（可以看成是进程）所使用的栈空间是内核栈，进程描述符也是在栈上分配的，位于栈的最底端
   */
  current = (struct task_struct *)(kern_stack_top - STACK_SIZE);

  current->state = TASK_RUNNABLE;
  current->pid = now_pid++;
  current->stack = current;   /* 设置栈底，使用的就是前面 entry.c 初始化的内核栈 */
  current->mm = NULL;

  current->next = current;    /* 单向循环链表 */
  running_proc_head = current;
}

void schedule() {
  if(current)
    change_task_to(current->next);
}

void change_task_to(struct task_struct *next) {
  if(current != next) {
    struct task_struct *prev = current;
    current = next;
    /* 寄存器切换 */
    switch_to(&(prev->context), &(current->context));
  }
}

