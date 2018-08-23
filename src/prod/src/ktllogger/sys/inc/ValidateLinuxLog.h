
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

class AsyncValidateLinuxLogForDDContext : public KAsyncContextBase, public KThreadPool::WorkItem
{
    K_FORCE_SHARED(AsyncValidateLinuxLogForDDContext);

    public:
        static NTSTATUS Create(
            __out AsyncValidateLinuxLogForDDContext::SPtr& Context,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
            );
        
        VOID
        StartValidate(
            __in LPCWSTR LogFile,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    protected:
        VOID
        OnStart(
            ) override ;

        VOID
        Execute() override ;

    private:
        LPCWSTR _LogFile;
};
