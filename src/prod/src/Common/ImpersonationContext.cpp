// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral TraceType_ImpersonationContext = "ImpersonationContext";

ImpersonationContext::ImpersonationContext(TokenHandleSPtr const & tokenHandle)
    : tokenHandle_(tokenHandle),
    impersonating_(false),
    lock_()
{
//LINUXTUDO
#if !defined(PLATFORM_UNIX)
    ASSERT_IF(!tokenHandle, "TokenHandle cannot be null");
#endif
}

ImpersonationContext::~ImpersonationContext()
{
//LINUXTUDO
#if !defined(PLATFORM_UNIX)
    auto error = RevertToSelf();
    ASSERT_IF(!error.IsSuccess(), "RevertToSelf should be a success. Error: {0}", error);
#endif    
}

ErrorCode ImpersonationContext::ImpersonateLoggedOnUser()
{
//LINUXTUDO
#if defined(PLATFORM_UNIX)
    Assert::CodingError("Not supported on Linux");
#else
    {
        AcquireExclusiveLock lock(lock_);

        if(impersonating_) { return ErrorCodeValue::Success; }

        if (::ImpersonateLoggedOnUser(tokenHandle_->Value))
        {
            impersonating_ = true;        
        }
        else
        {
            DWORD win32Error = ::GetLastError();
            return ErrorCode::TraceReturn(
                ErrorCode::FromWin32Error(win32Error),
                TraceTaskCodes::Common,
                TraceType_ImpersonationContext,
                "ImpersonateLoggedOnUser");
        }
    }
#endif
    return ErrorCodeValue::Success;
}

ErrorCode ImpersonationContext::RevertToSelf()
{     
//LINUXTUDO
#if defined(PLATFORM_UNIX)
    Assert::CodingError("Not supported on Linux");
#else
    {
        AcquireExclusiveLock lock(lock_);

        if(!impersonating_) { return ErrorCodeValue::Success; }

        if (::RevertToSelf())  
        {  
            impersonating_ = false;  
        }
        else
        {
            DWORD win32Error = ::GetLastError();
            return ErrorCode::TraceReturn(
                ErrorCode::FromWin32Error(win32Error),
                TraceTaskCodes::Common,
                TraceType_ImpersonationContext,
                "RevertToSelf");
        }
    }
        
    return ErrorCodeValue::Success;
#endif    
}
