// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Management::ImageStore;
using namespace ServiceModel;

StringLiteral const TraceComponent("NativeImageStoreProgressState");

NativeImageStoreProgressState::NativeImageStoreProgressState(
    INativeImageStoreProgressEventHandlerPtr const & eventHandler,
    wstring const & operationName,
    ServiceModel::ProgressUnitType::Enum const itemType)
    : IFileStoreServiceClientProgressEventHandler()
    , ComponentRoot()
    , eventHandler_(eventHandler)
    , itemType_(itemType)
    , completedItems_(0)
    , totalItems_(0)
    , lock_()
    , updateTimer_()
    , isTimerActive_(false)
    , updateUploadTimer_()
    , isUploadTimerActive_(false)
    , lastUpdateUploadTime_(Common::DateTime::Zero)
{
    this->OnCtor(operationName);
}

NativeImageStoreProgressState::NativeImageStoreProgressState(
    INativeImageStoreProgressEventHandlerPtr && eventHandler,
    wstring const & operationName,
    ServiceModel::ProgressUnitType::Enum const itemType)
    : IFileStoreServiceClientProgressEventHandler()
    , ComponentRoot()
    , eventHandler_(move(eventHandler))
    , itemType_(itemType)
    , completedItems_(0)
    , totalItems_(0)
    , lock_()
    , updateTimer_()
    , isTimerActive_(false)
    , updateUploadTimer_()
    , isUploadTimerActive_(false)
    , lastUpdateUploadTime_(Common::DateTime::Zero)
{
    this->OnCtor(operationName);
}

void NativeImageStoreProgressState::OnCtor(wstring const & operationName)
{
    traceId_ = wformatString("{0}:{1}", TraceThis, operationName);
    WriteInfo(TraceComponent, "{0}: ctor handler={1}", this->TraceId, (eventHandler_.get() != nullptr));
}

ServiceModel::ProgressUnitSPtr NativeImageStoreProgressState::GetProgressUnit(ProgressType const type)
{
	{
        AcquireReadLock grabReader(lock_);
        std::map<ProgressType, ProgressUnitSPtr>::const_iterator it = units_.find(type);
        if (it != units_.end())
        {
            return it->second;
        }
    }
    
    {
        AcquireWriteLock grabWriter(lock_);
        std::map<ProgressType, ProgressUnitSPtr>::const_iterator it = units_.find(type);
        if (it != units_.end())
        {
            return it->second;
        }

        ProgressUnitType::Enum unitType;
        if (type == ProgressType::ApplicationPackageReplicateProgress)
        {
            unitType = ProgressUnitType::Enum::Files;
        }
        else
        {
            unitType = ProgressUnitType::Enum::Bytes;
        }

        ProgressUnitSPtr progress = std::make_shared<ProgressUnit>(unitType);
        units_.insert(make_pair(type, progress));
        return progress;
    }
}

void NativeImageStoreProgressState::IncrementReplicatedFiles(size_t value)
{
    this->GetProgressUnit(ProgressType::ApplicationPackageReplicateProgress)->IncrementCompletedItems(value);
    this->StartUploadTimerIfNeeded();
}

void NativeImageStoreProgressState::IncrementTotalFiles(size_t value)
{
    this->GetProgressUnit(ProgressType::ApplicationPackageReplicateProgress)->IncrementTotalItems(value);
    this->StartUploadTimerIfNeeded();
}

void NativeImageStoreProgressState::IncrementTransferCompletedItems(size_t value)
{
    this->GetProgressUnit(ProgressType::ApplicationPackageTransferProgress)->IncrementCompletedItems(value);
    this->StartUploadTimerIfNeeded();
}

void NativeImageStoreProgressState::IncrementTotalTransferItems(size_t value)
{
    this->GetProgressUnit(ProgressType::ApplicationPackageTransferProgress)->IncrementTotalItems(value);
    this->StartUploadTimerIfNeeded();
}

void NativeImageStoreProgressState::IncrementCompletedItems(size_t value)
{
    completedItems_ += value;

    this->StartTimerIfNeeded();
}

void NativeImageStoreProgressState::IncrementTotalItems(size_t value)
{
    totalItems_ += value;

    this->StartTimerIfNeeded();
}

void NativeImageStoreProgressState::StopDispatchingUpdates()
{
    WriteInfo(TraceComponent, "{0}: stopped dispatching updates", this->TraceId);

    AcquireWriteLock lock(lock_);

    if (updateTimer_.get() != nullptr)
    {
        updateTimer_->Cancel();
    }

    eventHandler_ = INativeImageStoreProgressEventHandlerPtr();
}

bool NativeImageStoreProgressState::TryGetEventHandler(__out INativeImageStoreProgressEventHandlerPtr & result)
{
    AcquireReadLock lock(lock_);

    result = eventHandler_;

    return (result.get() != nullptr);
}

void NativeImageStoreProgressState::StartTimerIfNeeded()
{
    if (isTimerActive_.exchange(true))
    {
        return;
    }

    INativeImageStoreProgressEventHandlerPtr handler;
    if (!this->TryGetEventHandler(handler))
    {
        // Expected for downloads that do not happen as part of provisioning an application type
        return;
    }

    TimeSpan interval;
    auto error = handler->GetUpdateInterval(interval);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0}: GetUpdateInterval failed: {1}", this->TraceId, error);
        return;
    }

    if (interval <= TimeSpan::Zero)
    {
        auto config = ImageStoreServiceConfig::GetConfig().ClientDefaultProgressReportingInterval;

        if (config <= TimeSpan::Zero)
        {
            WriteInfo(
                TraceComponent,
                "{0} progress reporting disabled: interval={1}",
                this->TraceId,
                interval);

            return;
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0} overriding progress interval from config: requested={1} config={2}",
                this->TraceId,
                interval,
                config);

            interval = config;
        }
    }

    auto root = this->CreateComponentRoot();

    AcquireWriteLock lock(lock_);

    updateTimer_ = Timer::Create(
        TraceComponent,
        [this, root](TimerSPtr const &)
        {
            this->ProgressTimerCallback();
        },
        false); // allowConcurrency
    
    updateTimer_->Change(interval, interval);
}

void NativeImageStoreProgressState::ProgressTimerCallback()
{
    INativeImageStoreProgressEventHandlerPtr handler;
    if (!this->TryGetEventHandler(handler))
    {
        return;
    }
       
    auto completed = completedItems_.load();
    auto total = totalItems_.load();

    auto error = handler->OnUpdateProgress(completed, total, itemType_);
    if (error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} OnUpdateProgress: completed={1} total={2}",
            this->TraceId,
            completed,
            total);
    }
    else
    {
        WriteWarning(
            TraceComponent,
            "{0} OnUpdateProgress failed: completed={1} total={2} error={3}",
            this->TraceId,
            completed,
            total,
            error);
    }
}

void NativeImageStoreProgressState::StartUploadTimerIfNeeded()
{
    if (isUploadTimerActive_.exchange(true))
    {
        return;
    }

    INativeImageStoreProgressEventHandlerPtr handler;
    if (!this->TryGetEventHandler(handler))
    {
        // Expected for downloads that do not happen as part of provisioning an application type
        return;
    }

    TimeSpan interval;
    auto error = handler->GetUpdateInterval(interval);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0}: GetUpdateInterval failed: {1}", this->TraceId, error);
        return;
    }

    if (interval <= TimeSpan::Zero)
    {
        auto config = ImageStoreServiceConfig::GetConfig().ClientDefaultProgressReportingInterval;

        if (config <= TimeSpan::Zero)
        {
            WriteInfo(
                TraceComponent,
                "{0} progress reporting disabled: interval={1}",
                this->TraceId,
                interval);

            return;
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0} overriding progress interval from config: requested={1} config={2}",
                this->TraceId,
                interval,
                config);

            interval = config;
        }
    }

    auto root = this->CreateComponentRoot();

    AcquireWriteLock lock(lock_);

    updateUploadTimer_ = Timer::Create(
        TraceComponent,
        [this, root](TimerSPtr const &)
    {
        this->ProgressUploadTimerCallback();
    },
        false); // allowConcurrency

    updateUploadTimer_->Change(interval, interval);
}

void NativeImageStoreProgressState::ProgressUploadTimerCallback()
{
    INativeImageStoreProgressEventHandlerPtr handler;
    if (!this->TryGetEventHandler(handler))
    {
        return;
    }

    for (std::map<ProgressType, ProgressUnitSPtr>::const_iterator it = units_.begin(); it != units_.end(); ++it)
    {
        if (Common::DateTime(it->second->LastModifiedTimeInTicks) > lastUpdateUploadTime_)
        {
            auto error = handler->OnUpdateProgress(
                it->second->CompletedItems,
                it->second->TotalItems,
                it->second->ItemType);

            wstring type = it->first == ProgressType::ApplicationPackageReplicateProgress ? L"ApplicationPackageReplicateProgress" : L"ApplicationPackageTransferProgress";
            if (error.IsSuccess())
            {
                WriteInfo(
                    TraceComponent,
                    "{0} {1}: completed={2} total={3}",
                    this->TraceId,
                    type,
                    it->second->CompletedItems,
                    it->second->TotalItems);
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    "{0} {1} failed: completed={2} total={3} error={4}",
                    this->TraceId,
                    type,
                    it->second->CompletedItems,
                    it->second->TotalItems,
                    error);
            }
        }
    }

    lastUpdateUploadTime_ = DateTime::get_Now();
}

