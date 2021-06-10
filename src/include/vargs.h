/** 
 * 根据函数调用约定，可以通过第一个参数的地址以及类型推算出后续的参数
 * #define  va_list              char *
 * #define  va_start(p, first)   (p = (va_list)&first + sizeof(first))
 * #define  va_arg(p, next)      (*(next*)((p += sizeof(next) ) - sizeof(next)))
 * #define  va_end(p)            (p = (va_list)NULL)
 */

#ifndef INCLUDE_VARGS_H_
#define INCLUDE_VARGS_H_

// __builtin_va_* 都是 gcc 内置的函数和变量等
typedef __builtin_va_list va_list;
#define va_start(ap, type)  (__builtin_va_start(ap, type))
#define va_arg(ap, type)    (__builtin_va_arg(ap, type))
#define va_end(ap)

#endif  // INCLUDE_VARGS_H_