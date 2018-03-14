// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Store;
using namespace Management::HealthManager;

ServiceAttributesStoreData::ServiceAttributesStoreData()
    : AttributesStoreData()
    , serviceId_()
    , instanceId_(FABRIC_INVALID_INSTANCE_ID)
    , applicationName_()
    , serviceTypeName_()
    , attributeFlags_()
{
}

ServiceAttributesStoreData::ServiceAttributesStoreData(
    ServiceHealthId const & serviceId,
    Store::ReplicaActivityId const & activityId)
    : AttributesStoreData(activityId)
    , serviceId_(serviceId)
    , instanceId_(FABRIC_INVALID_INSTANCE_ID)
    , applicationName_()
    , serviceTypeName_()
    , attributeFlags_()
{
}

ServiceAttributesStoreData::ServiceAttributesStoreData(
    ServiceHealthId const & serviceId,
    ReportRequestContext const & context)
    : AttributesStoreData(context.ReplicaActivityId)
    , serviceId_(serviceId)
    , instanceId_(FABRIC_INVALID_INSTANCE_ID)
    , applicationName_()
    , serviceTypeName_()
    , attributeFlags_()
{
    instanceId_ = context.Report.EntityInformation->EntityInstance;
}

ServiceAttributesStoreData::~ServiceAttributesStoreData()
{
}

bool ServiceAttributesStoreData::ShouldUpdateAttributeInfo(ServiceModel::AttributeList const & attributeList) const
{
    for (auto it = attributeList.Attributes.begin(); it != attributeList.Attributes.end(); ++it)
    {
        if (it->first == HealthAttributeNames::ApplicationName)
        {
            if (!attributeFlags_.IsApplicationNameSet() || it->second != this->ApplicationName)
            {
                return true;
            }
        }
        else if (it->first == HealthAttributeNames::ServiceTypeName)
        {
            if (!attributeFlags_.IsServiceTypeNameSet() || it->second != this->ServiceTypeName)
            {
                return true;
            }
        }
    }

    // No new changes, no need to update attributes
    return false;
}

void ServiceAttributesStoreData::SetAttributes(
    ServiceModel::AttributeList const & attributeList)
{
    for (auto it = attributeList.Attributes.begin(); it != attributeList.Attributes.end(); ++it)
    {
        // Check whether the attributes have the expected names and map the values
        if (it->first == HealthAttributeNames::ApplicationName)
        {
            this->ApplicationName = it->second;
        }
        else if (it->first == HealthAttributeNames::ServiceTypeName)
        {
            this->ServiceTypeName = it->second;
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

Common::ErrorCode ServiceAttributesStoreData::HasAttributeMatch(
    std::wstring const & attributeName,
    std::wstring const & attributeValue,
    __out bool & isMatch) const
{
    isMatch = false;
    if (attributeName == HealthAttributeNames::ApplicationName)
    {
        if (!attributeFlags_.IsApplicationNameSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        isMatch = (attributeValue == this->ApplicationName);
    }
    else if (attributeName == HealthAttributeNames::ServiceTypeName)
    {
        if (!attributeFlags_.IsServiceTypeNameSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        isMatch = (attributeValue == this->ServiceTypeName);
    }
    else if (attributeName == HealthAttributeNames::ServiceName)
    {
        isMatch = (attributeValue == serviceId_.ServiceName);
    }
    else 
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode ServiceAttributesStoreData::GetAttributeValue(
    std::wstring const & attributeName,
    __inout std::wstring & attributeValue) const
{
    if (attributeName == HealthAttributeNames::ApplicationName)
    {
        if (!attributeFlags_.IsApplicationNameSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        attributeValue = this->ApplicationName;
    }
    else if (attributeName == HealthAttributeNames::ServiceTypeName)
    {
        if (!attributeFlags_.IsServiceTypeNameSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        attributeValue = this->ServiceTypeName;
    }
    else 
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ServiceModel::AttributeList ServiceAttributesStoreData::GetEntityAttributes() const
{
    ServiceModel::AttributeList attributes;
    if (attributeFlags_.IsApplicationNameSet())
    {
        attributes.AddAttribute(HealthAttributeNames::ApplicationName, applicationName_);
    }

    if (attributeFlags_.IsServiceTypeNameSet())
    {
        attributes.AddAttribute(HealthAttributeNames::ServiceTypeName, serviceTypeName_);
    }

    return attributes;
}

std::wstring ServiceAttributesStoreData::ConstructKey() const
{
    return serviceId_.ServiceName;
}

std::wstring const & ServiceAttributesStoreData::get_Type() const 
{ 
    return Constants::StoreType_ServiceHealthAttributes; 
}

void ServiceAttributesStoreData::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "{0}({1}:instanceId=={2},appName={3},serviceTypeName={4},{5})",
        this->Type,
        this->Key,
        instanceId_,
        applicationName_,
        serviceTypeName_,
        stateFlags_);
}

void ServiceAttributesStoreData::WriteToEtw(uint16 contextSequenceId) const
{
    HMCommonEvents::Trace->ServiceAttributesStoreDataTrace(
        contextSequenceId,
        this->Type,
        this->Key,
        instanceId_,
        applicationName_,
        serviceTypeName_,
        stateFlags_);
}
