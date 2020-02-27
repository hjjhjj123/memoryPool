#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dlmempool.h"
#include <assert.h>
int main()
{
	CDLMemPool *m_tComPool = NULL;
	m_tComPool = new CDLMemPool(2*SIZE_1MB,10,"testhjj");
	char * str = (char *)(m_tComPool->DlMalloc(128));
	printf("str p %p\n",str);
	char test[12] = "dadasd";
	strcpy(str,test);
	printf("str--%s \n",str);
	m_tComPool->DlFree(str);
	while(1)
	{
		sleep(10);
	}
}
