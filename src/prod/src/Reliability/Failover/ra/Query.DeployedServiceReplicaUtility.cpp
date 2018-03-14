// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace ::Query;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ReconfigurationAgentComponent::Query;
using namespace Infrastructure;
using namespace ServiceModel;

DeployedServiceReplicaUtility::DeployedServiceReplicaUtility()
{}

ErrorCode DeployedServiceReplicaUtility::GetReplicaQueryResult(
    EntityEntryBaseSPtr const & entityEntry,
    wstring const & applicationName,
    wstring const & serviceManifestName,
    __out DeployedServiceReplicaQueryResult & replicaQueryResult)
{
    ErrorCode error;
    ExecuteUnderReadLock<FailoverUnit>(
        entityEntry,
        [&applicationName, &serviceManifestName, &error, &replicaQueryResult, this](ReadOnlyLockedFailoverUnitPtr & lock)
        {
            error = GetDeployedServiceReplicaQueryResult(lock, applicationName, serviceManifestName, replicaQueryResult);
        });

    return error;
}

ErrorCode DeployedServiceReplicaUtility::GetReplicaQueryResult(
    EntityEntryBaseSPtr const & entityEntry,
    wstring const & serviceManifestName,
    int64 const & replicaId,
    __out wstring & runtimeId,
    __out FailoverUnitDescription & failoverUnitDescription,
    __out ReplicaDescription & replicaDescription,
    __out wstring & hostId,
    __out DeployedServiceReplicaQueryResult & replicaQueryResult)
{
    ErrorCode error;
    ExecuteUnderReadLock<FailoverUnit>(
        entityEntry,
        [
            &runtimeId,
            &failoverUnitDescription,
            &replicaDescription,
            &hostId,
            &replicaId,
            &serviceManifestName,
            &error,
            &replicaQueryResult, 
            this](ReadOnlyLockedFailoverUnitPtr & lock)
    {
        wstring applicationName = L"";

        if (!lock->IsRuntimeActive)
        {
            error = ErrorCodeValue::ObjectClosed;
            return;
        }

        runtimeId = lock->RuntimeId;
        failoverUnitDescription = lock->FailoverUnitDescription;
        replicaDescription = lock->LocalReplica.ReplicaDescription;
        hostId = lock->ServiceTypeRegistration->HostId;
        applicationName = lock->ServiceDescription.ApplicationName;

        // Ensure the replica ID matches the local replica id
        if (replicaId > 0 && replicaId != lock->LocalReplicaId)
        {
            error = ErrorCodeValue::REReplicaDoesNotExist;
            return;
        }

        error = GetDeployedServiceReplicaQueryResult(lock, applicationName, serviceManifestName, replicaQueryResult);
    });

    return error;
}

ErrorCode DeployedServiceReplicaUtility::GetDeployedServiceReplicaQueryResult(
    ReadOnlyLockedFailoverUnitPtr & lock,
    wstring const & applicationName,
    wstring const & serviceManifestName,
    __out DeployedServiceReplicaQueryResult & replicaQueryResult)
{
    if (!lock)
    {
        return ErrorCode(ErrorCodeValue::OperationFailed, L"Failed to get read lock.");
    }

    if (lock->IsClosed && lock->LocalReplicaDeleted)
    {
        return ErrorCode(ErrorCodeValue::REReplicaDoesNotExist, L"Replica does not exist.");
    }

    auto serviceDescription = lock->ServiceDescription;

    auto isAdHoc = false;
    if (StringUtility::Compare(applicationName, NamingUri::RootNamingUriString) == 0)
    {
        isAdHoc = true;
    }

    if (isAdHoc)
    {
        if (SystemServiceApplicationNameHelper::IsInternalServiceName(serviceDescription.Name))
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("Provided partiton {0} belongs to internal service.", lock->FailoverUnitId.Guid));
        }
    }
    else if (applicationName != serviceDescription.ApplicationName)
    {
        if (!SystemServiceApplicationNameHelper::IsSystemServiceApplicationName(applicationName) ||
            !SystemServiceApplicationNameHelper::IsSystemServiceName(serviceDescription.Name))
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("Provided application name {0} is not valid.", applicationName));
        }
    }

    if (!serviceManifestName.empty() &&
        serviceManifestName != serviceDescription.Type.ServicePackageId.ServicePackageName)
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("Provided service manifest name {0} is not valid.", serviceManifestName));
    }

    auto isStateful = lock->IsStateful;

    std::shared_ptr<wstring> servicePackageActivationIdSPtr;
    wstring codePackageName = L"";
    DWORD hostProcessId = 0;
    if (lock->ServiceTypeRegistration != nullptr)
    {
        servicePackageActivationIdSPtr = make_shared<wstring>(lock->ServiceTypeRegistration->ServicePackagePublicActivationId);
        codePackageName = lock->ServiceTypeRegistration->CodePackageName;
        hostProcessId = lock->ServiceTypeRegistration->HostProcessId;
    }

    auto failoverUnitId = lock->FailoverUnitId.Guid;
    auto localReplicaId = lock->LocalReplicaId;

    wstring serviceLocation = L"";
    auto replicaStatus = FABRIC_QUERY_SERVICE_REPLICA_STATUS_INVALID;
    if (!lock->IsClosed)
    {
        serviceLocation = lock->LocalReplica.ReplicaDescription.ServiceLocation;
        replicaStatus = lock->LocalReplica.QueryStatus;

        TESTASSERT_IF(replicaStatus == FABRIC_QUERY_SERVICE_REPLICA_STATUS_INVALID, "replica status must be assigned");
    }
    else
    {
        // ft is closed so local replica should be in dropped status
        replicaStatus = FABRIC_QUERY_SERVICE_REPLICA_STATUS_DROPPED;
    }

    auto serviceName = SystemServiceApplicationNameHelper::GetPublicServiceName(serviceDescription.Name);
    auto serviceTypeName = serviceDescription.Type.ServiceTypeName;
    auto servicePackageName = serviceDescription.Type.ServicePackageId.ServicePackageName;

    if (isStateful)
    {
        auto currentConfigRole = ReplicaRole::Unknown;
        auto previousConfigRole = ReplicaRole::Unknown;
        if (!lock->IsClosed)
        {
            currentConfigRole = lock->LocalReplica.CurrentConfigurationRole;
            previousConfigRole = lock->LocalReplica.PreviousConfigurationRole;
        }

        auto reconfigurationType = ReconfigurationType::None;
        if (lock->ReconfigurationStage != FailoverUnitReconfigurationStage::None)
        {
            reconfigurationType = lock->ReconfigurationType;
        }

        auto reconfigurationInformation = ReconfigurationInformation(
            ReplicaRole::ConvertToPublicReplicaRole(previousConfigRole),
            FailoverUnitReconfigurationStage::ConvertToPublicReconfigurationPhase(lock->ReconfigurationStage),
            ReconfigurationType::ConvertToPublicReconfigurationType(reconfigurationType),
            lock->ReconfigurationStartTime.ToDateTime());

        replicaQueryResult = DeployedServiceReplicaQueryResult(
            serviceName,
            serviceTypeName,
            servicePackageName,
            servicePackageActivationIdSPtr,
            codePackageName,
            failoverUnitId,
            localReplicaId,
            ReplicaRole::ConvertToPublicReplicaRole(currentConfigRole),
            replicaStatus,
            serviceLocation,
            hostProcessId,
            reconfigurationInformation);
    }
    else
    {
        replicaQueryResult = DeployedServiceReplicaQueryResult(
            serviceName,
            serviceTypeName,
            servicePackageName,
            servicePackageActivationIdSPtr,
            codePackageName,
            failoverUnitId,
            localReplicaId,
            replicaStatus,
            serviceLocation,
            hostProcessId);
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr DeployedServiceReplicaUtility::BeginGetReplicaDetailQuery(
    ReconfigurationAgent & ra,
    wstring const activityId,
    EntityEntryBaseSPtr const & entityEntry,
    wstring const & runtimeId,
    FailoverUnitDescription const & failoverUnitDescription,
    ReplicaDescription const & replicaDescription,
    wstring const & hostId,
    Common::AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<HostQueryAsyncOperation>(
        ra,
        activityId,
        entityEntry,
        runtimeId,
        failoverUnitDescription,
        replicaDescription,
        hostId,
        callback,
        parent);
}

ErrorCode DeployedServiceReplicaUtility::EndGetReplicaDetailQuery(
    AsyncOperationSPtr const & asyncOperation,
    __out ServiceModel::DeployedServiceReplicaDetailQueryResult & replicaDetailQueryResult)
{
    auto hostQueryAsyncOp = static_pointer_cast<HostQueryAsyncOperation>(asyncOperation);

    if (asyncOperation->Error.IsSuccess())
    {
        replicaDetailQueryResult = hostQueryAsyncOp->GetReplicaDetailQueryResult();
    }

    return asyncOperation->Error;
}
