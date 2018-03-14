// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Reliability;
using namespace Naming;

using namespace Client;

class ServiceNotificationResult::Version 
    : public ComponentRoot
    , public Api::IServiceEndpointsVersion
{
public:
    Version(GenerationNumber const & generation, int64 version)
        : generation_(generation)
        , version_(version)
    {
    }

    virtual LONG Compare(IServiceEndpointsVersion const & other) const
    {
        if (generation_ < other.GetGeneration())
        {
            return -1;
        }
        else if (generation_ > other.GetGeneration())
        {
            return +1;
        }
        else if (version_ < other.GetVersion())
        {
            return -1;
        }
        else if (version_ > other.GetVersion())
        {
            return +1;
        }
        else
        {
            return 0;
        }
    }

    virtual GenerationNumber GetGeneration() const
    {
        return generation_;
    }

    virtual int64 GetVersion() const
    {
        return version_;
    }

private:
    GenerationNumber generation_;
    int64 version_;
};

//
// ServiceNotificationResult
//

ServiceNotificationResult::ServiceNotificationResult(
    ResolvedServicePartitionSPtr const & rsp)
    : rsp_(rsp)
    , ste_()
    , generation_()
    , isMatchedPrimaryOnly_(false)
{
}

ServiceNotificationResult::ServiceNotificationResult(
    ServiceTableEntrySPtr const & ste,
    GenerationNumber const & generation)
    : rsp_()
    , ste_(ste)
    , generation_(generation)
    , isMatchedPrimaryOnly_(false)
{
}

ServiceTableEntry const & ServiceNotificationResult::get_Partition() const
{
    return (rsp_ ? rsp_->Locations : *ste_); 
}

void ServiceNotificationResult::GetNotification(
    __in ScopedHeap & heap, 
    __out FABRIC_SERVICE_NOTIFICATION & notification) const
{
    notification.ServiceName = heap.AddString(this->Partition.ServiceName);

    notification.PartitionId = this->Partition.ConsistencyUnitId.Guid.AsGUID();

    ServiceEndpointsUtility::SetEndpoints(
        this->Partition.ServiceReplicaSet,
        heap,
        notification.EndpointCount,
        &notification.Endpoints);

    if (rsp_)
    {
        notification.PartitionInfo = heap.AddItem<FABRIC_SERVICE_PARTITION_INFORMATION>().GetRawPointer();

        ServiceEndpointsUtility::SetPartitionInfo(
            this->Partition,
            rsp_->PartitionData,
            heap,
            *notification.PartitionInfo);
    }
    else
    {
        notification.PartitionInfo = NULL;
    }

    notification.Reserved = NULL;
}

IServiceEndpointsVersionPtr ServiceNotificationResult::GetVersion() const
{
    auto sPtr = rsp_
        ? make_shared<Version>(rsp_->Generation, rsp_->FMVersion)
        : make_shared<Version>(generation_, ste_->Version);

    return IServiceEndpointsVersionPtr(sPtr.get(), sPtr->CreateComponentRoot());
}

ServiceTableEntry const & ServiceNotificationResult::GetServiceTableEntry() const
{
    return this->Partition;
}
