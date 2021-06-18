#include "task.h"
#include "debug.h"
#include "gdt.h"
#include "heap.h"
#include "mstring.h"
#include "pmm.h"
#include "msched.h"
#include "vmm.h"

pid_t now_pid = 0;

/** 
 * 内核线程创建
 * 构造一个切换后可以弹出执行地址的初始栈，以便在第一次切换的时候可以有指令可以执行
 */
int32_t kernel_thread(int (*fn)(void *), void *arg) {
  struct task_struct *new_task = (struct task_struct *)kmalloc(STACK_SIZE);
  assert(new_task != NULL, "kern_thread: kmalloc error");
  bzero(new_task, sizeof(struct task_struct));

  new_task->state = TASK_RUNNABLE;
  new_task->stack = current;  /* 内核栈 */
  new_task->pid = now_pid++;
  new_task->mm = NULL;

  /**
   * 线程私有栈从分配 new_task 的堆上取了 STACK_SIZE 的空间，连接在 new_task 后面
   * 所以栈空间还是从高地址往低地址扩展的
   */
  uint32_t *stack_top = (uint32_t *)((uint32_t)new_task + STACK_SIZE);
  *(--stack_top) = (uint32_t)arg;           /* fn 参数 */
  *(--stack_top) = (uint32_t)kthread_exit;  /* 调用 fn 压栈的下一条指令 */
  *(--stack_top) = (uint32_t)fn;            /* fn 地址，在第一次调用的时候由线程切换函数的 ret 指令执行 */

  /* 设置 esp 寄存器为栈顶 */
  new_task->context.esp = (uint32_t)new_task + STACK_SIZE - sizeof(uint32_t) * 3;
  new_task->context.eflags = 0x200;

  /* 插入队列 */
  new_task->next = running_proc_head;
  struct task_struct *tail = running_proc_head;
  assert(tail != NULL, "Must init sched!");
  while(tail->next != running_proc_head) {
    tail = tail->next;
  }
  tail->next = new_task;

  return new_task->pid;
}

void kthread_exit() {
  register uint32_t val asm ("eax");
  printk("Thread exited with value %d\n", val);
  while(1);
}