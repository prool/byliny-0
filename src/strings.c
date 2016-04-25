/*
#include <stdio.h>
*/

inline char a_isascii(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $65,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $90,%b0\n\t"
	"jbe 2f\n\t"	
	"cmpb $97,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $122,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $192,%b0\n\t"
	"jb 1f\n\t"
	"jmp 2f\n\t"
	"1:\txorb %%al,%%al\n\t"
	"2:"
	:"=a" (__res) 
	:"0" (c));
return __res;
}

inline char a_islower(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $97,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $122,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $192,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $223,%b0\n\t"
	"jbe 2f\n\t"
	"1:\txorb %%al,%%al\n\t"
	"2:"
	:"=a" (__res) 
	:"0" (c));
return __res;
}

inline char a_isupper(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $65,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $90,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $224,%b0\n\t"
	"jb 1f\n\t"
	"jmp 2f\n\t"
	"1:\txorb %%al,%%al\n\t"
	"2:"
	:"=a" (__res) 
	:"0" (c));
return __res;
}

inline char a_isdigit(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $48,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $57,%b0\n\t"
	"jbe 2f\n\t"
	"1:\txorb %%al,%%al\n\t"
	"2:"
	:"=a" (__res) 
	:"0" (c));
return __res;
}

#if 1 // 1 = prool
inline char a_isalpha(char c)
{
if ((c>='a') && (c<='z')) return 1;
if ((c>='A') && (c<='Z')) return 1;
if (c>=192) return 1; // cyrillic in koi8-r
return 0;
}
#else
inline char a_isalpha(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $65,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $90,%b0\n\t"
	"jbe 2f\n\t"	
	"cmpb $97,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $122,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $192,%b0\n\t"
	"jb 1f\n\t"
	"jmp 2f\n\t"
	"1:\txorb %%al,%%al\n\t"
	"2:"
	:"=a" (__res) 
	:"0" (c));
return __res;
}
#endif

inline char a_isalnum(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $48,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $57,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $65,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $90,%b0\n\t"
	"jbe 2f\n\t"	
	"cmpb $97,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $122,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $192,%b0\n\t"
	"jb 1f\n\t"
	"jmp 2f\n\t"
	"1:\txorb %%al,%%al\n\t"
	"2:"
	:"=a" (__res) 
	:"0" (c));
return __res;
}

inline char a_isxdigit(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $48,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $57,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $65,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $70,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $97,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $102,%b0\n\t"
	"jbe 2f\n\t"
	"1:\txorb %%al,%%al\n\t"
	"2:"
	:"=a" (__res) 
	:"0" (c));
return __res;
}


inline char a_ucc(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $97,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $122,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $192,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $223,%b0\n\t"
	"jbe 2f\n\t"	
	"1:\taddb $32,%%al\n\t"
	"2:"
	:"=a" (__res) 
	:"0" (c));
return __res;
}

inline char a_lcc(char c)
{
register char __res;

__asm__ __volatile__(
	"movb %0,%%al\n\t"
	"movb %%al,%b0\n\t"
	"cmpb $65,%b0\n\t"
	"jb 1f\n\t"
	"cmpb $90,%b0\n\t"
	"jbe 2f\n\t"
	"cmpb $224,%b0\n\t"
	"jb 1f\n\t"
	"1:\taddb $224,%%al\n\t"
	"2:"
	:"=a" (__res) 
	:"0" (c));
return __res;
}


/*
main ()
{ int i;
  
  if (a_isascii(0)) {puts("0-OK\n\r");}
  if (a_isascii('a')) {puts("1-OK\n\r");}
  if (a_isascii('A')) {puts("2-OK\n\r");}
  if (a_isascii('3')) {puts("3-OK\n\r");}
  if (a_isascii('\n')) {puts("4-OK\n\r");}
  if (a_isascii(196)) {puts("5-OK\n\r");}
  if (a_isascii(158)) {puts("6-OK\n\r");}
  if (a_isascii(128)) {puts("7-OK\n\r");}
  for(i=32;i<=255;putchar(a_ucc(i)), i++); 
  puts("\n\r");
  for(i=32;i<=255;putchar(a_lcc(i)), i++);  
}
*/
