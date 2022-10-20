// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef __STATIC_ASSERT_H__
#define __STATIC_ASSERT_H__

// static_assert( cond, msg ) is now a compiler-supported intrinsic in Dev10 C++ compiler.
// Replaces previous uses of STATIC_ASSERT_MSG and COMPILE_TIME_ASSERT_MSG.

// Replaces previous uses of CPP_ASSERT
#define static_assert_n( n, cond ) static_assert( cond, #cond )

// Replaces previous uses of C_ASSERT and COMPILE_TIME_ASSERT
#define static_assert_no_msg( cond ) static_assert( cond, #cond )

#endif // __STATIC_ASSERT_H__

