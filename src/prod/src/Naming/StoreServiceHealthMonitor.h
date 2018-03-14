// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    namespace HealthMonitoredOperationName
    {
        enum Enum
        {
            PrimaryRecoveryStarted = 0,
            AOCreateName = 1,
            AODeleteName = 2,
            AOCreateService = 3,
            AODeleteService = 4,
            AOUpdateService = 5, 
            NOCreateName = 6,
            NODeleteName = 7,
            NOCreateService = 8,
            NODeleteService = 9,
            NOUpdateService = 10,
            NameExists = 11,
            EnumerateProperties = 12,
            EnumerateNames = 13,
            GetServiceDescription = 14,
            PrefixResolve = 15,
            PropertyBatch = 16,
            InvalidOperation = 17,
            PrimaryRecovery = 18,

            LastValidEnum = InvalidOperation
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum e);

        DECLARE_ENUM_STRUCTURED_TRACE(HealthMonitoredOperationName);
    }

    class HealthMonitoredOperation
    {
        DENY_COPY(HealthMonitoredOperation)

    public:
        HealthMonitoredOperation(
            Common::ActivityId const & activityId,
            HealthMonitoredOperationName::Enum operationName,
            std::wstring const & operationMetadata,
            Common::TimeSpan const & maxAllowedDuration);

        ~HealthMonitoredOperation();
        
        __declspec(property(get = get_OperationName)) HealthMonitoredOperationName::Enum OperationName;
        HealthMonitoredOperationName::Enum get_OperationName() const { return operationName_; }

        __declspec(property(get = get_ActivityId)) Common::ActivityId const & ActivityId;
        Common::ActivityId const & get_ActivityId() const { return activityId_; }

        __declspec(property(get = get_OperationMetadata)) std::wstring const & OperationMetadata;
        std::wstring const & get_OperationMetadata() const { return operationMetadata_; }

        // Checks whether the operation has been already reported on
        bool IsSlowDetected() const;

        // Shows whether the operation was completed.
        // The operation may be kept after completion so retries 
        // are measured against the initial start time.
        bool IsCompleted() const;

        // Checks whether an operation that previously respected the duration
        // has become too slow or
        // a slow operation should be reported again.
        bool MarkSendHealthReport(Common::StopwatchTime const & now);
                
        // If an operation is completed and no retry has been started in a long time
        // mark it for removal.
        bool ShouldBeRemoved(Common::StopwatchTime const & now) const;
                
        // The operation was restarted. For example, AO CreateService sends CreateService to NO, which times out.
        // The operation is completed with Timeout on NO, but AO retries. 
        // The next request on NO should not reset the start time, because the global operation takes longer than each retry.
        void ResetCompletedInfo(Common::ActivityId const & activityId);
                
        // The operation completed with error, mark the completed time.
        void Complete(Common::ErrorCode const & error);

        // Generate a health report based on the current state of the monitored operation.
        ServiceModel::HealthReport GenerateHealthReport(
            ServiceModel::EntityHealthInformationSPtr && entityInfo,
            Common::TimeSpan const & reportTTL,
            int64 reportSequenceNumber) const;
        
    private:
        std::wstring GenerateHealthReportProperty() const;
        std::wstring GenerateHealthReportExtraDescription() const;

    private:
        Common::ActivityId activityId_;
        HealthMonitoredOperationName::Enum operationName_;
        std::wstring operationMetadata_;
        Common::TimeSpan maxAllowedDuration_;
        Common::StopwatchTime startTime_;
        Common::StopwatchTime completedTime_;
        Common::ErrorCode completedError_;
        Common::StopwatchTime lastReportTime_;
        mutable Common::TimeSpan lastReportTTL_;
    };

    using HealthMonitoredOperationSPtr = std::shared_ptr<HealthMonitoredOperation>;

    // The unique identifier for a monitored operation is composed of the type and the operation metadata
    // NOTE: the metadata is kept as reference, to avoid extra copy of the string
    using HealthMonitoredOperationKey = std::pair<HealthMonitoredOperationName::Enum, std::wstring const &>;

    class StoreServiceHealthMonitor
        : public Common::RootedObject
        , public Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::NamingStoreService>
    {
        DENY_COPY( StoreServiceHealthMonitor );

    public:
        StoreServiceHealthMonitor(
            Common::ComponentRoot const & root,
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId,
            __in Reliability::FederationWrapper & federation);

        virtual ~StoreServiceHealthMonitor();

        // A monitored operation has been started, add to the list of monitored operations.
        // If there is another operation for the same key, re-use it and do not add the new one.
        // This is to prevent retries to generate reports with different start time and parameters.
        void AddMonitoredOperation(
            Common::ActivityId const & activityId,
            HealthMonitoredOperationName::Enum operationName,
            std::wstring const & operationMetadata,
            Common::TimeSpan const & maxAllowedDuration);

        // The monitored operation has completed successfully, clear the previous health report 
        // and remove it from the list of monitored operations.
        // If the operation came from a retry, it will not be in the list, in which case nothing is done.
        void CompleteSuccessfulMonitoredOperation(
            Common::ActivityId const & activityId,
            HealthMonitoredOperationName::Enum operationName,
            std::wstring const & operationMetadata);

        // The monitored operation has completed, clear the previous health report 
        // and remove it from the list of monitored operations.
        // If the operation came from a retry, it will not be in the list, in which case nothing is done.
        // keepOperation: For certain retryable errors, keep the operation for longer
        // to remember initial start time as opposed to retry time.
        void CompleteMonitoredOperation(
            Common::ActivityId const & activityId,
            HealthMonitoredOperationName::Enum operationName,
            std::wstring const & operationMetadata,
            Common::ErrorCode const & error,
            bool keepOperation);
        
        void Open();
        void Close();

    private:
        ServiceModel::EntityHealthInformationSPtr CreateNamingPrimaryEntityInfo() const;
        
        void OnTimerCallback();
        void DisableTimerCallerHoldsLock();
        void EnableTimerCallerHoldsLock();

    private:
        struct Comparator
        {
            bool operator()(HealthMonitoredOperationKey const & left, HealthMonitoredOperationKey const & right) const;
        };

        Reliability::FederationWrapper & federation_;
        Client::HealthReportingComponentSPtr healthClient_;

        RWLOCK(StoreServiceHealthMonitor, lock_);
        Common::TimerSPtr timer_;

        using OperationMap = std::map<HealthMonitoredOperationKey, HealthMonitoredOperationSPtr, Comparator>;
        using OperationMapEntry = std::pair<HealthMonitoredOperationKey, HealthMonitoredOperationSPtr>;
        OperationMap operations_;

        bool isOpen_;
    };

    using StoreServiceHealthMonitorUPtr = std::unique_ptr<StoreServiceHealthMonitor>;
}
