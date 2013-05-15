
#include <types.h>
#include <common.h>
#include <string.h>

char *strcpy(char *pdest, const char *psrc)
{
	char *ptmp;
	ptmp = pdest;

	while (*psrc)
		*ptmp++ = *psrc++;

	*ptmp = '\0';

	return pdest;
}


char *strncpy(char *pdest, const char *psrc, uint32_t n)
{
	char *ptmp;
	ptmp = pdest;

	while (*psrc && n-- > 0)
		*ptmp++ = *psrc++;
	
	if (n)
		*ptmp++ = '\0'; // fixme

	return pdest;
}

uint32_t strnlen(const char *psrc, uint32_t maxlen)
{
	const char *ptmp = psrc;

	while (*ptmp && maxlen) 
	{
		ptmp++;
		maxlen--;
	}

	return (ptmp -psrc);
} 

uint32_t strlen(const char *psrc)
{
	unsigned int len =0;

	while (*psrc++)
		len++;

	return len;
}

long strcmp (const char *pstr1, const char *pstr2)
{
	int ret;

	while (!(ret = *(unsigned char *)pstr1 - *(unsigned char *)pstr2) && *pstr1++ && *pstr2++);

	if (ret < 0)
		return -1;

	if (ret > 0)
		return 1;

	return 0;
}


long strncmp(const char *ps1,const char *ps2,uint32_t n)
{
	int ret;

	while (n)
	{
		if ((ret = *ps1 - *ps2) != 0 || '\0' == *ps1)
			return ret;
		ps1++;
		ps2++;
		n--;
	}
	return 0;
}


char *strcat(char *pdest, const char *psrc)
{
	char *porg;

	for (porg = pdest; *porg; porg++);

	while (*psrc)
		*porg++ = *psrc++;

	*porg = '\0';

	return pdest;
}

char *strncat(char *pdest, const char *psrc, uint32_t n)
{
	 char *ptmp;
	 ptmp = pdest;

	 while (*ptmp++);
	 ptmp--;

	 while (n-- && *psrc)
		*ptmp++ = *psrc++;
	 *ptmp = '\0';

	 return pdest;
}

long memcmp(const void* pdest, const void* psrc, uint32_t n)
{
	const uint8_t *ps, *pd;

	pd = (const uint8_t *) pdest;
	ps = (const uint8_t *) psrc;

	while (n-- > 0)
	{
		if (*pd != *ps)
			return *pd - *ps;

		ps++;
		pd++;
	}

	return 0;
}


void *memcpy(void *pdest, const void *psrc, uint32_t count)
{
	uint8_t *pd;
	const uint8_t *ps;

	pd = pdest;
	ps = psrc;
	while (count--)
		*pd++ = *ps++;

	return pdest;
}

void *memmove(void *pdest, const void *psrc, uint32_t count)
{
	if (pdest < psrc)
	{
		return memcpy(pdest, psrc, count);
	}
	else
	{
		char *p = pdest + count;
		const char *q = psrc + count;

		while (count--)
		{
			*--p = *--q;
		}
	}

	return pdest;
}

char *strchr(const char *psrc, uint32_t c)
{
	while (*psrc && *psrc != c)
		psrc++;

	if (*psrc == c)
		return (char *)psrc;
	else
		return NULL;
}

char *strrchr(const char *psrc, uint32_t c)
{
	long i = strlen((char *)psrc)-1;

	while ((i >= 0) && *(psrc + i) != c && i--);

	if (i >= 0)
		return (char *)psrc + i;
	else
		return NULL;
}

void *memset(void *psrc, int c, uint32_t count)
{
	char *ps = (char *) psrc;

	while (count--)
		*ps++ = c;

	return psrc;
}

int hatol(const char *str, uint32_t *phex)
{
	uint32_t ulVal = 0;
	const char *pTmp;


	if ('0' == *str && ('x' == *(str + 1) || 'X' == *(str + 1)))
		str += 2;

	pTmp = str;

	while (*pTmp != '\0')
	{
		if (!ISHEX(*pTmp))
			return  -1;

		if (*pTmp >= '0' && *pTmp <= '9')
		{
			ulVal <<= 4;
			ulVal |= *pTmp - '0';
		}
		else if (*pTmp >= 'a' && *pTmp <= 'f')
		{
			ulVal <<= 4;
			ulVal |= *pTmp - 'a' + 10;
		}
		else if (*pTmp >= 'A' && *pTmp <= 'F') 
		{
			ulVal <<= 4;
			ulVal |= *pTmp - 'A' + 10;
		}

		pTmp++;
	}

	*phex = ulVal;

	return 0;
}


