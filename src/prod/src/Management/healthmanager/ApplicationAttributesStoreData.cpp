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

ApplicationAttributesStoreData::ApplicationAttributesStoreData()
    : AttributesStoreData()
    , applicationId_()
    , instanceId_(FABRIC_INVALID_INSTANCE_ID)
    , appHealthPolicyString_()
    , appHealthPolicy_()
    , attributeFlags_()
    , applicationTypeName_()
    , applicationDefinitionKind_()
{
}

ApplicationAttributesStoreData::ApplicationAttributesStoreData(
    ApplicationHealthId const & applicationId,
    Store::ReplicaActivityId const & activityId)
    : AttributesStoreData(activityId)
    , applicationId_(applicationId)
    , instanceId_(FABRIC_INVALID_INSTANCE_ID)
    , appHealthPolicyString_()
    , appHealthPolicy_()
    , attributeFlags_()
    , applicationTypeName_()
    , applicationDefinitionKind_()
{
}

ApplicationAttributesStoreData::ApplicationAttributesStoreData(
    ApplicationHealthId const & applicationId,
    ReportRequestContext const & context)
    : AttributesStoreData(context.ReplicaActivityId)
    , applicationId_(applicationId)
    , instanceId_(FABRIC_INVALID_INSTANCE_ID)
    , appHealthPolicyString_()
    , appHealthPolicy_()
    , attributeFlags_()
    , applicationTypeName_()
    , applicationDefinitionKind_()
{
    instanceId_ = context.Report.EntityInformation->EntityInstance;
}

ApplicationAttributesStoreData::~ApplicationAttributesStoreData()
{
}

// Since multiple fields are being changed without a lock, the user of the attributes
// must make sure that the attributes are not changed while reads are in progress.
// This should be done by setting the staleness flag when updating the attributes:
// mark attributes as stale, create new attributes with new health policy, replace old attributes with new attributes
void ApplicationAttributesStoreData::SetAppHealthPolicy(ServiceModel::ApplicationHealthPolicy && policy)
{ 
    appHealthPolicyString_ = policy.ToString(); 
    attributeFlags_.SetAppHealthPolicy();
    appHealthPolicy_ = make_shared<ApplicationHealthPolicy>(move(policy));
}

void ApplicationAttributesStoreData::OnCompleteLoadFromStore()
{
    if (attributeFlags_.IsAppHealthPolicySet())
    {
        ApplicationHealthPolicy policy;
        auto error = ApplicationHealthPolicy::FromString(appHealthPolicyString_, policy);
        ASSERT_IFNOT(error.IsSuccess(), "{0}: error parsing app health policy: {1}", this->ReplicaActivityId.TraceId, appHealthPolicyString_);
        appHealthPolicy_ = make_shared<ApplicationHealthPolicy>(move(policy));
    }
}

Common::ErrorCode ApplicationAttributesStoreData::GetAppHealthPolicy(__inout ServiceModel::ApplicationHealthPolicySPtr & policy) const
{
    if (!this->AttributeSetFlags.IsAppHealthPolicySet())
    {
        HMEvents::Trace->NoHealthPolicy(applicationId_.ApplicationName, this->ReplicaActivityId.TraceId, *this);
        return ErrorCode(ErrorCodeValue::NotFound);
    }

    policy = appHealthPolicy_;
    ASSERT_IFNOT(policy, "GetAppHealthPolicy returned null policy: {0}", *this);

    return ErrorCode::Success();
}

bool ApplicationAttributesStoreData::ShouldUpdateAttributeInfo(ServiceModel::AttributeList const & attributeList) const
{
    for (auto it = attributeList.Attributes.begin(); it != attributeList.Attributes.end(); ++it)
    {
        if (it->first == HealthAttributeNames::ApplicationHealthPolicy)
        {
            if (!attributeFlags_.IsAppHealthPolicySet() || it->second != this->AppHealthPolicyString)
            {
                return true;
            }
        }
        else if (it->first == HealthAttributeNames::ApplicationTypeName)
        {
            if (!attributeFlags_.IsApplicationTypeNameSet() || it->second != this->ApplicationTypeName)
            {
                return true;
            }
        }
        else if (it->first == HealthAttributeNames::ApplicationDefinitionKind)
        {
            if (!attributeFlags_.IsApplicationDefinitionKindSet() || it->second != this->ApplicationDefinitionKind)
            {
                return true;
            }
        }
    }

    // No new changes, no need to update attributes
    return false;
}

void ApplicationAttributesStoreData::SetAttributes(
    ServiceModel::AttributeList const & attributeList)
{
    for (auto it = attributeList.Attributes.begin(); it != attributeList.Attributes.end(); ++it)
    {
        // Check whether the attributes have the expected names and map the values
        if (it->first == HealthAttributeNames::ApplicationHealthPolicy)
        {
            ApplicationHealthPolicy policy;
            auto error = ApplicationHealthPolicy::FromString(it->second, policy);
            if (error.IsSuccess())
            {
                appHealthPolicyString_ = it->second;
                attributeFlags_.SetAppHealthPolicy();
                appHealthPolicy_ = make_shared<ApplicationHealthPolicy>(move(policy));
            }
            else
            {
                HMEvents::Trace->DropAttribute(
                    this->ReplicaActivityId,
                    it->first,
                    it->second);
            }
        }
        else if (it->first == HealthAttributeNames::ApplicationTypeName)
        {
            this->ApplicationTypeName = it->second;
        }
        else if (it->first == HealthAttributeNames::ApplicationDefinitionKind)
        {
            applicationDefinitionKind_ = it->second;
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

Common::ErrorCode ApplicationAttributesStoreData::HasAttributeMatch(
    std::wstring const & attributeName,
    std::wstring const & attributeValue,
    __out bool & isMatch) const
{
    isMatch = false;
    if (attributeName == HealthAttributeNames::ApplicationHealthPolicy)
    {
        if (!attributeFlags_.IsAppHealthPolicySet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        isMatch = (attributeValue == appHealthPolicyString_);
    }
    else if (attributeName == HealthAttributeNames::ApplicationName)
    {
        isMatch = (applicationId_.ApplicationName == attributeValue);
    }
    else if (attributeName == HealthAttributeNames::ApplicationTypeName)
    {
        isMatch = (attributeValue == ApplicationTypeName);
    }
    else if (attributeName == HealthAttributeNames::ApplicationDefinitionKind)
    {
        DWORD applicationDefinitionKindFilter = static_cast<DWORD>(FABRIC_APPLICATION_DEFINITION_KIND_FILTER_DEFAULT);
        int applicationDefinitionKind = 0;
        if (StringUtility::TryFromWString<DWORD>(attributeValue, applicationDefinitionKindFilter)
         && StringUtility::TryFromWString<int>(this->ApplicationDefinitionKind, applicationDefinitionKind))
        {
            isMatch = Management::ClusterManager::ApplicationDefinitionKind::IsMatch(
                applicationDefinitionKindFilter,
                static_cast<Management::ClusterManager::ApplicationDefinitionKind::Enum>(applicationDefinitionKind));
        }
        else
        {
            isMatch = false;
        }
    }
    else 
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode ApplicationAttributesStoreData::GetAttributeValue(
    std::wstring const & attributeName,
    __inout std::wstring & attributeValue) const
{
    if (attributeName == HealthAttributeNames::ApplicationHealthPolicy)
    {
        if (!attributeFlags_.IsAppHealthPolicySet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        attributeValue = appHealthPolicyString_;
    }
    else if (attributeName == HealthAttributeNames::ApplicationTypeName)
    {
        if (!attributeFlags_.IsApplicationTypeNameSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        attributeValue = ApplicationTypeName;
    }
    else if (attributeName == HealthAttributeNames::ApplicationDefinitionKind)
    {
        if (!attributeFlags_.IsApplicationDefinitionKindSet())
        {
            attributeValue = ApplicationDefinitionKind;
        }
    }
    else 
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ServiceModel::AttributeList ApplicationAttributesStoreData::GetEntityAttributes() const
{
    ServiceModel::AttributeList attributes;
    if (attributeFlags_.IsAppHealthPolicySet())
    {
        attributes.AddAttribute(HealthAttributeNames::ApplicationHealthPolicy, appHealthPolicyString_);
    }

    if (attributeFlags_.IsApplicationTypeNameSet())
    {
        attributes.AddAttribute(HealthAttributeNames::ApplicationTypeName, ApplicationTypeName);
    }

    if (attributeFlags_.IsApplicationDefinitionKindSet())
    {
        attributes.AddAttribute(HealthAttributeNames::ApplicationDefinitionKind, applicationDefinitionKind_);
    }

    return attributes;
}

std::wstring ApplicationAttributesStoreData::ConstructKey() const
{
    return applicationId_.ApplicationName;
}

std::wstring const & ApplicationAttributesStoreData::get_Type() const 
{ 
    return Constants::StoreType_ApplicationHealthAttributes; 
}

void ApplicationAttributesStoreData::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "{0}({1}:instance={2},policy={3},{4},{5},{6})",
        this->Type,
        this->Key,
        instanceId_,
        appHealthPolicyString_,
        applicationTypeName_,
        applicationDefinitionKind_,
        stateFlags_);
}

void ApplicationAttributesStoreData::WriteToEtw(uint16 contextSequenceId) const
{
    HMCommonEvents::Trace->ApplicationAttributesStoreDataTrace(
        contextSequenceId,
        this->Type,
        this->Key,
        instanceId_,
        appHealthPolicyString_,
        applicationTypeName_,
        applicationDefinitionKind_,
        stateFlags_);
}
