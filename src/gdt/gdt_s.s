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