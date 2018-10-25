// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// 426
typedef CONST WCHAR *LPCWCHAR, *PCWCHAR;
typedef CONST WCHAR UNALIGNED *LPCUWCHAR, *PCUWCHAR;

// 592
typedef CONST UCHAR *PCUCHAR;

// 752
typedef char CCHAR;          // winnt
typedef short CSHORT;
typedef ULONG CLONG;

typedef CCHAR *PCCHAR;
typedef CSHORT *PCSHORT;
typedef CLONG *PCLONG;

// 839
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

// 857
#define NT_ERROR(Status) ((((ULONG)(Status)) >> 30) == 3)

// 1252
typedef enum _EVENT_TYPE {
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE;

// 1291
typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength), length_is(Length) ]
#endif // MIDL_PASS
    _Field_size_bytes_part_opt_(MaximumLength, Length) PCHAR Buffer;
} STRING;
typedef STRING *PSTRING;
typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;

// 1513
#define OBJ_CASE_INSENSITIVE    0x00000040L

// 1626
#define MINCHAR     0x80        // winnt
#define MAXCHAR     0x7f        // winnt
#define MINSHORT    0x8000      // winnt
#define MAXSHORT    0x7fff      // winnt
#define MINLONG     0x80000000  // winnt
#define MAXLONG     0x7fffffff  // winnt
#define MAXUCHAR    0xff        // winnt
#define MAXUSHORT   0xffff      // winnt
#define MAXULONG    0xffffffff  // winnt

#ifdef __cplusplus
}
#endif
