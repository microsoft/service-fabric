// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;

CancelFabricUpgradeContext::CancelFabricUpgradeContext(
    ServiceModel::FabricUpgradeSpecification && upgradeSpecification, 
    ReconfigurationAgent & ra)
:  ICancelUpgradeContext(upgradeSpecification.InstanceId),
   upgradeSpecification_(move(upgradeSpecification)),
   ra_(ra)
{
}

ICancelUpgradeContextSPtr CancelFabricUpgradeContext::Create(ServiceModel::FabricUpgradeSpecification && upgradeSpecification, ReconfigurationAgent & ra)
{
    return make_shared<CancelFabricUpgradeContext>(move(upgradeSpecification), ra);
}

void CancelFabricUpgradeContext::SendReply()
{
    Common::FabricVersionInstance inst(upgradeSpecification_.Version, upgradeSpecification_.InstanceId);
    NodeFabricUpgradeReplyMessageBody reply(inst);
    ra_.FMTransportObj.SendCancelFabricUpgradeReplyMessage(L"", reply);
}

CancelApplicationUpgradeContext::CancelApplicationUpgradeContext(UpgradeDescription && upgradeDescription, ReconfigurationAgent & ra)
: ICancelUpgradeContext(upgradeDescription.InstanceId),
  upgradeDescription_(move(upgradeDescription)),
  ra_(ra)
{
}

ICancelUpgradeContextSPtr CancelApplicationUpgradeContext::Create(UpgradeDescription && upgradeDescription, ReconfigurationAgent & ra)
{
    return make_shared<CancelApplicationUpgradeContext>(move(upgradeDescription), ra);
}

void CancelApplicationUpgradeContext::SendReply()
{
    vector<FailoverUnitInfo> dropped, up;
    NodeUpgradeReplyMessageBody reply(upgradeDescription_.Specification, move(dropped), move(up));
    ra_.FMTransportObj.SendCancelApplicationUpgradeReplyMessage(L"", reply);
}
