// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ContainerDescription::ContainerDescription()
    : applicationName_(),
    serviceName_(),
    applicationId_(),
    containerName_(),
    deploymentFolder_(),
    nodeWorkFolder_(),
    isolationMode_(ContainerIsolationMode::Enum::process),
    hostName_(),
    portBindings_(),
    logConfig_(),
    volumes_(),
    assignedIp_(),
    dnsServers_(),
    repositoryCredentials_(),
    healthConfig_(),
    securityOptions_(),
    entryPoint_(),
    groupContainerName_(),
    autoRemove_(true),
    runInteractive_(false),
    isContainerRoot_(false),
    codePackageName_(),
    servicePackageActivationId_(),
    partitionId_()
{
}

ContainerDescription::ContainerDescription(
    std::wstring const & applicationName,
    std::wstring const & serviceName,
    std::wstring const & applicationId,
    std::wstring const & containerName,
    std::wstring const & entryPoint,
    ContainerIsolationMode::Enum const & isolationMode,
    std::wstring const & hostName,
    std::wstring const & deploymentFolder,
    std::wstring const & nodeWorkFolder,
    std::wstring const & assignedIp,
    std::map<std::wstring, std::wstring> const & portBindings,
    LogConfigDescription const & logConfig,
    vector<ContainerVolumeDescription> const & volumes,
    vector<wstring> const & dnsServers,
    RepositoryCredentialsDescription const & repositoryCredentials,
    ContainerHealthConfigDescription const & healthConfig,
    vector<wstring> const & securityOptions,
    wstring const & groupContainerName,
    bool autoRemove,
    bool runInteractive,
    bool isContainerRoot,
    wstring const & codePackageName,
    wstring const & servicePackageActivationId,
    wstring const & partitionId)
    : applicationName_(applicationName),
    serviceName_(serviceName),
    applicationId_(applicationId),
    containerName_(containerName),
    entryPoint_(entryPoint),
    isolationMode_(isolationMode),
    hostName_(hostName),
    deploymentFolder_(deploymentFolder),
    nodeWorkFolder_(nodeWorkFolder),
    assignedIp_(assignedIp),
    portBindings_(portBindings),
    logConfig_(logConfig),
    volumes_(volumes),
    dnsServers_(dnsServers),
    repositoryCredentials_(repositoryCredentials),
    healthConfig_(healthConfig),
    securityOptions_(securityOptions),
    groupContainerName_(groupContainerName),
    autoRemove_(autoRemove),
    runInteractive_(runInteractive),
    isContainerRoot_(isContainerRoot),
    codePackageName_(codePackageName),
    servicePackageActivationId_(servicePackageActivationId),
    partitionId_(partitionId)
{
}

ContainerDescription::ContainerDescription(ContainerDescription const & other)
    : applicationName_(other.applicationName_),
    serviceName_(other.serviceName_),
    applicationId_(other.applicationId_),
    containerName_(other.containerName_),
    entryPoint_(other.entryPoint_),
    isolationMode_(other.isolationMode_),
    hostName_(other.hostName_),
    deploymentFolder_(other.deploymentFolder_),
    nodeWorkFolder_(other.nodeWorkFolder_),
    portBindings_(other.portBindings_),
    logConfig_(other.logConfig_),
    volumes_(other.volumes_),
    assignedIp_(other.assignedIp_),
    dnsServers_(other.dnsServers_),
    repositoryCredentials_(other.repositoryCredentials_),
    healthConfig_(other.healthConfig_),
    securityOptions_(other.securityOptions_),
    groupContainerName_(other.groupContainerName_),
    autoRemove_(other.autoRemove_),
    runInteractive_(other.runInteractive_),
    isContainerRoot_(other.isContainerRoot_),
    codePackageName_(other.codePackageName_),
    servicePackageActivationId_(other.servicePackageActivationId_),
    partitionId_(other.partitionId_)
{
}

ContainerDescription::ContainerDescription(ContainerDescription && other)
    : applicationName_(move(other.applicationName_)),
    serviceName_(move(other.serviceName_)),
    applicationId_(move(other.applicationId_)),
    containerName_(move(other.containerName_)),
    entryPoint_(move(other.entryPoint_)),
    isolationMode_(move(other.isolationMode_)),
    hostName_(move(other.hostName_)),
    deploymentFolder_(move(other.deploymentFolder_)),
    nodeWorkFolder_(move(other.nodeWorkFolder_)),
    portBindings_(move(other.portBindings_)),
    logConfig_(move(other.logConfig_)),
    volumes_(move(other.volumes_)),
    assignedIp_(move(other.assignedIp_)),
    dnsServers_(move(other.dnsServers_)),
    repositoryCredentials_(move(other.repositoryCredentials_)),
    healthConfig_(move(other.healthConfig_)),
    securityOptions_(move(other.securityOptions_)),
    groupContainerName_(move(other.groupContainerName_)),
    autoRemove_(other.autoRemove_),
    runInteractive_(other.runInteractive_),
    isContainerRoot_(other.isContainerRoot_),
    codePackageName_(move(other.codePackageName_)),
    servicePackageActivationId_(move(other.servicePackageActivationId_)),
    partitionId_(move(other.partitionId_))
{
}

ContainerDescription::~ContainerDescription()
{
}

ContainerDescription const & ContainerDescription::operator=(ContainerDescription const & other)
{
    if(this != &other)
    {
        this->applicationName_ = other.applicationName_;
        this->serviceName_ = other.serviceName_;
        this->applicationId_ = other.applicationId_;
        this->containerName_ = other.containerName_;
        this->entryPoint_ = other.entryPoint_;
        this->isolationMode_ = other.isolationMode_;
        this->hostName_ = other.hostName_;
        this->deploymentFolder_ = other.deploymentFolder_;
        this->nodeWorkFolder_ = other.nodeWorkFolder_;
        this->portBindings_ = other.portBindings_;
        this->logConfig_ = other.logConfig_;
        this->volumes_ = other.volumes_;
        this->assignedIp_ = other.assignedIp_;
        this->dnsServers_ = other.dnsServers_;
        this->repositoryCredentials_ = other.repositoryCredentials_;
        this->healthConfig_ = other.healthConfig_;
        this->securityOptions_ = other.securityOptions_;
        this->groupContainerName_ = other.groupContainerName_;
        this->autoRemove_ = other.autoRemove_;
        this->runInteractive_ = other.runInteractive_;
        this->isContainerRoot_ = other.isContainerRoot_;
        this->codePackageName_ = other.codePackageName_;
        this->servicePackageActivationId_ = other.servicePackageActivationId_;
        this->partitionId_ = other.partitionId_;
    }
    return *this;
}

ContainerDescription const & ContainerDescription::operator=(ContainerDescription && other)
{
    if(this != &other)
    {
        this->applicationName_ = move(other.applicationName_);
        this->serviceName_ = move(other.serviceName_);
        this->applicationId_ = move(other.applicationId_);
        this->containerName_ = move(other.containerName_);
        this->entryPoint_ = move(other.entryPoint_);
        this->isolationMode_ = move(other.isolationMode_);
        this->hostName_ = move(other.hostName_);
        this->deploymentFolder_ = move(other.deploymentFolder_);
        this->nodeWorkFolder_ = move(other.nodeWorkFolder_);
        this->portBindings_ = move(other.portBindings_);
        this->logConfig_ = move(other.logConfig_);
        this->volumes_ = move(other.volumes_);
        this->assignedIp_ = move(other.assignedIp_);
        this->dnsServers_ = move(other.dnsServers_);
        this->repositoryCredentials_ = move(other.repositoryCredentials_);
        this->healthConfig_ = move(other.healthConfig_);
        this->securityOptions_ = move(other.securityOptions_);
        this->groupContainerName_ = move(other.groupContainerName_);
        this->autoRemove_ = other.autoRemove_;
        this->runInteractive_ = other.runInteractive_;
        this->isContainerRoot_ = other.isContainerRoot_;
        this->codePackageName_ = move(other.codePackageName_);
        this->servicePackageActivationId_ = move(other.servicePackageActivationId_);
        this->partitionId_ = move(other.partitionId_);
    }
    return *this;
}

void ContainerDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerDescription { ");
    w.Write("ApplicationName = {0}", ApplicationName);
    w.Write("ServiceName = {0}", serviceName_);
    w.Write("ApplicationId = {0}", ApplicationId);
    w.Write("ContainerName = {0}, ", ContainerName);
    w.Write("EntryPoint = {0}", EntryPoint);
    w.Write("IsolationMode = {0}", IsolationMode);
    w.Write("Hostname = {0}", hostName_);
    w.Write("DeploymentFolder = {0}, ", DeploymentFolder);
    w.Write("NodeWorkingFolder = {0}, ", NodeWorkFolder);
    w.Write("LogConfig = {0}", LogConfig);
    w.Write("AssignedIP = {0}", assignedIp_);
    w.Write("GroupContainerName = {0}", groupContainerName_);
    w.Write("AutoRemove = {0}", autoRemove_);
    w.Write("RunInteractive = {0}", runInteractive_);
    w.Write("IsContainerRoot = {0}", isContainerRoot_);
    w.Write("HealthConfig = {0}", healthConfig_);
    w.Write("CodePackageName = {0}", codePackageName_);
    w.Write("ServicePackageActivationId = {0}", servicePackageActivationId_);
    w.Write("PartitionId = {0}", partitionId_);
    w.Write("}");
}

ErrorCode ContainerDescription::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_CONTAINER_DESCRIPTION & fabricContainerDescription) const
{
    fabricContainerDescription.ApplicationName = heap.AddString(this->ApplicationName);
    fabricContainerDescription.ServiceName = heap.AddString(this->ServiceName);
    fabricContainerDescription.ApplicationId = heap.AddString(this->ApplicationId);
    fabricContainerDescription.ContainerName = heap.AddString(this->ContainerName);
    fabricContainerDescription.DeploymentFolder = heap.AddString(this->DeploymentFolder);
    fabricContainerDescription.NodeWorkFolder = heap.AddString(this->NodeWorkFolder);
    fabricContainerDescription.AssignedIp = heap.AddString(this->AssignedIP);
    fabricContainerDescription.EntryPoint = heap.AddString(this->EntryPoint);
    fabricContainerDescription.HostName = heap.AddString(this->Hostname);
    fabricContainerDescription.GroupContainerName = heap.AddString(this->GroupContainerName);
    fabricContainerDescription.IsolationMode = ContainerIsolationMode::ToPublicApi(this->IsolationMode);

    auto portBindings = heap.AddItem<FABRIC_STRING_PAIR_LIST>();
    auto error = PublicApiHelper::ToPublicApiStringPairList(heap, this->PortBindings, *portBindings);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricContainerDescription.PortBindings = portBindings.GetRawPointer();

    auto logConfig = heap.AddItem<FABRIC_CONTAINER_LOG_CONFIG_DESCRIPTION>();
    error = this->LogConfig.ToPublicApi(heap, *logConfig);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricContainerDescription.LogConfig = logConfig.GetRawPointer();

    auto volumes = heap.AddItem<FABRIC_CONTAINER_VOLUME_DESCRIPTION_LIST>();
    error = PublicApiHelper::ToPublicApiList<
        ContainerVolumeDescription,
        FABRIC_CONTAINER_VOLUME_DESCRIPTION,
        FABRIC_CONTAINER_VOLUME_DESCRIPTION_LIST>(heap, this->ContainerVolumes, *volumes);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricContainerDescription.Volumes = volumes.GetRawPointer();

    auto dnsServers = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, this->DnsServers, dnsServers);
    fabricContainerDescription.DnsServers = dnsServers.GetRawPointer();

    auto securityOptions = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, this->SecurityOptions, securityOptions);
    fabricContainerDescription.SecurityOptions = securityOptions.GetRawPointer();

    auto  repoCredential = heap.AddItem<FABRIC_REPOSITORY_CREDENTIAL_DESCRIPTION>();
    error = this->RepositoryCredentials.ToPublicApi(heap, *repoCredential);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricContainerDescription.RepositoryCredential = repoCredential.GetRawPointer();

    auto  healthConfig = heap.AddItem<FABRIC_CONTAINER_HEALTH_CONFIG_DESCRIPTION>();
    error = this->HealthConfig.ToPublicApi(heap, *healthConfig);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricContainerDescription.HealthConfig = healthConfig.GetRawPointer();

    fabricContainerDescription.AutoRemove = this->AutoRemove;
    fabricContainerDescription.RunInteractive = this->RunInteractive;
    fabricContainerDescription.IsContainerRoot = this->IsContainerRoot;
    fabricContainerDescription.CodePackageName = heap.AddString(this->CodePackageName);
    fabricContainerDescription.ServicePackageActivationId = heap.AddString(this->ServicePackageActivationId);
    fabricContainerDescription.PartitionId = heap.AddString(this->PartitionId);

    fabricContainerDescription.Reserved = nullptr;

    return ErrorCode::Success();
}

