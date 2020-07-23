// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef __ENTRYPOINTS_h__
#define __ENTRYPOINTS_h__

#define BEGIN_ENTRYPOINT_THROWS \
    BEGIN_SO_INTOLERANT_CODE(GetThread()) \


#define END_ENTRYPOINT_THROWS   \
        END_SO_INTOLERANT_CODE;

#define BEGIN_ENTRYPOINT_THROWS_WITH_THREAD(____thread) \
    BEGIN_SO_INTOLERANT_CODE(____thread) \
	
#define END_ENTRYPOINT_THROWS_WITH_THREAD   \
        END_SO_INTOLERANT_CODE;

#define BEGIN_ENTRYPOINT_NOTHROW_WITH_THREAD(___thread) \
        BEGIN_SO_INTOLERANT_CODE_NO_THROW_CHECK_THREAD(return COR_E_STACKOVERFLOW)
        
#define END_ENTRYPOINT_NOTHROW_WITH_THREAD  \
        END_SO_INTOLERANT_CODE;

#define BEGIN_ENTRYPOINT_NOTHROW \
        BEGIN_SO_INTOLERANT_CODE_NO_THROW_CHECK_THREAD(return COR_E_STACKOVERFLOW)

#define END_ENTRYPOINT_NOTHROW  \
        END_SO_INTOLERANT_CODE;

extern void (*g_fpHandleSoftStackOverflow)(BOOL fSkipDebugger);
inline void FailedVoidEntryPoint()
{
    if (g_fpHandleSoftStackOverflow)        
    {                                       
        g_fpHandleSoftStackOverflow(FALSE);   
    }                                       
}
#define BEGIN_ENTRYPOINT_VOIDRET \
        BEGIN_SO_INTOLERANT_CODE_NO_THROW_CHECK_THREAD(FailedVoidEntryPoint();)

#define END_ENTRYPOINT_VOIDRET  \
        END_SO_INTOLERANT_CODE;

#define BEGIN_CLEANUP_ENTRYPOINT    \
        VALIDATE_BACKOUT_STACK_CONSUMPTION;
        
#define END_CLEANUP_ENTRYPOINT

#endif  // __ENTRYPOINTS_h__


