// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace ApiMonitoring;

namespace
{
    void TraceApiStart(MonitoringData const & monitoringData, ApiMonitoring::MonitoringComponentMetadata const & metaData)
    {
        ApiEventSource::Events->Start(
            monitoringData.PartitionId,
            monitoringData.Api,
            monitoringData.ReplicaId,
            metaData.NodeInstance.ToString(),
            metaData.NodeName);
    }

    void TraceApiSlow(MonitoringData const & monitoringData, ApiMonitoring::MonitoringComponentMetadata const & metaData)
    {
        ApiEventSource::Events->Slow(
            monitoringData.PartitionId,
            monitoringData.Api,
            monitoringData.ReplicaId,
			monitoringData.StartTime.ToDateTime(),
            metaData.NodeInstance.ToString(),
            metaData.NodeName);
    }

    void TraceApiFinish(MonitoringData const & monitoringData, TimeSpan elapsed, ErrorCode const & error, ApiMonitoring::MonitoringComponentMetadata const & metaData)
    {
        if (monitoringData.ServiceType.empty())
        {
            ApiEventSource::Events->Finish(
                monitoringData.PartitionId,
                monitoringData.Api,
                monitoringData.ReplicaId,
                elapsed.TotalMillisecondsAsDouble(),
                error,
                error.Message,
                metaData.NodeInstance.ToString(),
                metaData.NodeName);
        }
        else
        {
            ApiEventSource::Events->FinishWithServiceType(
                monitoringData.PartitionId,
                monitoringData.Api,
                monitoringData.ReplicaId,
                monitoringData.ServiceType,
                elapsed.TotalMillisecondsAsDouble(),
                error,
                error.Message,
                metaData.NodeInstance.ToString(),
                metaData.NodeName);
        }
    }
}

MonitoringComponentUPtr MonitoringComponent::Create(MonitoringComponentConstructorParameters parameters)
{
    return make_unique<MonitoringComponent>(
        parameters,
        TraceApiStart,
        TraceApiSlow,
        TraceApiFinish);
}

MonitoringComponent::MonitoringComponent(
    MonitoringComponentConstructorParameters parameters,
    ApiEventTraceCallback apiStartTraceCallback,
    ApiEventTraceCallback apiSlowTraceCallback,
    ApiFinishTraceCallback apiEndTraceCallback) : 
    apiStartTraceCallback_(apiStartTraceCallback),
    apiSlowTraceCallback_(apiSlowTraceCallback),
    apiEndTraceCallback_(apiEndTraceCallback),
    slowHealthReportCallback_(parameters.SlowHealthReportCallback),
    clearSlowHealthReportCallback_(parameters.ClearSlowHealthReportCallback),
    scanInterval_(parameters.ScanInterval),
    root_(*parameters.Root),
    isOpen_(false)
{
}

void MonitoringComponent::Open(MonitoringComponentMetadata const & metaData)
{
    AcquireWriteLock grab(lock_);
    ASSERT_IF(isOpen_, "Cannot be open");

	metaData_ = metaData;

    auto root = root_.CreateComponentRoot();

    // Create the timer
    timer_ = Common::Timer::Create(TimerTagDefault, 
        [this, root](Common::TimerSPtr const &)
    {
        this->OnTimer();
    });

    DisableTimerCallerHoldsLock();

    isOpen_ = true;
}

void MonitoringComponent::Close()
{
    AcquireWriteLock grab(lock_);
    if (!isOpen_)
    {
        return;
    }

    isOpen_ = false;

    timer_->Cancel();
    timer_.reset();
}

void MonitoringComponent::StartMonitoring(ApiCallDescriptionSPtr const & description)
{
    ASSERT_IF(description == nullptr, "Description cannot be null");

    TraceBeginIfEnabled(*description);

    {
        AcquireWriteLock grab(lock_);

        if (!isOpen_)
        {
            return;
        }

        AddToStoreCallerHoldsLock(description);

        if (store_.size() == 1)
        {
            EnableTimerCallerHoldsLock();
        }
    }
}

void MonitoringComponent::StopMonitoring(
    ApiCallDescriptionSPtr const & description,
    Common::ErrorCode const & error)
{
    ASSERT_IF(description == nullptr, "Description cannot be null");

    TraceEndIfEnabled(*description, error);

    FABRIC_SEQUENCE_NUMBER sequenceNumber = 0;

    {
        AcquireWriteLock grab(lock_);

        if (!isOpen_)
        {
            return;
        }

        RemoveFromStoreCallerHoldsLock(description);

        if (store_.empty())
        {
            DisableTimerCallerHoldsLock();
        }

        sequenceNumber = SequenceNumber::GetNext();
    }

    auto callbackItems = GenerateClearHealthCallbackList(*description, sequenceNumber);

    InvokeHealthReportCallback(callbackItems, clearSlowHealthReportCallback_, metaData_);
}

void MonitoringComponent::Test_OnTimer(StopwatchTime now)
{
	pair<vector<ExpiredApi>, vector<ExpiredApi>> actions;
    
    {
        AcquireWriteLock grab(lock_);

        if (!isOpen_)
        {
            return;
        }

        actions = FindExpiredItemsCallerHoldsLock(now);
    }
    
    TraceSlowIfEnabled(move(actions.first));
    
    auto callbackItems = GenerateSlowHealthCallbackList(move(actions.second));
    
    InvokeHealthReportCallback(callbackItems, slowHealthReportCallback_, metaData_);
}

void MonitoringComponent::DisableTimerCallerHoldsLock()
{
    timer_->Change(TimeSpan::MaxValue);
}

void MonitoringComponent::EnableTimerCallerHoldsLock()
{
    timer_->Change(scanInterval_, scanInterval_);;
}

void MonitoringComponent::AddToStoreCallerHoldsLock(
    ApiCallDescriptionSPtr const & description)
{
    MonitoredApiStore::iterator old;
    bool inserted;
    tie(old, inserted) = store_.insert(description);

    ASSERT_IF(!inserted, "Store already contains item");
}

void MonitoringComponent::RemoveFromStoreCallerHoldsLock(
    ApiCallDescriptionSPtr const & description)
{
    auto result = store_.erase(description);
    ASSERT_IF(result != 1, "element not found in store");
}

std::pair<vector<MonitoringComponent::ExpiredApi>, vector<MonitoringComponent::ExpiredApi>> MonitoringComponent::FindExpiredItemsCallerHoldsLock(
    Common::StopwatchTime now)
{
    vector<ExpiredApi> needTrace;
	vector<ExpiredApi> needHealth;

    for (auto & it : store_)
    {
		MonitoringActionsNeeded actions = it->GetActions(now);
		ExpiredApi api;

        if (actions.first)
        {
            api.ApiDescription = it;
			needTrace.push_back(api);
        }

		if (actions.second)
		{
			api.ApiDescription = it;
			api.SequenceNumber = SequenceNumber::GetNext();
			needHealth.push_back(api);
		}
    }

    return std::make_pair(needTrace, needHealth);
}

void MonitoringComponent::TraceBeginIfEnabled(
    ApiCallDescription const & description)
{
    if (description.Parameters.IsApiLifeCycleTraceEnabled)
    {
        apiStartTraceCallback_(description.Data, metaData_);
    }
}

void MonitoringComponent::TraceEndIfEnabled(
    ApiCallDescription const & description,
    Common::ErrorCode const & error)
{
    if (description.Parameters.IsApiLifeCycleTraceEnabled)
    {
        apiEndTraceCallback_(description.Data, Stopwatch::Now() - description.Data.StartTime, error, metaData_);
    }
}

void MonitoringComponent::TraceSlowIfEnabled(
    vector<ExpiredApi> const & expiredApis)
{
    for (auto const & i : expiredApis)
    {
        apiSlowTraceCallback_(i.ApiDescription->Data, metaData_);
    }
}

void MonitoringComponent::OnTimer()
{
    Test_OnTimer(Stopwatch::Now());
}

vector<MonitoringHealthEvent> MonitoringComponent::GenerateSlowHealthCallbackList(
    std::vector<ExpiredApi> const & expiredApis)
{
    vector<MonitoringHealthEvent> callbackItems;
    
    for (auto const & i : expiredApis)
    {
		callbackItems.push_back(i.ApiDescription->CreateHealthEvent(i.SequenceNumber));
    }

    return callbackItems;
}

vector<MonitoringHealthEvent> MonitoringComponent::GenerateClearHealthCallbackList(
    ApiCallDescription const & description,
    FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    vector<MonitoringHealthEvent> callbackItems;
    
    if (description.HasExpired && description.Parameters.IsHealthReportEnabled)
    {
        callbackItems.push_back(description.CreateHealthEvent(sequenceNumber));
    }

    return callbackItems;
}

void MonitoringComponent::InvokeHealthReportCallback(
    std::vector<MonitoringHealthEvent> const & items,
    HealthReportCallback const & callback,
	MonitoringComponentMetadata const & metaData)
{
    if (items.empty())
    {
        return;
    }

    ASSERT_IF(callback == nullptr, "Health callback cant be null");
    callback(items, metaData);
}
