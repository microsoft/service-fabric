// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Reliability::FailoverManagerComponent;
using namespace std;

UpdateServiceContext::UpdateServiceContext(ServiceInfoSPtr const& serviceInfo)
    : serviceInfo_(serviceInfo),
      readyNodes_(),
      pendingCount_(0),
      failoverUnitId_(Guid::Empty()),
      BackgroundThreadContext(wformatString("UpdateServiceContext.{0}.{1}", serviceInfo->Name, serviceInfo->ServiceDescription.UpdateVersion))
{
}

BackgroundThreadContextUPtr UpdateServiceContext::CreateNewContext() const
{
    return make_unique<UpdateServiceContext>(serviceInfo_);
}

void UpdateServiceContext::AddReadyNode(NodeId nodeId)
{
    if (readyNodes_.find(nodeId) == readyNodes_.end())
    {
        readyNodes_.insert(nodeId);
    }
}

void UpdateServiceContext::Merge(BackgroundThreadContext const& context)
{
    UpdateServiceContext const& other = dynamic_cast<UpdateServiceContext const&>(context);

    for (auto node = other.readyNodes_.begin(); node != other.readyNodes_.end(); ++node)
    {
        AddReadyNode(*node);
    }

    pendingCount_ += other.pendingCount_;
}

void UpdateServiceContext::Process(FailoverManager const& fm, FailoverUnit const& failoverUnit)
{
    UNREFERENCED_PARAMETER(fm);

    if (failoverUnit.ServiceInfoObj->Name == serviceInfo_->Name)
    {
        for (auto replica = failoverUnit.BeginIterator; replica != failoverUnit.EndIterator; ++replica)
        {
            if (replica->ServiceUpdateVersion != serviceInfo_->ServiceDescription.UpdateVersion &&
                replica->IsNodeUp)
            {
                AddReadyNode(replica->FederationNodeId);
            }
        }

        if (serviceInfo_->RepartitionInfo &&
            serviceInfo_->RepartitionInfo->IsRemoved(failoverUnit.Id))
        {
            pendingCount_++;
        }
    }
}

bool UpdateServiceContext::ReadyToComplete()
{
    return (readyNodes_.size() == 0 && pendingCount_ == 0);
}

void UpdateServiceContext::Complete(FailoverManager & fm, bool isContextCompleted, bool isEnumerationAborted)
{
    UNREFERENCED_PARAMETER(isEnumerationAborted);

    fm.WriteInfo(
        Constants::ServiceSource, serviceInfo_->Name,
        "UpdateService {0}: Instance={1}, UpdateVersion={2}, ReadyNodes={3}, PendingCount={4}, IsContextCompleted={5}",
        serviceInfo_->Name, serviceInfo_->Instance, serviceInfo_->ServiceDescription.UpdateVersion, readyNodes_, pendingCount_, isContextCompleted);

    if (isContextCompleted)
    {
        fm.ServiceCacheObj.CompleteServiceUpdateAsync(
            serviceInfo_->Name,
            serviceInfo_->Instance,
            serviceInfo_->ServiceDescription.UpdateVersion);
    }
    else
    {
        ServiceInfoSPtr serviceInfo = fm.ServiceCacheObj.GetService(serviceInfo_->Name);
        if (serviceInfo && 
            serviceInfo->Instance == serviceInfo_->Instance &&
            serviceInfo->IsServiceUpdateNeeded &&
            serviceInfo->ServiceDescription.UpdateVersion == serviceInfo_->ServiceDescription.UpdateVersion)
        {
            for (auto nodeId = readyNodes_.begin(); nodeId != readyNodes_.end(); ++nodeId)
            {
                NodeInfoSPtr nodeInfo = fm.NodeCacheObj.GetNode(*nodeId);
                if (nodeInfo && nodeInfo->IsUp)
                {
                    Transport::MessageUPtr request = RSMessage::GetNodeUpdateServiceRequest().CreateMessage(
                        NodeUpdateServiceRequestMessageBody(serviceInfo->ServiceDescription));

                    GenerationHeader header(fm.Generation, fm.IsMaster);
                    request->Headers.Add(header);

                    fm.SendToNodeAsync(move(request), nodeInfo->NodeInstance);
                }
            }

            serviceInfo->ClearUpdatedNodes();
        }
    }
}
