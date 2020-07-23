// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under GPLv2 license.
#include <linux/module.h>
#include <linux/kernel.h>
#include "sfblkdev.h"

int convert_ascii_to_utf16(wchar_t *dest, const char *src, int len)
{
    int i;
    for(i=0; i<len; i++)
    {
        *((wchar_t *)dest + i) = (int)src[i] & 0xff; 
    }
    return i*2;
}

int convert_utf16_to_ascii(uint8_t *dest, wchar_t *src, int len)
{
    int i;
    for(i=0; i<len/2; i++)
    {
        if((uint16_t)src[i] > 0xff)
            return -1;
        *(dest + i) = (uint16_t)src[i] & 0xff;
    }
    return i;
}
