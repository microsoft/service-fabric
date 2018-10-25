// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace ServiceModel;
using namespace Store;

using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ServiceContext");

RolloutContextType::Enum const ServiceContextType(RolloutContextType::Service);

ServiceContext::ServiceContext()
    : DeletableRolloutContext(ServiceContextType)
    , applicationId_()
    , applicationName_()
    , absoluteServiceName_()
    , serviceName_()
    , typeName_()
    , packageName_()
    , packageVersion_()
    , packageInstance_(0)
    , partitionedServiceDescriptor_()
    , isDefaultService_(false)
    , isCommitPending_(false)
{
}

ServiceContext::ServiceContext(
    ServiceModelApplicationId const & appId,
    NamingUri const & appName,
    NamingUri const & absoluteServiceName)
    : DeletableRolloutContext(ServiceContextType)
    , applicationId_(appId)
    , applicationName_(appName)
    , absoluteServiceName_(absoluteServiceName)
    , serviceName_(GetRelativeServiceName(appName.ToString(), absoluteServiceName.ToString()))
    , typeName_()
    , packageName_()
    , packageVersion_()
    , packageInstance_(0)
    , partitionedServiceDescriptor_()
    , isDefaultService_(false)
    , isCommitPending_(false)
{
}

ServiceContext::ServiceContext(
    ServiceModelApplicationId const & appId,
    Common::NamingUri const & appName,
    Common::NamingUri const & absoluteServiceName,
    bool const isForceDelete)
    : DeletableRolloutContext(ServiceContextType, isForceDelete)
    , applicationId_(appId)
    , applicationName_(appName)
    , absoluteServiceName_(absoluteServiceName)
    , serviceName_(GetRelativeServiceName(appName.ToString(), absoluteServiceName.ToString()))
    , typeName_()
    , packageName_()
    , packageVersion_()
    , packageInstance_(0)
    , partitionedServiceDescriptor_()
    , isDefaultService_(false)
    , isCommitPending_(false)
{
}

ServiceContext::ServiceContext(
    ServiceContext const & other)
    : DeletableRolloutContext(other)
    , applicationId_(other.applicationId_)
    , applicationName_(other.applicationName_)
    , absoluteServiceName_(other.absoluteServiceName_)
    , serviceName_(other.serviceName_)
    , typeName_(other.typeName_)
    , packageName_(other.packageName_)
    , packageVersion_(other.packageVersion_)
    , packageInstance_(other.packageInstance_)
    , partitionedServiceDescriptor_(other.partitionedServiceDescriptor_)
    , isDefaultService_(other.isDefaultService_)
    , isCommitPending_(false)
{
}

ServiceContext::ServiceContext(
    ServiceContext && other)
    : DeletableRolloutContext(move(other))
    , applicationId_(move(other.applicationId_))
    , applicationName_(move(other.applicationName_))
    , absoluteServiceName_(move(other.absoluteServiceName_))
    , serviceName_(move(other.serviceName_))
    , typeName_(move(other.typeName_))
    , packageName_(move(other.packageName_))
    , packageVersion_(move(other.packageVersion_))
    , packageInstance_(move(other.packageInstance_))
    , partitionedServiceDescriptor_(move(other.partitionedServiceDescriptor_))
    , isDefaultService_(move(other.isDefaultService_))
    , isCommitPending_(false)
{
}

ServiceContext & ServiceContext::operator=(
    ServiceContext && other)
{
    if (this != &other)
    {
        __super::operator = (move(other));
        applicationId_ = move(other.applicationId_);
        applicationName_ = move(other.applicationName_);
        absoluteServiceName_ = move(other.absoluteServiceName_);
        serviceName_ = move(other.serviceName_);
        typeName_ = move(other.typeName_);
        packageName_ = move(other.packageName_);
        packageVersion_ = move(other.packageVersion_);
        packageInstance_ = move(other.packageInstance_);
        partitionedServiceDescriptor_ = move(other.partitionedServiceDescriptor_);
        isDefaultService_ = move(other.isDefaultService_);
        isCommitPending_ = false;
    }

    return *this;
}

ServiceContext::ServiceContext(
    Store::ReplicaActivityId const & replicaActivityId,
    ServiceModelApplicationId const & appId,
    ServiceModelPackageName const & packageName,
    ServiceModelVersion const & packageVersion,
    uint64 packageInstance,
    Naming::PartitionedServiceDescriptor const & psd)
    : DeletableRolloutContext(ServiceContextType, replicaActivityId)
    , applicationId_(appId)
    , applicationName_(StringToNamingUri(psd.Service.ApplicationName))
    , absoluteServiceName_(StringToNamingUri(psd.Service.Name))
    , serviceName_(GetRelativeServiceName(psd.Service.ApplicationName, psd.Service.Name))
    , typeName_(psd.Service.Type.ServiceTypeName)
    , packageName_(packageName)
    , packageVersion_(packageVersion)
    , packageInstance_(packageInstance)
    , partitionedServiceDescriptor_(psd)
    , isDefaultService_(true)
    , isCommitPending_(false)
{
    // Qualify service type for internal create service calls
    partitionedServiceDescriptor_.MutableService.Type = ServiceTypeIdentifier(
        ServicePackageIdentifier(
            applicationId_.Value, 
            packageName_.Value),
        typeName_.Value);

    WriteNoise(
        TraceComponent, 
        "{0} ctor({1}, {2}, {3}, {4}, [{5}:{6}:{7}] PSD = {8}) : DefaultService",
        this->TraceId,
        applicationId_,
        applicationName_,
        absoluteServiceName_,
        typeName_,
        packageName_,
        packageVersion_,
        packageInstance_,
        partitionedServiceDescriptor_);
}

ServiceContext::ServiceContext(
    Common::ComponentRoot const & replica,
    ClientRequestSPtr const & clientRequest,
    ServiceModelApplicationId const & appId,
    ServiceModelPackageName const & packageName,
    ServiceModelVersion const & packageVersion,
    uint64 packageInstance,
    Naming::PartitionedServiceDescriptor const & psd)
    : DeletableRolloutContext(ServiceContextType, replica, clientRequest)
    , applicationId_(appId)
    , applicationName_(StringToNamingUri(psd.Service.ApplicationName))
    , absoluteServiceName_(StringToNamingUri(psd.Service.Name))
    , serviceName_(GetRelativeServiceName(psd.Service.ApplicationName, psd.Service.Name))
    , typeName_(psd.Service.Type.ServiceTypeName)
    , packageName_(packageName)
    , packageVersion_(packageVersion)
    , packageInstance_(packageInstance)
    , partitionedServiceDescriptor_(psd)
    , isDefaultService_(false)
    , isCommitPending_(false)
{
    // Qualify service type for internal create service calls
    partitionedServiceDescriptor_.MutableService.Type = ServiceTypeIdentifier(
        ServicePackageIdentifier(
            applicationId_.Value, 
            packageName_.Value),
        typeName_.Value);

    WriteNoise(
        TraceComponent, 
        "{0} ctor({1}, {2}, {3}, {4}, [{5}:{6}:{7}] PSD = {8})",
        this->TraceId,
        applicationId_,
        applicationName_,
        absoluteServiceName_,
        typeName_,
        packageName_,
        packageVersion_,
        packageInstance_,
        partitionedServiceDescriptor_);
}

ServiceContext::~ServiceContext()
{
    if (replicaSPtr_)
    {
        WriteNoise(
            TraceComponent,
            "{0} ~dtor",
            this->TraceId);
    }
}

ServiceQueryResult ServiceContext::ToQueryResult(wstring const & serviceTypeManifestVersion) const
{
    FABRIC_QUERY_SERVICE_STATUS serviceStatus = GetQueryStatus();
    bool isStateful = this->ServiceDescriptor.Service.IsStateful;

    if (isStateful)
    {
        return ServiceQueryResult::CreateStatefulServiceQueryResult(
            this->AbsoluteServiceName,
            this->ServiceTypeName.Value,
            serviceTypeManifestVersion,
            this->ServiceDescriptor.Service.HasPersistedState,
            serviceStatus);
    }
    else
    {
        return ServiceQueryResult::CreateStatelessServiceQueryResult(
            this->AbsoluteServiceName,
            this->ServiceTypeName.Value,
            serviceTypeManifestVersion,
            serviceStatus);
    }
}

FABRIC_QUERY_SERVICE_STATUS ServiceContext::GetQueryStatus() const
{
    FABRIC_QUERY_SERVICE_STATUS serviceStatus;
    if (this->IsPending)
    {
        serviceStatus = FABRIC_QUERY_SERVICE_STATUS_CREATING;
    }
    else if (this->IsDeleting)
    {
        serviceStatus = FABRIC_QUERY_SERVICE_STATUS_DELETING;
    }
    else if (this->IsFailed)
    {
        serviceStatus = FABRIC_QUERY_SERVICE_STATUS_FAILED;
    }
    else
    {
        serviceStatus = FABRIC_QUERY_SERVICE_STATUS_UNKNOWN;
    }

    return serviceStatus;
}

Common::ErrorCode ServiceContext::ReadApplicationServices(
    StoreTransaction const & storeTx, 
    ServiceModelApplicationId const & appId, 
    __out std::vector<ServiceContext> & contexts)
{
    return storeTx.ReadPrefix(Constants::StoreType_ServiceContext, ApplicationContext::GetKeyPrefix(appId), contexts);
}

std::wstring const & ServiceContext::get_Type() const
{
    return Constants::StoreType_ServiceContext;
}

std::wstring ServiceContext::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}{1}",
        ApplicationContext::GetKeyPrefix(applicationId_),
        serviceName_);
    return temp;
}

void ServiceContext::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "ServiceContext({0})[{1}, {2}, {3}, {4}, {5}]", 
        this->Status, 
        applicationId_, 
        applicationName_, 
        serviceName_, 
        typeName_, 
        isDefaultService_);
}

NamingUri ServiceContext::StringToNamingUri(std::wstring const & name)
{
    NamingUri namingUri;
    bool success = NamingUri::TryParse(name, namingUri);
    ASSERT_IFNOT(success, "Could not parse {0} into a NamingUri", name);

    return namingUri;
}

ServiceModelServiceName ServiceContext::GetRelativeServiceName(std::wstring const & appName, std::wstring const & absoluteServiceName)
{
    wstring serviceName(absoluteServiceName);

    StringUtility::Replace<wstring>(serviceName, appName, L"");
    StringUtility::Trim<wstring>(serviceName, L"/");

    return ServiceModelServiceName(serviceName);
}

