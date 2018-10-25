// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace ServiceModel;
using namespace Management::HealthManager;

ReportRequestContext::ReportRequestContext(
    Store::ReplicaActivityId const & replicaActivityId,
    Common::AsyncOperationSPtr const & owner,
    __in IReportRequestContextProcessor & requestProcessor,
    uint64 requestId,
    HealthReport && report,
    FABRIC_SEQUENCE_NUMBER sequenceStreamFromLsn,
    FABRIC_SEQUENCE_NUMBER sequenceStreamUpToLsn)
    : RequestContext(replicaActivityId, owner)
    , report_(move(report))
    , requestProcessor_(requestProcessor)
    , requestId_(requestId)
    , sequenceStreamFromLsn_(sequenceStreamFromLsn)
    , sequenceStreamUpToLsn_(sequenceStreamUpToLsn)
    , previousState_(FABRIC_HEALTH_STATE_INVALID)
{
}

ReportRequestContext::ReportRequestContext(ReportRequestContext && other)
    : RequestContext(move(other))
    , report_(move(other.report_))
    , requestProcessor_(other.requestProcessor_)
    , requestId_(move(other.requestId_))
    , sequenceStreamFromLsn_(move(other.sequenceStreamFromLsn_))
    , sequenceStreamUpToLsn_(move(other.sequenceStreamUpToLsn_))
    , previousState_(move(other.previousState_))
{
}

ReportRequestContext & ReportRequestContext::operator = (ReportRequestContext && other)
{
    if (this != &other)
    {
        report_ = move(other.report_);
        requestProcessor_ = other.requestProcessor_;
        requestId_ = move(other.requestId_);
        sequenceStreamFromLsn_ = move(other.sequenceStreamFromLsn_);
        sequenceStreamUpToLsn_ = move(other.sequenceStreamUpToLsn_);
        previousState_ = move(other.previousState_);
    }

    RequestContext::operator=(move(other));
    return *this;
}

ReportRequestContext::~ReportRequestContext()
{
}

bool ReportRequestContext::IsExpired() const
{
    return this->OwnerRemainingTime <= TimeSpan::Zero;
}

bool ReportRequestContext::CheckAndSetExpired() const
{
    if (IsExpired())
    {
        SetError(ErrorCode(ErrorCodeValue::Timeout));
        return true;
    }

    return false;
}

void ReportRequestContext::OnRequestCompleted()
{
    requestProcessor_.OnReportProcessed(this->Owner, *this, this->Error);
}

void ReportRequestContext::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "{0}:{1}",
        this->ReplicaActivityId.TraceId,
        report_);
}

void ReportRequestContext::WriteToEtw(uint16 contextSequenceId) const
{
    HMCommonEvents::Trace->ReportRequestContextTrace(
        contextSequenceId,
        this->ReplicaActivityId.TraceId,
        report_);
}

size_t ReportRequestContext::EstimateSize() const
{
    return report_.EstimateSize();
}

//
// Templated functions
//
template<> ClusterHealthId ReportRequestContext::GetEntityId<ClusterHealthId>() const;
template<> NodeHealthId ReportRequestContext::GetEntityId<NodeHealthId>() const;
template<> ReplicaHealthId ReportRequestContext::GetEntityId<ReplicaHealthId>() const;
template<> PartitionHealthId ReportRequestContext::GetEntityId<PartitionHealthId>() const;
template<> ServiceHealthId ReportRequestContext::GetEntityId<ServiceHealthId>() const;
template<> ApplicationHealthId ReportRequestContext::GetEntityId<ApplicationHealthId>() const;
template<> DeployedApplicationHealthId ReportRequestContext::GetEntityId<DeployedApplicationHealthId>() const;
template<> DeployedServicePackageHealthId ReportRequestContext::GetEntityId<DeployedServicePackageHealthId>() const;

template <> ClusterHealthId ReportRequestContext::GetEntityId<ClusterHealthId>() const
{
    ASSERT_IF(ReportKind != FABRIC_HEALTH_REPORT_KIND_CLUSTER, "ClusterHealthId requested for {0}", ReportKind);
    auto entityInfo = dynamic_cast<ServiceModel::ClusterEntityHealthInformation const*>(report_.EntityInformation.get());
    if (entityInfo == NULL)
    {
        Common::Assert::CodingError("entityInfo null for {0}", report_);
    }
    else
    {
        // Cluster doesn't have an entityId; there is just one entity of this type, so use the default id
        return *Constants::StoreType_ClusterTypeString;
    }
}

template <> NodeHealthId ReportRequestContext::GetEntityId<NodeHealthId>() const
{
    ASSERT_IF(ReportKind != FABRIC_HEALTH_REPORT_KIND_NODE, "NodeId requested for {0}", ReportKind);
    auto entityInfo = dynamic_cast<ServiceModel::NodeEntityHealthInformation const*>(report_.EntityInformation.get());
    if (entityInfo == NULL)
    {
        Common::Assert::CodingError("entityInfo null for {0}", report_);
    }
    else
    {
        // The node id computed on the client might use different node id generation scheme.
        // Recompute the node here so on the server side is generated with the correct serialization scheme
        const_cast<NodeEntityHealthInformation*>(entityInfo)->GenerateNodeId().ReadValue();
        return entityInfo->NodeId;
    }
}

template <> ReplicaHealthId ReportRequestContext::GetEntityId<ReplicaHealthId>() const
{
    if (ReportKind == FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA)
    {
        auto entityInfo = dynamic_cast<ServiceModel::StatefulReplicaEntityHealthInformation const*>(report_.EntityInformation.get());
        if (entityInfo == NULL)
        {
            Common::Assert::CodingError("entityInfo null for {0}", report_);
        }
        else
        {
            return ReplicaHealthId(entityInfo->PartitionId, entityInfo->ReplicaId);
        }
    }
    else if (ReportKind == FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE)
    {
        auto entityInfo = dynamic_cast<ServiceModel::StatelessInstanceEntityHealthInformation const*>(report_.EntityInformation.get());
        if (entityInfo == NULL)
        {
            Common::Assert::CodingError("entityInfo null for {0}", report_);
        }
        else
        {
            return ReplicaHealthId(entityInfo->PartitionId, entityInfo->ReplicaId);
        }
    }
    else
    {
        Common::Assert::CodingError("{0}: ReplicaId requested for {1}", report_, ReportKind);
    }
}

template <> PartitionHealthId ReportRequestContext::GetEntityId<PartitionHealthId>() const
{
    ASSERT_IF(ReportKind != FABRIC_HEALTH_REPORT_KIND_PARTITION, "PartitionId requested for {0}", ReportKind);
    auto entityInfo = dynamic_cast<ServiceModel::PartitionEntityHealthInformation const*>(report_.EntityInformation.get());
    if (entityInfo == NULL)
    {
        Common::Assert::CodingError("entityInfo null for {0}", report_);
    }
    else
    {
        return entityInfo->PartitionId;
    }
}

template <> ServiceHealthId ReportRequestContext::GetEntityId<ServiceHealthId>() const
{
    ASSERT_IF(ReportKind != FABRIC_HEALTH_REPORT_KIND_SERVICE, "ServiceTypeId requested for {0}", ReportKind);
    auto entityInfo = dynamic_cast<ServiceModel::ServiceEntityHealthInformation const*>(report_.EntityInformation.get());
    if (entityInfo == NULL)
    {
        Common::Assert::CodingError("entityInfo null for {0}", report_);
    }
    else
    {
        return ServiceHealthId(entityInfo->ServiceName);
    }
}

template <> ApplicationHealthId ReportRequestContext::GetEntityId<ApplicationHealthId>() const
{
    ASSERT_IF(ReportKind != FABRIC_HEALTH_REPORT_KIND_APPLICATION, "ApplicationTypeId requested for {0}", ReportKind);
    auto entityInfo = dynamic_cast<ServiceModel::ApplicationEntityHealthInformation const*>(report_.EntityInformation.get());
    if (entityInfo == NULL)
    {
        Common::Assert::CodingError("entityInfo null for {0}", report_);
    }
    else
    {
        return ApplicationHealthId(entityInfo->ApplicationName);
    }
}

template <> DeployedApplicationHealthId ReportRequestContext::GetEntityId<DeployedApplicationHealthId>() const
{
    ASSERT_IF(ReportKind != FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION, "DeployedApplicationId requested for {0}", ReportKind);
    auto entityInfo = dynamic_cast<ServiceModel::DeployedApplicationEntityHealthInformation const*>(report_.EntityInformation.get());
    if (entityInfo == NULL)
    {
        Common::Assert::CodingError("entityInfo null for {0}", report_);
    }
    else
    {
        const_cast<DeployedApplicationEntityHealthInformation*>(entityInfo)->GenerateNodeId().ReadValue();
        return DeployedApplicationHealthId(entityInfo->ApplicationName, entityInfo->NodeId);
    }
}

template <> DeployedServicePackageHealthId ReportRequestContext::GetEntityId<DeployedServicePackageHealthId>() const
{
    ASSERT_IF(ReportKind != FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE, "DeployedServicePackageId requested for {0}", ReportKind);
    auto entityInfo = dynamic_cast<ServiceModel::DeployedServicePackageEntityHealthInformation const*>(report_.EntityInformation.get());
    if (entityInfo == NULL)
    {
        Common::Assert::CodingError("entityInfo null for {0}", report_);
    }
    else
    {
        const_cast<DeployedServicePackageEntityHealthInformation*>(entityInfo)->GenerateNodeId().ReadValue();
        return DeployedServicePackageHealthId(
            entityInfo->ApplicationName,
            entityInfo->ServiceManifestName,
            entityInfo->ServicePackageActivationId,
            entityInfo->NodeId);
    }
}
