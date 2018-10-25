// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#include "internal/rt/intsafe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BYTE_MAX        0xff
#define LONG64_MIN      (-9223372036854775807i64 - 1)
#define LONG64_MAX      9223372036854775807i64

__inline
HRESULT
SizeTMult(
    size_t Multiplicand,
    size_t Multiplier,
    size_t* pResult)
{
    HRESULT hr = INTSAFE_E_ARITHMETIC_OVERFLOW;
    *pResult = SIZET_ERROR;

    ULONGLONG ull64Result = (ULONGLONG)((ULONGLONG)(ULONG)(Multiplicand) * (ULONG)(Multiplier));
    if (ull64Result <= SIZE_MAX)
    {
        *pResult = (SIZE_T)ull64Result;
        hr = S_OK;
    }

    return hr;
}

__inline
HRESULT
LongLongToULong(
    __in LONGLONG llOperand,
    __out __deref_out_range(==,llOperand) ULONG* pulResult)
{
    HRESULT hr;

    if ((llOperand >= 0) && (llOperand <= ULONG_MAX))
    {
        *pulResult = (ULONG)llOperand;
        hr = S_OK;
    }
    else
    {
        *pulResult = ULONG_ERROR;
        hr = INTSAFE_E_ARITHMETIC_OVERFLOW;
    }

    return hr;
}

#define Int64ToDWord    LongLongToULong

#ifdef __cplusplus
}
#endif
