; boot.s 内核从这里开始

; GNU mutilboot 规范的相关内容
MBOOT_HEADER_MAGIC  equ     0x1BADB002    ; 魔数，规范决定的
MBOOT_PAGE_ALIGN    equ     1 << 0        ; 引导模块按照 4KB 边界对齐
MBOOT_MEM_INFO      equ     1 << 1        ; 告诉 mutilboot 把内存空间的信息包含在 mutilboot 结构体中
MBOOT_HEADER_FLAGS  equ     MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO         ; 定义使用的 mutilboot 标记
MBOOT_CHECKSUM      equ     -(MBOOT_HEADER_MAGIC+MBOOT_HEADER_FLAGS)

; 符合 Multiboot 规范的 OS 映象需要这样一个 magic Multiboot 头
; Multiboot 头的分布必须如下表所示：
; ----------------------------------------------------------
; 偏移量  类型  域名        备注
;
;   0     u32   magic       必需
;   4     u32   flags       必需 
;   8     u32   checksum    必需 
;
; 我们只使用到这些就够了，更多的详细说明请参阅 GNU 相关文档
;-----------------------------------------------------------


[BITS 32]   ; 以 32-bit 方式编译
section .init.text      ; 临时代码段开始

dd MBOOT_HEADER_MAGIC   ; GRUB 通过魔数判断该映像是否支持
dd MBOOT_HEADER_FLAGS   ; GRUB 的一些加载选项
dd MBOOT_CHECKSUM       ; 

[GLOBAL start]          ; 提供给链接器，声明内核代码入口
[GLOBAL mboot_ptr_tmp]  ; 声明 struct mutiboot * 变量
[EXTERN kern_entry]     ; 声明内核 C 代码的入口函数

; 按照协议 GRUB 把一些计算机硬件和我们内核文件相关的信息放在了一个结构体中
; 并且将这个结构体指针放在了 ebx 寄存器中
start:
  cli                       ; 此时还没有设置好保护模式的中断处理，需要关闭中断
  mov [mboot_ptr_tmp], ebx  ; ebx 存储的地址存入全局变量
  mov esp, STACK_TOP        ; 设置内核栈地址
  mov ebp, 0                ; 帧指针改为 0
  and esp, 0ffffff0H        ; 栈地址 16 字节对齐
  call kern_entry           ; 调用内核入口函数

section .init.data          ; 开启分页之前临时的数据段
stack: times 1024 db 0      ; 临时的内核栈
STACK_TOP equ $-stack-1     ; 内核栈顶，$ 指代当前地址
mboot_ptr_tmp: dd 0         ; 全局的 mutilboot 指针