// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace MessageRetry;
using namespace Infrastructure;
using ReconfigurationAgentComponent::Communication::FMMessageData;
using ReconfigurationAgentComponent::Communication::FMMessageDescription;

FMMessagePendingEntityList::FMMessagePendingEntityList(
    Reliability::FailoverManagerId const & fm,
    std::wstring const & displayName,
    EntitySetCollection & setCollection,
    IThrottle & throttle,
    ReconfigurationAgent & ra) :
    entitySet_(EntitySet::Parameters(displayName, EntitySetIdentifier(EntitySetName::FailoverManagerMessageRetry, fm), nullptr)),
    ra_(ra),
    throttle_(&throttle)
{
    // A retry timer is not needed for this set
    // The retry is handled by the message retry component
    // Every time the set changes, a message needs to be sent
    // and Request will be called on the MessageRetryComponent
    // and that will ensure that the timer is armed appropriately
    setCollection.Add(entitySet_.Id, entitySet_, nullptr);
}

IClock const & FMMessagePendingEntityList::get_Clock() const
{
    return ra_.Clock;
}

FMMessagePendingEntityList::EnumerationResult FMMessagePendingEntityList::Enumerate(
    __inout Diagnostics::FMMessageRetryPerformanceData & perfData)
{    
    perfData.OnSnapshotStart();

    auto entityList = entitySet_.GetEntities();

    perfData.OnGenerationStart(entityList.size());

    auto rv = CreateResult(move(entityList));

    perfData.OnGenerationEnd(rv.Count);

    return rv;
}

FMMessagePendingEntityList::EnumerationResult FMMessagePendingEntityList::CreateResult(
    EntityEntryBaseList && entities)
{
    FMMessagePendingEntityList::EnumerationResult rv;

    auto limit = throttle_->GetCount(Clock.Now());

    rv.RetryType = ComputeRetryType(limit, entities.size());

    rv.Messages = CreateMessages(limit, move(entities));

    return rv;
}

vector<FMMessageDescription> FMMessagePendingEntityList::CreateMessages(
    int throttle,
    Infrastructure::EntityEntryBaseList && entities)
{
    vector<FMMessageDescription> rv;

    auto now = ra_.Clock.Now();

    for (auto & it : entities)
    {
        if (rv.size() == throttle)
        {
            break;
        }

        FMMessageData messageData;
        if (TryComposeFMMessage(now, it, messageData))
        {
            FMMessageDescription desc(move(it), move(messageData));
            rv.push_back(move(desc));
        }
    }

    return rv;
}

BackgroundWorkRetryType::Enum FMMessagePendingEntityList::ComputeRetryType(
    int throttle,
    size_t pendingItemCount) const
{
    if (pendingItemCount == 0)
    {
        return BackgroundWorkRetryType::None;
    }
    else if (pendingItemCount <= throttle)
    {
        return BackgroundWorkRetryType::Deferred;
    }
    else
    {
        return BackgroundWorkRetryType::Immediate;
    }
}

bool FMMessagePendingEntityList::TryComposeFMMessage(
    Common::StopwatchTime now,
    EntityEntryBaseSPtr const & entity,
    __out FMMessageData & message)
{
    bool wasCreated = false;

    ExecuteUnderReadLock<FailoverUnit>(
        entity,
        [&message, &wasCreated, now, this](ReadOnlyLockedFailoverUnitPtr & lock)
        {
            if (!lock)
            {
                wasCreated = false;
                return;
            }

            wasCreated = lock->TryComposeFMMessage(ra_.NodeInstance, now, message);
        });

    return wasCreated;
}

AsyncOperationSPtr FMMessagePendingEntityList::BeginUpdateEntitiesAfterSend(
    std::wstring const & activityId,
    Diagnostics::FMMessageRetryPerformanceData & perfData, // guaranteed to be kept alive by parent
    EnumerationResult const & enumerationResult, // guaranteed to be kept alive by parent
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    perfData.OnUpdateStart();

    auto now = ra_.Clock.Now();

    // todo: this code should ideally be in the async op but since it is one step it doesn't matter
    auto op = AsyncOperation::CreateAndStart<AsyncOperationImpl>(callback, parent);

    auto workCb = [this, &perfData, &enumerationResult, op](MultipleEntityWork & , ReconfigurationAgent &)
    {
        ASSERT_IF(enumerationResult.Count > numeric_limits<int>::max(), "Too many entities generated");
        throttle_->Update(static_cast<int>(enumerationResult.Count), Clock.Now());
        perfData.OnUpdateEnd(); 
        op->TryComplete(op);
    };

    auto work = make_shared<MultipleEntityWork>(activityId, workCb);

    for (size_t i = 0; i < enumerationResult.Count; i++)
    {
        int64 sequenceNumber = enumerationResult.Messages[i].Message.SequenceNumber;

        auto processor = [now, sequenceNumber](HandlerParameters & handlerParameters, JobItemContextBase & )
        {
            handlerParameters.AssertFTExists();
            
            handlerParameters.FailoverUnit->OnFMMessageSent(now, sequenceNumber, handlerParameters.ExecutionContext);
            
            return true;
        };

        UpdateEntityAfterFMMessageSendJobItem::Parameters parameters(
            enumerationResult.Messages[i].Source,
            work,
            processor,
            JobItemCheck::FTIsNotNull,
            *JobItemDescription::UpdateAfterFMMessageSend);

        auto job = make_shared<UpdateEntityAfterFMMessageSendJobItem>(move(parameters));

        work->AddFailoverUnitJob(move(job));
    }

    ra_.JobQueueManager.ExecuteMultipleFailoverUnitWork(move(work));

    return op;
}

ErrorCode FMMessagePendingEntityList::EndUpdateEntitiesAfterSend(
    AsyncOperationSPtr const & op)
{
    return AsyncOperation::End(op);
}
