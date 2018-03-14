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

StringLiteral const TraceComponent("ReplicaAttributesStoreData");

ReplicaAttributesStoreData::ReplicaAttributesStoreData()
    : AttributesStoreData()
    , replicaId_()
    , instanceId_(FABRIC_INVALID_INSTANCE_ID)
    , nodeId_()
    , nodeInstanceId_(FABRIC_INVALID_NODE_INSTANCE_ID)
    , attributeFlags_()
{
}

ReplicaAttributesStoreData::ReplicaAttributesStoreData(
    ReplicaHealthId const & replicaId,
    ReportRequestContext const & context)
    : AttributesStoreData(context.ReplicaActivityId)
    , replicaId_(replicaId)
    , instanceId_(FABRIC_INVALID_INSTANCE_ID)
    , nodeId_()
    , nodeInstanceId_(FABRIC_INVALID_NODE_INSTANCE_ID)
    , attributeFlags_()
{
    if (context.ReportKind == FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE)
    {
        instanceId_ = Constants::UnusedInstanceValue;
    }
    else
    {
        ASSERT_IFNOT(context.ReportKind == FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA, "Create replica from {0}: Invalid kind {1}", context, context.ReportKind);
        instanceId_ = context.Report.EntityInformation->EntityInstance;
    }
}

ReplicaAttributesStoreData::ReplicaAttributesStoreData(
    ReplicaHealthId const & replicaId,
    Store::ReplicaActivityId const & activityId)
    : AttributesStoreData(activityId)
    , replicaId_(replicaId)
    , nodeId_()
    , nodeInstanceId_(FABRIC_INVALID_NODE_INSTANCE_ID)
    , attributeFlags_()
{
}

ReplicaAttributesStoreData::~ReplicaAttributesStoreData()
{
}

bool ReplicaAttributesStoreData::ShouldUpdateAttributeInfo(ServiceModel::AttributeList const & attributeList) const
{
    for (auto it = attributeList.Attributes.begin(); it != attributeList.Attributes.end(); ++it)
    {
        if (it->first == HealthAttributeNames::NodeId)
        {
            if (!attributeFlags_.IsNodeIdSet() || it->second != wformatString(this->NodeId))
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

void ReplicaAttributesStoreData::SetAttributes(
    ServiceModel::AttributeList const & attributeList)
{
    for (auto it = attributeList.Attributes.begin(); it != attributeList.Attributes.end(); ++it)
    {
        // Check whether the attributes have the expected names and map the values
        if (it->first == HealthAttributeNames::NodeId)
        {
            NodeHealthId nodeId;
            bool success = LargeInteger::TryParse(it->second, nodeId);
            if (success)
            {
                this->NodeId = nodeId;
            }
            else
            {
                HMEvents::Trace->DropAttribute(
                    this->ReplicaActivityId,
                    it->first,
                    it->second);
            }
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

Common::ErrorCode ReplicaAttributesStoreData::HasAttributeMatch(
    std::wstring const & attributeName,
    std::wstring const & attributeValue,
    __out bool & isMatch) const
{
    isMatch = false;
    if (attributeName == HealthAttributeNames::NodeId)
    {
        if (!attributeFlags_.IsNodeIdSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        isMatch = (attributeValue == wformatString(this->NodeId));
    }
    else if (attributeName == HealthAttributeNames::NodeInstanceId)
    {
        if (!attributeFlags_.IsNodeInstanceIdSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        isMatch = (attributeValue == wformatString(this->NodeInstanceId));
    }
    else if (attributeName == QueryResourceProperties::Replica::ReplicaId || attributeName == QueryResourceProperties::Replica::InstanceId)
    {
        isMatch = (attributeValue == wformatString(replicaId_.ReplicaId));
    }
    else 
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode ReplicaAttributesStoreData::GetAttributeValue(
    std::wstring const & attributeName,
    __inout std::wstring & attributeValue) const
{
    if (attributeName == HealthAttributeNames::NodeId)
    {
        if (!attributeFlags_.IsNodeIdSet())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        attributeValue = wformatString(this->NodeId);
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

ServiceModel::AttributeList ReplicaAttributesStoreData::GetEntityAttributes() const
{
    ServiceModel::AttributeList attributes;
    if (attributeFlags_.IsNodeIdSet())
    {
        attributes.AddAttribute(HealthAttributeNames::NodeId, wformatString(nodeId_));
    }

    if (attributeFlags_.IsNodeInstanceIdSet())
    {
        attributes.AddAttribute(HealthAttributeNames::NodeInstanceId, wformatString(nodeInstanceId_));
    }

    return attributes;
}

std::wstring ReplicaAttributesStoreData::ConstructKey() const
{
    return replicaId_.ToString();
}

std::wstring const & ReplicaAttributesStoreData::get_Type() const 
{ 
    return Constants::StoreType_ReplicaHealthAttributes; 
}

void ReplicaAttributesStoreData::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "{0}({1}:nodeId='{2}',instanceId={3},nodeInstance={4},{5})",
        this->Type,
        this->Key,
        nodeId_,
        instanceId_,
        nodeInstanceId_,
        stateFlags_);
}

void ReplicaAttributesStoreData::WriteToEtw(uint16 contextSequenceId) const
{
    HMCommonEvents::Trace->ReplicaAttributesStoreDataTrace(
        contextSequenceId,
        this->Type,
        this->Key,
        nodeId_,
        instanceId_,
        nodeInstanceId_,
        stateFlags_);
}

Common::ErrorCode ReplicaAttributesStoreData::TryAcceptHealthReport(
    ServiceModel::HealthReport const & healthReport,
    __inout bool & skipLsnCheck,
    __out bool & replaceAttributesMetadata) const
{
    replaceAttributesMetadata = false;
    FABRIC_HEALTH_REPORT_KIND currentKind = this->UseInstance ? FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA : FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE;
    if (currentKind != healthReport.Kind)
    {
        if (healthReport.Kind == FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE)
        {
            // If current attributes are stateful and the new report is for stateless instance,
            // accept report if the current attributes have no instance set (no System report).
            replaceAttributesMetadata = (instanceId_ == FABRIC_INVALID_INSTANCE_ID);
        }
        else 
        {
            // If current attributes are stateless and the new report is for stateful replica,
            // accept report only if it has instance set (System report).
            replaceAttributesMetadata = !healthReport.EntityInformation->HasUnknownInstance();
        }

        if (replaceAttributesMetadata)
        {
            // The LSN should be checked against reports from the same source, to make sure the report is not stale
            skipLsnCheck = false;
            return ErrorCode::Success();
        }
        else
        {
            HealthManagerReplica::WriteInfo(
                TraceComponent,
                "Reject {0}: kind mismatch, existing {1}, received {2}",
                healthReport,
                *this,
                healthReport.Kind);
            return ErrorCode(ErrorCodeValue::InvalidArgument);
        }
    }

    // The new report has same kind as the previous ones
    return __super::TryAcceptHealthReport(healthReport, /*out*/ skipLsnCheck, /*out*/ replaceAttributesMetadata);
}
