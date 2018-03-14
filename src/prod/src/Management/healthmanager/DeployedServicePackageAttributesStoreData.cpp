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

DeployedServicePackageAttributesStoreData::DeployedServicePackageAttributesStoreData()
    : AttributesStoreData()
    , deployedServicePackageId_()
    , instanceId_(FABRIC_INVALID_INSTANCE_ID)
    , nodeName_()
    , nodeInstanceId_(FABRIC_INVALID_NODE_INSTANCE_ID)
    , attributeFlags_()
{
}

DeployedServicePackageAttributesStoreData::DeployedServicePackageAttributesStoreData(
    DeployedServicePackageHealthId const & deployedServicePackageId,
    Store::ReplicaActivityId const & activityId)
    : AttributesStoreData(activityId)
    , deployedServicePackageId_(deployedServicePackageId)
    , instanceId_(FABRIC_INVALID_INSTANCE_ID)
    , nodeName_()
    , nodeInstanceId_(FABRIC_INVALID_NODE_INSTANCE_ID)
    , attributeFlags_()
{
}

DeployedServicePackageAttributesStoreData::DeployedServicePackageAttributesStoreData(
    DeployedServicePackageHealthId const & deployedServicePackageId,
    ReportRequestContext const & context)
    : AttributesStoreData(context.ReplicaActivityId)
    , deployedServicePackageId_(deployedServicePackageId)
    , instanceId_(FABRIC_INVALID_INSTANCE_ID)
    , nodeName_()
    , nodeInstanceId_(FABRIC_INVALID_NODE_INSTANCE_ID)
    , attributeFlags_()
{
    instanceId_ = context.Report.EntityInformation->EntityInstance;
}

DeployedServicePackageAttributesStoreData::~DeployedServicePackageAttributesStoreData()
{
}

bool DeployedServicePackageAttributesStoreData::ShouldUpdateAttributeInfo(ServiceModel::AttributeList const & attributeList) const
{
    for (auto it = attributeList.Attributes.begin(); it != attributeList.Attributes.end(); ++it)
    {
        if (it->first == HealthAttributeNames::NodeName)
        {
            if (!attributeFlags_.IsNodeNameSet() || it->second != this->NodeName)
            {
                return true;
            }
        }
        else if (it->first == HealthAttributeNames::NodeInstanceId)
        {
            if (!attributeFlags_.IsNodeInstanceIdSet() || it->second != wformatString(this->NodeInstanceId))
            {
                return true;
            }
        }
    }

    // No new changes, no need to update attributes
    return false;
}

void DeployedServicePackageAttributesStoreData::SetAttributes(
    ServiceModel::AttributeList const & attributeList)
{
    for (auto it = attributeList.Attributes.begin(); it != attributeList.Attributes.end(); ++it)
    {
        // Check whether the attributes have the expected names and map the values
        if (it->first == HealthAttributeNames::NodeName)
        {
            this->NodeName = it->second;
        }
        else if (it->first == HealthAttributeNames::NodeInstanceId)
        {
            FABRIC_NODE_INSTANCE_ID nodeInstanceId;
            if (StringUtility::TryFromWString<FABRIC_NODE_INSTANCE_ID>(it->second, nodeInstanceId))
            {
                this->NodeInstanceId = nodeInstanceId;
            }
            else
            {
                 HMEvents::Trace->DropAttribute(
                    this->ReplicaActivityId,
                    it->first,
                    it->second);
            }
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

Common::ErrorCode DeployedServicePackageAttributesStoreData::HasAttributeMatch(
    std::wstring const & attributeName,
    std::wstring const & attributeValue,
    __out bool & isMatch) const
{
    isMatch = false;
    if (attributeName == HealthAttributeNames::ApplicationName)
    {
        isMatch = (attributeValue == deployedServicePackageId_.ApplicationName);
    }
    else if (attributeName == HealthAttributeNames::ServiceManifestName)
    {
        isMatch = (attributeValue == deployedServicePackageId_.ServiceManifestName);
    }
    else if (attributeName == HealthAttributeNames::ServicePackageActivationId)
    {
        isMatch = (attributeValue == deployedServicePackageId_.ServicePackageActivationId);
    }
    else if (attributeName == HealthAttributeNames::NodeId)
    {
        isMatch = (attributeValue == wformatString(deployedServicePackageId_.NodeId));
    }
    else if (attributeName == HealthAttributeNames::NodeName)
    {
        if (!attributeFlags_.IsNodeNameSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        isMatch = (attributeValue == this->NodeName);
    }
    else if (attributeName == HealthAttributeNames::NodeInstanceId)
    {
        if (!attributeFlags_.IsNodeInstanceIdSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        isMatch = (attributeValue == wformatString(this->NodeInstanceId));
    }
    else 
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode DeployedServicePackageAttributesStoreData::GetAttributeValue(
    std::wstring const & attributeName,
    __inout std::wstring & attributeValue) const
{
    if (attributeName == HealthAttributeNames::ApplicationName)
    {
        attributeValue = deployedServicePackageId_.ApplicationName;
    }
    else if (attributeName == HealthAttributeNames::ServiceManifestName)
    {
        attributeValue = deployedServicePackageId_.ServiceManifestName;
    }
    else if (attributeName == HealthAttributeNames::ServicePackageActivationId)
    {
        attributeValue = deployedServicePackageId_.ServicePackageActivationId;
    }
    else if (attributeName == HealthAttributeNames::NodeId)
    {
        attributeValue = wformatString(deployedServicePackageId_.NodeId);
    }
    else if (attributeName == HealthAttributeNames::NodeName)
    {
        if (!attributeFlags_.IsNodeNameSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        attributeValue = this->NodeName;
    }
    else if (attributeName == HealthAttributeNames::NodeInstanceId)
    {
        if (!attributeFlags_.IsNodeInstanceIdSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        attributeValue = wformatString(this->NodeInstanceId);
    }
    else 
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ServiceModel::AttributeList DeployedServicePackageAttributesStoreData::GetEntityAttributes() const
{
    ServiceModel::AttributeList attributes;

    if (attributeFlags_.IsNodeInstanceIdSet())
    {
        attributes.AddAttribute(HealthAttributeNames::NodeInstanceId, wformatString(nodeInstanceId_));
    }

    return attributes;
}

std::wstring DeployedServicePackageAttributesStoreData::ConstructKey() const
{
    return deployedServicePackageId_.ToString();
}

std::wstring const & DeployedServicePackageAttributesStoreData::get_Type() const 
{ 
    return Constants::StoreType_DeployedServicePackageHealthAttributes; 
}

void DeployedServicePackageAttributesStoreData::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "{0}({1}:nodeName='{2}',instanceId={3},nodeInstanceId={4},{5})",
        this->Type,
        this->Key,
        nodeName_,
        instanceId_,
        nodeInstanceId_,
        stateFlags_);
}

void DeployedServicePackageAttributesStoreData::WriteToEtw(uint16 contextSequenceId) const
{
    HMCommonEvents::Trace->DeployedServicePackageAttributesStoreDataTrace(
        contextSequenceId,
        this->Type,
        this->Key,
        nodeName_,
        instanceId_,
        nodeInstanceId_,
        stateFlags_);
}
