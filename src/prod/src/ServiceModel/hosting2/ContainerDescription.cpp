// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType_ContainerDescription("ContainerDescription");

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
    labels_(),
    assignedIp_(),
    dnsServers_(),
    repositoryCredentials_(),
    healthConfig_(),
    networkConfig_(),
    securityOptions_(),
    entryPoint_(),
    removeServiceFabricRuntimeAccess_(true),
#if defined(PLATFORM_UNIX)
    podDescription_(),
#endif
    groupContainerName_(),
    useDefaultRepositoryCredentials_(false),
    useTokenAuthenticationCredentials_(false),
    autoRemove_(true),
    runInteractive_(false),
    isContainerRoot_(false),
    codePackageName_(),
    servicePackageActivationId_(),
    partitionId_(),
    bindMounts_()
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
    std::map<std::wstring, std::wstring> const & portBindings,
    LogConfigDescription const & logConfig,
    vector<ContainerVolumeDescription> const & volumes,
    vector<ContainerLabelDescription> const & labels,
    vector<wstring> const & dnsServers,
    RepositoryCredentialsDescription const & repositoryCredentials,
    ContainerHealthConfigDescription const & healthConfig,
    ContainerNetworkConfigDescription const & networkConfig,
    vector<wstring> const & securityOptions,
#if defined(PLATFORM_UNIX)
    ContainerPodDescription const & podDesc,
#endif
    bool removeServiceFabricRuntimeAccess,
    wstring const & groupContainerName,
    bool useDefaultRepositoryCredentials,
    bool useTokenAuthenticationCredentials,
    bool autoRemove,
    bool runInteractive,
    bool isContainerRoot,
    wstring const & codePackageName,
    wstring const & servicePackageActivationId,
    wstring const & partitionId,
    map<wstring, wstring> const& bindMounts)
    : applicationName_(applicationName),
    serviceName_(serviceName),
    applicationId_(applicationId),
    containerName_(containerName),
    entryPoint_(entryPoint),
    isolationMode_(isolationMode),
    useDefaultRepositoryCredentials_(useDefaultRepositoryCredentials),
    useTokenAuthenticationCredentials_(useTokenAuthenticationCredentials),
    hostName_(hostName),
    deploymentFolder_(deploymentFolder),
    nodeWorkFolder_(nodeWorkFolder),
    assignedIp_(),
    portBindings_(portBindings),
    logConfig_(logConfig),
    volumes_(volumes),
    labels_(labels),
    dnsServers_(dnsServers),
    repositoryCredentials_(repositoryCredentials),
    healthConfig_(healthConfig),
    networkConfig_(networkConfig),
    securityOptions_(securityOptions),
    removeServiceFabricRuntimeAccess_(removeServiceFabricRuntimeAccess),
#if defined(PLATFORM_UNIX)
    podDescription_(podDesc),
#endif
    groupContainerName_(groupContainerName),
    autoRemove_(autoRemove),
    runInteractive_(runInteractive),
    isContainerRoot_(isContainerRoot),
    codePackageName_(codePackageName),
    servicePackageActivationId_(servicePackageActivationId),
    partitionId_(partitionId),
    bindMounts_(bindMounts)
{
}

void ContainerDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerDescription { ");
    w.Write("ApplicationName = {0} ", ApplicationName);
    w.Write("ServiceName = {0} ", serviceName_);
    w.Write("ApplicationId = {0} ", ApplicationId);
    w.Write("ContainerName = {0} ", ContainerName);
    w.Write("EntryPoint = {0} ", EntryPoint);
    w.Write("IsolationMode = {0} ", IsolationMode);
    w.Write("Hostname = {0} ", hostName_);
    w.Write("DeploymentFolder = {0} ", DeploymentFolder);
    w.Write("NodeWorkingFolder = {0} ", NodeWorkFolder);
    w.Write("LogConfig = {0} ", LogConfig);
    w.Write("RemoveServiceFabricRuntimeAccess = {0} ", removeServiceFabricRuntimeAccess_);
#if defined(PLATFORM_UNIX)
    w.Write("ContainerPodDescription = {0} ", podDescription_);
#endif
    w.Write("GroupContainerName = {0} ", groupContainerName_);
    w.Write("UseDefaultRepositoryCredentials = {0} ", UseDefaultRepositoryCredentials);
    w.Write("UseTokenAuthenticationCredentials = {0} ", UseTokenAuthenticationCredentials);
    w.Write("AutoRemove = {0} ", autoRemove_);
    w.Write("RunInteractive = {0} ", runInteractive_);
    w.Write("IsContainerRoot = {0} ", isContainerRoot_);
    w.Write("HealthConfig = {0} ", healthConfig_);
    w.Write("CodePackageName = {0} ", codePackageName_);
    w.Write("ServicePackageActivationId = {0} ", servicePackageActivationId_);
    w.Write("PartitionId = {0} ", partitionId_);
    w.Write("Container Network Config = {0} ", networkConfig_);
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


    auto fabricContainerDescEx1 = heap.AddItem<FABRIC_CONTAINER_DESCRIPTION_EX1>();
    fabricContainerDescEx1->UseDefaultRepositoryCredentials = this->UseDefaultRepositoryCredentials;

    auto fabricContainerDescEx2 = heap.AddItem<FABRIC_CONTAINER_DESCRIPTION_EX2>();
    fabricContainerDescEx2->UseTokenAuthenticationCredentials = this->UseTokenAuthenticationCredentials;

    auto fabricContainerDescEx3 = heap.AddItem<FABRIC_CONTAINER_DESCRIPTION_EX3>();

    auto labels = heap.AddItem<FABRIC_CONTAINER_LABEL_DESCRIPTION_LIST>();
    error = PublicApiHelper::ToPublicApiList<
        ContainerLabelDescription,
        FABRIC_CONTAINER_LABEL_DESCRIPTION,
        FABRIC_CONTAINER_LABEL_DESCRIPTION_LIST>(heap, this->ContainerLabels, *labels);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricContainerDescEx1->Labels = labels.GetRawPointer();
    fabricContainerDescEx1->RemoveServiceFabricRuntimeAccess = this->removeServiceFabricRuntimeAccess_;

    auto bindMounts = heap.AddItem<FABRIC_STRING_PAIR_LIST>();
    error = PublicApiHelper::ToPublicApiStringPairList(heap, this->BindMounts, *bindMounts);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricContainerDescEx1->BindMounts = bindMounts.GetRawPointer();

    auto containerNetworkConfig = heap.AddItem<FABRIC_CONTAINER_NETWORK_CONFIG_DESCRIPTION>();
    error = this->NetworkConfig.ToPublicApi(heap, *containerNetworkConfig);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricContainerDescEx3->ContainerNetworkConfigDescription = containerNetworkConfig.GetRawPointer();

    fabricContainerDescription.Reserved = fabricContainerDescEx1.GetRawPointer();
    fabricContainerDescEx1->Reserved = fabricContainerDescEx2.GetRawPointer();
    fabricContainerDescEx2->Reserved = fabricContainerDescEx3.GetRawPointer();
    fabricContainerDescEx3->Reserved = nullptr;

    return ErrorCode::Success();
}

ContainerDescription::~ContainerDescription()
{
    WriteInfo(TraceType_ContainerDescription, "Destructing ContainerDescription.");
};