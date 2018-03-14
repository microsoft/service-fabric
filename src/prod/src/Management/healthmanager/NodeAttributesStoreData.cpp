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

NodeAttributesStoreData::NodeAttributesStoreData()
    : AttributesStoreData()
    , nodeId_()
    , nodeInstanceId_(FABRIC_INVALID_NODE_INSTANCE_ID)
    , nodeName_()
    , ipAddressOrFqdn_()
    , upgradeDomain_()
    , faultDomain_()
    , attributeFlags_()
{
}

NodeAttributesStoreData::NodeAttributesStoreData(
    NodeHealthId const & nodeId,
    Store::ReplicaActivityId const & activityId)
    : AttributesStoreData(activityId)
    , nodeId_(nodeId)
    , nodeInstanceId_(FABRIC_INVALID_NODE_INSTANCE_ID)
    , nodeName_()
    , ipAddressOrFqdn_()
    , upgradeDomain_()
    , faultDomain_()
    , attributeFlags_()
{
}

NodeAttributesStoreData::NodeAttributesStoreData(
    NodeHealthId const & nodeId,
    ReportRequestContext const & context)
    : AttributesStoreData(context.ReplicaActivityId)
    , nodeId_(nodeId)
    , nodeInstanceId_(FABRIC_INVALID_NODE_INSTANCE_ID)
    , nodeName_()
    , ipAddressOrFqdn_()
    , upgradeDomain_()
    , faultDomain_()
    , attributeFlags_()
{
    nodeInstanceId_ = static_cast<FABRIC_NODE_INSTANCE_ID>(context.Report.EntityInformation->EntityInstance);
}

NodeAttributesStoreData::~NodeAttributesStoreData()
{
}

bool NodeAttributesStoreData::ShouldUpdateAttributeInfo(ServiceModel::AttributeList const & attributeList) const
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
        else if (it->first == HealthAttributeNames::IpAddressOrFqdn)
        {
            if (!attributeFlags_.IsIpAddressOrFqdnSet() || it->second != this->IpAddressOrFqdn)
            {
                return true;
            }
        }
        else if (it->first == HealthAttributeNames::UpgradeDomain)
        {
            if (!attributeFlags_.IsUpgradeDomainSet() || it->second != this->UpgradeDomain)
            {
                return true;
            }
        }
        else if (it->first == HealthAttributeNames::FaultDomain)
        {
            if (!attributeFlags_.IsFaultDomainSet() || it->second != this->FaultDomain)
            {
                return true;
            }
        }
    }

    // No new changes, no need to update attributes
    return false;
}

void NodeAttributesStoreData::SetAttributes(
    ServiceModel::AttributeList const & attributeList)
{
    for (auto it = attributeList.Attributes.begin(); it != attributeList.Attributes.end(); ++it)
    {
        // Check whether the attributes have the expected names and map the values
        if (it->first == HealthAttributeNames::NodeName)
        {
            this->NodeName = it->second;
        }
        else if (it->first == HealthAttributeNames::IpAddressOrFqdn)
        {
            this->IpAddressOrFqdn = it->second;
        }
        else if (it->first == HealthAttributeNames::UpgradeDomain)
        {
            this->UpgradeDomain = it->second;
        }
        else if (it->first == HealthAttributeNames::FaultDomain)
        {
            this->FaultDomain = it->second;
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

Common::ErrorCode NodeAttributesStoreData::HasAttributeMatch(
    std::wstring const & attributeName,
    std::wstring const & attributeValue,
    __out bool & isMatch) const
{
    isMatch = false;
    if (attributeName == HealthAttributeNames::NodeName)
    {
        if (!attributeFlags_.IsNodeNameSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        isMatch = (attributeValue == this->NodeName);
    }
    else if (attributeName == HealthAttributeNames::IpAddressOrFqdn)
    {
        if (!attributeFlags_.IsIpAddressOrFqdnSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        isMatch = (attributeValue == this->IpAddressOrFqdn);
    }
    else if (attributeName == HealthAttributeNames::UpgradeDomain)
    {
        if (!attributeFlags_.IsUpgradeDomainSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        isMatch = (attributeValue == this->UpgradeDomain);
    }
    else if (attributeName == HealthAttributeNames::FaultDomain)
    {
        if (!attributeFlags_.IsFaultDomainSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        isMatch = (attributeValue == this->FaultDomain);
    }
    else 
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode NodeAttributesStoreData::GetAttributeValue(
    std::wstring const & attributeName,
    __inout std::wstring & attributeValue) const
{
    if (attributeName == HealthAttributeNames::NodeName)
    {
        if (!attributeFlags_.IsNodeNameSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        attributeValue = this->NodeName;
    }
    else if (attributeName == HealthAttributeNames::IpAddressOrFqdn)
    {
        if (!attributeFlags_.IsIpAddressOrFqdnSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        attributeValue = this->IpAddressOrFqdn;
    }
    else if (attributeName == HealthAttributeNames::UpgradeDomain)
    {
        if (!attributeFlags_.IsUpgradeDomainSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        attributeValue = this->UpgradeDomain;
    }
    else if (attributeName == HealthAttributeNames::FaultDomain)
    {
        if (!attributeFlags_.IsFaultDomainSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        attributeValue = this->FaultDomain;
    }
    else 
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ServiceModel::AttributeList NodeAttributesStoreData::GetEntityAttributes() const
{
    ServiceModel::AttributeList attributes;

    // NodeName will be set in NodeEntityHealthInformation so no need to add to attribute list.

    if (attributeFlags_.IsIpAddressOrFqdnSet())
    {
        attributes.AddAttribute(HealthAttributeNames::IpAddressOrFqdn, ipAddressOrFqdn_);
    }

    if (attributeFlags_.IsUpgradeDomainSet())
    {
        attributes.AddAttribute(HealthAttributeNames::UpgradeDomain, upgradeDomain_);
    }

    if (attributeFlags_.IsFaultDomainSet())
    {
        attributes.AddAttribute(HealthAttributeNames::FaultDomain, faultDomain_);
    }

    return attributes;
}

std::wstring NodeAttributesStoreData::ConstructKey() const
{
    return nodeId_.ToString();
}

std::wstring const & NodeAttributesStoreData::get_Type() const 
{ 
    return Constants::StoreType_NodeHealthAttributes; 
}

void NodeAttributesStoreData::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "{0}({1}:name='{2}',addr='{3}',UD='{4}',FD='{5}',instanceId={6},{7})",
        this->Type,
        this->Key,
        nodeName_ ,
        ipAddressOrFqdn_,
        upgradeDomain_,
        faultDomain_,
        nodeInstanceId_,
        stateFlags_);
}

void NodeAttributesStoreData::WriteToEtw(uint16 contextSequenceId) const
{
    HMCommonEvents::Trace->NodeAttributesStoreDataTrace(
        contextSequenceId,
        this->Type,
        this->Key,
        nodeName_,
        ipAddressOrFqdn_,
        upgradeDomain_,
        faultDomain_,
        nodeInstanceId_,
        stateFlags_);
}
