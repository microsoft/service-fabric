/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    ktlcommon.h

    Description:
      Kernel Tempate Library (KTL): Common headers and definitions.

    History:
      Peng Li (pengli)  30-September-2010         Initial version.

--*/


#pragma once

//
// Use /W3 for Windows headers to avoid noisy build error under /W4. 
//
#pragma warning(push,3)

extern "C" {

#if KTL_USER_MODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <objbase.h>
#include <specstrings.h>
#include <perflib.h>
#include <winperf.h>
#include <winhttp.h>
#include <http.h>
#include <Shlwapi.h>
#include <wincrypt.h>

#else
#include <ntosp.h>
#include <zwapi.h>
#include <wsk.h>
#endif

#include <security.h>
#include <winerror.h>
#include <intrin.h>
#if !defined(PLATFORM_UNIX)
#include <intsafe.h>
#endif
}

#pragma warning(pop)

#define K_UNIQUE_NAME_2(x,y) x##y
#define K_UNIQUE_NAME_1(x,y) K_UNIQUE_NAME_2(x,y)
#define K_MAKE_UNIQUE_NAME(x) K_UNIQUE_NAME_1(x,__COUNTER__)

//
// Code annotation tags.
//
#define FAILABLE
#define NOFAIL

//
// Generic macros to control allocation/construction/copy behavior.
//
#define K_DENY_ASSIGNMENT(class_name) private: class_name& operator = (class_name const&);
#define K_DENY_COPY_CONSTRUCTOR(class_name) private: class_name(class_name const&);
#define K_DENY_COPY(class_name) K_DENY_ASSIGNMENT(class_name); K_DENY_COPY_CONSTRUCTOR(class_name)
#define K_NO_DYNAMIC_ALLOCATE() private:void *operator new(size_t); private:void operator delete(void*);

//
// Calculate the size of a field in a structure of type type.
//

#ifndef FIELD_SIZE
#define FIELD_SIZE(type, field) (sizeof(((type *)0)->field))
#endif

// A debug target KTL Assert() that will failfast the runtime environment
#if KTL_USER_MODE
#define KAssert NT_ASSERT
#else
#define KAssert ASSERT
#endif

//
// Define KFatal
//   KFatal is being deprecated as the name does not clearly convey its semantics.
//   Please use KInvariant instead.
//

//
// KInvariant may be hooked by an application so that the application
// can perform any logging or data collection about the situation. The
// hook may also decide to continue execution without crashing or
// asserting. Continuing execution shold almost NEVER be the case.
//
//
// NOTE:  KInvariant and KFatal should never be used within a
//        KInvariant callout as this would be recursive.
//
// Parameters:
//
//    Condition is the text string for the condition that failed
//
//    File is the name of the file where the condition failed
//
//    Line is the line number for the source code of the failing KInvariant
//
// Return:
//
//    TRUE if there should be an assertion in user mode and bugcheck in kernel
//
//    FALSE if the execution should continue. This should almost NEVER be the case.
//
//
typedef BOOLEAN (*KInvariantCalloutType)(
    __in LPCSTR Condition,
    __in LPCSTR File,
    __in ULONG Line
    );


//
// This api is used to set a new KInvariant callout. Although there is
// only one global where the latest callout is set, the api returns the
// previously set callout and thus allows callers to "stack" callouts
// by chaining them.
//
// Parameters:
//
//    Callout is the new KInvariant callout to use
//
// Return:
//
//    The previously set KInvariant callout
//
//
KInvariantCalloutType SetKInvariantCallout(
    __in KInvariantCalloutType Callout
);

extern KInvariantCalloutType KInvariantCallout;

#if KTL_USER_MODE
#define KFatal(_exp)     ( ((!(_exp)) && (KInvariantCallout(#_exp, __FILE__, __LINE__)))  ? \
        (__annotation(L"Debug", L"AssertFail", L ## #_exp), \
	DbgRaiseAssertionFailure(), FALSE) : \
	TRUE)
#else
#define RVD_BUGCHECK_CODE (0xE0010001)

//
// Suppress Prefast warning 28159 (Consider using 'error logging or driver shutdown' instead of 'KeBugCheckEx').
// KFatal is only meant to be used when a fatal error has happened.
// Use of KeBugCheckEx() is justified.
//
#define KFatal(cond) \
    __pragma(warning(disable:28159)) \
    ( ((!(cond)) && (KInvariantCallout(#cond, __FILE__, __LINE__)))  ? \
    (KeBugCheckEx(RVD_BUGCHECK_CODE, (ULONG_PTR) #cond, (ULONG_PTR) __FILE__, __LINE__, NULL ), FALSE) : \
    TRUE)
#endif

// Invariant assert for both debug and release build
#define KInvariant(_exp) KFatal(_exp)

// Make sure __min()/__max() are defined
#ifndef __min
#define __min(a,b)  (((a) < (b)) ? (a) : (b))
#endif /*__min*/
#ifndef __max
#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#endif /*__max*/

// Safe memcpy and arithmetic primitives
// Strictly speaking, comparing destPtr and srcPtr with <, <=, >, or >= is undefined if they point to different objects or arrays.
// However the result should be correct for any architecture with a flat address space, which nearly all modern architectures have.
// If we would like to run debug builds on an architecture with e.g. segmented memory, this assert may need to be disabled.
// ALSO: assert will not detect overlapping regions mapped by separate address spaces, e.g. by mmap
#pragma intrinsic(memcpy)
#define KMemCpySafe(destPtr, destSize, srcPtr, srcSize) \
{\
    KAssert((reinterpret_cast<uintptr_t>((PVOID)(destPtr)) >= reinterpret_cast<uintptr_t>((PVOID)(srcPtr)) + srcSize) \
        ||  (reinterpret_cast<uintptr_t>((PVOID)(destPtr)) + srcSize <= reinterpret_cast<uintptr_t>((PVOID)(srcPtr)))); \
    KAssert((destSize) >= (srcSize)); \
    (destSize); \
    memcpy((destPtr), (srcPtr), (srcSize)); \
}
