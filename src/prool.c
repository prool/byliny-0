// prool's recoding functions
// proolix@gmail.com www.prool.kharkov.org mud.kharkov.org

#include <stdio.h>
#include <string.h>
#include <iconv.h>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "prool.h"

#define MAX_PASS 100

char encrypted_password[MAX_PASS];

char *crypt_prool(char *key, char *salt)
{int i;
for (i=0;i<MAX_PASS;i++) encrypted_password[i]=0;
strcpy(encrypted_password,key);
i=0;
while(encrypted_password[i]) {encrypted_password[i++]++;}
return encrypted_password;
}

void utf8_to_koi(char *str_i, char *str_o)
{
	iconv_t cd;
	size_t len_i, len_o = MAX_SOCK_BUF;
	size_t i;

	if ((cd = iconv_open("KOI8-R", "UTF-8")) == (iconv_t) - 1)
	{
		printf("utf8_to_koi: iconv_open error\n");
		return;
	}
	len_i = strlen(str_i);
	if ((i=iconv(cd, &str_i, &len_i, &str_o, &len_o)) == (size_t) - 1)
	{
		printf("utf8_to_koi: iconv error\n");
		// return;
	}
	if (iconv_close(cd) == -1)
	{
		printf("utf8_to_koi: iconv_close error\n");
		return;
	}
}

void koi_to_utf8(char *str_i, char *str_o)
{
	iconv_t cd;
	size_t i;
	size_t len_i = MAX_SOCK_BUF;
	size_t len_o = MAX_SOCK_BUF*2;

	if ((cd = iconv_open("UTF-8", "KOI8-R")) == (iconv_t) - 1)
	{
		printf("koi_to_utf8: iconv_open error\n");
		return;
	}
	len_i = strlen(str_i);
	//for(i=0;i<len_o;i++) *str_o=0;
	if ((i=iconv(cd, &str_i, &len_i, &str_o, &len_o)) == (size_t) - 1)
	{
		printf("koi_to_utf8: iconv error\n");
		// return;
	}
	if (iconv_close(cd) == -1)
	{
		printf("koi_to_utf8: iconv_close error\n");
		return;
	}
}

void outhex(char *str)
{
while(*str)
	{
	printf("'%X' ", *str++&0xFFU);
	}
printf("\n");
}
