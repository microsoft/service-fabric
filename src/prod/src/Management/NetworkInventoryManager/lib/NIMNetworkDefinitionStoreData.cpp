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

NIMNetworkDefinitionStoreData::NIMNetworkDefinitionStoreData()
    : StoreData(),
    sequenceNumber_(0)
{
}

NIMNetworkDefinitionStoreData::NIMNetworkDefinitionStoreData(NetworkDefinitionSPtr networkDefinition)
    : StoreData(),
    sequenceNumber_(0)
{
    this->networkDefinition_ = networkDefinition;
    this->Id = this->networkDefinition_->NetworkId;
}


std::wstring const & NIMNetworkDefinitionStoreData::GetStoreType()
{
    return FailoverManagerStore::NetworkDefinitionType;
}

std::wstring const & NIMNetworkDefinitionStoreData::GetStoreKey() const
{
    return this->id_;
}

void NIMNetworkDefinitionStoreData::PostUpdate(Common::DateTime updateTime)
{
    lastUpdateTime_ = updateTime;
}

