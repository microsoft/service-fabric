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
using namespace ServiceModel;
using namespace Infrastructure;

using Node::NodeDeactivationInfo;

namespace
{
    std::wstring GetActivityId(bool isFmm)
    {
        if (isFmm)
        {
            return L"NodeUpAck_FMM";
        }
        else
        {
            return L"NodeUpAck_FM";
        }
    }
}
NodeUpAckProcessor::NodeUpAckProcessor(ReconfigurationAgent & ra)
: ra_(ra)
{
}

void NodeUpAckProcessor::ProcessNodeUpAck(
    wstring && ,
    NodeUpAckMessageBody && body,
    FailoverManagerId const & sender)
{
    wstring activityId = sender.IsFmm ? L"NodeUpAck_FMM" : L"NodeUpAck_FM";

    if (TryUpgradeFabric(sender, activityId, body))
    {
        return;
    }

    NodeDeactivationInfo info(body.IsNodeActivated, body.NodeActivationSequenceNumber);
    ra_.NodeDeactivationMessageProcessorObj.ProcessNodeUpAck(activityId, sender, info);

    ProcessNodeUpAck(sender, move(activityId), move(body));
}

bool NodeUpAckProcessor::TryUpgradeFabric(
    FailoverManagerId const & sender,
    std::wstring const & activityId, 
    NodeUpAckMessageBody const & nodeUpAckBody)
{
    // FMM cannot initiate fabric upgrade at node up
    if (sender.IsFmm)
    {
        return false;
    }

    auto nodeVersion = ra_.Reliability.NodeConfig->NodeVersion;
    auto const & incomingVersion = nodeUpAckBody.FabricVersionInstance;

    Upgrade::FabricUpgradeStalenessChecker stalenessChecker;
    auto action = stalenessChecker.CheckFabricUpgradeAtNodeUp(nodeVersion, incomingVersion);

    switch (action)
    {
    case Upgrade::FabricUpgradeStalenessCheckResult::Assert:
        Assert::CodingError("Unexpected node version instance {0} in node up ack. Local node is at {1}", incomingVersion, nodeVersion);

    case Upgrade::FabricUpgradeStalenessCheckResult::UpgradeNotRequired:
        return false;

    case Upgrade::FabricUpgradeStalenessCheckResult::UpgradeRequired:
        {
            ServiceModel::FabricUpgradeSpecification upgradeSpec(
                incomingVersion.Version,
                incomingVersion.InstanceId,
                ServiceModel::UpgradeType::Rolling,
                true,
                false);

            auto upgrade = make_unique<Upgrade::FabricUpgrade>(activityId, nodeVersion.Version.CodeVersion, move(upgradeSpec), true, ra_);
            auto upgradeStateMachine = Upgrade::UpgradeStateMachine::Create(move(upgrade), ra_);

            ra_.UpgradeMessageProcessorObj.ProcessUpgradeMessage(*Reliability::Constants::FabricApplicationId, upgradeStateMachine);

            return true;
        }
    default:
        Assert::CodingError("Unknown action {0}", static_cast<int>(action));
    }
}

void NodeUpAckProcessor::ProcessNodeUpAck(
    FailoverManagerId const & sender,
    std::wstring && activityId,
    NodeUpAckMessageBody && body)
{
    NodeUpAckInput input(move(body));

    auto failoverUnits = ra_.LocalFailoverUnitMapObj.GetFailoverUnitEntries(sender);

    auto isNodeActivated = ra_.NodeStateObj.GetNodeDeactivationState(sender).IsActivated;

    auto work = make_shared<MultipleFailoverUnitWorkWithInput<NodeUpAckInput>>(
        activityId,
        [this, sender, isNodeActivated](MultipleEntityWork & inner, ReconfigurationAgent & )
        {
            ra_.StateObj.OnNodeUpAckProcessed(sender);
            ra_.NodeStateObj.GetPendingReplicaUploadStateProcessor(sender).ProcessNodeUpAck(inner.ActivityId, isNodeActivated);
        },
        move(input));

    auto handler = [](HandlerParameters & handlerParameters, JobItemContextBase & context)
    {
        return handlerParameters.RA.NodeUpAckProcessorObj.NodeUpAckFTProcessor(handlerParameters, context);
    };

    ra_.JobQueueManager.CreateJobItemsAndStartMultipleFailoverUnitWork(
        work,
        failoverUnits,
        [handler](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
        {
            NodeUpAckJobItem::Parameters jobItemParameters(
                entry,
                work,
                handler,
                JobItemCheck::Default,
                *JobItemDescription::NodeUpAck,
                move(JobItemContextBase()));

            return make_shared<NodeUpAckJobItem>(move(jobItemParameters));
        });
}

bool NodeUpAckProcessor::NodeUpAckFTProcessor(HandlerParameters & handlerParameters, JobItemContextBase & )
{
    handlerParameters.AssertHasWork();
    handlerParameters.AssertFTExists();

    auto & failoverUnit = handlerParameters.FailoverUnit;
    auto const & input = handlerParameters.Work->GetInput<NodeUpAckInput>();

    auto const & upgrades = input.Body.Upgrades;
    auto isActivated = handlerParameters.RA.NodeStateObj.GetNodeDeactivationState(failoverUnit->Owner).IsActivated;

    failoverUnit->ProcessNodeUpAck(upgrades, isActivated, handlerParameters.ExecutionContext);

    return true;
}
