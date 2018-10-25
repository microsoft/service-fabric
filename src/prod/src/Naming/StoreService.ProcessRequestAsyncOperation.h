// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
#define DEFINE_PERF_COUNTERS( CounterName ) \
    Common::PerformanceCounterData & GetRatePerfCounter() { return this->PerfCounters.RateOf##CounterName; } \
    Common::PerformanceCounterData & GetDurationPerfCounterBase() { return this->PerfCounters.DurationOf##CounterName##Base; } \
    Common::PerformanceCounterData & GetDurationPerfCounter() { return this->PerfCounters.DurationOf##CounterName; } \
    HealthMonitoredOperationName::Enum GetHealthOperationName() const override { return HealthMonitoredOperationName::CounterName; } \


    class StoreService::ProcessRequestAsyncOperation :
        public ActivityTracedComponent<Common::TraceTaskCodes::NamingStoreService>,
        public Common::AsyncOperation
    {
    public:
        typedef std::function<void(Common::AsyncOperationSPtr const &)> RetryCallback;

        ProcessRequestAsyncOperation(
            Transport::MessageUPtr &&,
            __in NamingStore &,
            StoreServiceProperties &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        static Common::ErrorCode End(Common::AsyncOperationSPtr const &, __out Transport::MessageUPtr &);
        static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

    protected:
        void OnStart(Common::AsyncOperationSPtr const &);
        virtual void OnCompleted();
                
        // wrappers for communicating with other store services
        Common::AsyncOperationSPtr BeginRequestReplyToPeer(
            Transport::MessageUPtr &&,
            Federation::NodeInstance const &,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndRequestReplyToPeer(
            Common::AsyncOperationSPtr const &,
            __out Transport::MessageUPtr &);                

        // resolve async operation for naming services
        Common::AsyncOperationSPtr BeginResolveNameLocation(
            Common::NamingUri const &,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndResolveNameLocation(
            Common::AsyncOperationSPtr const &, 
            __out SystemServices::SystemServiceLocation &);

        bool TryGetNameFromRequest(Transport::MessageUPtr const & request);
        void SetName(Common::NamingUri const &);

        virtual Common::PerformanceCounterData & GetRatePerfCounter() = 0;
        virtual Common::PerformanceCounterData & GetDurationPerfCounterBase() = 0;
        virtual Common::PerformanceCounterData & GetDurationPerfCounter() = 0;

        virtual Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr &&) = 0;
        virtual void PerformRequest(Common::AsyncOperationSPtr const &) = 0;

        // Determines whether service group reserved characters are allowed in the URI.
        // By default, names should not contain service group related characters.
        // Derived classes that allow them should override the method.
        virtual bool AllowNameFragment() { return true; }
        
        bool TryAcceptRequestAtAuthorityOwner(Common::AsyncOperationSPtr const &);
        bool TryAcceptRequestAtNameOwner(Common::AsyncOperationSPtr const &);

        bool IsLocalRetryable(Common::ErrorCode const &);
        virtual void CompleteOrScheduleRetry(Common::AsyncOperationSPtr const &, Common::ErrorCode &&, RetryCallback const &);
        void CompleteOrRecoverPrimary(Common::AsyncOperationSPtr const &, Common::ErrorCode &&, RetryCallback const &);
        void TimeoutOrScheduleRetry(Common::AsyncOperationSPtr const &, RetryCallback const &);

        // general helper methods for subclasses
        Common::TimeSpan const GetRemainingTime();
        bool HasRunOutOfTime();

        __declspec(property(get=get_Node)) Federation::NodeInstance const & Node;
        __declspec(property(get=get_Store)) NamingStore & Store;
        __declspec(property(get=get_Properties)) StoreServiceProperties & Properties;
        __declspec(property(get=get_PerfCounters)) NamingPerformanceCounters & PerfCounters;
        __declspec(property(get=get_PsdCache)) Naming::PsdCache & PsdCache;
        __declspec(property(get=get_Name)) Common::NamingUri const & Name;
        __declspec(property(get=get_RequestInstance)) __int64 RequestInstance;
        __declspec(property(get=get_IsPrimaryRecovery)) bool IsPrimaryRecovery;
        __declspec(property(put=put_Reply)) Transport::MessageUPtr const & Reply;
        __declspec(property(get=get_NameString)) std::wstring const & NameString;
        __declspec(property(get=get_IsForce, put=put_IsForce)) bool IsForce;
        __declspec(property(get=get_OperationRetryStopwatch)) Common::Stopwatch OperationRetryStopwatch;
        Common::Stopwatch get_OperationRetryStopwatch() const { return stopwatch_; }

        Federation::NodeInstance const & get_Node() const { return properties_.Instance; }
        NamingStore & get_Store() { return namingStore_; }     
        StoreServiceProperties & get_Properties() { return properties_; }
        NamingPerformanceCounters & get_PerfCounters() { return *properties_.PerfCounters; }
        Naming::PsdCache & get_PsdCache() { return properties_.PsdCache; }
        Common::NamingUri const & get_Name() const { return name_; }     
        __int64 get_RequestInstance() const { return requestInstance_; }
        bool get_IsPrimaryRecovery() const { return isPrimaryRecovery_; }
        void SetIsPrimaryRecovery() { isPrimaryRecovery_ = true; }
        void put_Reply(Transport::MessageUPtr && value) { replyResult_ = std::move(value); }
        std::wstring const & get_NameString() const { return nameString_; }  
        bool get_IsForce() const { return isForce_; }
        void put_IsForce(bool isForce) { isForce_ = isForce; }

        void ProcessRecoveryHeader(Transport::MessageUPtr &&);

        virtual HealthMonitoredOperationName::Enum GetHealthOperationName() const = 0;
        virtual Common::TimeSpan GetProcessOperationHealthDuration() const;
        virtual void AddHealthMonitoredOperation();
        virtual void CompleteHealthMonitoredOperation();

    private:
        void ScheduleRetry(Common::AsyncOperationSPtr const &, Common::TimeSpan const delay, RetryCallback const &);
        virtual bool ShouldTerminateProcessing() const { return false; }

        Transport::MessageUPtr request_;
        NamingStore & namingStore_;
        StoreServiceProperties & properties_;
        Common::TimeoutHelper timeoutHelper_;

        // Harvested from message body or headers
        //
        Common::NamingUri name_;
        __int64 requestInstance_;
        bool isPrimaryRecovery_;    // Indicates this request is being processed in the context of a primary recovery operation
        Common::Stopwatch stopwatch_;

        // Cache the name string, to avoid computing it all the time
        std::wstring nameString_;
        bool isForce_;

        ServiceLocationVersion lastNameLocationVersion_;
        Common::TimerSPtr retryTimer_;
        Common::ExclusiveLock timerLock_;

        Transport::MessageUPtr replyResult_;
    };
}
