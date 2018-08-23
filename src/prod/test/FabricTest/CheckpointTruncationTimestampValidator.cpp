// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace Common;
using namespace FabricTest;
using namespace TestCommon;
using namespace ServiceModel;
using namespace Api;
using namespace ReliabilityTestApi;

StringLiteral const TraceSource("CheckpointTruncationTimestampValidator");

// Delay between verifications
double const ValidationDelayInMs = 500;

// The allowed time between failures is equal to ValidationDelayInMS * 1000 * ElapsedValidationTimeFactor
double const ElapsedValidationTimeFactor = 10;

CheckpointTruncationTimestampValidator::CheckpointTruncationTimestampValidator(FabricTestDispatcher & dispatcher)
    : dispatcher_(dispatcher)
    , serviceNames_()
    , periodicCheckpointAndLogTruncationTimestampMap_()
    , failureTimestampMap_()
    , isActive_(false)
    , validationLock_()
{
}

void CheckpointTruncationTimestampValidator::AddService(std::wstring & serviceName)
{
    AcquireWriteLock lock(validationLock_);

    auto it = std::find(serviceNames_.begin(), serviceNames_.end(), serviceName);
    if (it != serviceNames_.end())
    {
        // Service already exists
        return;
    }

    serviceNames_.push_back(serviceName);

    if (serviceNames_.size() == 1)
    {
        isActive_ = true;

        auto root = FABRICSESSION.CreateComponentRoot();
        Threadpool::Post([this, root]()
        {
            Run();
        }, TimeSpan::FromMilliseconds(ValidationDelayInMs));
    }
}

void CheckpointTruncationTimestampValidator::RemoveService(std::wstring & serviceName)
{
    AcquireWriteLock lock(validationLock_);
    auto it = std::find(serviceNames_.begin(), serviceNames_.end(), serviceName);
    if(it != serviceNames_.end())
    {
        std::remove(serviceNames_.begin(), serviceNames_.end(), serviceName);
    }
}

void CheckpointTruncationTimestampValidator::Stop()
{
    AcquireWriteLock lock(validationLock_);
    isActive_ = false;
}

void CheckpointTruncationTimestampValidator::Run()
{
    AcquireWriteLock lock(validationLock_);
    if (!isActive_)
    {
        return;
    }

    auto fm = FABRICSESSION.FabricDispatcher.GetFM();

    for (auto service : serviceNames_)
    {
        if (fm == nullptr)
        {
            break;
        }

        auto partitionList = fm->FindFTByServiceName(service);
        for (auto partition : partitionList)
        {
            auto partitionId = partition.Id.Guid;
            auto replicaList = partition.GetReplicas();

            for (auto replica : replicaList)
            {
                auto replicaId = replica.ReplicaId;
                auto nodeId = replica.FederationNodeId;
                auto replicaState = replica.ReplicaState;
                auto replicaStatus = replica.ReplicaStatus;
                auto serviceLocation = replica.ServiceLocation;
                auto replicaRole = replica.CurrentConfigurationRole;

                // Ensure a time is not validated prior to recovery completions
                // TODO: Should this also prevent traces during timer callback? 
                if (replicaState != Reliability::ReplicaStates::Enum::Ready)
                {
                    continue;
                }

                if (replicaStatus != Reliability::ReplicaStatus::Enum::Up)
                {
                    continue;
                }

                if (replicaRole != Reliability::ReplicaRole::Enum::Primary &&
                    replicaRole != Reliability::ReplicaRole::Enum::Secondary)
                {
                    continue;
                }

                auto key = wformatString("{0}.{1}", partitionId, replicaId);

                // Find a reference to this replica
                ITestStoreServiceSPtr testStoreService;

                bool foundService = FABRICSESSION.FabricDispatcher.Federation.TryFindStoreService(serviceLocation, testStoreService);
                if(!foundService)
                {
                    continue;
                }

                auto resolvedPid = testStoreService->GetPartitionId();
                if (resolvedPid != partitionId.ToString())
                {
                    continue;
                }

                if (periodicCheckpointAndLogTruncationTimestampMap_.find(key) == periodicCheckpointAndLogTruncationTimestampMap_.end())
                {
                    // Add new replica key with 0 for lastPeriodicTruncation/Checkpoint timestamps if necessary
                    periodicCheckpointAndLogTruncationTimestampMap_[key] = make_tuple(0, 0);
                }

                auto currentTimestamps = periodicCheckpointAndLogTruncationTimestampMap_[key];
                auto currentLastChkpt = std::get<0>(currentTimestamps);
                auto currentLastTrunc = std::get<1>(currentTimestamps);

                // Query service instance
                LONG64 queriedLastChkpt;
                LONG64 queriedLastTrunc;

                NTSTATUS status = testStoreService->GetPeriodicCheckpointAndTruncationTimestamps(queriedLastChkpt, queriedLastTrunc);
                if (!NT_SUCCESS(status))
                {
                    // SF_STATUS_OBJECT_CLOSED can be returned if LoggingReplicator members are not yet initialized prior ti query
                    TestSession::FailTestIfNot(
                        status == SF_STATUS_OBJECT_CLOSED,
                        "{0} - Unexpected status code {1} returned from GetPeriodicCheckpointAndTruncationTimestamps",
                        key,
                        status);

                    continue;
                }

                auto currentChkptTimeStamp = Common::TimeSpan::FromTicks(currentLastChkpt);
                auto currentTruncTimeStamp = Common::TimeSpan::FromTicks(currentLastTrunc);

                auto queriedChkptTimestamp = Common::TimeSpan::FromTicks(queriedLastChkpt);
                auto queriedTruncTimestamp = Common::TimeSpan::FromTicks(queriedLastTrunc);

                Trace.WriteNoise(
                    TraceSource,
                    "{0}. Node:{9} | <lastcheckpoint,lasttruncation> \r\nCurrent:<{1},{2}> ({3}, {4}). \r\nReceived: <{5},{6}> ({7}, {8}). ",
                    key,
                    currentLastChkpt,
                    currentLastTrunc,
                    currentChkptTimeStamp,
                    currentTruncTimeStamp,
                    queriedLastChkpt,
                    queriedLastTrunc,
                    queriedChkptTimestamp,
                    queriedTruncTimestamp,
                    nodeId.ToString());

                bool checkpointValidationFailed = queriedLastChkpt < currentLastChkpt;

                if (checkpointValidationFailed)
                {
                    auto currentTimestampTicks = DateTime::Now().Ticks;

                    // Update failure map
                    if (failureTimestampMap_.find(key) == failureTimestampMap_.end())
                    {
                        failureTimestampMap_[key] = currentTimestampTicks;
                    }

                    auto elapsedFromLastFailure = TimeSpan::FromTicks(currentTimestampTicks - failureTimestampMap_[key]);
                    auto maxTimeBetweenFailures = TimeSpan::FromSeconds(ValidationDelayInMs * 1000 * ElapsedValidationTimeFactor);

                    auto failureMsg = formatString(
                        "{0} : Unexpected last checkpoint time. Previously {1}({2}), now {3}({4}). Time Elapsed from last validation failure: {5}. Max. allowed time between failures: {6} \n{7}.",
                        key,
                        currentLastChkpt,
                        currentChkptTimeStamp,
                        queriedLastChkpt,
                        queriedChkptTimestamp,
                        elapsedFromLastFailure,
                        maxTimeBetweenFailures,
                        GetTraceViewerQuery(partitionId, replicaId));

                    if (elapsedFromLastFailure > maxTimeBetweenFailures)
                    {
                        TestSession::FailTestIfNot(
                            queriedLastChkpt >= currentLastChkpt,
                            "{0}",
                            failureMsg);
                    }
                    else
                    {
                        TestSession::WriteWarning(
                            TraceSource,
                            "{0}",
                            failureMsg);

                        continue;
                    }
                }

                TestSession::FailTestIfNot(
                    queriedLastTrunc >= currentLastTrunc,
                    "{0} : Unexpected last truncation time. Previously {1}, now {2}. \n{3}",
                    key,
                    currentLastTrunc,
                    queriedLastTrunc,
                    GetTraceViewerQuery(partitionId, replicaId));

                // Remove from failure map after validation succeeds
                if (failureTimestampMap_.find(key) != failureTimestampMap_.end())
                {
                    failureTimestampMap_.erase(key);
                    Trace.WriteNoise(
                        TraceSource,
                        "{0}. Removed from failed timestamps");
                }

                if (queriedLastChkpt != currentLastChkpt || queriedLastTrunc != currentLastTrunc)
                {
                    Trace.WriteInfo(
                        TraceSource,
                        "{0}. Updating from <{1},{2}> ({3}, {4}). to <{5},{6}> ({7}, {8}). on Node {9} ",
                        key,
                        currentLastChkpt,
                        currentLastTrunc,
                        currentChkptTimeStamp,
                        currentTruncTimeStamp,
                        queriedLastChkpt,
                        queriedLastTrunc,
                        queriedChkptTimestamp,
                        queriedTruncTimestamp,
                        nodeId.ToString());

                    // Update values in map
                    std::tuple<LONG64, LONG64> newValues = make_tuple(queriedLastChkpt, queriedLastTrunc);
                    periodicCheckpointAndLogTruncationTimestampMap_[key] = newValues;

                    // Confirm values actually updated
                    auto updatedTime = periodicCheckpointAndLogTruncationTimestampMap_[key];
                    LONG64 updatedLastChkpt = std::get<0>(updatedTime);
                    LONG64 updatedLastTrunc = std::get<1>(updatedTime);

                    bool updatedValues = (updatedLastChkpt == queriedLastChkpt && updatedLastTrunc == queriedLastTrunc);
                    if (!updatedValues)
                    {
                        Trace.WriteError(
                            TraceSource,
                            "{0} FAILED TO UPDATE <lastcheckpoint,lasttruncation> from <{1},{2}> to <{3},{4}>",
                            key,
                            currentLastChkpt,
                            currentLastTrunc,
                            updatedLastChkpt,
                            updatedLastTrunc);

                        TestSession::FailTest("Failed to update");
                    }
                }
            }
        };
    }
    auto root = FABRICSESSION.CreateComponentRoot();
    Threadpool::Post([this, root]()
    {
        Run();
    }, TimeSpan::FromMilliseconds(ValidationDelayInMs));
}

std::wstring CheckpointTruncationTimestampValidator::GetTraceViewerQuery(Common::Guid partitionId, int64 replicaId)
{
    return wformatString(
        "\r\n(type~^General.CheckpointTruncationTimestampValidator && text~\"{0}.{1}\") || (type~^LR.Periodic|Api && type~{0} && text~{1})\n\n",
        partitionId.ToString(),
        replicaId);
}