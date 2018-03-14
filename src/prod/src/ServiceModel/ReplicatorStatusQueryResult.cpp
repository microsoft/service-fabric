// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ReplicatorStatusQueryResult::ReplicatorStatusQueryResult(FABRIC_REPLICA_ROLE role)
    : role_(role)
{
}

ReplicatorStatusQueryResult::ReplicatorStatusQueryResult()
    : role_(FABRIC_REPLICA_ROLE_UNKNOWN)
{

}

ReplicatorStatusQueryResult::ReplicatorStatusQueryResult(ReplicatorStatusQueryResult && other)
    : role_(other.role_)
{
}

ReplicatorStatusQueryResult const & ReplicatorStatusQueryResult::operator = (ReplicatorStatusQueryResult && other)
{
    if (this != &other)
    {
        this->role_ = other.role_;
    }

    return *this;
}

ReplicatorStatusQueryResultSPtr ReplicatorStatusQueryResult::CreateSPtr(FABRIC_REPLICA_ROLE role)
{
    switch (role)
    {
    case FABRIC_REPLICA_ROLE_PRIMARY:
        return move(make_shared<PrimaryReplicatorStatusQueryResult>());
    case FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY:
    case FABRIC_REPLICA_ROLE_IDLE_SECONDARY:
        return move(make_shared<SecondaryReplicatorStatusQueryResult>());
    default:
        return move(make_shared<ReplicatorStatusQueryResult>());
    }
}

Serialization::IFabricSerializable * ReplicatorStatusQueryResult::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr || 
        typeInformation.length != sizeof(FABRIC_REPLICA_ROLE))                                                   
    {                                                                                                                                           
        return nullptr;                                                                                                                         
    }                                                                                                                                           
                                                                                                                                                        
    FABRIC_REPLICA_ROLE role = *(reinterpret_cast<FABRIC_REPLICA_ROLE const *>(typeInformation.buffer));
     
    return CreateNew(role);
}

ReplicatorStatusQueryResult * ReplicatorStatusQueryResult::CreateNew(FABRIC_REPLICA_ROLE role)      
{ 
    switch (role)
    {
    case FABRIC_REPLICA_ROLE_PRIMARY:
        return new PrimaryReplicatorStatusQueryResult();
    case FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY:
    case FABRIC_REPLICA_ROLE_IDLE_SECONDARY:
        return new SecondaryReplicatorStatusQueryResult();
    default:
        return new ReplicatorStatusQueryResult();
    }
}

NTSTATUS ReplicatorStatusQueryResult::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&role_);
    typeInformation.length = sizeof(role_);   
    return STATUS_SUCCESS;
}

ErrorCode ReplicatorStatusQueryResult::ToPublicApi(
    __in ScopedHeap & heap, 
    __out FABRIC_REPLICATOR_STATUS_QUERY_RESULT & publicResult) const
{
    UNREFERENCED_PARAMETER(heap);
    UNREFERENCED_PARAMETER(publicResult);
    publicResult.Role = role_;
    publicResult.Value = nullptr;
    return ErrorCodeValue::Success;
}

ErrorCode ReplicatorStatusQueryResult::FromPublicApi(
    __in FABRIC_REPLICATOR_STATUS_QUERY_RESULT const & publicResult)
{
    UNREFERENCED_PARAMETER(publicResult);
    this->role_ = FABRIC_REPLICA_ROLE_UNKNOWN;
    return ErrorCodeValue::Success;
}

void ReplicatorStatusQueryResult::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ReplicatorStatusQueryResult::ToString() const
{
    return wformatString("Role = {0}", role_);
}
