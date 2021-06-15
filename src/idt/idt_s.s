; nasm 宏定义语法： %macro 宏名字 参数个数，另外 %n 代表第 n 个参数

; 定义两个宏来统一处理有错误代码和没有错误代码的中断，以便在清理的时候不需要分类
; 函数的参数是中断号，用来声明和定义该中断号的中断处理函数
; 主要体现在没有错误代码的中断 push 0 来占用一个错误代码位置
%macro ISR_NOERRCODE 1
[GLOBAL isr%1]          ; 声明一个函数
isr%1:                  ; 定义该声明的函数
  cli                   ; 关闭中断
  push 0                ; push 无效的中断错误代码
  push %1               ; push 中断号
  jmp isr_common_stub   ; 中断处理函数的公共部分
%endmacro

%macro ISR_ERRCODE 1
[GLOBAL isr%1]
isr%1:
  cli
  push %1
  jmp isr_common_stub
%endmacro

; 使用上面声明的宏定义中断处理函数
ISR_NOERRCODE  0    ; 0 #DE 除 0 异常
ISR_NOERRCODE  1    ; 1 #DB 调试异常
ISR_NOERRCODE  2    ; 2 NMI
ISR_NOERRCODE  3    ; 3 BP 断点异常 
ISR_NOERRCODE  4    ; 4 #OF 溢出 
ISR_NOERRCODE  5    ; 5 #BR 对数组的引用超出边界 
ISR_NOERRCODE  6    ; 6 #UD 无效或未定义的操作码 
ISR_NOERRCODE  7    ; 7 #NM 设备不可用(无数学协处理器) 
ISR_ERRCODE    8    ; 8 #DF 双重故障(有错误代码) 
ISR_NOERRCODE  9    ; 9 协处理器跨段操作
ISR_ERRCODE   10    ; 10 #TS 无效TSS(有错误代码) 
ISR_ERRCODE   11    ; 11 #NP 段不存在(有错误代码) 
ISR_ERRCODE   12    ; 12 #SS 栈错误(有错误代码) 
ISR_ERRCODE   13    ; 13 #GP 常规保护(有错误代码) 
ISR_ERRCODE   14    ; 14 #PF 页故障(有错误代码) 
ISR_NOERRCODE 15    ; 15 CPU 保留 
ISR_NOERRCODE 16    ; 16 #MF 浮点处理单元错误 
ISR_ERRCODE   17    ; 17 #AC 对齐检查 
ISR_NOERRCODE 18    ; 18 #MC 机器检查 
ISR_NOERRCODE 19    ; 19 #XM SIMD(单指令多数据)浮点异常

; 20 ~ 31 Intel 保留
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31
; 32 ～ 255 用户自定义
ISR_NOERRCODE 255


; 所有中断处理函数共有的保护现场操作

[GLOBAL isr_common_stub]
[EXTERN isr_handler]
; 中断服务程序
isr_common_stub:
  pusha             ; push edi, esi, ebp, esp, ebx, edx, ecx, eax
  mov ax, ds
  push eax          ; 保存数据段描述符
  
  mov ax, 0x10      ; 加载内核数据段描述符表
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

  push esp          ; 此时 esp 寄存器里面的值等价于 pt_regs 结构体指针
  call isr_handler  ; 每个中断处理函数自己的逻辑部分
  add esp, 4        ; 下面开始恢复寄存器的值

  pop ebx;          ; 恢复原来的数据段描述符
  mov ds, bx
  mov es, bx
  mov fs, bx
  mov gs, bx
  mov ss, bx
  popa              ; pop edi, esi, ebp, esp, ebx, edx, ecx, eax
  add esp, 8        ; 清理栈里面的 error code 和 isr
  iret
.end:

[GLOBAL idt_flush]
idt_flush:
  mov eax, [esp + 4]  ; 和设置 GDTR 的时候一样先获取参数，也就是传递进来的中断描述符表地址
  lidt [eax]          ; 设置 IDTR 寄存器
  ret
.end: