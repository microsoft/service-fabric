// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Reliability::FailoverManagerComponent;
using namespace Management::NetworkInventoryManager;

NIMNetworkNodeAllocationStoreData::NIMNetworkNodeAllocationStoreData()
    : StoreData()
{
}

NIMNetworkNodeAllocationStoreData::NIMNetworkNodeAllocationStoreData(
    NodeMapEntrySPtr & nodeAllocationMap,
    std::wstring networkId,
    std::wstring nodeName,
    Federation::NodeInstance & nodeInstance)
    :   StoreData(),
        networkId_(networkId),
        nodeName_(nodeName),
        nodeInstance_(nodeInstance)

{
    this->nodeAllocationMap_ = nodeAllocationMap;
    this->id_ = wformatString("{0}-{1}-{2}", networkId, nodeName, nodeInstance.InstanceId);
}

std::wstring const & NIMNetworkNodeAllocationStoreData::GetStoreType()
{
    return FailoverManagerStore::NetworkNodeType;
}

std::wstring const & NIMNetworkNodeAllocationStoreData::GetStoreKey() const
{
    return id_;
}

void NIMNetworkNodeAllocationStoreData::PostUpdate(Common::DateTime updateTime)
{
    lastUpdateTime_ = updateTime;
}

