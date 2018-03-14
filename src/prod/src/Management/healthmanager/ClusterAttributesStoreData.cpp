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

ClusterAttributesStoreData::ClusterAttributesStoreData()
    : AttributesStoreData()
    , healthPolicyString_()
    , upgradeHealthPolicyString_()
    , applicationHealthPoliciesString_()
    , healthPolicy_()
    , upgradeHealthPolicy_()
    , attributeFlags_()
    , applicationHealthPolicies_()
{
    stateFlags_.ExpectSystemReports = false;
}

ClusterAttributesStoreData::ClusterAttributesStoreData(
    ClusterHealthId const &,
    Store::ReplicaActivityId const & activityId)
    : AttributesStoreData(activityId)
    , healthPolicyString_()
    , upgradeHealthPolicyString_()
    , applicationHealthPoliciesString_()
    , healthPolicy_()
    , upgradeHealthPolicy_()
    , attributeFlags_()
    , applicationHealthPolicies_()
{
    stateFlags_.ExpectSystemReports = false;
}

ClusterAttributesStoreData::ClusterAttributesStoreData(
    ClusterHealthId const &,
    ReportRequestContext const & context)
    : AttributesStoreData(context.ReplicaActivityId)
    , healthPolicyString_()
    , upgradeHealthPolicyString_()
    , applicationHealthPoliciesString_()
    , healthPolicy_()
    , upgradeHealthPolicy_()
    , attributeFlags_()
    , applicationHealthPolicies_()
{
    stateFlags_.ExpectSystemReports = false;
}

ClusterAttributesStoreData::ClusterAttributesStoreData(
    ClusterHealthId const &,
    ServiceModel::ClusterHealthPolicySPtr const & healthPolicy,
    ServiceModel::ClusterUpgradeHealthPolicySPtr const & upgradeHealthPolicy,
    ServiceModel::ApplicationHealthPolicyMapSPtr const & applicationHealthPolicies,
    Store::ReplicaActivityId const & activityId)
    : AttributesStoreData(activityId)
    , healthPolicyString_()
    , upgradeHealthPolicyString_()
    , healthPolicy_(healthPolicy)
    , upgradeHealthPolicy_(upgradeHealthPolicy)
    , attributeFlags_()
    , applicationHealthPolicies_(applicationHealthPolicies)
{
    stateFlags_.ExpectSystemReports = false;
    if (healthPolicy)
    {
        healthPolicyString_ = healthPolicy->ToString();
        attributeFlags_.SetHealthPolicy();
    }
    
    if (upgradeHealthPolicy)
    {
        upgradeHealthPolicyString_ = upgradeHealthPolicy->ToString();
        attributeFlags_.SetUpgradeHealthPolicy();
    }

    if (applicationHealthPolicies)
    {
        applicationHealthPoliciesString_ = applicationHealthPolicies->ToString();
        attributeFlags_.SetApplicationHealthPolicies();
    }
}

ClusterAttributesStoreData::~ClusterAttributesStoreData()
{
}

// The policy is changed through the attributes list. It comes as a string.
// Do not deserialize it into an object yet, to avoid unnecessary deserialization.
void ClusterAttributesStoreData::set_HealthPolicyString(std::wstring const & value) 
{ 
    healthPolicyString_ = value; 
    attributeFlags_.SetHealthPolicy();
    healthPolicy_.reset();
}

void ClusterAttributesStoreData::set_UpgradeHealthPolicyString(std::wstring const & value)
{
    upgradeHealthPolicyString_ = value;
    attributeFlags_.SetUpgradeHealthPolicy();
    upgradeHealthPolicy_.reset();
}

void ClusterAttributesStoreData::set_ApplicationHealthPoliciesString(std::wstring const & value)
{
    applicationHealthPoliciesString_ = value;
    attributeFlags_.SetApplicationHealthPolicies();
    applicationHealthPolicies_.reset();
}

ServiceModel::ClusterHealthPolicySPtr const & ClusterAttributesStoreData::get_HealthPolicy() const
{
    if (!healthPolicy_)
    {
        ClusterHealthPolicy policy;
        auto error = ClusterHealthPolicy::FromString(healthPolicyString_, policy);
        ASSERT_IFNOT(error.IsSuccess(), "{0}: invalid health policy", *this);
        healthPolicy_ = make_shared<ClusterHealthPolicy>(move(policy));
    }

    return healthPolicy_;
}

ServiceModel::ClusterUpgradeHealthPolicySPtr const & ClusterAttributesStoreData::get_UpgradeHealthPolicy() const
{
    if (!upgradeHealthPolicy_)
    {
        ClusterUpgradeHealthPolicy policy;
        auto error = ClusterUpgradeHealthPolicy::FromString(upgradeHealthPolicyString_, policy);
        ASSERT_IFNOT(error.IsSuccess(), "{0}: invalid upgradeHealth policy", *this);
        upgradeHealthPolicy_ = make_shared<ClusterUpgradeHealthPolicy>(move(policy));
    }

    return upgradeHealthPolicy_;
}

ServiceModel::ApplicationHealthPolicyMapSPtr const & ClusterAttributesStoreData::get_ApplicationHealthPolicies() const
{
    if (!applicationHealthPolicies_)
    {
        ApplicationHealthPolicyMap policies;
        auto error = ApplicationHealthPolicyMap::FromString(applicationHealthPoliciesString_, policies);
        ASSERT_IFNOT(error.IsSuccess(), "{0}: invalid application health policy map", *this);
        applicationHealthPolicies_ = make_shared<ApplicationHealthPolicyMap>(move(policies));
    }

    return applicationHealthPolicies_;
}

bool ClusterAttributesStoreData::ShouldUpdateAttributeInfo(ServiceModel::AttributeList const & attributeList) const
{
    for (auto it = attributeList.Attributes.begin(); it != attributeList.Attributes.end(); ++it)
    {
        if (it->first == HealthAttributeNames::ClusterHealthPolicy)
        {
            if (!attributeFlags_.IsHealthPolicySet() || it->second != this->HealthPolicyString)
            {
                return true;
            }
        }
        else if (it->first == HealthAttributeNames::ClusterUpgradeHealthPolicy)
        {
            if (!attributeFlags_.IsUpgradeHealthPolicySet() || it->second != this->UpgradeHealthPolicyString)
            {
                return true;
            }
        }
        else if (it->first == HealthAttributeNames::ApplicationHealthPolicies)
        {
            if (!attributeFlags_.IsApplicationHealthPoliciesSet() || it->second != this->ApplicationHealthPoliciesString)
            {
                return true;
            }
        }
    }

    // No new changes, no need to update attributes
    return false;
}

void ClusterAttributesStoreData::SetAttributes(
    ServiceModel::AttributeList const & attributeList)
{
    for (auto it = attributeList.Attributes.begin(); it != attributeList.Attributes.end(); ++it)
    {
        // Check whether the attributes have the expected names and map the values
        if (it->first == HealthAttributeNames::ClusterHealthPolicy)
        {
            ClusterHealthPolicy policy;
            auto error = ClusterHealthPolicy::FromString(it->second, policy);
            if (error.IsSuccess())
            {
                this->HealthPolicyString = it->second;
                healthPolicy_ = make_shared<ClusterHealthPolicy>(move(policy));
            }
            else
            {
                HMEvents::Trace->DropAttribute(
                    this->ReplicaActivityId,
                    it->first,
                    it->second);
            }
        }
        else if (it->first == HealthAttributeNames::ClusterUpgradeHealthPolicy)
        {
            ClusterUpgradeHealthPolicy policy;
            auto error = ClusterUpgradeHealthPolicy::FromString(it->second, policy);
            if (error.IsSuccess())
            {
                this->UpgradeHealthPolicyString = it->second;
                upgradeHealthPolicy_ = make_shared<ClusterUpgradeHealthPolicy>(move(policy));
            }
            else
            {
                HMEvents::Trace->DropAttribute(
                    this->ReplicaActivityId,
                    it->first,
                    it->second);
            }
        }
        else if (it->first == HealthAttributeNames::ApplicationHealthPolicies)
        {
            ApplicationHealthPolicyMap policies;
            auto error = ApplicationHealthPolicyMap::FromString(it->second, policies);
            if (error.IsSuccess())
            {
                this->ApplicationHealthPoliciesString = it->second;
                applicationHealthPolicies_ = make_shared<ApplicationHealthPolicyMap>(move(policies));
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

Common::ErrorCode ClusterAttributesStoreData::HasAttributeMatch(
    std::wstring const & attributeName,
    std::wstring const & attributeValue,
    __out bool & isMatch) const
{
    isMatch = false;
    if (attributeName == HealthAttributeNames::ClusterHealthPolicy)
    {
        if (!attributeFlags_.IsHealthPolicySet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        isMatch = (attributeValue == this->healthPolicyString_);
    }
    else if (attributeName == HealthAttributeNames::ClusterUpgradeHealthPolicy)
    {
        if (!attributeFlags_.IsUpgradeHealthPolicySet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        isMatch = (attributeValue == this->upgradeHealthPolicyString_);
    }
    else if (attributeName == HealthAttributeNames::ApplicationHealthPolicies)
    {
        if (!attributeFlags_.IsApplicationHealthPoliciesSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        isMatch = (attributeValue == this->applicationHealthPoliciesString_);
    }
    else 
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode ClusterAttributesStoreData::GetAttributeValue(
    std::wstring const & attributeName,
    __inout std::wstring & attributeValue) const
{
    if (attributeName == HealthAttributeNames::ClusterHealthPolicy)
    {
        if (!attributeFlags_.IsHealthPolicySet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        attributeValue = this->healthPolicyString_;
    }
    else if (attributeName == HealthAttributeNames::ClusterUpgradeHealthPolicy)
    {
        if (!attributeFlags_.IsUpgradeHealthPolicySet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        attributeValue = this->upgradeHealthPolicyString_;
    }
    else if (attributeName == HealthAttributeNames::ApplicationHealthPolicies)
    {
        if (!attributeFlags_.IsApplicationHealthPoliciesSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        attributeValue = this->applicationHealthPoliciesString_;
    }
    else 
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ServiceModel::AttributeList ClusterAttributesStoreData::GetEntityAttributes() const
{
    ServiceModel::AttributeList attributes;
    if (attributeFlags_.IsHealthPolicySet())
    {
        attributes.AddAttribute(HealthAttributeNames::ClusterHealthPolicy, healthPolicyString_);
    }

    if (attributeFlags_.IsUpgradeHealthPolicySet())
    {
        attributes.AddAttribute(HealthAttributeNames::ClusterUpgradeHealthPolicy, upgradeHealthPolicyString_);
    }

    if (attributeFlags_.IsApplicationHealthPoliciesSet())
    {
        attributes.AddAttribute(HealthAttributeNames::ApplicationHealthPolicies, applicationHealthPoliciesString_);
    }

    return attributes;
}

std::wstring ClusterAttributesStoreData::ConstructKey() const
{
    return Constants::StoreType_ClusterTypeString; 
}

std::wstring const & ClusterAttributesStoreData::get_Type() const 
{ 
    return Constants::StoreType_ClusterHealthAttributes; 
}

void ClusterAttributesStoreData::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("{0} {1} {2} {3} {4}", this->Key, healthPolicyString_, stateFlags_, upgradeHealthPolicyString_, applicationHealthPoliciesString_);
}

void ClusterAttributesStoreData::WriteToEtw(uint16 contextSequenceId) const
{
    HMCommonEvents::Trace->ClusterAttributesStoreDataTrace(
        contextSequenceId,
        this->Key,
        healthPolicyString_,
        stateFlags_,
        upgradeHealthPolicyString_,
        applicationHealthPoliciesString_);
}
