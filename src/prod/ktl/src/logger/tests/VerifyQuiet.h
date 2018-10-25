/*++

Copyright (c) 2012 Microsoft Corporation

Module Name:

    VerifyQuiet.h

Abstract:

    Header for defining VERIFY_IS macros

--*/


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
