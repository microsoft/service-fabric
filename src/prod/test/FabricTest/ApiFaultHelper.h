// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ApiFaultHelper
    {
        DENY_COPY(ApiFaultHelper);

    public:
        enum ComponentName
        {
            Service = 0,
            Provider = 1,
            Replicator = 2,
            ServiceOperationPump = 3,
            Hosting = 4,
            SP2 = 5, // IStateProvider2 interface which is implemented by TestStateProvider2 - State provider for transactional replicator
            SP2HighFrequencyApi = 6 // IStateProvider2 interface which has a high call frequency- Currently just apply and unlock
        };

        enum FaultType
        {
            Failure = 0,
            ReportFaultPermanent = 0x1,
            ReportFaultTransient = 0x2,
            Delay = 0x3,
            SetLocationWhenActive = 0x4,
            Block = 0x5
        };

        enum ApiName
        {
            // Hosting APIs
            DownloadApplication,
            AnalyzeApplicationUpgrade,
            UpgradeApplication,
            DownloadFabric,
            ValidateFabricUpgrade,
            FabricUpgrade,
            FindServiceTypeRegistration,
            GetHostId
        };

        static ApiFaultHelper & Get();

        void AddFailure(
            __in std::wstring const & nodeId,
            __in std::wstring const & serviceName,
            __in std::wstring const & strFailureIn);

        void RemoveFailure(
            __in std::wstring const & nodeId,
            __in std::wstring const & serviceName,
            __in std::wstring const & strFailureIn);

        bool ShouldFailOn(
            __in Federation::NodeId nodeId,
            __in std::wstring const& serviceName,
            __in ComponentName compName,
            __in std::wstring const& operationName,
            __in FaultType faultType = Failure) const;

        bool ShouldFailOn(
            __in std::wstring const& nodeId,
            __in std::wstring const& serviceName,
            __in ComponentName compName,
            __in std::wstring const& operationName,
            __in FaultType faultType = Failure) const;

        bool ShouldFailOn(
            __in Federation::NodeId nodeId,
            __in std::wstring const& serviceName,
            __in ComponentName compName,
            __in ApiName apiName,
            __in FaultType faultType = Failure) const;

        Common::TimeSpan GetAsyncCompleteDelayTime();
        Common::TimeSpan GetApiDelayInterval();

        __declspec(property(get = get_AreRandomApiFaultsEnabled)) bool AreRandomApiFaultsEnabled;
        bool get_AreRandomApiFaultsEnabled() const
        {
            return randomApiFaultsEnabled_;
        }

        void SetRandomApiFaultsEnabled(bool value);
        void SetMaxApiDelayInterval(Common::TimeSpan maxApiDelayInterval);
        void SetMinApiDelayInterval(Common::TimeSpan minApiDelayInterval);
        void SetRandomFaults(std::map<std::wstring, double, Common::IsLessCaseInsensitiveComparer<std::wstring>> randomFaults);

    private:
        static BOOL CALLBACK InitConfigFunction(PINIT_ONCE, PVOID, PVOID *);
        static INIT_ONCE initOnce_;
        static Common::Global<ApiFaultHelper> singleton_;

        std::wstring GetFailureName(ComponentName compName, std::wstring const& operationName, FaultType faultType) const;
        std::wstring GetFailureName(ComponentName compName, ApiName api, FaultType faultType) const;
        double GetRandomFaultRatio(ComponentName compName, FaultType faultType) const;
        double GetRandomFaultRatio(std::wstring const& name) const;

        double GetRandomDouble() const;

        ApiFaultHelper();

        BehaviorStore failureStore_;

        bool randomApiFaultsEnabled_;

        Common::TimeSpan maxApiDelayInterval_;
        Common::TimeSpan minApiDelayInterval_;

        mutable Common::ExclusiveLock randomLock_;
        mutable Common::Random random_;

        std::map<std::wstring, double, Common::IsLessCaseInsensitiveComparer<std::wstring>> randomFaults_;

        static Common::StringLiteral const TraceSource;
    };
};
