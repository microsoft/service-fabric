// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

//
// A large portion of implementation are copied from Visual Studio
// 2010 CRT. The rest is from MACBU mbustring.
//
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>	// for memmove
#include <errno.h>
#include <limits.h>		// for MB_LEN_MAX
#include <mbusafecrt.h>

// Implementations were stolen from the MSL implementations
/*size_t
mbstowcs(wchar_t * wcstring, const char * mbstring, size_t nwchars)
{
	size_t rtn = 0;
   	errno = rtcpal_mbstowcs_s(&rtn, wcstring, nwchars, mbstring, _TRUNCATE);
	return rtn;
}*/

 int wmemcmp(const wchar_t * src1, const wchar_t * src2, size_t n)
{
	int diff = 0;

	while (n)
	{
		diff = *src1 - *src2;
		if (diff) break;
		src1++;
		src2++;
		n--;
	}

	return(diff);
}

 
wchar_t *
wmemchr(const wchar_t * src, wchar_t val, size_t n)
{
	while (n)
	{
		if (*src == val) return (wchar_t*)(src);
		src++;
		n--;
	}

	return(NULL);
}


wchar_t *
wmemmove(wchar_t * dst, const wchar_t * src, size_t n)
{
	return (wchar_t*)( memmove(dst, src, n * sizeof(wchar_t)) );
}


wchar_t *
wmemcpy(wchar_t * dst, const wchar_t * src, size_t n)
{
	return (wchar_t*)( memcpy(dst, src, n * sizeof(wchar_t)) );
}


wchar_t *
wmemset(wchar_t * dst, wchar_t val, size_t n)
{
	wchar_t* save = dst;

	while (n)
	{
		*dst++ = val;
		n--;
	}

	return save;
}


size_t
wcslen(const wchar_t * str)
{
	size_t len = 0;

	while(*str++) {
		len++;
	}

	return len;
}


size_t
wcsnlen(const wchar_t * str, size_t maxsize)
{
    size_t n;

    /* Note that we do not check if s == NULL, because we do not
     * return errno_t...
     */

    for (n = 0; n < maxsize && *str; n++, str++)
        ;

    return n;
}


int
wcscmp(const wchar_t * str1, const wchar_t * str2)
{
	while(*str1 && *str2 && *str1 == *str2) {
		str1++;
		str2++;
	}

#if (WCHAR_MAX > 0xFFFFu)
    return ((uint32_t)(*str1)) - ((uint32_t)(*str2));
#else    
	return (short)(*str1) - (short)(*str2);
#endif
}


int
wcsicmp(const wchar_t * str1, const wchar_t * str2)
{
    /*
     * The correct implementation needs to convert from UTF-16 to
     * UTF-32 and then call towupper.
     *
     * Our simplified version works fine for media stack since
     * its string are all ASCII.
     */
    while(*str1 && *str2 && towupper(*str1) == towupper(*str2)) {
        str1++;
        str2++;
    }

#if (WCHAR_MAX > 0xFFFFu)
    return ((uint32_t)(towupper(*str1))) - ((uint32_t)(towupper(*str2)));
#else
    return (short)(towupper(*str1)) - (short)(towupper(*str2));
#endif
}


wchar_t *
wcscpy(wchar_t * dst, const wchar_t * src)
{
	wchar_t * p = dst;

	while((*p++ = *src++) != 0) {
		;
	}

	return dst;
}

/*
 * Copied from VC2010 CRT
 */

int
wcsncmp(const wchar_t * first, const wchar_t * last, size_t count)
{
    if (!count)
        return(0);

    while (--count && *first && *first == *last)
    {
        first++;
        last++;
    }

    return((int)(*first - *last));
}

/*
 * Copied from VC2010 CRT
 */

wchar_t *
wcsncpy(wchar_t * dest, const wchar_t * source, size_t count)
{
    wchar_t *start = dest;

    while (count && (*dest++ = *source++))    /* copy string */
        count--;

    if (count)                              /* pad out with zeroes */
        while (--count)
            *dest++ = L'\0';

    return(start);
}

/*
 * Copied from VC2010 CRT
 */

wchar_t *
wcschr(const wchar_t * string, const wchar_t ch)
{
    while (*string && *string != (wchar_t)ch)
        string++;

    if (*string == (wchar_t)ch)
        return((wchar_t *)string);

    return(NULL);
}

/*
 * Copied from VC2010 CRT
 */

wchar_t *
wcsstr(const wchar_t * wcs1, const wchar_t * wcs2)
{
    wchar_t *cp = (wchar_t *) wcs1;
    wchar_t *s1, *s2;

    if ( !*wcs2)
        return (wchar_t *)wcs1;

    while (*cp) {
        s1 = cp;
        s2 = (wchar_t *) wcs2;

        while ( *s1 && *s2 && !(*s1-*s2) )
                s1++, s2++;

        if (!*s2)
                return(cp);

        cp++;
    }

    return(NULL);
}

/*
 * Copied from VC2010 CRT
 */

size_t
wcsspn (const wchar_t * string, const wchar_t * control)
{
    wchar_t *str = (wchar_t *) string;
    wchar_t *ctl;

    /* 1st char not in control string stops search */
    while (*str) {
        for (ctl = (wchar_t *)control; *ctl != *str; ctl++) {
            if (*ctl == (wchar_t)0) {
                /*
                 * reached end of control string without finding a match
                 */
                return (size_t)(str - string);
            }
        }
        str++;
    }
    /*
     * The whole string consisted of characters from control
     */
    return (size_t)(str - string);
}


size_t
wcscspn (const wchar_t * string, const wchar_t * control)
{
    wchar_t *str = (wchar_t *) string;
    wchar_t *wcset;

    /* 1st char in control string stops search */
    while (*str) {
        for (wcset = (wchar_t *)control; *wcset; wcset++) {
            if (*wcset == *str) {
                return (size_t)(str - string);
            }
        }
        str++;
    }
    return (size_t)(str - string);
}


wchar_t *
wcslwr(wchar_t *str)
{
	wchar_t * saved = str;

	while (*str)
	{
		*str = towlower (*str);
		str++;
	}

	return saved;
}


int
wcslwr_s(wchar_t * _Str, size_t _SizeInWords)
{
	int count = 0;
	while (*_Str && count < _SizeInWords)
	{
		*_Str = towlower (*_Str);
		_Str++;
		count++;
	}

	return 0;
}

/*
 * Copied from VC2010 CRT
 */

wchar_t *
wcsncat(wchar_t * front, const wchar_t * back, size_t count)
{
    wchar_t *start = front;

    while (*front++)
            ;
    front--;

    while (count--)
        if (!(*front++ = *back++))
            return(start);

    *front = L'\0';
    return(start);
}

/*
 * Copied from VC2010 CRT
 */

wchar_t *
wcsrchr(const wchar_t * string, wchar_t ch)
{
    wchar_t *start = (wchar_t *)string;

    while (*string++)                       /* find end of string */
        ;
                                            /* search towards front */
    while (--string != start && *string != (wchar_t)ch)
        ;

    if (*string == (wchar_t)ch)             /* wchar_t found ? */
        return( (wchar_t *)string );

    return(NULL);
}

/*
 * Copied from VC2010 CRT
 */

wchar_t *
wcsrev(wchar_t * string)
{
    wchar_t *start = string;
    wchar_t *left = string;
    wchar_t ch;

    while (*string++)                 /* find end of string */
        ;
    string -= 2;

    while (left < string)
    {
        ch = *left;
        *left++ = *string;
        *string-- = ch;
    }

    return(start);
}

/*
 * CLR PAL defined _wcsicmp as wcscasecmp
 */

int wcscasecmp(
   const wchar_t *string1,
   const wchar_t *string2 
)
{
    return wcsicmp(string1, string2);
}



double wcstod(
   const wchar_t *nptr,
   wchar_t **endptr 
)
{
    double dRet = 0.0;
    int size = 0;
    const wchar_t* pSrc = nptr;
    char* pDest = NULL;
    char* pDestEnd;

    size = wcslen(pSrc) + 1;
    pDest = (char *)malloc(size);

    if (pDest == NULL)
    {
        goto Exit;
    }

    for(int i = 0; i < size; i++)
    {    
        pDest[i] = (char) pSrc[i];
    }

    dRet = strtod(pDest, &pDestEnd);
    if (NULL != endptr)
    {
        *endptr = (wchar_t *)pSrc + (pDestEnd - pDest);
    }

Exit:
    if (NULL != pDest)
    {
        free(pDest);
        pDest = NULL;
    }

    return dRet;
}