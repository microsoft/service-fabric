// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace ServiceModel;

StringLiteral const TraceProposal("Proposal");
StringLiteral const TraceReport("Report");

RebuildContext::RebuildContext(FailoverManager & fm, RwLock & lock)
    :   fm_(fm),
        updateCompleted_(false),
        recoverCompleted_(false),
        lock_(lock)
{
    auto root = fm_.CreateAsyncOperationRoot();
    timer_ = Timer::Create(TimerTagDefault, [this, root] (Common::TimerSPtr const&) { this->OnTimer(); });
}

Reliability::GenerationNumber const& RebuildContext::get_Generation() const
{
    if (generationNumber_)
    {
        return *generationNumber_;
    }

    return fm_.Generation; 
}

bool RebuildContext::IsActive() const
{
    return (fm_.IsActive && !fm_.IsReady);
}

void RebuildContext::Start()
{
    generationNumber_ = make_unique<GenerationNumber>(DateTime::Now().Ticks, fm_.Federation.Id);
    ErrorCode error = fm_.SetGeneration(*generationNumber_);
    if (error.IsSuccess())
    {
        generationNumber_ = nullptr;
    }

    fm_.InBuildFailoverUnitCacheObj.OnStartRebuild();

    StartProposal();
}

void RebuildContext::Stop()
{
    if (proposeContext_)
    {
        CloseProposalContext();
    }

    timer_->Cancel();
}

void RebuildContext::CloseProposalContext()
{
    if (proposeContext_)
    {
        IMultipleReplyContextSPtr context = proposeContext_;
        Threadpool::Post([context] { context->Close(); });
    }
}

void RebuildContext::StartProposal()
{
    pendingNodes_.clear();

    CloseProposalContext();

    fm_.WriteInfo(TraceProposal, "Start proposal generation {0}", Generation);

    MessageUPtr message = RSMessage::GetGenerationProposal().CreateMessage();
    message->Headers.Add(GenerationHeader(Generation, fm_.IsMaster));

    proposeContext_ = fm_.Federation.BroadcastRequest(move(message), FailoverConfig::GetConfig().MinRebuildRetryInterval);

    // A NULL proposeContext_ means that the FederationSubsystem is shutting down.
    // If so, we can ignore further processing, as the FM will also go down.
    if (proposeContext_)
    {
        PostReceiveForProposalReply(true);
    }
}

void RebuildContext::PostReceiveForProposalReply(bool canProcessSynchronously)
{
    IMultipleReplyContextSPtr proposeContext = proposeContext_;

    AsyncOperationSPtr operation = proposeContext_->BeginReceive(
        FailoverConfig::GetConfig().MaxRebuildRetryInterval,
        [this, proposeContext] (AsyncOperationSPtr const & operation)
            {
                if (!operation->CompletedSynchronously)
                {
                    this->ProcessProposalReply(operation, proposeContext);
                }
            },
        fm_.CreateAsyncOperationRoot());

    if (operation->CompletedSynchronously)
    {
        if (canProcessSynchronously)
        {
            ProcessProposalReplyCallerHoldingLock(operation, proposeContext);
        }
        else
        {
            auto root = fm_.CreateComponentRoot();

            Threadpool::Post([this, proposeContext, operation, root]
            {
                this->ProcessProposalReply(operation, proposeContext);
            });
        }
    }
}

void RebuildContext::ProcessProposalReply(AsyncOperationSPtr const & operation, IMultipleReplyContextSPtr const & proposeContext)
{
    AcquireExclusiveLock grab(lock_);
    ProcessProposalReplyCallerHoldingLock(operation, proposeContext);
}

void RebuildContext::ProcessProposalReplyCallerHoldingLock(AsyncOperationSPtr const & operation, IMultipleReplyContextSPtr const & proposeContext)
{
    MessageUPtr reply;
    NodeInstance from;
    ErrorCode result = proposeContext->EndReceive(operation, reply, from);
    if (proposeContext_ != proposeContext || !IsActive())
    {
        return;
    }

    if (result.IsError(ErrorCodeValue::Timeout))
    {
        auto healthReportThreshold = fm_.IsMaster ? FailoverConfig::GetConfig().MinRebuildRetryInterval : FailoverConfig::GetConfig().RebuildHealthReportThreshold;
        auto elapsedTime = Common::Stopwatch::Now() - fm_.ActivateTime;
        if (elapsedTime > healthReportThreshold)
        {
            fm_.HealthClient->AddHealthReport(fm_.HealthReportFactoryObj.GenerateRebuildStuckHealthReport(elapsedTime, healthReportThreshold));
        }
        fm_.WriteWarning(TraceProposal, "Rebuild broadcast is taking longer than expected to complete");
        PostReceiveForProposalReply(false);
        return;
    }
    else if (!result.IsSuccess())
    {
        fm_.WriteWarning(TraceProposal, "Proposal {0} failed {1}", Generation, result);
        return;
    }

    if (reply)
    {
        if (reply->Action == RSMessage::GetGFUMTransfer().Action)
        {
            GFUMMessageBody body;
            if (reply->GetBody(body))
            {
                fm_.ProcessGFUMTransfer(body);

                CloseProposalContext();

                updateCompleted_ = true;
                recoverCompleted_ = true;
                Complete();
            }
            else
            {
                fm_.WriteWarning(
                    TraceProposal, "Restarting rebuild because deserialization of GFUMTransfer message from {0} failed with {1}",
                    from, reply->Status);

                Start();
            }
        }
        else
        {
            GenerationProposalReplyMessageBody body;
            if (reply->GetBody(body))
            {
                if (body.Error.IsSuccess())
                {
                    ASSERT_IF(body.ProposedGeneration != Generation,
                        "Generation mismatch, current {0}, reply {1} from {2}",
                        Generation, body.ProposedGeneration, from);

                    if (body.CurrentGeneration == body.ProposedGeneration)
                    {
                        if (!updateCompleted_)
                        {
                            NodeUp(from);
                        }

                        PostReceiveForProposalReply(false);
                    }
                    else
                    {
                        fm_.WriteInfo(TraceProposal,
                            "Generation {0} rejected from {1} with {2}",
                            Generation, from, body.CurrentGeneration);

                        generationNumber_ = make_unique<GenerationNumber>(body.CurrentGeneration, fm_.Federation.Id);
                        ErrorCode error = fm_.SetGeneration(*generationNumber_);
                        if (error.IsSuccess())
                        {
                            generationNumber_ = nullptr;
                        }

                        StartProposal();
                    }
                }
                else
                {
                    fm_.WriteWarning(TraceProposal, "Ignoring node {0}: {1}", from, body.Error);

                    PostReceiveForProposalReply(false);
                }
            }
            else
            {
                fm_.WriteWarning(
                    TraceProposal, "Restarting rebuild because deserialization of GenerationProposalReply message from {0} failed with {1}",
                    from, reply->Status);

                Start();
            }
        }
    }
    else
    {
        proposeContext_->Close();
        proposeContext_ = nullptr;
        fm_.WriteInfo(TraceProposal, "Completed proposal generation {0}", Generation);

        StartUpdate();
    }
}

void RebuildContext::NodeUp(Federation::NodeInstance const & nodeInstance)
{
    auto it = pendingNodes_.find(nodeInstance.Id);
    if (it == pendingNodes_.end() || it->second.InstanceId < nodeInstance.InstanceId)
    {
        fm_.WriteInfo(TraceReport, "{0} adding pending node {1}",
            Generation, nodeInstance);

        pendingNodes_[nodeInstance.Id] = nodeInstance;
    }
}

void RebuildContext::NodeDown(Federation::NodeInstance const & nodeInstance)
{
    auto it = pendingNodes_.find(nodeInstance.Id);
    if (it != pendingNodes_.end() && it->second.InstanceId <= nodeInstance.InstanceId)
    {
        fm_.WriteInfo(TraceReport, "{0} removing pending node {1}",
            Generation, it->second);

        pendingNodes_.erase(it);

        if (pendingNodes_.empty())
        {
            updateCompleted_ = true;
            Complete();
        }
    }
}

MessageUPtr RebuildContext::CreateGenerationUpdateMessage()
{
    MessageUPtr message;

    if (fm_.IsMaster)
    {
        message = RSMessage::GetGenerationUpdate().CreateMessage();
    }
    else 
    {
        GenerationUpdateMessageBody body(fm_.PreOpenFMServiceEpoch);
        message = RSMessage::GetGenerationUpdate().CreateMessage(body);
    }

    message->Headers.Add(GenerationHeader(Generation, fm_.IsMaster));

    return message;
}

void RebuildContext::StartUpdate()
{
    if (IsActive())
    {
        if (fm_.IsMaster && !fm_.Federation.Token.Contains(NodeId::MinNodeId))
        {
            return;
        }

        fm_.Federation.Broadcast(CreateGenerationUpdateMessage());

        timer_->Change(FailoverConfig::GetConfig().MinRebuildRetryInterval);
    }
}

void RebuildContext::OnTimer()
{
    GenerationNumber generation;
    vector<NodeInstance> targets;

    {
        AcquireExclusiveLock grab(lock_);

        if (!IsActive())
        {
            return;
        }

        if (updateCompleted_)
        {
            Complete();
            return;
        }

        generation = Generation;

        for (auto it = pendingNodes_.begin(); it != pendingNodes_.end(); ++it)
        {
            targets.push_back(it->second);
        }

        timer_->Change(FailoverConfig::GetConfig().MaxRebuildRetryInterval);
    }

    fm_.WriteWarning(TraceReport, "Rebuild is waiting for nodes: {0}", targets);
    auto healthReportThreshold = fm_.IsMaster ? FailoverConfig::GetConfig().MinRebuildRetryInterval : FailoverConfig::GetConfig().RebuildHealthReportThreshold;
    auto elapsedTime = Common::Stopwatch::Now() - fm_.ActivateTime;
    if (elapsedTime > healthReportThreshold)
    {
        fm_.HealthClient->AddHealthReport(fm_.HealthReportFactoryObj.GenerateRebuildStuckHealthReport(targets, elapsedTime, healthReportThreshold));
    }

    for (NodeInstance const & target : targets)
    {
        fm_.SendToNodeAsync(
            CreateGenerationUpdateMessage(),
            target,
            FailoverConfig::GetConfig().MinRebuildRetryInterval,
            FailoverConfig::GetConfig().MaxRebuildRetryInterval);
    }
}

void RebuildContext::ProcessGenerationUpdateReject(Transport::Message & message, Federation::NodeInstance const & from)
{
    GenerationNumber generation;
    if (!message.GetBody(generation))
    {
        fm_.WriteWarning(TraceProposal, "Dropped proposal update from {0} because deserialization failed {1}", from, message.Status);
        return;
    }

    if (IsActive() && generation >= Generation)
    {
        fm_.WriteInfo(TraceProposal,
            "Generation {0} rejected during update from {1} with {2}",
            Generation, from, generation);

        generationNumber_ = make_unique<GenerationNumber>(generation, fm_.Federation.Id);
        ErrorCode error = fm_.SetGeneration(*generationNumber_);
        if (error.IsSuccess())
        {
            generationNumber_ = nullptr;
        }

        StartProposal();
    }
}

MessageUPtr RebuildContext::ProcessLFUMUpload(LFUMMessageBody const& body, Federation::NodeInstance const & from)
{
    MessageUPtr reply = RSMessage::GetLFUMUploadReply().CreateMessage();

    if (generationNumber_ && !fm_.SetGeneration(*generationNumber_).IsSuccess())
    {
        reply = nullptr;
    }
    else if (IsActive() && body.Generation == Generation)
    {
        generationNumber_ = nullptr;

        auto it = pendingNodes_.find(from.Id);
        if (it != pendingNodes_.end() && it->second.InstanceId == from.InstanceId)
        {
            ErrorCode result(ErrorCodeValue::Success);

            if (body.AnyReplicaFound)
            {
                fm_.InBuildFailoverUnitCacheObj.AnyReplicaFound = true;

                if (!fm_.IsMaster)
                {
                    result = fm_.ServiceCacheObj.CreateTombstoneService();
                }
            }

            for (FailoverUnitInfo const & failoverUnitInfo : body.FailoverUnitInfos)
            {
                ErrorCode e = fm_.InBuildFailoverUnitCacheObj.AddFailoverUnitReport(failoverUnitInfo, from);
                result = ErrorCode::FirstError(e, result);
            }

            if (result.IsSuccess())
            {
                NodeDescription const & node = body.Node;

                if (!fm_.IsMaster &&
                    ((fm_.FabricUpgradeManager.CurrentVersionInstance == Common::FabricVersionInstance::Invalid &&
                      node.VersionInstance != Common::FabricVersionInstance::Invalid) ||
                     fm_.FabricUpgradeManager.CurrentVersionInstance.InstanceId < node.VersionInstance.InstanceId))
                {
                    result = fm_.FabricUpgradeManager.UpdateCurrentVersionInstance(node.VersionInstance);
                }

                if (result.IsSuccess())
                {
                    // TODO: processing load information.

                    NodeDescription nodeDescription(
                        node.VersionInstance,
                        node.NodeUpgradeDomainId,
                        node.NodeFaultDomainIds,
                        node.NodePropertyMap,
                        node.NodeCapacityRatioMap,
                        node.NodeCapacityMap,
                        node.NodeName,
                        node.NodeType,
                        node.IpAddressOrFQDN,
                        node.ClusterConnectionPort,
                        node.HttpGatewayPort);

                    NodeInfoSPtr nodeInfo = make_shared<NodeInfo>(
                        from,
                        move(nodeDescription),
                        true);

                    FabricVersionInstance versionInstance;
                    result = fm_.NodeCacheObj.NodeUp(move(nodeInfo), false /* IsVersionGatekeepingNeeded */, versionInstance);
                }
            }

            if (result.IsSuccess())
            {
                pendingNodes_.erase(it);

                fm_.WriteInfo(TraceReport,
                    "Generation {0} processed upload from {1}, pending {2}\r\n{3}",
                    Generation, from, pendingNodes_.size(), body);

                if (pendingNodes_.empty())
                {
                    updateCompleted_ = true;
                    Complete();
                }
            }
            else
            {
                fm_.WriteWarning(TraceReport,
                    "Generation {0} failed to process upload from {1}: {2}",
                    Generation, from, result);

                reply = nullptr;
            }
        }
    }

    return reply;
}

void RebuildContext::CheckRecovery()
{
    if (!recoverCompleted_)
    {
        if (fm_.NodeCacheObj.UpCount >= static_cast<size_t>(FailoverConfig::GetConfig().ExpectedClusterSize))
        {
            if (fm_.IsMaster && !fm_.InBuildFailoverUnitCacheObj.AnyReplicaFound)
            {
                fm_.CreateFMService();
            }

            recoverCompleted_ = true;
        }
        else
        {
            fm_.WriteWarning(TraceReport,
                "FM service not created because cluster is below expected size. {0} nodes up, expected {1}.",
                fm_.NodeCacheObj.UpCount,
                FailoverConfig::GetConfig().ExpectedClusterSize);
        }
    }
}

void RebuildContext::Complete()
{
    ErrorCode result = fm_.InBuildFailoverUnitCacheObj.OnRebuildComplete();
    if (!result.IsSuccess())
    {
        fm_.WriteWarning(TraceReport, "Rebuild completed with error {0}", result);
    }

    timer_->Cancel();

    CheckRecovery();

    pendingNodes_.clear();

    fm_.WriteInfo(TraceReport, "Generation {0} rebuild completed", Generation);

    fm_.CompleteLoad();

    fm_.HealthClient->AddHealthReport(fm_.HealthReportFactoryObj.GenerateClearRebuildStuckHealthReport());
}
