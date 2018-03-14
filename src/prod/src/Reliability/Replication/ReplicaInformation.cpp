// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using std::wstring;
using Common::Assert;

ReplicaInformation::ReplicaInformation(
    FABRIC_REPLICA_ID const & id, 
    ::FABRIC_REPLICA_ROLE const & role,
    wstring const & replicatorAddress,
    bool mustCatchup,
    FABRIC_SEQUENCE_NUMBER currentProgress,
    FABRIC_SEQUENCE_NUMBER catchupCapability)
    :   info_(), 
        ex1Info_(),
        replicatorAddress_(replicatorAddress),
        transportId_()
{
    info_.Id = id;
    info_.Role = role;
    info_.ReplicatorAddress = replicatorAddress_.c_str();
    info_.CurrentProgress = currentProgress;
    info_.CatchUpCapability = catchupCapability;
    info_.Reserved = &ex1Info_;
    ex1Info_.MustCatchup = mustCatchup;
    ex1Info_.Reserved = nullptr;
}

ReplicaInformation::ReplicaInformation(FABRIC_REPLICA_INFORMATION const * value)
    : info_(*value),
    ex1Info_(*(static_cast<FABRIC_REPLICA_INFORMATION_EX1*>(value->Reserved))),
    replicatorAddress_(),
    transportId_()
{
    ASSERT_IF(ex1Info_.Reserved != NULL, "FABRIC_REPLICA_INFORMATION_EX1 Reserved must be null.");

    ASSERT_IF(value->ReplicatorAddress == NULL, "ReplicatorAddress cannot be null.");
    wstring endpoint = wstring(value->ReplicatorAddress);
    wstring::size_type index =  endpoint.find(Constants::ReplicationEndpointDelimiter);
    ASSERT_IF(index == wstring::npos, "ReplicatorAddress {0} should contain delimiter {1}.", endpoint, Constants::ReplicationEndpointDelimiter);
    index =  endpoint.find(Constants::ReplicationEndpointDelimiter, index+1);
    ASSERT_IF(index == wstring::npos, "ReplicatorAddress {0} should contain delimiter {1} before incarnationId.", endpoint, Constants::ReplicationEndpointDelimiter);
    replicatorAddress_ = endpoint.substr(0, index);
    transportId_ = endpoint.substr(index + 1, endpoint.size() - (index + 1));
    info_.ReplicatorAddress = endpoint.c_str();
}

ReplicaInformation::ReplicaInformation(ReplicaInformation const & other)
    : info_(other.info_),
    ex1Info_(other.ex1Info_),
    replicatorAddress_(other.replicatorAddress_),
    transportId_(other.transportId_)
{
    info_.ReplicatorAddress = replicatorAddress_.c_str();
}

ReplicaInformation::ReplicaInformation(ReplicaInformation && other)
    : info_(other.info_),
    ex1Info_(other.ex1Info_),
    replicatorAddress_(std::move(other.replicatorAddress_)),
    transportId_(std::move(other.transportId_))
{
    info_.ReplicatorAddress = replicatorAddress_.c_str();
}

ReplicaInformation::~ReplicaInformation()
{
}

void ReplicaInformation::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    w.Write("{0}@{1}: {2}, {3} {4}", Id, ReplicatorAddress, Role, CurrentProgress, CatchUpCapability);
}

} // end namespace ReplicationComponent
} // end namespace Reliability
