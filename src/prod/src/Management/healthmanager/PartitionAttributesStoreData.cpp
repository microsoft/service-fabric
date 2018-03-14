// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Store;
using namespace Query;
using namespace Management::HealthManager;

PartitionAttributesStoreData::PartitionAttributesStoreData()
    : AttributesStoreData()
    , partitionId_()
    , serviceName_()
    , attributeFlags_()
{
}

PartitionAttributesStoreData::PartitionAttributesStoreData(
    PartitionHealthId const & partitionId,
    Store::ReplicaActivityId const & activityId)
    : AttributesStoreData(activityId)
    , partitionId_(partitionId)
    , serviceName_()
    , attributeFlags_()
{
}

PartitionAttributesStoreData::PartitionAttributesStoreData(
    PartitionHealthId const & partitionId,
    ReportRequestContext const & context)
    : AttributesStoreData(context.ReplicaActivityId)
    , partitionId_(partitionId)
    , serviceName_()
    , attributeFlags_()
{
}

PartitionAttributesStoreData::~PartitionAttributesStoreData()
{
}

bool PartitionAttributesStoreData::ShouldUpdateAttributeInfo(ServiceModel::AttributeList const & attributeList) const
{
    for (auto it = attributeList.Attributes.begin(); it != attributeList.Attributes.end(); ++it)
    {
        if (it->first == HealthAttributeNames::ServiceName)
        {
            if (!attributeFlags_.IsServiceNameSet() || it->second != this->ServiceName)
            {
                return true;
            }
        }
    }

    // No new changes, no need to update attributes
    return false;
}

void PartitionAttributesStoreData::SetAttributes(
    ServiceModel::AttributeList const & attributeList)
{
    for (auto it = attributeList.Attributes.begin(); it != attributeList.Attributes.end(); ++it)
    {
        if (it->first == HealthAttributeNames::ServiceName)
        {
            this->ServiceName = it->second;
        }
        else 
        {
            HMEvents::Trace->DropAttribute(
                this->ReplicaActivityId,
                it->first,
                it->second);
        }
    }
}

Common::ErrorCode PartitionAttributesStoreData::HasAttributeMatch(
    std::wstring const & attributeName,
    std::wstring const & attributeValue,
    __out bool & isMatch) const
{
    isMatch = false;
    if (attributeName == HealthAttributeNames::ServiceName)
    {
        if (!attributeFlags_.IsServiceNameSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        isMatch = (attributeValue == this->ServiceName);
    }
    else if (attributeName == QueryResourceProperties::Partition::PartitionId)
    {
        isMatch = (attributeValue == wformatString(partitionId_));
    }
    else 
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode PartitionAttributesStoreData::GetAttributeValue(
    std::wstring const & attributeName,
    __inout std::wstring & attributeValue) const
{
    if (attributeName == HealthAttributeNames::ServiceName)
    {
        if (!attributeFlags_.IsServiceNameSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        attributeValue = this->ServiceName;
    }
    else 
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ServiceModel::AttributeList PartitionAttributesStoreData::GetEntityAttributes() const
{
    ServiceModel::AttributeList attributes;
    if (attributeFlags_.IsServiceNameSet())
    {
        attributes.AddAttribute(HealthAttributeNames::ServiceName, serviceName_);
    }

    return attributes;
}

std::wstring PartitionAttributesStoreData::ConstructKey() const
{
    return partitionId_.ToString();
}

std::wstring const & PartitionAttributesStoreData::get_Type() const 
{ 
    return Constants::StoreType_PartitionHealthAttributes; 
}

void PartitionAttributesStoreData::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "{0}({1}:serviceName='{2}',{3})",
        this->Type,
        this->Key,
        serviceName_,
        stateFlags_);
}

void PartitionAttributesStoreData::WriteToEtw(uint16 contextSequenceId) const
{
    HMCommonEvents::Trace->PartitionAttributesStoreDataTrace(
        contextSequenceId,
        this->Type,
        this->Key,
        serviceName_,
        stateFlags_);
}
