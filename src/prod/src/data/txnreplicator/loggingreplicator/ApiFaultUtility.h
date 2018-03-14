// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    typedef enum ApiName
    {
        Invalid,
        ReplicateAsync,
        ApplyAsync,
        Unlock,
        PrepareCheckpoint,
        PerformCheckpoint,
        CompleteCheckpoint,
        WaitForLogFlushUptoLsn,

    }ApiName;
        
    class ApiFaultUtility final
        : public KObject<ApiFaultUtility>
        , public KShared<ApiFaultUtility>
    {
        K_FORCE_SHARED(ApiFaultUtility)

    public:

        static ApiFaultUtility::SPtr Create(__in KAllocator & allocator);

        ktl::Awaitable<NTSTATUS> WaitUntilSignaledAsync(__in ApiName apiName);

        NTSTATUS WaitUntilSignaled(__in ApiName apiName);

        void BlockApi(__in ApiName apiName);

        void DelayApi(
            __in ApiName apiName,
            __in Common::TimeSpan const & duration);

        void FailApi(
            __in ApiName apiName,
            __in NTSTATUS errorCode);

        void ClearFault(__in ApiName apiName);

    private:

        typedef struct FaultInfo
        {
            FaultInfo()
                : apiName_(ApiName::Invalid)
                , delayDuration_(Common::TimeSpan::Zero)
                , errorCode_(STATUS_SUCCESS)
            {
            }

            FaultInfo(
                __in ApiName apiName,
                __in Common::TimeSpan const & delayDuration,
                __in NTSTATUS errorCode)
                : apiName_(apiName)
                , delayDuration_(delayDuration)
                , errorCode_(errorCode)
            {
            }
            
            ApiName apiName_;
            Common::TimeSpan delayDuration_;
            NTSTATUS errorCode_;

        }FaultInfo;

        void AddFaultInfo(__in FaultInfo const & faultInfo);
        FaultInfo GetFaultInfo(__in ApiName apiName);

        KSpinLock lock_;
        KArray<FaultInfo> faultData_;
    };
}
