// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION( NodeQueryResult )

NodeQueryResult::NodeQueryResult()
    : nodeName_()
    , ipAddressOrFQDN_()
    , nodeType_()
    , codeVersion_()
    , configVersion_()
    , nodeStatus_(FABRIC_QUERY_NODE_STATUS_INVALID)
    , nodeUpTimeInSeconds_(0)
    , nodeDownTimeInSeconds_(0)
    , nodeUpAt_(DateTime::Zero)
    , nodeDownAt_(DateTime::Zero)
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
    , isSeedNode_(false)
    , upgradeDomain_()
    , faultDomain_()
    , nodeId_()
    , instanceId_(0)
    , nodeDeactivationInfo_()
    , clusterConnectionPort_(0)
    , isStopped_(false)
{
}

NodeQueryResult::NodeQueryResult(
    std::wstring const & nodeName,
    std::wstring const & ipAddressOrFQDN,
    std::wstring const & nodeType,
    std::wstring const & codeVersion,
    std::wstring const & configVersion,
    FABRIC_QUERY_NODE_STATUS nodeStatus,
    int64 nodeUpTimeInSeconds,
    int64 nodeDownTimeInSeconds,
    DateTime nodeUpAt,
    DateTime nodeDownAt,
    bool isSeedNode,
    std::wstring const & upgradeDomain,
    std::wstring const & faultDomain,
    Federation::NodeId const & nodeId,
    uint64 instanceId,
    NodeDeactivationQueryResult && nodeDeactivationInfo,
    unsigned short httpGatewayPort,
    ULONG clusterConnectionPort,
    bool isStopped)
    : nodeName_(nodeName)
    , ipAddressOrFQDN_(ipAddressOrFQDN)
    , nodeType_(nodeType)
    , codeVersion_(codeVersion)
    , configVersion_(configVersion)
    , nodeStatus_(nodeStatus)
    , nodeUpTimeInSeconds_(nodeUpTimeInSeconds)
    , nodeDownTimeInSeconds_(nodeDownTimeInSeconds)
    , nodeUpAt_(nodeUpAt)
    , nodeDownAt_(nodeDownAt)
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
    , isSeedNode_(isSeedNode)
    , upgradeDomain_(upgradeDomain)
    , faultDomain_(faultDomain)
    , nodeId_(nodeId)
    , instanceId_(instanceId)
    , nodeDeactivationInfo_(move(nodeDeactivationInfo))
    , httpGatewayPort_(httpGatewayPort)
    , clusterConnectionPort_(clusterConnectionPort)
    , isStopped_(isStopped)
{
}

NodeQueryResult::NodeQueryResult(
    std::wstring && nodeName,
    std::wstring && ipAddressOrFQDN,
    std::wstring && nodeType,
    bool isSeedNode,
    std::wstring && upgradeDomain,
    std::wstring && faultDomain,
    Federation::NodeId const & nodeId)
    : nodeName_(move(nodeName))
    , ipAddressOrFQDN_(move(ipAddressOrFQDN))
    , nodeType_(move(nodeType))
    , codeVersion_(L"unknown")
    , configVersion_(L"unknown")
    , nodeStatus_(FABRIC_QUERY_NODE_STATUS_INVALID)
    , nodeUpTimeInSeconds_(false)
    , nodeDownTimeInSeconds_(false)
    , nodeUpAt_(DateTime::Zero)
    , nodeDownAt_(DateTime::Zero)
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
    , isSeedNode_(isSeedNode)
    , upgradeDomain_(move(upgradeDomain))
    , faultDomain_(move(faultDomain))
    , nodeId_(nodeId)
    , instanceId_(FABRIC_INVALID_NODE_INSTANCE_ID)
    , nodeDeactivationInfo_()
    , clusterConnectionPort_(0)
    , isStopped_(false)
{
}

NodeQueryResult::NodeQueryResult(
    std::wstring const & nodeName,
    std::wstring const & ipAddressOrFQDN,
    std::wstring const & nodeType,
    bool isSeedNode,
    std::wstring const & upgradeDomain,
    std::wstring const & faultDomain,
    Federation::NodeId const & nodeId)
    : nodeName_(nodeName)
    , ipAddressOrFQDN_(ipAddressOrFQDN)
    , nodeType_(nodeType)
    , codeVersion_(L"unknown")
    , configVersion_(L"unknown")
    , nodeStatus_(FABRIC_QUERY_NODE_STATUS_INVALID)
    , nodeUpTimeInSeconds_(false)
    , nodeDownTimeInSeconds_(false)
    , nodeUpAt_(DateTime::Zero)
    , nodeDownAt_(DateTime::Zero)
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
    , isSeedNode_(isSeedNode)
    , upgradeDomain_(upgradeDomain)
    , faultDomain_(faultDomain)
    , nodeId_(nodeId)
    , instanceId_(FABRIC_INVALID_NODE_INSTANCE_ID)
    , nodeDeactivationInfo_()
    , clusterConnectionPort_(0)
    , isStopped_(false)
{
}

NodeQueryResult::~NodeQueryResult()
{
}

void NodeQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_NODE_QUERY_RESULT_ITEM & publicNodeQueryResult) const
{
    publicNodeQueryResult.NodeName = heap.AddString(nodeName_);
    publicNodeQueryResult.IpAddressOrFQDN = heap.AddString(ipAddressOrFQDN_);
    publicNodeQueryResult.NodeType = heap.AddString(nodeType_);
    publicNodeQueryResult.CodeVersion = heap.AddString(codeVersion_);
    publicNodeQueryResult.ConfigVersion = heap.AddString(configVersion_);
    publicNodeQueryResult.NodeStatus = nodeStatus_;
    publicNodeQueryResult.NodeUpTimeInSeconds = nodeUpTimeInSeconds_;
    publicNodeQueryResult.AggregatedHealthState = healthState_;
    publicNodeQueryResult.IsSeedNode = isSeedNode_ ? TRUE : FALSE;
    publicNodeQueryResult.UpgradeDomain = heap.AddString(upgradeDomain_);
    publicNodeQueryResult.FaultDomain = heap.AddString(faultDomain_);

    auto publicNodeQueryResultEx1 = heap.AddItem<FABRIC_NODE_QUERY_RESULT_ITEM_EX1>();
    nodeId_.ToPublicApi(publicNodeQueryResultEx1->NodeId);

    auto publicNodeQueryResultEx2 = heap.AddItem<FABRIC_NODE_QUERY_RESULT_ITEM_EX2>();
    publicNodeQueryResultEx2->NodeInstanceId = instanceId_;
    publicNodeQueryResultEx1->Reserved = publicNodeQueryResultEx2.GetRawPointer();

    auto publicNodeQueryResultEx3 = heap.AddItem<FABRIC_NODE_QUERY_RESULT_ITEM_EX3>();
    auto publicNodeDeactivationInfo = heap.AddItem<FABRIC_NODE_DEACTIVATION_QUERY_RESULT_ITEM>();
    nodeDeactivationInfo_.ToPublicApi(heap, *publicNodeDeactivationInfo);
    publicNodeQueryResultEx3->NodeDeactivationInfo = publicNodeDeactivationInfo.GetRawPointer();
    publicNodeQueryResultEx2->Reserved = publicNodeQueryResultEx3.GetRawPointer();

    auto publicNodeQueryResultEx4 = heap.AddItem<FABRIC_NODE_QUERY_RESULT_ITEM_EX4>();
    publicNodeQueryResultEx4->IsStopped = isStopped_;

    auto publicNodeQueryResultEx5 = heap.AddItem<FABRIC_NODE_QUERY_RESULT_ITEM_EX5>();
    publicNodeQueryResultEx5->NodeDownTimeInSeconds = nodeDownTimeInSeconds_;

    auto publicNodeQueryResultEx6 = heap.AddItem<FABRIC_NODE_QUERY_RESULT_ITEM_EX6>();
    publicNodeQueryResultEx6->NodeUpAt = nodeUpAt_.AsFileTime;
    publicNodeQueryResultEx6->NodeDownAt = nodeDownAt_.AsFileTime;
    publicNodeQueryResultEx6->Reserved = NULL;

    publicNodeQueryResultEx5->Reserved = publicNodeQueryResultEx6.GetRawPointer();
    publicNodeQueryResultEx4->Reserved = publicNodeQueryResultEx5.GetRawPointer();
    publicNodeQueryResultEx3->Reserved = publicNodeQueryResultEx4.GetRawPointer();

    publicNodeQueryResult.Reserved = publicNodeQueryResultEx1.GetRawPointer();
}

ErrorCode NodeQueryResult::FromPublicApi(__in FABRIC_NODE_QUERY_RESULT_ITEM const& publicNodeQueryResult)
{
    nodeName_ = publicNodeQueryResult.NodeName;
    ipAddressOrFQDN_ = publicNodeQueryResult.IpAddressOrFQDN;
    nodeType_ = publicNodeQueryResult.NodeType;
    codeVersion_ = publicNodeQueryResult.CodeVersion;
    configVersion_ = publicNodeQueryResult.ConfigVersion;
    nodeStatus_ = publicNodeQueryResult.NodeStatus;
    nodeUpTimeInSeconds_ = publicNodeQueryResult.NodeUpTimeInSeconds;
    healthState_ = publicNodeQueryResult.AggregatedHealthState;
    isSeedNode_ = publicNodeQueryResult.IsSeedNode ? true : false;
    upgradeDomain_ = publicNodeQueryResult.UpgradeDomain;
    faultDomain_ = publicNodeQueryResult.FaultDomain;

    if (publicNodeQueryResult.Reserved != NULL)
    {
        FABRIC_NODE_QUERY_RESULT_ITEM_EX1 * publicNodeQueryResultEx1 = (FABRIC_NODE_QUERY_RESULT_ITEM_EX1*) publicNodeQueryResult.Reserved;
        nodeId_.FromPublicApi(publicNodeQueryResultEx1->NodeId);

        if(publicNodeQueryResultEx1->Reserved != NULL)
        {
            FABRIC_NODE_QUERY_RESULT_ITEM_EX2 * publicNodeQueryResultEx2 = (FABRIC_NODE_QUERY_RESULT_ITEM_EX2*) publicNodeQueryResultEx1->Reserved;
            instanceId_ = publicNodeQueryResultEx2->NodeInstanceId;

            if (publicNodeQueryResultEx2->Reserved != NULL)
            {
                FABRIC_NODE_QUERY_RESULT_ITEM_EX3 * publicNodeQueryResultEx3 = (FABRIC_NODE_QUERY_RESULT_ITEM_EX3*)publicNodeQueryResultEx2->Reserved;
                ErrorCode error = nodeDeactivationInfo_.FromPublicApi(*publicNodeQueryResultEx3->NodeDeactivationInfo);
                if (!error.IsSuccess())
                {
                    return error;
                }

                if(publicNodeQueryResultEx3->Reserved != NULL)
                {
                    FABRIC_NODE_QUERY_RESULT_ITEM_EX4 * publicNodeQueryResultEx4 = (FABRIC_NODE_QUERY_RESULT_ITEM_EX4*)publicNodeQueryResultEx3->Reserved;
                    isStopped_ = publicNodeQueryResultEx4->IsStopped ? true : false;

                    if(publicNodeQueryResultEx4->Reserved != NULL)
                    {
                        FABRIC_NODE_QUERY_RESULT_ITEM_EX5 * publicNodeQueryResultEx5 = (FABRIC_NODE_QUERY_RESULT_ITEM_EX5*)publicNodeQueryResultEx4->Reserved;
                        nodeDownTimeInSeconds_ = publicNodeQueryResultEx5->NodeDownTimeInSeconds;

                        if(publicNodeQueryResultEx5->Reserved != NULL)
                        {
                            FABRIC_NODE_QUERY_RESULT_ITEM_EX6 * publicNodeQueryResultEx6 = (FABRIC_NODE_QUERY_RESULT_ITEM_EX6*)publicNodeQueryResultEx5->Reserved;
                            nodeUpAt_ =  DateTime(publicNodeQueryResultEx6->NodeUpAt);
                            nodeDownAt_ =  DateTime(publicNodeQueryResultEx6->NodeDownAt);
                        }
                    }
                }
            }
        }
    }

    return ErrorCode::Success();
}

void NodeQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring NodeQueryResult::ToString() const
{
    // TODO: ToString needs to be changed since JSON doesnt serialize the clusterconnectionport
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<NodeQueryResult&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

std::wstring NodeQueryResult::CreateContinuationToken() const
{
    return PagingStatus::CreateContinuationToken<Federation::NodeId>(nodeId_);
}
