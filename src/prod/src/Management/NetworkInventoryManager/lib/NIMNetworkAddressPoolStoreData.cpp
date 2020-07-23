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

NIMNetworkMACAddressPoolStoreData::NIMNetworkMACAddressPoolStoreData()
    : StoreData()
{
}

NIMNetworkMACAddressPoolStoreData::NIMNetworkMACAddressPoolStoreData(MACAllocationPoolSPtr & macPool,
    VSIDAllocationPoolSPtr & vsidPool,
    std::wstring networkId,
    std::wstring poolName)
    : StoreData(),
    networkId_(networkId),
    poolName_(poolName),
    vsidPool_(vsidPool),
    macPool_(macPool)
{
    this->id_ = wformatString("{0}-{1}", networkId, poolName);
}

std::wstring const & NIMNetworkMACAddressPoolStoreData::GetStoreType()
{
    return FailoverManagerStore::NetworkMACAddressPoolType;
}

std::wstring const & NIMNetworkMACAddressPoolStoreData::GetStoreKey() const
{
    return this->id_;
}

void NIMNetworkMACAddressPoolStoreData::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.WriteLine("NIMNetworkMACAddressPoolStoreData: Network: [{0}], id: [{1}]", 
        networkId_, id_);
}

void NIMNetworkMACAddressPoolStoreData::PostUpdate(Common::DateTime updateTime)
{
    lastUpdateTime_ = updateTime;
}


NIMNetworkIPv4AddressPoolStoreData::NIMNetworkIPv4AddressPoolStoreData()
    : StoreData()
{
}

NIMNetworkIPv4AddressPoolStoreData::NIMNetworkIPv4AddressPoolStoreData(IPv4AllocationPoolSPtr & ipPool,
    std::wstring networkId,
    std::wstring poolName)
    : StoreData(),
    networkId_(networkId),
    poolName_(poolName)
{
    this->ipPool_ = ipPool;
    this->id_ = wformatString("{0}-{1}", networkId, poolName);
}

std::wstring const & NIMNetworkIPv4AddressPoolStoreData::GetStoreType()
{
    return FailoverManagerStore::NetworkIPv4AddressPoolType;
}

std::wstring const & NIMNetworkIPv4AddressPoolStoreData::GetStoreKey() const
{
    return this->id_;
}

void NIMNetworkIPv4AddressPoolStoreData::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.WriteLine("NIMNetworkIPv4AddressPoolStoreData: Network: [{0}], id: [{1}]", 
        networkId_, id_);
}

void NIMNetworkIPv4AddressPoolStoreData::PostUpdate(Common::DateTime updateTime)
{
    lastUpdateTime_ = updateTime;
}