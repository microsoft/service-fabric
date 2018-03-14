// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

UnplacedReplicaInformationQueryResult::UnplacedReplicaInformationQueryResult()
{
}

UnplacedReplicaInformationQueryResult::UnplacedReplicaInformationQueryResult(
    std::wstring serviceName,
    Common::Guid partitionId,
    std::vector<std::wstring> && unplacedReplicaDetails)
    : serviceName_(move(serviceName))
    , partitionId_(partitionId)
    , unplacedReplicaDetails_(move(unplacedReplicaDetails))
{
}


UnplacedReplicaInformationQueryResult::UnplacedReplicaInformationQueryResult(UnplacedReplicaInformationQueryResult const & other) :
    serviceName_(other.serviceName_),
    partitionId_(other.partitionId_),
    unplacedReplicaDetails_(other.unplacedReplicaDetails_)
{
}

UnplacedReplicaInformationQueryResult::UnplacedReplicaInformationQueryResult(UnplacedReplicaInformationQueryResult && other) :
    serviceName_(move(other.serviceName_)),
    partitionId_(move(other.partitionId_)),
    unplacedReplicaDetails_(move(other.unplacedReplicaDetails_))
{
}

UnplacedReplicaInformationQueryResult const & UnplacedReplicaInformationQueryResult::operator = (UnplacedReplicaInformationQueryResult const & other)
{
    if (&other != this)
    {
        serviceName_ = other.serviceName_;
        partitionId_ = other.partitionId_;
        unplacedReplicaDetails_ = other.unplacedReplicaDetails_;
    }

    return *this;
}

UnplacedReplicaInformationQueryResult const & UnplacedReplicaInformationQueryResult::operator = (UnplacedReplicaInformationQueryResult && other)
{
    if (&other != this)
    {
        serviceName_ = move(other.serviceName_);
        partitionId_ = move(other.partitionId_);
        unplacedReplicaDetails_ = move(other.unplacedReplicaDetails_);
    }

    return *this;
}

void UnplacedReplicaInformationQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_UNPLACED_REPLICA_INFORMATION & queryResult) const
{

    queryResult.ServiceName = heap.AddString(serviceName_);
    queryResult.PartitionId = partitionId_.AsGUID();

    auto reasonsList = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, unplacedReplicaDetails_, reasonsList);
    queryResult.UnplacedReplicaReasons = reasonsList.GetRawPointer();


}

ErrorCode UnplacedReplicaInformationQueryResult::FromPublicApi(__in FABRIC_UNPLACED_REPLICA_INFORMATION const& queryResult)
{
    ErrorCode error = ErrorCode::Success();
    serviceName_ = queryResult.ServiceName;
    partitionId_ = Guid(queryResult.PartitionId);
    unplacedReplicaDetails_.clear();

    auto unplacedReplicaInformation = queryResult.UnplacedReplicaReasons;
    auto reasonItem =(LPCWSTR*) unplacedReplicaInformation->Items;

    for(uint i=0;i!=unplacedReplicaInformation->Count;++i)
    {
        std::wstring reason;
        auto hr = StringUtility::LpcwstrToWstring((*(reasonItem+i)), false, reason);

        unplacedReplicaDetails_.push_back(reason);

        if (FAILED(hr))
        {
            return ErrorCode::FromHResult(hr);
        }
    }

    return ErrorCode::Success();
}

void UnplacedReplicaInformationQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring UnplacedReplicaInformationQueryResult::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<UnplacedReplicaInformationQueryResult&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}
