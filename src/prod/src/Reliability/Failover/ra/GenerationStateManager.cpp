// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Transport;
using namespace Infrastructure;

GenerationStateManager::GenerationStateManager(ReconfigurationAgent & ra)
: ra_(ra)
{
}

#pragma region Generation Header

void GenerationStateManager::AddGenerationHeader(Message & message, bool isForFMReplica)
{
    AcquireReadLock grab(lock_);
    message.Headers.Add(GenerationHeader(GetGenerationState(isForFMReplica).SendGeneration, isForFMReplica));
}

bool GenerationStateManager::CheckGenerationHeader(GenerationHeader const & generationHeader, wstring const & action, Federation::NodeInstance const& from)
{
    AcquireReadLock grab(lock_);
    GenerationState const & generations = GetGenerationState(generationHeader.IsForFMReplica);
    if (generationHeader.Generation < generations.ReceiveGeneration)
    {
        RAEventSource::Events->GenerationIgnoreMessage(ra_.NodeInstanceIdStr, action, from, generationHeader, generations);

        return false;
    }

    return true;
}

#pragma endregion

#pragma region Generation Update

void GenerationStateManager::ProcessGenerationUpdate(
    std::wstring const & activityId,
    GenerationHeader const & generationHeader, 
    GenerationUpdateMessageBody const & requestBody, 
    Federation::NodeInstance const& from)
{
    // Process the update if it is from FM or FMM
    if (generationHeader.IsForFMReplica)
    {
        ProcessGenerationUpdateFromFmm(activityId, generationHeader, requestBody, from);
    }
    else
    {
        ProcessGenerationUpdateFromFM(activityId, generationHeader, requestBody, from);
    }
}

void GenerationStateManager::ProcessGenerationUpdateFromFmm(
    std::wstring const & activityId, 
    GenerationHeader const & generationHeader, 
    GenerationUpdateMessageBody const & , 
    Federation::NodeInstance const& )
{
    GenerationState & generationState = fmReplicaGenerations_;

    if (!TryUpdateReceiveGenerationFromGenerationUpdateMessage(activityId, generationState, generationHeader))
    {
        return;
    }

    StartUploadLfum(activityId, generationHeader);
}

void GenerationStateManager::ProcessGenerationUpdateFromFM(
    std::wstring const & activityId, 
    GenerationHeader const & generationHeader, 
    GenerationUpdateMessageBody const & requestBody, 
    Federation::NodeInstance const& from)
{
    EntityEntryBaseSPtr fmFailoverUnitEntry;
    auto result = PreProcessGenerationUpdateFromFM(activityId, generationHeader, from, fmFailoverUnitEntry);

    switch (result)
    {
    case PreProcessGenerationUpdateFromFMResult::StaleMessage:
        return;

    case PreProcessGenerationUpdateFromFMResult::UploadLfum:
        {
            // Exclude the fm replica
            StartUploadLfum(activityId, generationHeader);
            break;
        }

    case PreProcessGenerationUpdateFromFMResult::EnqueueFMReplicaJobItem:
        // The FM FT needs to be examined to see if it needs to be dropped before generation update can be processed
        {
            ASSERT_IF(fmFailoverUnitEntry == nullptr, "PreProcess must have returned the FM FT");

            auto handler = [](HandlerParameters & handlerParameters, GenerationUpdateJobItemContext & context)
            {
                return handlerParameters.RA.GenerationStateManagerObj.GenerationUpdateProcessor(handlerParameters, context);
            };

            GenerationUpdateJobItemContext context(requestBody.FMServiceEpoch, generationHeader);

            // If Generationupdate has been processed upload the lfum even if the node starts to close
            // The ra can take a snapshot at this time
            // otherwise since we do not reject the context rebuild is stalled
            GenerationUpdateJobItem::Parameters parameters(
                fmFailoverUnitEntry,
                activityId,
                handler,
                static_cast<JobItemCheck::Enum>(JobItemCheck::FTIsNotNull | JobItemCheck::RAIsOpenOrClosing),
                *JobItemDescription::GenerationUpdate,
                move(context));

            auto jobItem = make_shared<GenerationUpdateJobItem>(move(parameters));

            ra_.JobQueueManager.ScheduleJobItem(move(jobItem));

            break;
        }

    default:
        Assert::CodingError("unknown value {0}", static_cast<int>(result));
    }
}

GenerationStateManager::PreProcessGenerationUpdateFromFMResult::Enum GenerationStateManager::PreProcessGenerationUpdateFromFM(
    std::wstring const & activityId, 
    GenerationHeader const & generationHeader, 
    Federation::NodeInstance const & from,
    EntityEntryBaseSPtr & fmFailoverUnitEntry)
{
    GenerationState & generationState = commonGenerations_;

    // It is important to hold on to a write lock at this point
    // This is so that no AddPrimary in the current generation is processed if this code path accesses the LFUM
    AcquireWriteLock grab(lock_);
    
    // If the message is stale then return here itself
    if (!TryValidateGenerationUpdateMessageCallerHoldsLock(activityId, generationState, generationHeader))
    {
        return PreProcessGenerationUpdateFromFMResult::StaleMessage;
    }

    if (ra_.NodeInstance == from)
    {
        // The generation update is not stale
        // Process it and start the LFUM upload

        UpdateReceiveGenerationFromGenerationUpdateMessageCallerHoldsLock(activityId, generationState, generationHeader);

        return PreProcessGenerationUpdateFromFMResult::UploadLfum;
    }
    else
    {
        // This generation update message needs to acquire the FM failover unit to check if the FT needs to be dropped
        // First look in the LFUM to see if the FM FT exists or not
        fmFailoverUnitEntry = ra_.LocalFailoverUnitMapObj.GetEntry(FailoverUnitId(Constants::FMServiceGuid));
        if (fmFailoverUnitEntry == nullptr)
        {
            // FM FT does not exist
            // Update the receive generation and upload the LFUM
            UpdateReceiveGenerationFromGenerationUpdateMessageCallerHoldsLock(activityId, generationState, generationHeader);

            return PreProcessGenerationUpdateFromFMResult::UploadLfum;
        }

        return PreProcessGenerationUpdateFromFMResult::EnqueueFMReplicaJobItem;
    }
}

bool GenerationStateManager::GenerationUpdateProcessor(HandlerParameters & handlerParameters, GenerationUpdateJobItemContext & context)
{
    // This is always executed for messages from FM
    // This is executed in the scenario where a GenerationUpdate is received from the FM
    // and it is not known whether the existing FM FT on this node needs to be dropped or not
    // The message processing will have enqueued a job item for the FM FT and at this point the FM FT Lock is held
    ASSERT_IF(context.GenerationHeader.IsForFMReplica, "IsForFMReplica indicates that the message is from FMM. This is only executed if GenerationUPdate has arrived from FM");
    handlerParameters.AssertFTExists();

    GenerationState & generationState = commonGenerations_;
    GenerationHeader const & generationHeader = context.GenerationHeader;
    LockedFailoverUnitPtr & failoverUnit = handlerParameters.FailoverUnit;

    {
        // Acquire the generation lock
        AcquireWriteLock grab(lock_);

        // If the generation update message is now stale then drop it
        // This is needed because a thread switch has happened in the time since the message was received 
        // and the lock was released and reacquired
        if (!TryValidateGenerationUpdateMessageCallerHoldsLock(handlerParameters.ActivityId, generationState, generationHeader))
        {
            return true;
        }

        if (context.FMServiceEpoch > failoverUnit->CurrentConfigurationEpoch && 
            failoverUnit->State != FailoverUnitStates::Closed)
        {
            if (failoverUnit->LocalReplicaOpen)
            {
                // LocalReplicaopen is important so that ra has processed node up ack here 
                // it is incorrect to drop the replica without processing node up ack
                // do not delete the replica here because there is a scenario where the fmm has already
                // accepted the replica up for this ft and then the generation update arrives
                // if the ra force deletes it then the fmm will have a replica which does not exist on the node
                // this must be abort so that replica dropped is sent to the fm which 
                // will eventually clean up the state
                ra_.CloseLocalReplica(handlerParameters, ReplicaCloseMode::ForceAbort, ActivityDescription::Empty);
            }

            RAEventSource::Events->GenerationUpdate(ra_.NodeInstanceIdStr, context.FMServiceEpoch, handlerParameters.ActivityId);

            return true;
        }

        // Update the generation
        UpdateReceiveGenerationFromGenerationUpdateMessageCallerHoldsLock(handlerParameters.ActivityId, generationState, generationHeader);
    }

    // Start uploading the LFUM
    StartUploadLfum(handlerParameters.ActivityId, generationHeader);

    return true;
}

bool GenerationStateManager::TryUpdateReceiveGenerationFromGenerationUpdateMessage(
    std::wstring const & activityId, 
    GenerationState & generations,
    GenerationHeader const & generationHeader)
{
    AcquireWriteLock grab(lock_);

    if (!TryValidateGenerationUpdateMessageCallerHoldsLock(activityId, generations, generationHeader))
    {
        return false;
    }

    UpdateReceiveGenerationFromGenerationUpdateMessageCallerHoldsLock(activityId, generations, generationHeader);

    return true;
}

void GenerationStateManager::UpdateReceiveGenerationFromGenerationUpdateMessageCallerHoldsLock(
    wstring const & activityId,
    GenerationState & generations,
    GenerationHeader const & generationHeader)
{
    generations.ReceiveGeneration = generationHeader.Generation;

    RAEventSource::Events->GenerationSet(ra_.NodeInstanceIdStr, generations, generationHeader, activityId);
}

bool GenerationStateManager::TryValidateGenerationUpdateMessageCallerHoldsLock(
    wstring const & activityId,
    GenerationState & generations,
    GenerationHeader const & generationHeader)
{
    if (generationHeader.Generation != generations.ProposedGeneration)
    {
        RAEventSource::Events->GenerationUpdateRejected(ra_.NodeInstanceIdStr, generations, generationHeader, activityId);

        GenerationNumber body = generations.ProposedGeneration;
        MessageUPtr reply = RSMessage::GetGenerationUpdateReject().CreateMessage(body);
        if (generationHeader.IsForFMReplica)
        {
            ra_.Federation.SendToFMM(move(reply));
        }
        else
        {
            ra_.Federation.SendToFM(move(reply));
        }

        return false;
    }

    return true;
}

#pragma endregion

#pragma region Generation Proposal

GenerationProposalReplyMessageBody GenerationStateManager::ProcessGenerationProposal(Message & request, bool & isGfumUploadOut)
{
    GenerationHeader generationHeader = GenerationHeader::ReadFromMessage(request);

    AcquireWriteLock grab(lock_);

    GenerationState & generations = GetGenerationState(generationHeader.IsForFMReplica);
    if (generationHeader.Generation > generations.ProposedGeneration)
    {
        generations.ProposedGeneration = generationHeader.Generation;

        RAEventSource::Events->GenerationSet(ra_.NodeInstanceIdStr, generations, generationHeader, wformatString(request.MessageId));
    }

    isGfumUploadOut = generationHeader.IsForFMReplica;
    return GenerationProposalReplyMessageBody(generationHeader.Generation, generations.ProposedGeneration);
}

#pragma endregion

#pragma region LFUM Upload Reply

void GenerationStateManager::ProcessLFUMUploadReply(GenerationHeader const & generationHeader)
{
    AcquireWriteLock grab(lock_);

    GenerationState & generations = GetGenerationState(generationHeader.IsForFMReplica);
    if (generationHeader.Generation != generations.ReceiveGeneration)
    {
        RAEventSource::Events->GenerationIgnoreLFUMUploadReply(ra_.NodeInstance, generations, generationHeader);

        return;
    }

    generations.SendGeneration = generationHeader.Generation;

    RAEventSource::Events->GenerationUpdatedSendGeneration(ra_.NodeInstanceIdStr, generations, generationHeader);
}

#pragma endregion

void GenerationStateManager::ProcessNodeUpAck(std::wstring const & activityId, GenerationHeader const & generationHeader)
{
    AcquireWriteLock grab(lock_);

    GenerationState & generations = GetGenerationState(generationHeader.IsForFMReplica);
    if (generations.ReceiveGeneration <= generationHeader.Generation)
    {
        generations.ReceiveGeneration = generationHeader.Generation;
        generations.SendGeneration = generationHeader.Generation;

        if (generations.ProposedGeneration < generationHeader.Generation)
        {
            generations.ProposedGeneration = generationHeader.Generation;
        }

        RAEventSource::Events->GenerationSet(ra_.NodeInstanceIdStr, generations, generationHeader, activityId);
    }
}

#pragma region Lfum Upload Code

void GenerationStateManager::StartUploadLfum(std::wstring const & activityId, GenerationHeader const & header)
{
    bool anyReplicaFound = ra_.AnyReplicaFound(!header.IsForFMReplica);

    EntityEntryBaseList failoverUnits;
    if (header.IsForFMReplica)
    {
        failoverUnits = ra_.LocalFailoverUnitMapObj.GetFMFailoverUnitEntries();        
    }
    else
    {
        failoverUnits = ra_.LocalFailoverUnitMapObj.GetAllFailoverUnitEntries(true);
    }

    // Create the work input
    GetLfumInput input(header, anyReplicaFound);

    auto work = make_shared<MultipleFailoverUnitWorkWithInput<GetLfumInput>>(
        activityId,
        [] (MultipleEntityWork & inner, ReconfigurationAgent & ra)
        {
            ra.GenerationStateManagerObj.FinishGetLfumProcessing(inner);
        },
        move(input));

    ra_.JobQueueManager.CreateJobItemsAndStartMultipleFailoverUnitWork(
        work,
        failoverUnits,
        [](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
        {
            auto handler = [](HandlerParameters & handlerParameters, GetLfumJobItemContext & context)
            {
                return handlerParameters.RA.GenerationStateManagerObj.GetLfumProcessor(handlerParameters, context);
            };

            // If Generationupdate has been processed upload the lfum even if the node starts to close
            // The ra can take a snapshot at this time
            // otherwise since we do not reject the context rebuild is stalled
            GetLfumJobItem::Parameters jobItemParameters(
                entry,
                work,
                handler,
                static_cast<JobItemCheck::Enum>(JobItemCheck::FTIsNotNull | JobItemCheck::RAIsOpenOrClosing),
                *JobItemDescription::GetLfum,
                move(GetLfumJobItemContext()));

            return make_shared<GetLfumJobItem>(move(jobItemParameters));
        });
}

bool GenerationStateManager::GetLfumProcessor(HandlerParameters & handlerParameters, GetLfumJobItemContext & context)
{
    handlerParameters.AssertFTExists();
    handlerParameters.AssertHasWork();

    FailoverUnitInfo configuration;
    bool hasConfiguration = handlerParameters.FailoverUnit->TryGetConfiguration(handlerParameters.RA.NodeInstance, false, configuration);
    
    if (hasConfiguration)
    {
        context.SetConfiguration(move(configuration));
    }

    return true;
}

void GenerationStateManager::FinishGetLfumProcessing(MultipleEntityWork const & work)
{
    GetLfumInput const & input = work.GetInput<GetLfumInput>();

    vector<FailoverUnitInfo> failoverUnitInfo;
    for(auto it = work.JobItems.begin(); it != work.JobItems.end(); ++it)
    {
        auto & context = (*it)->GetContext<GetLfumJobItem>();
        context.AddToList(failoverUnitInfo);
    }

    RAEventSource::Events->GenerationUploadingLfum(
        ra_.NodeInstanceIdStr,
        input.GenerationHeader,
        work.ActivityId,
        failoverUnitInfo);

    if (input.GenerationHeader.IsForFMReplica)
    {
        ra_.Reliability.UploadLFUMForFMReplicaSet(
            input.GenerationHeader.Generation,
            move(failoverUnitInfo),
            input.AnyReplicaFound);
    }
    else
    {
        ra_.Reliability.UploadLFUM(
            input.GenerationHeader.Generation,
            move(failoverUnitInfo),
            input.AnyReplicaFound);
    }
}

#pragma endregion

#pragma region Helpers

GenerationState & GenerationStateManager::GetGenerationState(bool isForFMReplica)
{
    return (isForFMReplica ? fmReplicaGenerations_ : commonGenerations_);
}

#pragma endregion
