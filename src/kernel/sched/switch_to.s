[GLOBAL switch_to]

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