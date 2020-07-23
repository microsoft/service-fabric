// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "./sys/inc/ktllogger.h"
#include "KtlLoggerNode.h"

namespace KtlLogger
{
    class KtlLoggerInitializationContext : public KAsyncContextBase

    {
        K_FORCE_SHARED_WITH_INHERITANCE(KtlLoggerInitializationContext);

        public:
            static NTSTATUS Create(
                __in KAllocator& Allocator,
                __in ULONG AllocationTag,
                __in KtlLoggerNode* KtlLoggerNode,
                __out KtlLoggerInitializationContext::SPtr& Context
            );

            VOID
            StartInitializeKtlLogger(
                __in BOOLEAN UseInprocLogger,
                __in KtlLogManager::MemoryThrottleLimits& MemoryLimits,
                __in KtlLogManager::AcceleratedFlushLimits& AccelerateFlushLimits,
                __in KtlLogManager::SharedLogContainerSettings& SharedLogSettings,
                __in LPCWSTR NodeWorkDirectory,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

        protected:
            VOID OperationCompletion(
                __in_opt KAsyncContextBase* const ParentAsync,
                __in KAsyncContextBase& Async
                );

            VOID OpenCloseCompletion(
                __in KAsyncContextBase* const ParentAsync,
                __in KAsyncServiceBase& Async,
                __in NTSTATUS Status);

            VOID CloseContainerCompletion(
                __in_opt KAsyncContextBase* const Parent,
                __in KtlLogContainer& LogContainer,
                __in NTSTATUS Status
            );
                    
            VOID
            OnStart(
                ) override ;

            VOID
            OnReuse(
                ) override ;

            VOID
            OnCompleted(
                ) override ;

            VOID
            OnCancel(
                ) override ;

        private:
            enum { Initial,
                   OpenLogManagerX,
                   ConfigureThrottleSettings3, ConfigureThrottleSettings2, ConfigureThrottleSettings, ConfigureSharedLog,
                   ConfigureAccelerateFlushSettings,
                   OpenSharedLog, CloseSharedLog,
                   CloseLogManagerX,
                   Completed } _State;

            VOID
            FSMContinue(
                __in NTSTATUS Status
                );


        private:
            //
            // General members
            //
            KtlLoggerNode* _KtlLoggerNode;

            //
            // Parameters to api
            //
            BOOLEAN _UseInprocLogger;
            KtlLogManager::MemoryThrottleLimits _MemoryLimits;
            KtlLogManager::SharedLogContainerSettings _SharedLogSettings;
            KtlLogManager::AcceleratedFlushLimits _AccelerateFlushLimits;
            WCHAR _NodeWorkDirectory[MAX_PATH];

            //
            // Members needed for functionality
            //
            NTSTATUS _FinalStatus;
            KtlLogManager::AsyncConfigureContext::SPtr _ConfigureAsync;
            KtlLogManager::SPtr _LogManager;
            KtlLogContainer::SPtr _LogContainer;
            KBuffer::SPtr _InBuffer;
            KBuffer::SPtr _OutBuffer;
            ULONG _Result;
    };
}

