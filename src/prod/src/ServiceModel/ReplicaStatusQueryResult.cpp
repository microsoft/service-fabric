// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ReplicaStatusQueryResult::ReplicaStatusQueryResult(FABRIC_SERVICE_REPLICA_KIND kind)
    : kind_(kind)
{
}

ReplicaStatusQueryResult::ReplicaStatusQueryResult()
    : kind_(FABRIC_SERVICE_REPLICA_KIND_INVALID)
{

}

ReplicaStatusQueryResult::ReplicaStatusQueryResult(ReplicaStatusQueryResult && other)
    : kind_(other.kind_)
{
}

ReplicaStatusQueryResult const & ReplicaStatusQueryResult::operator = (ReplicaStatusQueryResult && other)
{
    if (this != &other)
    {
        this->kind_ = other.kind_;
    }

    return *this;
}

ReplicaStatusQueryResultSPtr ReplicaStatusQueryResult::CreateSPtr(FABRIC_SERVICE_REPLICA_KIND kind)
{
    switch (kind)
    {
    case FABRIC_SERVICE_REPLICA_KIND_KEY_VALUE_STORE:
        return move(make_shared<KeyValueStoreQueryResult>());
    default:
        return move(make_shared<ReplicaStatusQueryResult>());
    }
}

Serialization::IFabricSerializable * ReplicaStatusQueryResult::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr || 
        typeInformation.length != sizeof(FABRIC_SERVICE_REPLICA_KIND))                                                   
    {                                                                                                                                           
        return nullptr;                                                                                                                         
    }                                                                                                                                           
                                                                                                                                                        
    FABRIC_SERVICE_REPLICA_KIND kind = *(reinterpret_cast<FABRIC_SERVICE_REPLICA_KIND const *>(typeInformation.buffer));
     
    return CreateNew(kind);
}

ReplicaStatusQueryResult * ReplicaStatusQueryResult::CreateNew(FABRIC_SERVICE_REPLICA_KIND kind)      
{ 
    switch (kind)
    {
    case FABRIC_SERVICE_REPLICA_KIND_KEY_VALUE_STORE:
        return new KeyValueStoreQueryResult();
    default:
        return new ReplicaStatusQueryResult();
    }
}

NTSTATUS ReplicaStatusQueryResult::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&kind_);
    typeInformation.length = sizeof(kind_);   
    return STATUS_SUCCESS;
}

void ReplicaStatusQueryResult::ToPublicApi(
    __in ScopedHeap & heap, 
    __out FABRIC_REPLICA_STATUS_QUERY_RESULT & publicResult) const
{
    UNREFERENCED_PARAMETER(heap);
    UNREFERENCED_PARAMETER(publicResult);
    publicResult.Kind = kind_;
    publicResult.Value = nullptr;
}

ErrorCode ReplicaStatusQueryResult::FromPublicApi(
    __in FABRIC_REPLICA_STATUS_QUERY_RESULT const & publicResult)
{
    UNREFERENCED_PARAMETER(publicResult);
    this->kind_ = FABRIC_SERVICE_REPLICA_KIND_INVALID;
    return ErrorCodeValue::Success;
}

void ReplicaStatusQueryResult::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ReplicaStatusQueryResult::ToString() const
{
    return wformatString("Kind = {0}", static_cast<int>(kind_));
}
