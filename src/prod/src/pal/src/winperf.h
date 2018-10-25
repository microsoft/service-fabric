// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// 188
#define PERF_SIZE_DWORD         0x00000000  // 32 bit field
#define PERF_SIZE_LARGE         0x00000100  // 64 bit field
#define PERF_SIZE_ZERO          0x00000200  // for Zero Length fields
#define PERF_SIZE_VARIABLE_LEN  0x00000300  // length is in CounterLength field
                                            //  of Counter Definition struct
#define PERF_TYPE_NUMBER        0x00000000  // a number (not a counter)
#define PERF_TYPE_COUNTER       0x00000400  // an increasing numeric value
#define PERF_TYPE_TEXT          0x00000800  // a text field
#define PERF_TYPE_ZERO          0x00000C00  // displays a zero

#define PERF_NUMBER_HEX         0x00000000  // display as HEX value
#define PERF_NUMBER_DECIMAL     0x00010000  // display as a decimal integer
#define PERF_NUMBER_DEC_1000    0x00020000  // display as a decimal/1000

#define PERF_COUNTER_VALUE      0x00000000  // display counter value
#define PERF_COUNTER_RATE       0x00010000  // divide ctr / delta time
#define PERF_COUNTER_FRACTION   0x00020000  // divide ctr / base
#define PERF_COUNTER_BASE       0x00030000  // base value used in fractions
#define PERF_COUNTER_ELAPSED    0x00040000  // subtract counter from current time
#define PERF_COUNTER_QUEUELEN   0x00050000  // Use Queuelen processing func.
#define PERF_COUNTER_HISTOGRAM  0x00060000  // Counter begins or ends a histogram
#define PERF_COUNTER_PRECISION  0x00070000  // divide ctr / private clock

#define PERF_TEXT_UNICODE       0x00000000  // type of text in text field
#define PERF_TEXT_ASCII         0x00010000  // ASCII using the CodePage field

#define PERF_TIMER_TICK         0x00000000  // use system perf. freq for base
#define PERF_TIMER_100NS        0x00100000  // use 100 NS timer time base units
#define PERF_OBJECT_TIMER       0x00200000  // use the object timer freq

#define PERF_DELTA_COUNTER      0x00400000  // compute difference first
#define PERF_DELTA_BASE         0x00800000  // compute base diff as well
#define PERF_INVERSE_COUNTER    0x01000000  // show as 1.00-value (assumes:
#define PERF_MULTI_COUNTER      0x02000000  // sum of multiple instances

#define PERF_DISPLAY_NO_SUFFIX  0x00000000  // no suffix
#define PERF_DISPLAY_PER_SEC    0x10000000  // "/sec"
#define PERF_DISPLAY_PERCENT    0x20000000  // "%"
#define PERF_DISPLAY_SECONDS    0x30000000  // "secs"
#define PERF_DISPLAY_NOSHOW     0x40000000  // value is not displayed

// 251
#define PERF_COUNTER_COUNTER        \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_PER_SEC)

#define PERF_COUNTER_TIMER          \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_PERCENT)

// 263
#define PERF_COUNTER_QUEUELEN_TYPE  \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_QUEUELEN |\
            PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)

// 268
#define PERF_COUNTER_LARGE_QUEUELEN_TYPE  \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_QUEUELEN |\
            PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)

// 274
#define PERF_COUNTER_100NS_QUEUELEN_TYPE  \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_QUEUELEN |\
            PERF_TIMER_100NS | PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)

// 285
#define PERF_COUNTER_BULK_COUNT     \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_PER_SEC)

// 297
#define PERF_COUNTER_RAWCOUNT       \
            (PERF_SIZE_DWORD | PERF_TYPE_NUMBER | PERF_NUMBER_DECIMAL |\
            PERF_DISPLAY_NO_SUFFIX)

// Same as PERF_COUNTER_RAWCOUNT except its size is a large integer
#define PERF_COUNTER_LARGE_RAWCOUNT       \
            (PERF_SIZE_LARGE | PERF_TYPE_NUMBER | PERF_NUMBER_DECIMAL |\
            PERF_DISPLAY_NO_SUFFIX)

#define PERF_COUNTER_RAWCOUNT_HEX       \
            (PERF_SIZE_DWORD | PERF_TYPE_NUMBER | PERF_NUMBER_HEX |\
            PERF_DISPLAY_NO_SUFFIX)

#define PERF_COUNTER_LARGE_RAWCOUNT_HEX       \
            (PERF_SIZE_LARGE | PERF_TYPE_NUMBER | PERF_NUMBER_HEX |\
            PERF_DISPLAY_NO_SUFFIX)

// 322
#define PERF_SAMPLE_FRACTION        \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION |\
            PERF_DELTA_COUNTER | PERF_DELTA_BASE | PERF_DISPLAY_PERCENT)

#define PERF_SAMPLE_COUNTER         \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)

#define PERF_COUNTER_TIMER_INV      \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_INVERSE_COUNTER | \
            PERF_DISPLAY_PERCENT)

#define PERF_SAMPLE_BASE            \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |\
            PERF_DISPLAY_NOSHOW |\
            0x00000001)  // for compatibility with pre-beta versions

#define PERF_AVERAGE_TIMER          \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION |\
            PERF_DISPLAY_SECONDS)

// 364
#define PERF_AVERAGE_BASE           \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |\
            PERF_DISPLAY_NOSHOW |\
            0x00000002)  // for compatibility with pre-beta versions

#define PERF_AVERAGE_BULK           \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION  |\
            PERF_DISPLAY_NOSHOW)

#define PERF_OBJ_TIME_TIMER    \
            (PERF_SIZE_LARGE   | PERF_TYPE_COUNTER  | PERF_COUNTER_RATE |\
             PERF_OBJECT_TIMER | PERF_DELTA_COUNTER | PERF_DISPLAY_PERCENT)

// 386
#define PERF_100NSEC_TIMER          \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_TIMER_100NS | PERF_DELTA_COUNTER | PERF_DISPLAY_PERCENT)

#define PERF_100NSEC_TIMER_INV      \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_TIMER_100NS | PERF_DELTA_COUNTER | PERF_INVERSE_COUNTER  |\
            PERF_DISPLAY_PERCENT)

#define PERF_COUNTER_MULTI_TIMER    \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_DELTA_COUNTER | PERF_TIMER_TICK | PERF_MULTI_COUNTER |\
            PERF_DISPLAY_PERCENT)

#define PERF_COUNTER_MULTI_TIMER_INV \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_DELTA_COUNTER | PERF_MULTI_COUNTER | PERF_TIMER_TICK |\
            PERF_INVERSE_COUNTER | PERF_DISPLAY_PERCENT)

#define PERF_COUNTER_MULTI_BASE     \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |\
            PERF_MULTI_COUNTER | PERF_DISPLAY_NOSHOW)

#define PERF_100NSEC_MULTI_TIMER   \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_DELTA_COUNTER  |\
            PERF_COUNTER_RATE | PERF_TIMER_100NS | PERF_MULTI_COUNTER |\
            PERF_DISPLAY_PERCENT)

#define PERF_100NSEC_MULTI_TIMER_INV \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_DELTA_COUNTER  |\
            PERF_COUNTER_RATE | PERF_TIMER_100NS | PERF_MULTI_COUNTER |\
            PERF_INVERSE_COUNTER | PERF_DISPLAY_PERCENT)

// 438
#define PERF_RAW_FRACTION           \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION |\
            PERF_DISPLAY_PERCENT)

#define PERF_LARGE_RAW_FRACTION           \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION |\
            PERF_DISPLAY_PERCENT)

// 448
#define PERF_RAW_BASE               \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |\
            PERF_DISPLAY_NOSHOW |\
            0x00000003)  // for compatibility with pre-beta versions

#define PERF_LARGE_RAW_BASE               \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |\
            PERF_DISPLAY_NOSHOW )

// 464
#define PERF_ELAPSED_TIME           \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_ELAPSED |\
            PERF_OBJECT_TIMER | PERF_DISPLAY_SECONDS)

#define PERF_COUNTER_DELTA      \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_VALUE |\
            PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)

#define PERF_COUNTER_LARGE_DELTA      \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_VALUE |\
            PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)

#define PERF_PRECISION_SYSTEM_TIMER \
        (PERF_SIZE_LARGE    | PERF_TYPE_COUNTER     | PERF_COUNTER_PRECISION    | \
         PERF_TIMER_TICK    | PERF_DELTA_COUNTER    | PERF_DISPLAY_PERCENT   )

#define PERF_PRECISION_100NS_TIMER  \
        (PERF_SIZE_LARGE    | PERF_TYPE_COUNTER     | PERF_COUNTER_PRECISION    | \
         PERF_TIMER_100NS   | PERF_DELTA_COUNTER    | PERF_DISPLAY_PERCENT   )

#define PERF_PRECISION_OBJECT_TIMER \
        (PERF_SIZE_LARGE    | PERF_TYPE_COUNTER     | PERF_COUNTER_PRECISION    | \
         PERF_OBJECT_TIMER  | PERF_DELTA_COUNTER    | PERF_DISPLAY_PERCENT   )

// 531
#define PERF_DETAIL_NOVICE          100 // The uninformed can understand it
#define PERF_DETAIL_ADVANCED        200 // For the advanced user
#define PERF_DETAIL_EXPERT          300 // For the expert user
#define PERF_DETAIL_WIZARD          400 // For the system designer

#ifdef __cplusplus
}
#endif
