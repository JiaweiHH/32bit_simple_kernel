<!--从零开始编写一个能够在 qemu 上运行的小型内核-->

> 开发环境：qemu-system-i386、gcc、nasm、intel 汇编、AT&T 汇编、make
>
> [Bran's Kernel Development Tutorial: Introduction (osdever.net)](http://www.osdever.net/bkerndev/Docs/intro.htm)；
> [NASM Tutorial (lmu.edu)](https://cs.lmu.edu/~ray/notes/nasmtutorial/)；
> [JamesM's kernel development tutorials (jamesmolloy.co.uk)](http://www.jamesmolloy.co.uk/tutorial_html/)；
> [The little book about OS development (littleosbook.github.io)](https://littleosbook.github.io/#segmentation)；
>
> [x86架构操作系统内核的实现 (0xffffff.org)](http://wiki.0xffffff.org/)；[hurley25/hurlex-doc: hurlex 小内核分章节代码和文档 (github.com)](https://github.com/hurley25/hurlex-doc)

# 计算机启动过程

首先关于独立编址和统一编址的概念：我们的主板上除了内存还有 BIOS、显卡、声卡、网卡等外部设备，CPU 访问这些设备同样需要使用地址进行访问

- 统一编址指的是把所有外设的存储单元对应的端口直接编址在虚拟地址空间里面
- 独立编址就是端口没有编址在地址空间里面，而是另行独立编址

按下电源首先第一步执行的是 *CPU 重置*，主板加电之后在电压尚未稳定之前北桥控制芯片会向 CPU 发出重置信号，此时 CPU 就会进行初始化，电压稳定之后就会撤销 reset 信号。

第二步，CPU 开始进入 `取指令-译指-执行` 的循环，第一条指令地址是 0xfffffff0，该地址指向 BIOS 芯片里面（地址空间里面有外设的地址），BIOS 芯片里面的指令是固化的

第三步是 BIOS 的硬件自检过程，BIOS 会对计算机的各个部件开始初始化，如果有错误会发出报警音

第四步，BIOS 初始化完成之后就开始在外部存储设备中寻找操作系统。BIOS 里面有一张启动设备表，BIOS 会按照表里面列出的顺序查找可以启动的设备

- 如果存储设备的第一个扇区中 512B 的最后两个字节是 0x55 和 0xAA 那么就表示这个设备是可以启动的，BIOS 就会启动该设备，后续的设备不再检查

第五步，BIOS 找到可以启动的设备之后就将该设备的第一个扇区加载到 0x7c00 地址，并且跳转过去执行

- 该 512B 里面存放的是载入操作系统的代码 或者是查找和载入内核的代码，称为 bootloader 程序

我们不需要自己实现 bootloader，而是使用 GNU 的多操作系统启动程序 GRUB，GRUB 允许在计算机内同时拥有多个操作系统并在计算机启动的时候选择一个运行。GRUB 通过 mutilboot 规范引导操作系统

> mutilboot 规范：在机器上安装一个新的操作系统时通常意味着要引入一套全新的引导机制，每种的安装和运行界面都不相同，基本上你不能选择某个操作系统的引导程序。mutilboot 指出了引导程序和操作系统之间的接口，这样符合规范的引导程序就可以引导任何符合规范的操作系统。这份规范并不关心引导程序应该如何工作——只关心它们引导操作系统时的接口

# boot.s

ld 链接脚本里面指定入口 `ENTRY(start)` 表明代码从 start 开始，剩下的便是与 mutilboot 相关的概念

```asm
; boot.s 内核从这里开始

; GNU mutilboot 规范的相关内容
MBOOT_HEADER_MAGIC  equ     0x1BADB002    ; 魔数，规范决定的
MBOOT_PAGE_ALIGN    equ     1 << 0        ; 引导模块按照 4KB 边界对齐
MBOOT_MEM_INFO      equ     1 << 1        ; 告诉 mutilboot 把内存空间的信息包含在 mutilboot 结构体中
MBOOT_HEADER_FLAGS  equ     MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO         ; 定义使用的 mutilboot 标记
MBOOT_CHECKSUM      equ     -(MBOOT_HEADER_MAGIC+MBOOT_HEADER_FLAGS)

; 符合Multiboot规范的 OS 映象需要这样一个 magic Multiboot 头
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
section .text

dd MBOOT_HEADER_MAGIC   ; GRUB 通过魔数判断该映像是否支持
dd MBOOT_HEADER_FLAGS   ; GRUB 的一些加载选项
dd MBOOT_CHECKSUM       ; 

[GLOBAL start]          ; 提供给链接器，声明内核代码入口
[GLOBAL glb_mboot_ptr]  ; 声明 struct mutiboot * 变量
[EXTERN kern_entry]     ; 声明内核 C 代码的入口函数

; 按照协议 GRUB 把一些计算机硬件和我们内核文件相关的信息放在了一个结构体中
; 并且将这个结构体指针放在了 ebx 寄存器中
start:
  cli                       ; 此时还没有设置好保护模式的中断处理，需要关闭中断
  mov esp, STACK_TOP        ; 设置内核栈地址
  mov ebp, 0                ; 帧指针改为 0
  and esp, 0ffffff0H        ; 栈地址 16 字节对齐
  mov [glb_mboot_ptr], ebx  ; ebx 存储的指针存入全局变量
  call kern_entry           ; 调用内核入口函数
stop:       ; 入口函数返回之后执行这里
  hlt       ; 停机指令
  jmp stop

section .bss
stack:
  resb 32768    ; reserve 32768 bytes 作为内核栈
glb_mboot_ptr:
  resb 4        ; reserve 4 bytes，全局的 mutilboot 结构体指针
STACK_TOP equ $-stack-1 ; 内核栈顶，$ 指代当前地址
```

# 屏幕显示

地址空间 1MB 以下有很多地址都是 IO 映射的，0xB8000 到 0xBFFFF 的地址空间是显卡工作在文本模式的显存

> 显卡是负责提供显示的内容的，因此显卡需要显示的数据必须要有个地方存储，这块区域就叫做显存，在文本模式下访问显存的地址就是上述地址

文本模式下的显卡会把屏幕划分成 25*80 总共 2000 个字符，0xB8000 开始的那部分地址就是映射到这部分显存，每两个字节代表一个 ASCII 字符

```
* Bit:     | 15 14 13 12 11 10 9 8 | 7 6 5 4 | 3 2 1 0 |
* Content: | ASCII                 | FG      | BG      |
```

显卡除了显示内容的存储单元，还有显示控制单元 （可以用来移动光标），它们被编址在独立的 I/O 空间里 需要用特殊的 in/out 指令去访问。由于控制单元的寄存器很多，无法一一映射到 I/O 端口的地址空间，因此使用 0x3D4 端口作为寄存器索引，0x3D5 端口作为寄存器的值

```asm
; 例如可以直接修改 0xB8000 地址的内容就可以在显示器上显示了
mov [0x00B8000], 0x4128		; 显示绿色 fore，黑色 back 的字符 'A'

; 通过 0x3D4 和 0x3D5 移动光标到 position 80
global outb	; 使得 outb 标签在此文件外面也可见
outb:
  out 0x3D4, 14			; high byte
  out 0x3D5, 0x00
  out 0x3D4, 15			; low byte
  out 0x3D5, 0x50
```

# 函数调用栈

> 前置知识：函数调用约定，ebp、esp 寄存器

前面提到 GRUB mutilboot 规范会把一个结构体的内存地址保存在 ebx 寄存器，该结构体里面描述了段表的起始地址（即第一个段描述符的地址）和段表字符串表的地址

我们可以自己定义这个结构体，然后在 boot.s 里面导出 ebx 的值，通过自定义结构体来访问结构体的内容。具体的

1. 通过 mutilboot_t->addr 确定第一个段描述符的地址，mutilboot_t->shndx 确定段表字符串表在段表中的索引
2. 然后就可以通过段表起始地址和段表字符串表，找到符号表和字符串表的地址
   - 符号表里面保存了该符号的名字在字符串表的索引号，通过字符串表我们就可以找到想要的符号的地址
   - 符号表里面还保存了该符号的起始地址和大小
3. 接下来使用 ebp + 1 保存的上一个函数调用前压入栈的返回地址，在符号表里面根据区间找到该函数所属的符号，再在字符串表找到函数的名字打印出来
4. 由于 ebp 里面保存了上一个函数的 ebp 地址，重复步骤三就可以打印出整个函数调用栈

> 所用到的一些数据结构：`mutilboot_t, elf_section_header_t, elf_symbol_t, char *(.strtab)`
>
> mutilboot_t 表示了 ebx 指向的那块内存的结构，通过里面的 addr 找到段描述符的地址；elf_section_header_t 表示了段描述符 用来找到段表字符串表、符号表、字符串表；elf_symbol 符号表，使用它确定符号的区间以及符号名字在字符串表的索引；字符串表就是一个 char* 指针起始

相关代码见 debug.c 和 melf.c

# 实模式和保护模式

### 8086 实模式时代

8086 的 CPU 是 16bit 的，的逻辑地址使用的是 `segment:offset` 的形式，segment 的值放入 segment register 就是告诉 cpu 现在要访问这个 segment 对应的内存区域，然后再根据 offset 找到对应的字节。地址转换：`segment << 4 + offset`，例如 0xA000:0x5F00 转换之后的地址是 0xA5F00

> << 4 相当于 *16，而一个 segment register 刚好是 16bit，实模式下只能寻址 1MB 的内存

寄存器直接指向 segment，cs(code segment), ds(data segment), ss(stack segment)，ES、FS、GS 一般都是指向数据段

<img src="https://pic3.zhimg.com/80/v2-c1d7f5b64042384804be4a509e93474a_1440w.jpg" alt="img" style="zoom:50%;" />

### 80286/80386 保护模式时代

8086 时代只有实模式，到了 80286 有了保护模式。80286 也是使用 segmentation，但是<u>此时的 segment register 存的不是 segment 的起始地址而是一个 segment selector 了</u>，通过这个段选择子可以在 GDT 表获得 segment descriptor，段描述符里面存有段的起始地址以及一些相关的标志位，<u>GDT 表的起始地址保存在 GDTR 寄存器中</u>。这种模式称为保护模式

> GDT 在内存中的位置是没有固定位置的，所以需要使用一个专门的寄存器保存 GDT 的地址信息

segment register 的 bit 分布如下：

```
Bit:     | 15                                3 | 2  | 1 0 |
Content: | offset (index)                      | ti | rpl |
offset 表示在 GDT 中的索引
```

GDT 中每个段描述符是 8bytes，结构如下

<img src="https://pic4.zhimg.com/80/v2-9df1db9256e405b3eb252471760fe53f_1440w.jpg" alt="img" style="zoom:50%;" />

寄存器、GDT 和地址空间的映射关系如下

<img src="https://pic1.zhimg.com/80/v2-7a8c61a37a16abd57d3506911d886d30_1440w.jpg" alt="img" style="zoom:50%;" />

GDTR 寄存器结构：

<img src="/Users/li/Library/Application Support/typora-user-images/image-20210611104821244.png" alt="image-20210611104821244" style="zoom:50%;" />

> 现在的 x86 分段和分页共存 每个 segment 被进一步划分成多个 pages，我们可以让所有 segment 的地址都从 0 开始，这样就相当于没有 segment 了，这种内存划分方式称为 **平坦内存模型**。

## 实模式到保护模式

现代的操作系统都是运行在保护模式的，但是实模式依旧存在，因为 BIOS 等引导程序还需要使用它。保护模式依赖于 GDT/LDT 和 page table，但是它们的建立只能在实模式下进行；x86 上电之后就进入实模式，**通过设置 CR0 寄存器的 PE 位可以进入保护模式**

在进入保护模式之前：

1. 在内存里面放好 GDT
2. 初始化 GDT 表的内容
3. 设置好 GDTR 寄存器以及相关的 segment register 寄存器

### 初始化 GDT 内容

```c
#include "gdt.h"
#include "mstring.h"

#define GDT_LENGTH 5

gdt_entry_t gdt_entries[GDT_LENGTH];
gdt_ptr_t gdt_ptr;

/**
 * @num: 段描述符索引
 * @base: 段描述符对应的基址
 * @limit: 段描述符表示的段的长度
 * @access: 访问标志，对应段描述符的第 6 个 byte
 * @gran: 其他访问标志
 */
static void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit,
                         uint8_t access, uint8_t gran);
/* 声明内核栈地址 */
extern uint32_t stack;

void init_gdt() {
  gdt_ptr.limit = sizeof(gdt_entry_t) * GDT_LENGTH - 1;
  gdt_ptr.base = (uint32_t)&gdt_entries;

  /* 采用平坦内存模型，设置段描述符 */
  gdt_set_gate(0, 0, 0, 0, 0);                  /* intel 文档规定第一个必须是 0 */
  gdt_set_gate(1, 0, 0xffffffff, 0x9A, 0xCF);   /* 指令段 */
  gdt_set_gate(2, 0, 0xffffffff, 0x92, 0xCF);   /* 数据段 */
  gdt_set_gate(3, 0, 0xffffffff, 0xFA, 0xCF);   /* 用户模式代码段 */
  gdt_set_gate(4, 0, 0xffffffff, 0xF2, 0xCF);   /* 用户模式数据段 */
  
  /* 加载全局描述符表到 GDTR 寄存器 */
  gdt_flush((uint32_t)&gdt_ptr);
}

static void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit,
                         uint8_t access, uint8_t gran) {
  /* 设置基址 */
  gdt_entries[num].base_low = base & 0xffff;
  gdt_entries[num].base_middle = (base >> 16) & 0xff;
  gdt_entries[num].base_high = (base >> 24) & 0xff;

  /* 设置限长 */
  gdt_entries[num].limit_low = limit & 0xffff;
  gdt_entries[num].granularty = (limit >> 16) & 0x0f;

  /* 设置标志位 */
  gdt_entries[num].granularty |= gran & 0xf0;
  gdt_entries[num].access = access;
}
```

### 设置 GDTR 寄存器和 segment register

```asm
[GLOBAL gdt_flush]

; 该函数没有 push ebp 等操作，所以 esp 直接指向调用方压入栈的下一条指令地址
; esp + 4 指向了 gdt_flush 的参数，也就是 gdt_ptr 地址

; 设置数据段选择寄存器和代码段选择寄存器
; 数据段通过 ax 间接赋值
; 代码段通过 far jump 完成设置
gdt_flush:
  mov eax, [esp + 4]  ; 参数存入 eax 寄存器
  lgdt [eax]          ; 设置 GDTR 寄存器

  ; 加载数据段描述符，x86里不能把立即数直接往 DS 里送，得通过 AX 中转一下
  ; 段描述符的构成：13bit index + 3bit others，所以 0x10 对应的描述符索引是 2
  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax
  ; 在这里 cs 寄存器还是旧的 cs
  ; far jump 需要切换 segment 重新查找 GDT，首先设置 cs 寄存器然后使用它的绝对地址跳转到 .flush
  jmp 0x08:.flush

.flush:
  ; 现在 cs 寄存器变成 0x08 了
  ret
```

> 在 GRUB 里面已经有 GDT 了，我们这里所做的工作相当于自定义了一个新的 GDT，并且把 GDTR 寄存器更新

> 参考：
> [segmentation和保护模式（一） - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/67735248)；
> [segmentation和保护模式（二） - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/67714693)；
> [segmentation和保护模式（三） - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/67576012)

# 中断描述符

中断控制器是一个简单的电子芯片，作用是将汇集的多路中断管线采用复用方式只通过一条中断线和 cpu 相连。于是由于只有一条中断线，为了区分各个设备就有了中断号

> 实际上 cpu 的中断管脚有两根 NMI 和 INTR。NMI 管脚触发的中断是需要无条件立即处理的，INTR 的管脚则是我们通常认为的中断，它可以被屏蔽

下面展示中断描述符的结构，每个中断描述符是 8bytes

```
// high 32bit
Bit:     | 31              16 | 15 | 14 13 | 12 | 11 | 10 9 8 | 7 6 5 | 4 3 2 1 0 |
Content: | offset high        | P  | DPL   | 0  | D  | 1  1 0 | 0 0 0 | reserved  |
// low 32bit
Bit:     | 31              16 | 15              0 |
Content: | segment selector   | offset low        |

offset low + offset high 合并起来就是中断处理函数的地址
```

中断描述符和段描述符一样，都需要在内存中存放一个描述符表，中断描述符表的地址存放在 IDTR 寄存器中。为了设置 IDTR 寄存器我们的步骤如下：

1. 定义中断描述符表，这是一个由 256 个中断描述符组成的数组
2. 初始化中断描述符表中的中断描述符，其中 0-19 号中断是 cpu 异常中断，20-31 号中断是 intel 保留的，32-255 是用户自定义中断
3. 将中断描述符表的地址设置到 IDTR 寄存器中

对于上述的步骤 2，在中断描述符里面需要设置中断处理函数的地址，因此我们需要预先定义对应的中断处理函数。在这里使用 nasm 汇编的宏定义每一个中断服务例程，注意这里的 isr 还没有包含逻辑处理部分，逻辑处理部分放置在中断处理函数里面

```asm
; 这里只展示没有错误码的中断的 isr 的声明和定义

; 首先定义一个宏，该宏有一个参数，其中这个参数就是中断号
; 然后在下面声明一个全局的函数，使用 isr + 第一个参数拼接成函数名
; 最后定义这个函数，综上相当于这个宏是用来定义函数的
%macro ISR_NOERRCODE 1
[GLOBAL isr%1]  ; 声明一个函数
isr%1:          ; 定义该声明的函数
  cli           ; 关闭中断
  push 0        ; push 无效的中断错误代码
  push %1       ; push 中断号
  jmp isr_common_stub	; 这里面会调用中断处理函数
%endmacro

; 这里使用这个宏声明和定义这个函数
ISR_NOERRCODE 255		; 定义 isr255 这个函数
```

在 `isr_common_stub` 函数里面会做三件事

1. 保护现场，保存寄存器（中断号已经在之前就保存下来了）
2. 调用逻辑处理部分
3. 恢复现场

# 时钟中断

中断的来源除了来自于硬件自身的 NMI 中断和来自于软件的 `int n` 指令造成的软件中断之外，还有来自于外部硬件设备的中断称之为 IRQ。这些中断通过 PIC(Programmable Interrupt Controller) 进行控制并传递给 CPU。在这里我们使用 8259A PIC

<img src="/Users/li/Library/Application Support/typora-user-images/image-20210615191436422.png" alt="image-20210615191436422" style="zoom:50%;" />

前面提到 0-31 号中断是 cpu 使用和保留的，因此这里的中断需要从 32 开始一直到 47

在使用 pic 之前需要对其进行初始化，主要包括以下内容：

- 初始化主片和从片
- 设置主片和从片的中断号开始
- 连接主片和从片
- 开启主片和从片的中断

IRQ 中断的初始化和 isr 差不多，都是通过一个 nasm 宏定义 IRQ 函数，然后在 IRQ 函数中调用 common 函数，common 中调用中断处理函数

```c
/**
 * ISR 处理过程：isri -> isr_common_stub -> isr_handler -> 具体的处理函数
 * IRQ 处理过程：irqi -> irq_common_stub -> irq_handler -> 具体的处理函数
 */
```

初始化并设置好 IRQ 函数之后，就可以添加特定的中断了，这里我们添加时钟中断。我们首先需要注册一个时钟中断函数添加到中断处理函数数组中以便后续通过中断号调用

```c
register_interrupt_handler(IRQ0, timer_callback);	// IRQ0 = 32
```

时钟中断由 8253 实现，时钟中断发生的频率 `*frequency = 1193180/计数器的初始值*`，因此 `*计数器的初始值 = 1193180/frequency*`，我们需要往 8253 特定的端口写入计数器的值

```c
/* 计数器初始值，时钟中断的频率是 HZ / divisor */
uint32_t divisor = HZ / frequency;

/* 先设置 0x43 端口，0x43 端口占 8bit，每个 bit 有特殊意义，总之就是按照要求设置 */
outb(0x43, 0x36);
uint8_t low = (uint8_t)(divisor & 0xff);
uint8_t high = (uint8_t)((divisor >> 8) & 0xff);
/* 0x40 端口设置计数器的值，需要分两次进行，先写低字节再写高字节 */
outb(0x40, low);
outb(0x40, high);
```

上面两部分合并在一起就是 `init_timer` 函数，其参数是 `frequency`，调用了 init_timer 函数之后时钟中断便可以触发了

> 关于 8253 相关知识：*https://juejin.cn/post/6938051055733178399*

# 物理内存管理

开启分页和没有开启分页地址映射流程：开启了平坦模式的情况下逻辑地址就是线性地址

- 没有开启分页
  - 逻辑地址 -> 段机制处理 -> 线性地址 = 物理地址
- 开启分页
  - 逻辑地址 -> 段机制处理 -> 线性地址 -> 页机制处理 -> 物理地址

物理内存管理存在两个关键的问题：

1. 如何获取物理内存的大小和地址
2. 怎么表示和存储映射关系，需要一组数据结构来管理内存
3. 申请和释放物理内存的算法

针对第一个问题，mutilboot 协议已经为我们获取了物理内存的信息并保存在结构体里面了，结构体里面存储了 `mmap_entry` 数组的首地址，每个 mmap_entry 表示一段物理内存

> 可能我们还需要获取内核加载到物理内存的位置信息，这个可以通过链接器脚本来实现。因为链接器知道所有的地址信息，所以只需要在链接脚本里面导出内核的地址就可以使用了

第二个问题和第三个问题，在 Linux 中使用的是伙伴系统来管理物理内存，在我们这里只是简单的把物理内存按页大小分割，并把每一页的首地址存放在一个自定义的栈里。在需要分配的时候从栈顶取一页，回收的时候压入栈顶

物理内存管理模块的关键：

- mutilboot 里面存放了 BIOS 探测到的物理内存信息（保存了该信息的在内存的首地址和该信息的大小），从起始地址开始存放了一个 mmap_entry 数组，数组的每一个元素都是一段物理内存信息
- 链接器脚本可以获取内核在内存中的地址信息
- 使用栈保存每一页的首地址用来做分配和回收

# 虚拟内存管理

前面我们写过一个链接脚本，里面定一个内核加载的地址是 0x100000，但是在 32bit 地址空间下内核对应的虚拟地址范围是 3GB ~ 4GB。**所以我们需要把内核加载到虚拟地址的 3GB 以上**

一个简单的方法可能会被想到的是，直接在链接脚本中更改加载的地址为 0xc0000000，这样一来就达到了我们的目的。**但是这样是不行的**，原因在于：我们的 GRUB 是在 1MB 物理地址处加载内核的，假如我们直接更改的话（注意就只是更改链接脚本这一处），当我们想要访问 0xc0000000 以上内核代码对应的地址的时候会发现没有地址映射，所以就找不到任何物理内存中的信息

> GRUB 会把内核加载到 1MB 开始的物理内存中，而当我们没有开启分页的时候直接访问的逻辑地址会经过 GDT 转换成线性地址，此时的线性地址就是物理地址

所以我们需要有一段程序和数据按照 0x100000 的地址进行重定位，然后帮助我们设置好一个临时的页表，再跳转到内核入口函数中执行。我们把这段程序和数据存储在 .init 段，其余段都重定位到 3GB 的地址，链接脚本部分内容如下

```
PROVIDE(kern_start = 0xC0100000);    /* 链接器导出内核地址起始位置 */
/* 段起始位置 */
. = 0x100000;
.init.text :
{
*(.init.text)
. = ALIGN(4096);
}
.init.data :
{
*(.init.data)
. = ALIGN(4096);
}
. += 0xC0000000;

.text : AT(ADDR(.text) - 0xC0000000)
{
*(.text)
. = ALIGN(4096);
}
...
```

此外我们的 elf 相关结构体和 mutilboot_t 结构体里面存的地址都是物理地址，因为这些内容是在 grub 期间获取的，所以我们需要把这些地址重定位到 3GB 的地址空间

```c
/** 
  * 更新全局 mutilboot_t 指针，
  * mboot_ptr_tmp 指针存储的地址是物理地址，
  * 加上 PAGE_OFFSET 变成现在的内核地址用来做访问
  */
glb_mboot_ptr = mboot_ptr_tmp + PAGE_OFFSET;

elf.strtab = (const char *)(sh[i].addr + PAGE_OFFSET);	// elf 也同理
```

然后我们在设置好链接脚本之后，会从 boot.s 里面开始执行，在那里我们设置从 .init.text 开始执行，使用的数据是 .init.data

```asm
; 省略部分内容，此处使用一个临时的栈
section .init.text      ; 临时代码段开始
[GLOBAL start]          ; 提供给链接器，声明内核代码入口
[GLOBAL mboot_ptr_tmp]  ; 声明 struct mutiboot * 变量
[EXTERN kern_entry]     ; 声明内核 C 代码的入口函数
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
```

在 start 里面会进入 kern_entry 函数执行，这个函数是放在 .init.text 段里面的所以不需要担心地址重定位的问题。另外在 .init.data 段里面声明了临时的页目录和页表

```c
/* 内核使用的临时页表和页目录，保存在 1MB 以下的内存地址 */
__attribute__((section(".init.data"))) pgd_t *pgd_tmp   = (pgd_t *)0x1000;
__attribute__((section(".init.data"))) pgd_t *pte_low   = (pgd_t *)0x2000;
__attribute__((section(".init.data"))) pgd_t *pte_high  = (pgd_t *)0x3000;

/* 内核入口函数 */
__attribute__((section(".init.text"))) void kern_entry() {
  /**
   * kern_entry 部分省略，主要流程是
   * 1. 设置临时页目录的页表项，pgd_tmp[0] 和 pgd_tmp[PGD_INDEX(PAGE_OFFSET)]，设置为 pte_low 和 pte_high
   * 2. 为 pte_low 和 ptw_high 管理的内存建立映射，这两部分内存都映射到物理内存的前 4MB 处，
   *    关于为什么都映射到同一个物理内存是因为 在 kern_entry 里面会开启分页，
   *    一旦开启分页，CPU 立即就会进入分页机制去运行，此时所有 的寻址都会按照分页机制的原则去进行，
   *    而 kern_entry 函数本身是按照 1MB 起始地址生成的虚拟地址，
   *    如果不映射低端的虚拟地址的话，kern_entry 开启分页之后的代码访问就会出错
   * 3. 设置临时页表，更改 cr3 寄存器
   * 4. 启用分页，这只 cr0 寄存器的分页 bit
   * 5. 切换内核栈，改为 3GB+ 地址的内核栈
   * 6. 更新 mutilboot_t 结构体的地址为 mutilboot_ptr_tmp + PGAE_OFFSET(起始这两个指针里面存的地址都映射到同一个物理地址)
   * 7. 调用内核初始化函数
   */
}
```

到这里我们就设置好临时页表了，不需要担心在访问 3GB+ 的内核地址的时候会找不到地址映射。在内核初始化函数里面，我们会调用 vmm 初始化函数，我们的初始化 vmm 函数保存在物理地址的 0 ~ 4MB 处，这里的代码我们已经建立过映射了

vmm 的初始化函数会建立一个新的页目录和页表

```c
/* 内核页目录，里面保存的页表的物理地址 */
pgd_t pgd_kern[PGD_SIZE] __attribute__((aligned(PAGE_SIZE)));
/* 内核页表，里面保存的是物理页框 */
static pte_t pte_kern[PTE_COUNT][PTE_SIZE] __attribute__((aligned(PAGE_SIZE)));

void init_vmm() {
  /**
   * 主要是
   * 1. 建立 pgd_kern 和 pte_kern 的映射关系
   * 2. pte_kern 和 物理页框的映射关系
   * 3. 注册缺页中断，缺页中断是 14 号中断
   * 4. 切换 cr3 寄存器为最新的页目录
   */
}
```

到此为止我们就不需要临时页表了，临时页表只是提供给我们访问初始化代码用的，因为我们在访问内核初始化代码的时候总是需要一个地址映射的。一旦我们的 vmm 初始化完成了就不需要临时页表了，因为临时页表比较小无法保存太多的映射

其余的就是 vmm 管理就是

- 主动建立 va 和 pa 的映射关系
- 取消 va 的映射关系
- 获取某个 va 的映射关系

做完上述内容就完成把内核代码和数据重定位到 3GB+ 的地址空间了

> 页表里面保存的下一级页表是物理地址，因为页表是给 MMU 使用的 而只有 cpu 才使用虚拟地址，所以页表里面保存的是下一级页表的物理地址

> 参考：
> [Linux系统的启动过程 - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/108084783)
> [Linux启动引导程序（GRUB）加载内核的过程_u010783226的专栏-CSDN博客](https://blog.csdn.net/u010783226/article/details/106070429)
> [内核引导 - Linux源代码导读报告 (ustc.edu.cn)](http://home.ustc.edu.cn/~boj/courses/linux_kernel/1_boot.html)

# 堆管理

实现 kmalloc 和 kfree 的功能，使用侵入式链表管理方法，`一个内存块 = 管理结构 header_t + 可用内存`

> 在 Linux 中 kmalloc 和 vmalloc 都是在申请内存的时候直接分配物理内存的；kmalloc 分配得到的物理内存是连续的，vmalloc 分配得到的物理内存不是连续的；而用户态的 malloc 只是先申请虚拟内存，并不实际分配物理内存

主要实现以下功能：

- 分配指定大小的堆内存
- 回收指定大小的堆内存
- 分配堆内存的时候内存块的切分
- 回收堆内存的时候内存块的合并
  - 先合并在尝试回收物理内存保证了 当前块的大小足够大能够满足一个 PAGE_SIZE
- 回收堆内存的时候，物理内存的回收
  - 只有当一个 page 里面没有其余内存块使用的时候才进行物理页的回收
- 分配内存块
  - 当 start + len > heap_max 才进行内存块分配，此时物理内存会一页一页分配
  - 循环分配直到新分配的物理内存可以容纳所需的大小

# 内核线程

内核线程作为运行在内核态的一个逻辑执行流，拥有私有的栈空间，除此之外它不拥有资源，所有内核线程拥有相同的页表 共享所有的全局数据

> 我们这里创建的内核线程放在一个全局链表中顺序调度

### 初始化第一个进程

```c
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
```

内核线程创建模块首先需要进行初始化，初始化的任务就是给当前的内核执行流创建一个 进程描述符，并作为第一个进程，此后所有创建的内核线程作为该进程的线程 共享内核资源、使用同一个地址空间

第一个进程（线程）的栈就是内核栈，后续的内核线程使用的栈是额外的

### 创建内核线程

主要是创建一个进程描述符，并且给该内核线程分配自己的栈空间，设置私有上下文的寄存器，最后插入链表

这里的栈其实是在堆上分配了一块 task_struct 的空间，然后在这块空间上再上移了 `STACK_SIZE` 作为栈顶，栈的返回就是 [stack_top, task_struct-end]，从上往下增长

```c
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

```

上述有一个很关键的是，在创建了该内核线程的时候，在其栈上放入的 `arg` `kthread_exit()` `fn`，按照这个顺序存放的原因在于

- 当进行线程切换的时候，涉及到上下文切换，在上下文切换函数 ret 指令执行之前 当前的上下文寄存器已经变成新任务的上下文了。所以 ret 指令会在新任务的上下文上取出下一条指令的地址（类似函数调用返回一样）
- 当我们按照上述顺序将参数和函数压栈时，`arg` 相当于切换到该任务时执行的参数，`kthread_exit()` 相当于下一条指令地址，`fn` 就是需要跳转执行的函数。整个过程和函数调用类似

```asm
; 具体的线程切换操作，关键在于寄存器的保存与恢复
; 当 ret 指令返回的时候，弹出的返回地址是 new task 在进行任务切换之前保存的地址
switch_to:
  mov eax, [esp + 4]    ; 指向传递过来的第一个参数，也就是 old task context
  mov [eax + 0], esp    ; 保存 old task 的寄存器
  mov [eax + 4], ebp
  mov [eax + 8], ebx
  mov [eax + 12], esi
  mov [eax + 16], edi
  pushf
  pop ecx
  mov [eax + 20], ecx
  mov eax, [esp + 8]    ; 现在 eax 保存的是 new task context 的地址
  mov esp, [eax + 0]    ; 设置相关寄存器为 new task context
  mov ebp, [eax + 4]
  mov ebx, [eax + 8]
  mov esi, [eax + 12]
  mov edi, [eax + 16]
  mov eax, [eax + 20]
  push eax
  popf
  ret
```

由于我们在调用 switch_to 时候传入的参数是 `switch_to(&(prev->context), &(current->context));`，所以根据参数从右至左压栈，switch_to 函数的第一个 `mov eax, [esp + 4]` 获取的是 prev 任务的上下文并进行上下文保存

