// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

GlobalWString EntityHealthInformation::Delimiter = make_global<wstring>(L"+");

INITIALIZE_BASE_DYNAMIC_SIZE_ESTIMATION(EntityHealthInformation)

bool EntityHealthInformation::LessThanComparitor::operator() (
    EntityHealthInformationSPtr const & first, 
    EntityHealthInformationSPtr const & second) const
{
    ASSERT_IFNOT(first, "EntityHealthInformationSPtr first should exist");
    ASSERT_IFNOT(second, "EntityHealthInformationSPtr second should exist");
    return first->EntityId < second->EntityId;
}

EntityHealthInformation::EntityHealthInformation()
    : kind_(FABRIC_HEALTH_REPORT_KIND_INVALID)
    , entityId_()
{
}

EntityHealthInformation::EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND kind)
    : kind_(kind)
    , entityId_()
{
}

EntityHealthInformation::~EntityHealthInformation()
{
}

EntityHealthInformationSPtr EntityHealthInformation::CreateClusterEntityHealthInformation()
{
    return make_shared<ClusterEntityHealthInformation>();
}

EntityHealthInformationSPtr EntityHealthInformation::CreatePartitionEntityHealthInformation(
    Guid const& partitionId)
{
    return make_shared<PartitionEntityHealthInformation>(partitionId);
}

EntityHealthInformationSPtr EntityHealthInformation::CreateStatefulReplicaEntityHealthInformation(
    Guid const& partitionId,
    FABRIC_REPLICA_ID replicaId,
    FABRIC_INSTANCE_ID replicaInstanceId)
{
    return make_shared<StatefulReplicaEntityHealthInformation>(partitionId, replicaId, replicaInstanceId);
}

EntityHealthInformationSPtr EntityHealthInformation::CreateStatelessInstanceEntityHealthInformation(
    Guid const& partitionId,
    FABRIC_INSTANCE_ID instanceId)
{
    return make_shared<StatelessInstanceEntityHealthInformation>(partitionId, instanceId);
}

EntityHealthInformationSPtr EntityHealthInformation::CreateNodeEntityHealthInformation(
    LargeInteger const& nodeId,
    wstring const& nodeName,
    FABRIC_NODE_INSTANCE_ID nodeInstanceId)
{
    return make_shared<NodeEntityHealthInformation>(nodeId, nodeName, nodeInstanceId);
}

EntityHealthInformationSPtr EntityHealthInformation::CreateDeployedServicePackageEntityHealthInformation(
    std::wstring const & applicationName,          
    std::wstring const & serviceManifestName,
    std::wstring const & servicePackageActivationId,
    Common::LargeInteger const & nodeId,
    std::wstring const & nodeName,
    FABRIC_INSTANCE_ID servicePackageInstanceId)
{
    return make_shared<DeployedServicePackageEntityHealthInformation>(applicationName, serviceManifestName, servicePackageActivationId, nodeId, nodeName, servicePackageInstanceId);
}

EntityHealthInformationSPtr EntityHealthInformation::CreateDeployedApplicationEntityHealthInformation(
    std::wstring const & applicationName,      
    Common::LargeInteger const & nodeId,
    std::wstring const & nodeName,
    FABRIC_INSTANCE_ID applicationInstanceId)
{
    return make_shared<DeployedApplicationEntityHealthInformation>(applicationName, nodeId, nodeName, applicationInstanceId);
}

EntityHealthInformationSPtr EntityHealthInformation::CreateServiceEntityHealthInformation(
    wstring const & serviceName, FABRIC_INSTANCE_ID instanceId)
{
    return make_shared<ServiceEntityHealthInformation>(serviceName, instanceId);
}

EntityHealthInformationSPtr EntityHealthInformation::CreateApplicationEntityHealthInformation(
    wstring const & applicationName,
    FABRIC_INSTANCE_ID applicationInstanceId)
{
    return make_shared<ApplicationEntityHealthInformation>(applicationName, applicationInstanceId);
}

ErrorCode EntityHealthInformation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
    __out FABRIC_HEALTH_REPORT & healthReport) const
{
    UNREFERENCED_PARAMETER(heap);
    UNREFERENCED_PARAMETER(commonHealthInformation);
    healthReport.Kind = FABRIC_HEALTH_REPORT_KIND_INVALID;
    healthReport.Value = NULL;
    return ErrorCode::Success();
}

ErrorCode EntityHealthInformation::FromPublicApi(
    FABRIC_HEALTH_REPORT const & healthReport,
    __inout HealthInformation & commonHealthInformation,
    __out AttributeList & attributes)
{
    UNREFERENCED_PARAMETER(healthReport);
    UNREFERENCED_PARAMETER(commonHealthInformation);
    UNREFERENCED_PARAMETER(attributes);

    kind_ = FABRIC_HEALTH_REPORT_KIND_INVALID;
    return ErrorCode(ErrorCodeValue::InvalidArgument);
}

Serialization::IFabricSerializable * EntityHealthInformation::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr || 
        typeInformation.length != sizeof(FABRIC_HEALTH_REPORT_KIND))                                                   
    {                                                                                                                                           
        return nullptr;                                                                                                                         
    }                                                                                                                                           
                                                                                                                                                        
    FABRIC_HEALTH_REPORT_KIND kind = *(reinterpret_cast<FABRIC_HEALTH_REPORT_KIND const *>(typeInformation.buffer));
    return CreateNew(kind);
}

EntityHealthInformation * EntityHealthInformation::CreateNew(FABRIC_HEALTH_REPORT_KIND kind)      
{
    switch (kind)
    {
    case FABRIC_HEALTH_REPORT_KIND_CLUSTER:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_CLUSTER>::CreateNew();
    case FABRIC_HEALTH_REPORT_KIND_NODE:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_NODE>::CreateNew();
    case FABRIC_HEALTH_REPORT_KIND_PARTITION:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_PARTITION>::CreateNew();
    case FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA>::CreateNew();
    case FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE>::CreateNew();
    case FABRIC_HEALTH_REPORT_KIND_SERVICE:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_SERVICE>::CreateNew();    
    case FABRIC_HEALTH_REPORT_KIND_APPLICATION:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_APPLICATION>::CreateNew();    
    case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION>::CreateNew();    
    case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE>::CreateNew();    
    default: 
        return new EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_INVALID); 
    }
}

EntityHealthInformationSPtr EntityHealthInformation::CreateSPtr(FABRIC_HEALTH_REPORT_KIND kind)
{
    switch (kind)
    {
    case FABRIC_HEALTH_REPORT_KIND_CLUSTER:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_CLUSTER>::CreateSPtr();    
    case FABRIC_HEALTH_REPORT_KIND_NODE:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_NODE>::CreateSPtr();
    case FABRIC_HEALTH_REPORT_KIND_PARTITION:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_PARTITION>::CreateSPtr();
    case FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA>::CreateSPtr();
    case FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE>::CreateSPtr();
    case FABRIC_HEALTH_REPORT_KIND_SERVICE:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_SERVICE>::CreateSPtr();
    case FABRIC_HEALTH_REPORT_KIND_APPLICATION:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_APPLICATION>::CreateSPtr();
    case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION>::CreateSPtr();    
    case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE:
        return EntityHealthInformationSerializationTypeActivator<FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE>::CreateSPtr();    
    default: 
        return make_shared<EntityHealthInformation>(FABRIC_HEALTH_REPORT_KIND_INVALID); 
    }
}

NTSTATUS EntityHealthInformation::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&kind_);
    typeInformation.length = sizeof(kind_);   
    return STATUS_SUCCESS;
}

void EntityHealthInformation::WriteTo(__in TextWriter& writer, FormatOptions const &) const
{
    writer.Write("{0}: {1}", kind_, this->EntityId);
    if (this->EntityInstance != FABRIC_INVALID_INSTANCE_ID)
    {
        writer.Write(": {0}", this->EntityInstance);
    }
}

wstring EntityHealthInformation::ToString() const
{
    wstring result;
    StringWriter(result).Write(*this);
    return result;
}

Common::ErrorCode EntityHealthInformation::CheckAgainstInstance(
    FABRIC_INSTANCE_ID currentInstance, 
    bool isDeleteReport, 
    __out bool & cleanPreviousReports) const
{
    if (this->EntityInstance == currentInstance)
    {
        // Accept report if it has same instance as the existing one.
        // For delete report, clean reports as the entity is deleted
        // Delete report can be accepted if instance is same as current, even for unknown
        cleanPreviousReports = isDeleteReport;
        return ErrorCode::Success();
    }
    else if (this->EntityInstance > currentInstance)
    {
        // New instance, previous reports are not applicable anymore
        cleanPreviousReports = true;
        return ErrorCode::Success();
    }
    else if (this->HasUnknownInstance())
    {
        if (isDeleteReport)
        {
            // Delete report can only be accepted if it has higher or equal instance
            return ErrorCode(ErrorCodeValue::InvalidArgument);
        }

        cleanPreviousReports = false;
        return ErrorCode::Success();
    }
    else
    {
        // Stale instance
        wstring errorMessage = wformatString("{0} {1}, {2}.", HMResource::GetResources().HealthStaleReportDueToInstance, this->EntityInstance, currentInstance);
        return ErrorCode(ErrorCodeValue::HealthStaleReport, move(errorMessage));
    }    
}
