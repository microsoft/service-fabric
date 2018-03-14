// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace ServiceModel;

BackgroundManager::EnumerationContext::EnumerationContext()
    : count_(0), asyncCommitCount_(0)
{
}

BackgroundManager::EnumerationContext::EnumerationContext(
    std::vector<BackgroundThreadContextUPtr> && threadContexts)
    : threadContexts_(std::move(threadContexts)), count_(0), asyncCommitCount_(0)
{
}

void BackgroundManager::EnumerationContext::Reset(std::vector<BackgroundThreadContextUPtr> && threadContexts)
{
    threadContexts_ = std::move(threadContexts);
    unprocessedFailoverUnits_.clear();
    count_ = 0;
    asyncCommitCount_ = 0;
}

BackgroundManager::BackgroundManager(FailoverManager & fm, Common::ComponentRoot const & root)
    : RootedObject(root),
    fm_(fm),
    isActive_(true),
    isRunning_(false),
    isRescheduled_(false),
    activeThreadCount_(0),
    enumerationAborted_(false),
    enumerationCompleted_(true),
    isThrottled_(false),
    actionCount_(0),
    asyncCommitCount_(0),
    asyncCompletionCount_(0),
    lastCleanupTime_(DateTime::Now()),
    lastLoadPersistTime_(lastCleanupTime_),
    lastStateTraceTime_(DateTime::Zero),
    lastAdminStateTraceTime_(DateTime::Zero),
    isStateTraceEnabled_(false),
    isAdminTraceEnabled_(false),
    isFMServiceHealthReported_(false)
{
    stateTraceInterval_ = fm_.NodeConfig.StateTraceInterval;
    adminStateTraceInterval_ = FailoverConfig::GetConfig().AdminStateTraceInterval;

    auto componentRoot = Root.CreateComponentRoot();
    timer_ = Common::Timer::Create(
        TimerTagDefault, 
        [this, componentRoot] (Common::TimerSPtr const&) mutable { this->fm_.BackgroundManagerObj.Start(); }, true);

    statelessTasks_.push_back(make_unique<StateUpdateTask>(fm, fm.NodeCacheObj));
    statelessTasks_.push_back(make_unique<StatelessCheckTask>(fm, fm.NodeCacheObj));
    statelessTasks_.push_back(make_unique<PendingTask>(fm));

    statefulTasks_.push_back(make_unique<StateUpdateTask>(fm, fm.NodeCacheObj));
    statefulTasks_.push_back(make_unique<ReconfigurationTask>(&fm));
    statefulTasks_.push_back(make_unique<PlacementTask>(fm));
    statefulTasks_.push_back(make_unique<PendingTask>(fm));
}

wstring const& BackgroundManager::get_Id() const { return fm_.Id; }

void BackgroundManager::Start()
{
    {
        AcquireExclusiveLock lock(lockObject_);

        if (isActive_ && !isRunning_)
        {
            isRunning_ = true;
        }
        else
        {
            return;
        }
    }

    RunPeriodicTask();
}

void BackgroundManager::Stop()
{
    AcquireExclusiveLock lock(lockObject_);

    timer_->Cancel();
    isActive_ = false;
}

void BackgroundManager::ScheduleRun()
{
    {
        AcquireExclusiveLock lock(lockObject_);
        if (!isActive_)
        {
            return;
        }

        if (isRunning_)
        {
            isRescheduled_ = true;
            return;
        }

        timer_->Change(TimeSpan::MaxValue);
        isRunning_ = true;
    }

    auto root = fm_.CreateComponentRoot();
    Threadpool::Post([this, root]
    {
        this->fm_.BackgroundManagerObj.RunPeriodicTask();
    });
}

void BackgroundManager::CreateThreadContexts()
{
    fm_.Events.PeriodicTaskBegin(fm_.Id, PeriodicTaskName::CreateContexts);
    auto startTime = Stopwatch::Now();

    vector<BackgroundThreadContextUPtr> contexts;
    vector<BackgroundThreadContextUPtr> applicationUpgradeContexts;

    // Add ThreadContext for node deactivation.
    fm_.NodeCacheObj.AddDeactivateNodeContext(contexts);

    // Add ThreadContext for fabric upgrade.
    fm_.FabricUpgradeManager.AddFabricUpgradeContext(contexts);

    // Add ThreadContext(s) for application upgrade(s).
    fm_.ServiceCacheObj.AddApplicationUpgradeContexts(applicationUpgradeContexts);

    int count = static_cast<int>(contexts.size());
    if (!applicationUpgradeContexts.empty())
    {
        count++;
    }

    int r = Random().Next(count);
    if (r < static_cast<int>(contexts.size()))
    {
        AddThreadContext(move(contexts[r]));
    }
    else
    {
        for (auto it = applicationUpgradeContexts.begin(); it != applicationUpgradeContexts.end(); it++)
        {
            AddThreadContext(move(*it));
        }
    }

    // Add ThreadContext(s) for updated and deleted service(s).
    fm_.ServiceCacheObj.AddServiceContexts();

    // Add ThreadContext for FailoverUnit health report.
    fm_.FailoverUnitCacheObj.AddThreadContexts();

    // Add ThreadContext for performance counters.
    if (!(fm_.IsMaster))
    {
        AddThreadContext(make_unique<FailoverUnitCountsContext>());
    }

    oldContexts_.clear();

    auto duration = Stopwatch::Now() - startTime;
    fm_.Events.CreateContextsPeriodicTaskEnd(Id, duration, contexts.size(), applicationUpgradeContexts.size());
}

void BackgroundManager::AddThreadContext(BackgroundThreadContextUPtr && context)
{
    for (auto it = currentContexts_.begin(); it != currentContexts_.end(); ++it)
    {
        ASSERT_IF((*it)->ContextId == context->ContextId,
            "Duplicate context for {0}", context->ContextId);
    }

    for (auto it = oldContexts_.begin(); it != oldContexts_.end(); ++it)
    {
        if ((*it)->ContextId == context->ContextId)
        {
            context->TransferUnprocessedFailoverUnits(*(*it));
        }
    }

    currentContexts_.push_back(move(context));
}

void BackgroundManager::RunPeriodicTask()
{
    // For test to disable periodic task
    if (FailoverConfig::GetConfig().PeriodicStateScanInterval == TimeSpan::MaxValue)
    {
        return;
    }

    iterationStartTime_ = Stopwatch::Now();

    bool isClusterPaused = fm_.NodeCacheObj.IsClusterPaused;

    bool enabledBalancing = !isClusterPaused && !fm_.FabricUpgradeManager.Upgrade;
    bool enabledConstraintCheck = !isClusterPaused &&
        (!fm_.FabricUpgradeManager.Upgrade || FailoverConfig::GetConfig().EnableConstraintCheckDuringFabricUpgrade);

    fm_.PLB.SetMovementEnabled(enabledConstraintCheck, enabledBalancing);

    if (isClusterPaused)
    {
        ScheduleNextRun();
        return;
    }

    isStateTraceEnabled_ = (DateTime::Now() - lastStateTraceTime_) > stateTraceInterval_;
    isAdminTraceEnabled_ = (DateTime::Now() - lastAdminStateTraceTime_) > adminStateTraceInterval_;

    fm_.Events.PeriodicTaskBegin(Id, PeriodicTaskName::FTBackgroundManager);

    DateTime now = DateTime::Now();
    bool cleanup = (now - lastCleanupTime_) > FailoverConfig::GetConfig().PeriodicStateCleanupScanInterval;
    if (cleanup)
    {
        // Cleanup ServiceTypes for which no ServiceInfo exists.
        fm_.ServiceCacheObj.CleanupServiceTypesAndApplications();

        // Cleanup LoadMetrics for which no FailoverUnit exists.
        CleanupLoadMetrics();

        lastCleanupTime_ = now;
    }

    if ((now - lastLoadPersistTime_) > FailoverConfig::GetConfig().PeriodicLoadPersistInterval)
    {
        fm_.LoadCacheObj.PeriodicPersist();
        lastLoadPersistTime_ = now;
    }

    fm_.InBuildFailoverUnitCacheObj.CheckQuorumLoss();

    fm_.NodeCacheObj.StartPeriodicTask();
    fm_.ServiceCacheObj.StartPeriodicTask();

    fm_.TraceQueueCounts();

    CreateThreadContexts();

    enumeratedCount_ = 0;
    actionCount_ = 0;
    asyncCommitCount_ = 0;
    asyncCompletionCount_ = 0;
    unprocessedFailoverUnits_.clear();
    enumerationAborted_ = false;
    enumerationCompleted_ = false;

    // There should not be any active threads running.
    ASSERT_IFNOT(activeThreadCount_ == 0, "Active thread count is not zero.");

    activeThreadCount_ = FailoverConfig::GetConfig().BackgroundThreadCount;
    if (activeThreadCount_ == 0)
    {
        activeThreadCount_ = Environment::GetNumberOfProcessors();
    }

    visitor_ = fm_.FailoverUnitCacheObj.CreateVisitor(true, TimeSpan::Zero, true);

    // This thread itself will be performing the task as well.
    int threadsToInvoke = activeThreadCount_ - 1;
    for (int i = 0; i < threadsToInvoke; i++)
    {
        auto root = Root.CreateComponentRoot();
        Threadpool::Post([this, root] () { this->Process(); });
    }

    // Start processing FailoverUnits on this thread.
    Process();
}

void BackgroundManager::OnPeriodicTaskEnd()
{
    fm_.FailoverUnitCounters->NumberOfUnprocessedFailoverUnits.Value = static_cast<PerformanceCounterValue>(unprocessedFailoverUnits_.size());
    fm_.FailoverUnitCounters->NumberOfFailoverUnitActions.Value = static_cast<PerformanceCounterValue>(actionCount_);

    visitor_ = nullptr;

    for (auto it = currentContexts_.begin(); it != currentContexts_.end(); ++it)
    {
        bool isReady = (*it)->ReadyToComplete();
        bool areAllFailoverUnitsProcessed = !enumerationAborted_ && (*it)->MergeUnprocessedFailoverUnits(unprocessedFailoverUnits_);
        bool isCompleted = isReady && areAllFailoverUnitsProcessed;

        if (isReady && !enumerationAborted_)
        {
            fm_.Events.BackgroundThreadContextCompleted(wformatString(*it), isCompleted);
        }

        (*it)->Complete(fm_, isCompleted, enumerationAborted_);

        if (!isReady)
        {
            (*it)->ClearUnprocessedFailoverUnits();
        }
    }

    swap(currentContexts_, oldContexts_);

    if (fm_.IsMaster &&
        (isStateTraceEnabled_ || !isFMServiceHealthReported_))
    {
        ReportFMHealth();
    }

    DateTime now = DateTime::Now();
    if (isStateTraceEnabled_)
    {
        lastStateTraceTime_ = now;
        isStateTraceEnabled_ = false;
    }

    if (isAdminTraceEnabled_)
    {
        lastAdminStateTraceTime_ = now;
        isAdminTraceEnabled_ = false;
    }

    TimeSpan duration = Stopwatch::Now() - iterationStartTime_;
    fm_.Events.FTPeriodicTaskEnd(Id, enumerationAborted_, unprocessedFailoverUnits_.size(), actionCount_, duration.TotalMilliseconds());

    ScheduleNextRun();
}

void BackgroundManager::ScheduleNextRun()
{
    {
        AcquireExclusiveLock lock(lockObject_);

        if (!isRescheduled_)
        {
            isRunning_ = false;

            if (isActive_)
            {
                timer_->Change(FailoverConfig::GetConfig().PeriodicStateScanInterval);
            }

            return;
        }
        else
        {
            isRescheduled_ = false;
        }
    }

    RunPeriodicTask();
}

void BackgroundManager::CleanupLoadMetrics()
{
    vector<FailoverUnitId> failoverUnitIds;
    fm_.LoadCacheObj.GetAllFailoverUnitIds(failoverUnitIds);

    for (FailoverUnitId const& id : failoverUnitIds)
    {
        if (!fm_.FailoverUnitCacheObj.FailoverUnitExists(id))
        {
            fm_.LoadCacheObj.DeleteLoads(id);
        }
    }
}

bool BackgroundManager::IsEnumerationCompleted()
{
    if (!enumerationCompleted_ &&
        (!isActive_ || isRescheduled_ || actionCount_ >= FailoverConfig::GetConfig().MaxActionsPerIteration))
    {
        fm_.Events.BackgroundEnumerationAborted(isActive_, isRescheduled_, actionCount_);

        enumerationAborted_ = true;
        enumerationCompleted_ = true;
    }

    return enumerationCompleted_;
}

bool BackgroundManager::EnableThrottledThread()
{
    AcquireExclusiveLock lock(throttleLock_);
    if (isThrottled_ || enumerationCompleted_)
    {
        return false;
    }

    isThrottled_ = true;
    vector<BackgroundThreadContextUPtr> threadContexts;
    for (BackgroundThreadContextUPtr const & context : currentContexts_)
    {
        threadContexts.push_back(context->CreateNewContext());
    }

    throttleContext_.Reset(move(threadContexts));

    return true;
}

bool BackgroundManager::Resume()
{
    // Use lock to serialize processing from throttle "thread" since it is not a
    // physical thread.
    AcquireExclusiveLock lock(throttleLock_);
    if (!isThrottled_)
    {
        return false;
    }

    return Process(throttleContext_, true);
}

void BackgroundManager::Process()
{
    fm_.Events.BackgroundThreadStart();

    vector<BackgroundThreadContextUPtr> threadContexts;
    for (BackgroundThreadContextUPtr const & context : currentContexts_)
    {
        threadContexts.push_back(context->CreateNewContext());
    }

    EnumerationContext enumerationContext(move(threadContexts));
    Process(enumerationContext, false);
}

bool BackgroundManager::Process(EnumerationContext & enumerationContext, bool isThrottledThread)
{
    bool asyncCommit = false;
    bool enableThrottledThread = false;
    bool throttled = false;

    while (!IsEnumerationCompleted())
    {
        bool result;
        FailoverUnitId failoverUnitId;
        LockedFailoverUnitPtr failoverUnit = visitor_->MoveNext(result, failoverUnitId);
        if (failoverUnit)
        {
            enumerationContext.count_++;

            if (!throttled && fm_.Store.IsThrottleNeeded)
            {
                throttled = true;
                if (!isThrottledThread)
                {
                    enableThrottledThread = EnableThrottledThread();
                }
            }

            bool isSync = ProcessFailoverUnit(move(failoverUnit), true);
            if (isSync)
            {
                for (auto it = enumerationContext.threadContexts_.begin(); it != enumerationContext.threadContexts_.end(); ++it)
                {
                    (*it)->Process(fm_, *failoverUnit);
                }

                if (!failoverUnit.Release(false, true))
                {
                    // Theoretically we should loop if this again returns true,
                    // but the chance is very small and if there is bug it can
                    // cause infinite loop, so for now we are not doing such
                    // optimization.
                    ProcessFailoverUnit(move(failoverUnit), false);
                }
            }
            else
            {
                asyncCommit = true;
                enumerationContext.asyncCommitCount_++;
                if (isThrottled_)
                {
                    break;
                }
            }
        }
        else if (result)
        {
            enumerationCompleted_ = true;
        }

        if (!result)
        {
            enumerationContext.unprocessedFailoverUnits_.push_back(failoverUnitId);
        }
    }

    if (isThrottledThread)
    {
        if (!enumerationCompleted_)
        {
            return asyncCommit;
        }

        isThrottled_ = false;
    }

    fm_.Events.BackgroundThreadEndStatistics(
        enumerationContext.count_,
        enumerationContext.unprocessedFailoverUnits_.size(),
        enumerationContext.asyncCommitCount_,
        asyncCompletionCount_,
        enumerationCompleted_,
        isThrottledThread);

    bool iterationCompleted;
    bool periodicTaskCompleted;
    {
        AcquireExclusiveLock lock(fm_.CommitQueue.GetLockObject());

        unprocessedFailoverUnits_.insert(enumerationContext.unprocessedFailoverUnits_.begin(), enumerationContext.unprocessedFailoverUnits_.end());

        for (size_t i = 0; i < enumerationContext.threadContexts_.size(); i++)
        {
            currentContexts_[i]->Merge(*(enumerationContext.threadContexts_[i]));
        }

        if (!enableThrottledThread)
        {
            activeThreadCount_--;
        }

        enumeratedCount_ += enumerationContext.count_;
        asyncCommitCount_ += enumerationContext.asyncCommitCount_;

        iterationCompleted = (activeThreadCount_ == 0);
        periodicTaskCompleted = (iterationCompleted && asyncCommitCount_ == asyncCompletionCount_);
    }

    if (iterationCompleted)
    {
        ASSERT_IF(isThrottled_, "Throttle enabled when enumeration is completed");

        fm_.Events.BackgroundThreadIterationEnd(enumeratedCount_, unprocessedFailoverUnits_.size(), asyncCommitCount_, enumerationAborted_);

        if (periodicTaskCompleted)
        {
            OnPeriodicTaskEnd();
        }
    }

    return asyncCommit;
}

void BackgroundManager::ProcessThreadContexts(FailoverUnit const & failoverUnit, bool isSuccess)
{
    if (isSuccess)
    {
        for (auto it = currentContexts_.begin(); it != currentContexts_.end(); ++it)
        {
            (*it)->Process(fm_, failoverUnit);
        }
    }
    else
    {
        unprocessedFailoverUnits_.insert(failoverUnit.Id);
    }

    asyncCompletionCount_++;

    if ((asyncCompletionCount_ == asyncCommitCount_) && (activeThreadCount_ == 0))
    {
        auto root = fm_.CreateComponentRoot();
        Threadpool::Post([this, root]
        {
            this->OnPeriodicTaskEnd();
        });
    }
}

void BackgroundManager::PreProcessFailoverUnit(
    LockedFailoverUnitPtr & failoverUnit,
    vector<StateMachineActionUPtr> & actions)
{
    vector<StateMachineTaskUPtr> const & tasks = failoverUnit->IsStateful ? statefulTasks_ : statelessTasks_;
    vector<DynamicStateMachineTaskUPtr> const & dynamicTasks = failoverUnit.GetExecutingTasks();

    fm_.FTEvents.FTProcessing(failoverUnit->Id.Guid, static_cast<uint16>(dynamicTasks.size()));

    failoverUnit->ProcessingStartTime = Stopwatch::Now();

    // Update the nodes and service pointers if they are stale.
    failoverUnit->UpdatePointers(fm_, fm_.NodeCacheObj, fm_.ServiceCacheObj);

    for (DynamicStateMachineTaskUPtr const & task : dynamicTasks)
    {
        task->CheckFailoverUnit(failoverUnit, actions);
    }

    // CITs set MaxActionsPerIteration to 0 to disable state machine processing.
    if (FailoverConfig::GetConfig().MaxActionsPerIteration > 0)
    {
        for (StateMachineTaskUPtr const& task : tasks)
        {
            task->CheckFailoverUnit(failoverUnit, actions);
        }
    }

    if (!fm_.IsMaster &&
        (failoverUnit->PersistenceState == PersistenceState::NoChange) &&
        ((failoverUnit->HealthSequence < fm_.FailoverUnitCacheObj.InvalidatedHealthSequence) || failoverUnit->IsHealthReportNeeded()))
    {
        failoverUnit.EnableUpdate();
        failoverUnit->UpdateForHealthReport = true;
    }
}

void BackgroundManager::PostProcessFailoverUnit(
    LockedFailoverUnitPtr & failoverUnit,
    vector<StateMachineActionUPtr> const & actions,
    bool updated,
    int replicaDifference,
    int64 commitDuration,
    int64 plbDuration,
    bool isBackground)
{
    if (!updated)
    {
        if (actions.size() > 0 || replicaDifference != 0)
        {
            fm_.FTEvents.TraceFTAction(*failoverUnit, actions, replicaDifference);
        }
    }
    else
    {
        fm_.FailoverUnitCounters->FailoverUnitCommitDurationBase.Increment();
        fm_.FailoverUnitCounters->FailoverUnitCommitDuration.IncrementBy(static_cast<PerformanceCounterValue>(commitDuration));

        fm_.FTEvents.TraceFTUpdateBackground(failoverUnit, actions, replicaDifference, commitDuration, plbDuration);
    }

    for (StateMachineActionUPtr const & action : actions)
    {
        int actionCount = action->PerformAction(fm_);
        if (isBackground)
        {
            actionCount_ += actionCount;
        }
    }
}

bool BackgroundManager::ProcessFailoverUnit(
    LockedFailoverUnitPtr && failoverUnit,
    bool isBackground)
{
    vector<StateMachineActionUPtr> actions;

    PreProcessFailoverUnit(failoverUnit, actions);

    bool updated = (failoverUnit->PersistenceState != PersistenceState::NoChange);
    int replicaDifference = failoverUnit->ReplicaDifference;

    if (!updated)
    {
        if (replicaDifference != 0)
        {
            ErrorCode error = fm_.FailoverUnitCacheObj.UpdateFailoverUnit(failoverUnit);
            ASSERT_IFNOT(error.IsSuccess(), "UpdateFailoverUnit with no persistence failed: {0}", error);
        }
        else if (ConsistencyUnitId::IsReserved(failoverUnit->Id.Guid) && isBackground && IsAdminTraceEnabled)
        {
            fm_.FTEvents.FTStateAdmin(failoverUnit->IdString, wformatString(*failoverUnit));
        }
        else if (ConsistencyUnitId::IsReserved(failoverUnit->Id.Guid) && isBackground && IsStateTraceEnabled)
        {
            fm_.FTEvents.FTState(failoverUnit->Id.Guid, *failoverUnit);
        }

        PostProcessFailoverUnit(failoverUnit, actions, false, replicaDifference, 0, 0, isBackground);

        return true;
    }
    else if (failoverUnit.SkipPersistence)
    {
        int64 plbDuration(0);
        replicaDifference = failoverUnit->ReplicaDifference;
        ErrorCode error = fm_.FailoverUnitCacheObj.PostUpdateFailoverUnit(
            failoverUnit,
            PersistenceState::NoChange,
            false, // ShouldUpdateLookupVersion
            false, // ShouldReportHealth
            ErrorCode::Success(),
            plbDuration);

        failoverUnit->PostUpdate(DateTime::Now(), false);
        failoverUnit->PersistenceState = PersistenceState::NoChange;
        failoverUnit->IsPersistencePending = true;

        if (error.IsSuccess())
        {
            PostProcessFailoverUnit(
                failoverUnit,
                actions,
                true, // IsUpdated
                replicaDifference,
                0, // CommitDuration
                plbDuration, //PLBDuration
                isBackground);
        }

        return true;
    }

    fm_.FailoverUnitCacheObj.UpdateFailoverUnitAsync(move(failoverUnit), move(actions), isBackground);

    return false;
}

void BackgroundManager::ReportFMHealth()
{
    LockedFailoverUnitPtr failoverUnit;
    fm_.FailoverUnitCacheObj.TryGetLockedFailoverUnit(FailoverUnitId(Constants::FMServiceGuid), failoverUnit);

    if (failoverUnit)
    {
        failoverUnit->UpdateHealthState();
        fm_.HealthClient->AddHealthReport(
            fm_.HealthReportFactoryObj.GenerateFailoverUnitHealthReport(*failoverUnit));
        if (!isFMServiceHealthReported_)
        {
            AttributeList attributes;
            attributes.AddAttribute(*HealthAttributeNames::ApplicationName, SystemServiceApplicationNameHelper::SystemServiceApplicationName);
            attributes.AddAttribute(*HealthAttributeNames::ServiceTypeName, ServiceTypeIdentifier::FailoverManagerServiceTypeId->ServiceTypeName);

            // Dummy instance id for FM service, it is never created/deleted.
            fm_.HealthClient->AddHealthReport(HealthReport::CreateSystemHealthReport(
                SystemHealthReportCode::FMM_ServiceCreated,
                EntityHealthInformation::CreateServiceEntityHealthInformation(SystemServiceApplicationNameHelper::PublicFMServiceName, failoverUnit->ServiceInfoObj->Instance),
                L"" /*extraDescription*/,
                move(attributes)));

            isFMServiceHealthReported_ = true;
        }
    }
}
