// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class BackgroundManager : public Common::RootedObject
        {
            DENY_COPY(BackgroundManager);

        public:
            BackgroundManager(FailoverManager & fm, Common::ComponentRoot const & root);

            __declspec (property(get=get_Id)) std::wstring const& Id;
            std::wstring const& BackgroundManager::get_Id() const;

            __declspec (property(get=get_IsStateTraceEnabled)) bool IsStateTraceEnabled;
            bool get_IsStateTraceEnabled() const { return isStateTraceEnabled_; }

            __declspec (property(get = get_IsAdminTraceEnabled)) bool IsAdminTraceEnabled;
            bool get_IsAdminTraceEnabled() const { return isAdminTraceEnabled_; }

            void Start();
            void Stop();
            void ScheduleRun();

            bool IsThrottled() const
            {
                return isThrottled_;
            }

            bool Resume();

            // Processes a FailoverUnit by running it through the state machine. If the state machine
            // generates actions, performs the actions if the retry policy allows for it.
            bool ProcessFailoverUnit(LockedFailoverUnitPtr && failoverUnit, bool isBackground);

            void AddThreadContext(BackgroundThreadContextUPtr && context);

            void PostProcessFailoverUnit(
                LockedFailoverUnitPtr & failoverUnit,
                std::vector<StateMachineActionUPtr> const & actions,
                bool updated,
                int replicaDifference,
                int64 commitDuration,
                int64 plbDuration,
                bool isBackground);

            void ProcessThreadContexts(FailoverUnit const & failoverUnit, bool isSuccess);

        private:
            struct EnumerationContext
            {
                EnumerationContext();
                EnumerationContext(std::vector<BackgroundThreadContextUPtr> && threadContexts);

                void Reset(std::vector<BackgroundThreadContextUPtr> && threadContexts);

                std::vector<BackgroundThreadContextUPtr> threadContexts_;
                std::vector<FailoverUnitId> unprocessedFailoverUnits_;
                int count_;
                int asyncCommitCount_;
            };

            FailoverManager & fm_;

            // Whether the background manager is active. Note this is different from whether the FM is active.
            // This is because the FM creates a new background manager every time it is activated.
            bool isActive_;

            // Whether the background task is running.
            bool isRunning_;

            // Whether background task is rescheduled to start over again.
            bool isRescheduled_;

            // The timer for the periodic task.
            Common::TimerSPtr timer_;

            // Number of active threads for the current periodic run.
            LONG activeThreadCount_;

            Common::ExclusiveLock throttleLock_;
            bool volatile isThrottled_;
            EnumerationContext throttleContext_;

            bool enumerationAborted_;
            bool enumerationCompleted_;
            std::set<FailoverUnitId> unprocessedFailoverUnits_;

            // The state machine tasks for stateless services and stateful services
            std::vector<StateMachineTaskUPtr> statelessTasks_;
            std::vector<StateMachineTaskUPtr> statefulTasks_;

            FailoverUnitCache::VisitorSPtr visitor_;

            std::vector<BackgroundThreadContextUPtr> currentContexts_;
            std::vector<BackgroundThreadContextUPtr> oldContexts_;

            int enumeratedCount_;
            int actionCount_;
            int asyncCommitCount_;
            int asyncCompletionCount_;

            Common::DateTime lastCleanupTime_;
            Common::DateTime lastLoadPersistTime_;

            Common::TimeSpan stateTraceInterval_;
            Common::DateTime lastStateTraceTime_;
            bool isStateTraceEnabled_;

            Common::TimeSpan adminStateTraceInterval_;
            Common::DateTime lastAdminStateTraceTime_;
            bool isAdminTraceEnabled_;

            Common::StopwatchTime iterationStartTime_;

            bool isFMServiceHealthReported_;

            Common::ExclusiveLock lockObject_;

            // Invoked for each periodic task. It loads FailoverUnits into memory, create worker threads,
            // and process all the FailoverUnits.
            void RunPeriodicTask();

            void OnPeriodicTaskEnd();

            void ScheduleNextRun();

            bool IsEnumerationCompleted();

            // This is executed by each worker thread. It processes FailoverUnits until there is no one left.
            void Process();
            bool Process(EnumerationContext & enumerationContext, bool isThrottledThread);
            
            bool EnableThrottledThread();

            void PreProcessFailoverUnit(
                LockedFailoverUnitPtr & failoverUnit,
                std::vector<StateMachineActionUPtr> & actions);

            void CleanupLoadMetrics();

            void CreateThreadContexts();

            void TraceState();

            void ReportFMHealth();
        };
    }
}
