// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedServiceReplicaQueryResult");

DeployedServiceReplicaQueryResult::DeployedServiceReplicaQueryResult()
    : serviceResultKind_(FABRIC_SERVICE_KIND_INVALID)
{
}

DeployedServiceReplicaQueryResult::DeployedServiceReplicaQueryResult(
    std::wstring const & serviceName,
    std::wstring const & serviceTypeName,
    std::wstring const & serviceManifestName,
	std::shared_ptr<wstring> const & servicePackageActivationIdFilterSPtr,
    std::wstring const & codePackageName,
    Common::Guid const & partitionId,
    int64 replicaId,
    FABRIC_REPLICA_ROLE replicaRole, 
    FABRIC_QUERY_SERVICE_REPLICA_STATUS replicaStatus,
    std::wstring const & address,
    int64 const & hostProcessId,
    ReconfigurationInformation const & reconfigurationInformation)
    : serviceResultKind_(FABRIC_SERVICE_KIND_STATEFUL)
    , serviceName_(serviceName)
    , serviceTypeName_(serviceTypeName)
    , serviceManifestName_(serviceManifestName)
    , servicePackageActivationIdFilterSPtr_(servicePackageActivationIdFilterSPtr)
    , codePackageName_(codePackageName)
    , partitionId_(partitionId)
    , replicaId_(replicaId)
    , instanceId_(0)
    , replicaRole_(replicaRole)
    , replicaStatus_(replicaStatus)
    , address_(address)
    , hostProcessId_(hostProcessId)
    , reconfigurationInformation_(reconfigurationInformation)
{
}

DeployedServiceReplicaQueryResult::DeployedServiceReplicaQueryResult(
    std::wstring const & serviceName,
    std::wstring const & serviceTypeName,
    std::wstring const & serviceManifestName,
	std::shared_ptr<wstring> const & servicePackageActivationIdFilterSPtr,
    std::wstring const & codePackageName,
    Common::Guid const & partitionId,
    int64 instanceId,
    FABRIC_QUERY_SERVICE_REPLICA_STATUS replicaStatus,
    std::wstring const & address,
    int64 hostProcessId)
    : serviceResultKind_(FABRIC_SERVICE_KIND_STATELESS)
    , serviceName_(serviceName)
    , serviceTypeName_(serviceTypeName)
    , serviceManifestName_(serviceManifestName)
    , servicePackageActivationIdFilterSPtr_(servicePackageActivationIdFilterSPtr)
    , codePackageName_(codePackageName)
    , partitionId_(partitionId)
    , replicaId_(0)
    , instanceId_(instanceId)
    , replicaRole_(FABRIC_REPLICA_ROLE_NONE)
    , replicaStatus_(replicaStatus)
    , address_(address)
    , hostProcessId_(hostProcessId)
    , reconfigurationInformation_(ReconfigurationInformation())
{
}

void DeployedServiceReplicaQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM & publicDeployedServiceReplica) const 
{
    publicDeployedServiceReplica.Kind = serviceResultKind_;
    if (serviceResultKind_ == FABRIC_SERVICE_KIND_STATEFUL)
    {
        auto statefulService = heap.AddItem<FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM>();
        ToPublicApi(heap, *statefulService);
        publicDeployedServiceReplica.Value = statefulService.GetRawPointer();
    }
    else if(serviceResultKind_ == FABRIC_SERVICE_KIND_STATELESS)
    {
        auto statelessService = heap.AddItem<FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM>();
        ToPublicApi(heap, *statelessService);
        publicDeployedServiceReplica.Value = statelessService.GetRawPointer();
    }
}

void DeployedServiceReplicaQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM & statefulService) const
{
    statefulService.Address = heap.AddString(address_);
    statefulService.CodePackageName = heap.AddString(codePackageName_);
    statefulService.PartitionId = partitionId_.AsGUID();
    statefulService.ReplicaId = replicaId_;
    statefulService.ReplicaRole = replicaRole_;
    statefulService.ReplicaStatus = replicaStatus_;
    statefulService.Reserved = nullptr;
    statefulService.ServiceManifestVersion = heap.AddString(serviceManifestVersion_);
    statefulService.ServiceName = heap.AddString(serviceName_);
    statefulService.ServiceTypeName = heap.AddString(serviceTypeName_);
    
    auto extended1 = heap.AddItem<FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM_EX1>();
    extended1->ServiceManifestName = heap.AddString(serviceManifestName_);

    auto extended2 = heap.AddItem<FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM_EX2>();
    if (servicePackageActivationIdFilterSPtr_ != nullptr)
    {
        extended2->ServicePackageActivationId = heap.AddString(*servicePackageActivationIdFilterSPtr_);
    }

    auto extended3 = heap.AddItem<FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM_EX3>();
    extended3->HostProcessId = hostProcessId_;

    auto reconfigurationInformation = heap.AddItem<FABRIC_RECONFIGURATION_INFORMATION_QUERY_RESULT>();
    reconfigurationInformation_.ToPublicApi(heap, *reconfigurationInformation);
    extended3->ReconfigurationInformation = reconfigurationInformation.GetRawPointer();

    extended2->Reserved = extended3.GetRawPointer();
    extended1->Reserved = extended2.GetRawPointer();
    statefulService.Reserved = extended1.GetRawPointer();
}

void DeployedServiceReplicaQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM & statelessService) const
{
        statelessService.Address = heap.AddString(address_);
        statelessService.CodePackageName = heap.AddString(codePackageName_);
        statelessService.PartitionId = partitionId_.AsGUID();
        statelessService.InstanceId = instanceId_;
        statelessService.ReplicaStatus = replicaStatus_;
        statelessService.Reserved = nullptr;
        statelessService.ServiceManifestVersion = heap.AddString(serviceManifestVersion_);
        statelessService.ServiceName = heap.AddString(serviceName_);
        statelessService.ServiceTypeName = heap.AddString(serviceTypeName_);

        auto extended1 = heap.AddItem<FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM_EX1>();
        extended1->ServiceManifestName = heap.AddString(serviceManifestName_);

        auto extended2 = heap.AddItem<FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM_EX2>();
        if (servicePackageActivationIdFilterSPtr_ != nullptr)
        {
            extended2->ServicePackageActivationId = heap.AddString(*servicePackageActivationIdFilterSPtr_);
        }

        auto extended3 = heap.AddItem<FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM_EX3>();
        extended3->HostProcessId = hostProcessId_;

        extended2->Reserved = extended3.GetRawPointer();
        extended1->Reserved = extended2.GetRawPointer();
        statelessService.Reserved = extended1.GetRawPointer();
}

void DeployedServiceReplicaQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());    
}

std::wstring DeployedServiceReplicaQueryResult::ToString() const
{
    wstring replicaString = L"";

    replicaString = wformatString("PartitionId = {0}, Name = {1}, TypeName = {2}, ManifestVersion = {3}, CodePackageName = {4}, ReplicaId = {5}, InstanceId = {6}, ReplicaStatus = {7}, ReplicaRole = {8}, Address = {9}, ManifestName = {10}",
        partitionId_,
        serviceName_,
        serviceTypeName_,
        serviceManifestVersion_,
        codePackageName_,
        replicaId_,
        instanceId_,
        replicaStatus_,
        replicaRole_,
        address_,
        serviceManifestName_);

    replicaString = wformatString("{0}, ProcessId = {1}, ReconfigurationInformation = {2}",
        replicaString,
        hostProcessId_,
        reconfigurationInformation_.ToString());

    if (servicePackageActivationIdFilterSPtr_ != nullptr)
    {
        auto servicePackageActivationIdFilter = *servicePackageActivationIdFilterSPtr_;
        if (!servicePackageActivationIdFilter.empty())
        {
            replicaString = wformatString("{0}, {1}", replicaString, servicePackageActivationIdFilter);
        }
    }

    return wformatString("Kind = {0}, {1}",
        static_cast<int>(serviceResultKind_),
        replicaString);
}
