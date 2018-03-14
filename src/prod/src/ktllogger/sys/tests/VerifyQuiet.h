// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#if !defined(PLATFORM_UNIX)
#include "WexTestClass.h"

#ifdef VERIFY_QUIET
#undef VERIFY_IS_TRUE
# define VERIFY_IS_TRUE(__condition, ...) if (! (__condition)) {  DebugBreak(); (bool)WEX::TestExecution::Private::MacroVerify::IsTrue((__condition), (L#__condition), PRIVATE_VERIFY_ERROR_INFO, __VA_ARGS__); };
#endif

#ifdef KInvariant
#undef KInvariant
#endif
#define KInvariant(x) VERIFY_IS_TRUE( (x) ? TRUE : FALSE )
#else
# define VERIFY_IS_TRUE(__condition, ...) if (! (__condition)) {  printf("[FAIL] File %s Line %d\n", __FILE__, __LINE__); KInvariant(FALSE); };
#endif
