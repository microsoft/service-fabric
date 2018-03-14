// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class MultipleReplicaCloseCompletionCheckAsyncOperation : public Common::AsyncOperation
        {
        public:
            typedef std::function<void(std::wstring const &, Infrastructure::EntityEntryBaseList const &, ReconfigurationAgent &)> PendingReplicaCallback;
            
            // Check if the replica is closed
            // Return null if it is closed or the service type registration if it is not
            // The replica close mode provided to the close completion check class is passed in as an argument
            typedef std::function<bool (ReplicaCloseMode, FailoverUnit &)> IsReplicaClosedFunctionPtr;

            struct Parameters
            {
                Parameters() :
                    MonitoringIntervalEntry(nullptr),
                    MaxWaitBeforeTerminationEntry(nullptr),
                    RA(nullptr),
                    TerminateReason(Hosting::TerminateServiceHostReason::FabricUpgrade),
                    Check(Infrastructure::JobItemCheck::RAIsOpen)
                {
                }

                Infrastructure::IClockSPtr Clock;

                // The job item check used for the job item
                Infrastructure::JobItemCheck::Enum Check;

                // ActivityId: Used for tracing
                std::wstring ActivityId;

                // The FTs that are being closed
                Infrastructure::EntityEntryBaseList FTsToClose;

                // The close mode - used to check whether close is complete
                ReplicaCloseMode CloseMode;

                // The timer interval after which to poll and check if close is complete
                TimeSpanConfigEntry const * MonitoringIntervalEntry;

                // The duration after which the FTs not closed should be terminated 
                // Can be null if termination is not enabled
                TimeSpanConfigEntry const * MaxWaitBeforeTerminationEntry;

                ReconfigurationAgent * RA;

                // The callback which is invoked when FTs have not yet finished close
                // With the set of FTs still pending close
                PendingReplicaCallback Callback;

                // The reason that should be provided to hosting subsystem 
                Hosting::TerminateServiceHostReason::Enum TerminateReason;

                // The function to use to check if the replica is closed
                // By default it is the IsReplicaClosed public static method 
                IsReplicaClosedFunctionPtr IsReplicaClosedFunction;
            };

            MultipleReplicaCloseCompletionCheckAsyncOperation(
                Parameters && parameters,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);

            void Test_StartCompletionCheck(
                Common::AsyncOperationSPtr const& thisSPtr);

            __declspec(property(get = get_Test_Timer)) Common::TimerSPtr Test_Timer;
            Common::TimerSPtr get_Test_Timer() const { return timer_; }

            __declspec(property(get = get_Test_IsInternalCompleted)) bool Test_IsInternalCompleted;
            bool get_Test_IsInternalCompleted() const { return isCompleted_; }

            static bool IsReplicaClosed(
                ReplicaCloseMode closeMode, 
                FailoverUnit & ft);

        protected:
            void OnStart(Common::AsyncOperationSPtr const& thisSPtr) override;
            void OnCancel() override;

        private:
            typedef std::vector<Hosting2::ServiceTypeRegistrationSPtr> ServiceTypeRegistrationList;

            __declspec(property(get = get_IsTerminateEnabled)) bool IsTerminateEnabled;
            bool get_IsTerminateEnabled() const { return maxWaitBeforeTermination_ != nullptr; }

            void UpdateStateAndPerformAction(
                Common::AsyncOperationSPtr const& thisSPtr,
                Infrastructure::EntityEntryBaseList && pendingFTs,
                ServiceTypeRegistrationList const & toBeTerminatedHosts);

            void ProcessFailoverUnitWorkCompletion(
                Infrastructure::MultipleEntityWork const & work,
                Common::StopwatchTime now,
                __out Infrastructure::EntityEntryBaseList & pendingFTs,
                __out ServiceTypeRegistrationList & toBeTerminatedHosts) const;

            void StartCloseCompletionCheck(
                Common::AsyncOperationSPtr const& thisSPtr);

            bool CloseCompletionCheckProcessor(
                Infrastructure::HandlerParameters & handlerParameters, 
                CheckReplicaCloseProgressJobItemContext & context);

            void OnCloseCompletionCheckComplete(
                Common::AsyncOperationSPtr const& thisSPtr,
                Common::StopwatchTime now,
                Infrastructure::MultipleEntityWork const & work);

            void Complete(
                Common::AsyncOperationSPtr const& thisSPtr);

            Common::TimerSPtr CreateTimer(
                Common::AsyncOperationSPtr const & thisSPtr);

            Infrastructure::EntityEntryBaseList GetFailoverUnitsForWhichToCreateJobItems() const;

            static void CloseTimerIfNotNull(Common::TimerSPtr const & timer);

            Common::StopwatchTime startTime_;            
            Infrastructure::JobItemCheck::Enum check_;
            std::wstring activityId_;
            Hosting::TerminateServiceHostReason::Enum reason_;
            ReplicaCloseMode closeMode_;
            PendingReplicaCallback callback_;
            TimeSpanConfigEntry const & monitoringInterval_;
            TimeSpanConfigEntry const * maxWaitBeforeTermination_;
            IsReplicaClosedFunctionPtr isReplicaClosedFunction_;
            ReconfigurationAgent & ra_;
            Infrastructure::IClockSPtr clock_;

            // State that is guarded by lock
            Common::RwLock mutable lock_;
            Infrastructure::EntityEntryBaseList ftsToClose_;
            Common::TimerSPtr timer_;
            bool isCompleted_;
            bool hasTerminateBeenCalled_;
        };
    }
}



