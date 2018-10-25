// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ContainerConfig.h"

using namespace Hosting2;
using namespace Common;
using namespace ServiceModel;

Common::WStringLiteral const ContainerConfig::ImageParameter(L"Image");
Common::WStringLiteral const ContainerConfig::EntryPointParameter(L"Entrypoint");
Common::WStringLiteral const ContainerConfig::CmdParameter(L"Cmd");
Common::WStringLiteral const ContainerConfig::EnvParameter(L"Env");
Common::WStringLiteral const ContainerConfig::HostConfigParameter(L"HostConfig");
Common::WStringLiteral const ContainerConfig::LabelsParameter(L"Labels");
Common::WStringLiteral const ContainerConfig::ExposedPortsParameter(L"ExposedPorts");
Common::WStringLiteral const ContainerConfig::NetworkingConfigParameter(L"NetworkingConfig");
Common::WStringLiteral const ContainerConfig::HostnameParameter(L"Hostname");
Common::WStringLiteral const ContainerConfig::LogPathParameter(L"LogPath");
Common::WStringLiteral const ContainerConfig::TtyParameter(L"Tty");
Common::WStringLiteral const ContainerConfig::OpenStdinParameter(L"OpenStdin");
Common::WStringLiteral const ContainerConfig::UserLogsDirectoryEnvVarName(L"UserLogsDirectory");

Common::WStringLiteral const GetContainerResponse::StateParameter(L"State");
Common::WStringLiteral const ContainerState::ErrorParameter(L"Error");

Common::WStringLiteral const ContainerState::PidParameter(L"Pid");
Common::WStringLiteral const ContainerState::IsDeadParameter(L"Dead");

Common::GlobalWString const ContainerConfig::FabricPackageFileNameEnvironment = make_global<wstring>(L"FabricPackageFileName");
// Starting from this version there is no need to limit cpu to 1% of total machine capacity
double const ContainerConfig::DockerVersionThreshold = 17.06;

uint64 const MegaBytesToBytesMultiplier = 1024 * 1024;

ContainerConfig::ContainerConfig() :
    Image(),
    Env(),
    Cmd(),
    EntryPoint(),
    ExposedPorts(),
    NetworkConfig(),
    HostConfig(),
    Hostname(),
    Tty(false),
    OpenStdin(false)
{
}

ContainerConfig::ContainerConfig(ContainerConfig && other) :
    Image(move(other.Image)),
    Env(move(other.Env)),
    Cmd(move(other.Cmd)),
    EntryPoint(move(other.EntryPoint)),
    ExposedPorts(move(other.ExposedPorts)),
    NetworkConfig(move(other.NetworkConfig)),
    HostConfig(move(other.HostConfig)),
    Hostname(move(other.Hostname)),
    Tty(other.Tty),
    OpenStdin(other.OpenStdin)
{
}

ContainerConfig::~ContainerConfig()
{
}

ContainerConfig const & ContainerConfig::operator= (ContainerConfig && other)
{
    if (this != &other)
    {
        this->Image = move(other.Image);
        this->Env = move(other.Env);
        this->Cmd = move(other.Cmd);
        this->EntryPoint = move(other.EntryPoint);
        this->ExposedPorts = move(other.ExposedPorts);
        this->HostConfig = move(other.HostConfig);
        this->NetworkConfig = move(other.NetworkConfig);
        this->Hostname = move(other.Hostname);
        this->Tty = other.Tty;
        this->OpenStdin = other.OpenStdin;
    }

    return *this;
}

void ContainerConfig::AddVolumesToBinds(
    vector<wstring> & binds,
    vector<ContainerVolumeDescription> const & volumeDescriptions)
{
    for (auto const & volumeDescription : volumeDescriptions)
    {
        if (volumeDescription.IsReadOnly)
        {
            binds.push_back(
                wformatString(
                    "{0}:{1}:ro",
                    volumeDescription.Source,
                    volumeDescription.Destination));
        }
        else
        {
            binds.push_back(
                wformatString(
                    "{0}:{1}",
                    volumeDescription.Source,
                    volumeDescription.Destination));
        }
    }
}

// This function, as well as the ContainerConfig class, is only used by ClearContainers support
ErrorCode ContainerConfig::CreateContainerConfig(
    vector<ContainerVolumeDescription> const & localVolumes,
    vector<ContainerVolumeDescription> const & mappedVolumes,
    ProcessDescription const & processDescription,
    ContainerDescription const & containerDescription,
    std::wstring const & dockerVersion,
    ContainerConfig & config)
{
    config.Image = processDescription.ExePath;
    vector<wstring> envVars;
    wstring packageFilePath;
    wstring containerPackageFilePath;
    auto containerWFRoot = HostingConfig::GetConfig().ContainerPackageRootFolder;

    if (!processDescription.EnvironmentBlock.empty())
    {
        EnvironmentMap envMap;
        vector<wchar_t> envBlock = processDescription.EnvironmentBlock;
        LPVOID penvBlock = envBlock.data();
        Environment::FromEnvironmentBlock(penvBlock, envMap);

        for (auto iter = envMap.begin(); iter != envMap.end(); iter++)
        {
            wstring result = wformatString("{0}={1}", iter->first, iter->second);
            envVars.push_back(result);
        }
    }

    wstring fabricCodePath;
    auto error = FabricEnvironment::GetFabricCodePath(fabricCodePath);
    if (!error.IsSuccess())
    {
        return error;
    }

    wstring fabricLogRoot;
    error = FabricEnvironment::GetFabricLogRoot(fabricLogRoot);
    if (!error.IsSuccess())
    {
        return error;
    }

    wstring fabricContainerLogRoot = Path::Combine(fabricLogRoot, L"Containers");
    if (!Directory::Exists(fabricContainerLogRoot))
    {
        error = Directory::Create2(fabricContainerLogRoot);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    wstring fabricContainerRoot = Path::Combine(fabricContainerLogRoot, containerDescription.ContainerName);
    error = Directory::Create2(fabricContainerRoot);
    if (!error.IsSuccess())
    {
        return error;
    }

    wstring fabricContainerTraceRoot = Path::Combine(fabricContainerRoot, L"Traces");
    error = Directory::Create2(fabricContainerTraceRoot);
    if (!error.IsSuccess())
    {
        return error;
    }

    wstring fabricContainerQueryTraceRoot = Path::Combine(fabricContainerRoot, L"QueryTraces");
    error = Directory::Create2(fabricContainerQueryTraceRoot);
    if (!error.IsSuccess())
    {
        return error;
    }

    auto resourceGovPolicy = processDescription.ResourceGovernancePolicy;
    vector<wstring> binds;

    // Set path to UT file
    // TODO: Bug#9728016 - Disable the bind until windows supports mounting file onto container
    // auto pathToHostUnreliableTransportSettings = Path::Combine(containerDescription.NodeWorkFolder, Transport::UnreliableTransportConfig::SettingsFileName);

    auto binRoot = wformatString("FabricCodePath={0}", fabricCodePath);
    envVars.push_back(binRoot);
    auto deploymentFolderBind = wformatString("{0}:{1}", processDescription.AppDirectory, processDescription.AppDirectory);
    if (!packageFilePath.empty())
    {
        auto configDir = Path::GetDirectoryName(packageFilePath);
        auto configPath = wformatString("{0}:{1}:ro", configDir, configDir);
        binds.push_back(configPath);
    }

    binds.push_back(deploymentFolderBind);

    if (!fabricCodePath.empty())
    {
        auto binBinding = wformatString("{0}:{1}:ro", fabricCodePath, fabricCodePath);
        binds.push_back(binBinding);
    }

    if (!fabricContainerRoot.empty())
    {
        auto binBinding = wformatString("{0}:{1}", fabricContainerRoot, fabricContainerRoot);
        binds.push_back(binBinding);
    }

    auto logRoot = wformatString("FabricLogRoot={0}", fabricContainerRoot);
    envVars.push_back(logRoot);

    // Container Log Path
#if defined(PLATFORM_UNIX)

    auto containerInstanceId = ClearContainerHelper::GetNewContainerInstanceIdAtomic(containerDescription, TimeSpan::MaxValue);
    auto logPath = ClearContainerHelper::GetContainerRelativeLogPath(containerDescription, containerInstanceId);
    auto logDirectory = ClearContainerHelper::GetSandboxLogDirectory(containerDescription);
    auto logMountDirectory = ClearContainerHelper::GetLogMountDirectory(containerDescription);
    auto fullLogPath = ClearContainerHelper::GetContainerFullLogPath(containerDescription, containerInstanceId);

    auto podLogDirectory = wformatString("{0}={1}", ContainerConfig::UserLogsDirectoryEnvVarName, logMountDirectory);
    envVars.push_back(podLogDirectory);
    config.LogPath = logPath;

    // Labels
    for (auto it = containerDescription.ContainerLabels.begin(); it != containerDescription.ContainerLabels.end(); ++it)
    {
        config.Labels.insert(make_pair(it->Name, it->Value));
    }
    config.Labels.insert(make_pair(Constants::ContainerLabels::ApplicationNameLabelKeyName, containerDescription.ApplicationName));
    config.Labels.insert(make_pair(Constants::ContainerLabels::ApplicationIdLabelKeyName, containerDescription.ApplicationId));
    config.Labels.insert(make_pair(Constants::ContainerLabels::DigestedApplicationNameLabelKeyName, ClearContainerHelper::GetDigestedApplicationName(containerDescription)));
    config.Labels.insert(make_pair(Constants::ContainerLabels::ServiceNameLabelKeyName, containerDescription.ServiceName));
    config.Labels.insert(make_pair(Constants::ContainerLabels::SimpleApplicationNameLabelKeyName, ClearContainerHelper::GetSimpleApplicationName(containerDescription)));
    config.Labels.insert(make_pair(Constants::ContainerLabels::CodePackageNameLabelKeyName, containerDescription.CodePackageName));
    config.Labels.insert(make_pair(Constants::ContainerLabels::CodePackageInstanceLabelKeyName, std::to_wstring(containerInstanceId)));
    config.Labels.insert(make_pair(Constants::ContainerLabels::ServicePackageActivationIdLabelKeyName, containerDescription.ServicePackageActivationId));
    config.Labels.insert(make_pair(Constants::ContainerLabels::PartitionIdLabelKeyName, containerDescription.PartitionId));
    config.Labels.insert(make_pair(Constants::ContainerLabels::PlatformLabelKeyName, Constants::ContainerLabels::PlatformLabelKeyValue));
    auto queryFilterValue = wformatString(L"{0}_{1}_{2}",
                                          containerDescription.ServiceName,
                                          containerDescription.CodePackageName,
                                          containerDescription.PartitionId);
    config.Labels.insert(make_pair(Constants::ContainerLabels::SrppQueryFilterLabelKeyName, queryFilterValue));
#endif
    config.HostConfig.NetworkMode = L"host";

    if (!containerDescription.PortBindings.empty())
    {
        config.HostConfig.NetworkMode = L"bridge";
    }

    if (resourceGovPolicy.BlockIOWeight > 0)
    {
        config.HostConfig.BlockIOWeight = processDescription.ResourceGovernancePolicy.BlockIOWeight;
    }

    // Mount the UT settings file
    // TODO: Bug#9728016 - Disable the bind until windows supports mounting file onto container
    // auto utFileBind = wformatString("{0}:{1}:ro", pathToHostUnreliableTransportSettings, pathToHostUnreliableTransportSettings);
    // binds.push_back(utFileBind);

    AddVolumesToBinds(binds, localVolumes);
    AddVolumesToBinds(binds, mappedVolumes);

#if defined(PLATFORM_UNIX)
    // this is for the log mount
    binds.push_back(wformatString("{0}:{0}:ro", logMountDirectory));
#endif

    if (containerDescription.AssignedIP.empty())
    {
        auto networkingMode = wformatString("{0}={1}", Common::ContainerEnvironment::ContainerNetworkingModeEnvironmentVariable, NetworkType::EnumToString(NetworkType::Other));
        envVars.push_back(networkingMode);
    }
    else
    {
        auto networkingMode = wformatString("{0}={1}", Common::ContainerEnvironment::ContainerNetworkingModeEnvironmentVariable, NetworkType::EnumToString(NetworkType::Open));
        envVars.push_back(networkingMode);
    }
    //populate process debug parameters
    auto debugParams = processDescription.DebugParameters;
    for (auto it = debugParams.ContainerMountedVolumes.begin(); it != debugParams.ContainerMountedVolumes.end(); ++it)
    {
        binds.push_back(*it);
    }
    for (auto it = debugParams.ContainerEnvironmentBlock.begin(); it != debugParams.ContainerEnvironmentBlock.end(); ++it)
    {
        envVars.push_back(*it);
    }
    vector<wstring> commands;
    wstring delimiter = L",";

    StringUtility::Split(processDescription.Arguments, commands, delimiter);

    auto containerNameVar = wformatString("{0}={1}", Constants::EnvironmentVariable::ContainerName,containerDescription.ContainerName);
    envVars.push_back(containerNameVar);
    config.Env = envVars;
    config.Cmd = commands;

    if (!debugParams.ContainerEntryPoints.empty())
    {
        config.EntryPoint = debugParams.ContainerEntryPoints;
    }
    else if (!containerDescription.EntryPoint.empty())
    {
        vector<wstring> entryPoints;
        StringUtility::Split(containerDescription.EntryPoint, entryPoints, delimiter);
        config.EntryPoint = entryPoints;
    }

    wstring namespaceId;
    bool sharingNetworkNamespace = false;
    if (!containerDescription.GroupContainerName.empty())
    {
        namespaceId = ContainerHostConfig::GetContainerNamespace(containerDescription.GroupContainerName);
        config.HostConfig.IpcMode = namespaceId;
        config.HostConfig.UsernsMode = namespaceId;
        config.HostConfig.PidMode = namespaceId;
        config.HostConfig.UTSMode = namespaceId;

    }

    //network namespace can only be shared for open network config
    if (!namespaceId.empty() &&
        !containerDescription.AssignedIP.empty())
    {
        config.HostConfig.NetworkMode = namespaceId;
        config.HostConfig.PublishAllPorts = false;
        sharingNetworkNamespace = true;
    }
    else if (!containerDescription.PortBindings.empty())
    {
        for (auto it = containerDescription.PortBindings.begin(); it != containerDescription.PortBindings.end(); it++)
        {
            config.HostConfig.PortBindings[it->first].push_back(PortBinding(it->second));
            config.ExposedPorts[it->first] = ExposedPort();
        }
        config.HostConfig.PublishAllPorts = false;
    }
    else if (!containerDescription.AssignedIP.empty())
    {
        config.NetworkConfig.EndpointConfig.UnderlayConfig.IpamConfig.IPv4Address = containerDescription.AssignedIP;
        config.HostConfig.NetworkMode = L"servicefabric_network";
        config.HostConfig.PublishAllPorts = false;
    }
    else
    {
        config.HostConfig.PublishAllPorts = true;
    }
    if(!sharingNetworkNamespace)
    {
        config.HostConfig.Dns = containerDescription.DnsServers;

        auto searchOptions = ContainerConfig::GetDnsSearchOptions(containerDescription.ApplicationName);
        if (!searchOptions.empty())
        {
            config.HostConfig.DnsSearch.push_back(searchOptions);
        }
    }
    if (!processDescription.CgroupName.empty())
    {
        config.HostConfig.CgroupName = processDescription.CgroupName;
    }
    if (resourceGovPolicy.MemoryInMB > 0)
    {
        config.HostConfig.Memory = static_cast<uint64>(resourceGovPolicy.MemoryInMB) * MegaBytesToBytesMultiplier;
    }
    if (resourceGovPolicy.MemorySwapInMB > 0)
    {
        //just a precaution here - in practise it does not matter whether we use int64 or uint64
        //as that is a huge limit in any case
        uint64 swapMemory = static_cast<uint64>(resourceGovPolicy.MemorySwapInMB) * MegaBytesToBytesMultiplier;
        if (swapMemory > INT64_MAX)
        {
            swapMemory = INT64_MAX;
        }
        config.HostConfig.MemorySwap = static_cast<int64> (swapMemory);
    }
    if (resourceGovPolicy.MemoryReservationInMB > 0)
    {
        config.HostConfig.MemoryReservation = static_cast<uint64>(resourceGovPolicy.MemoryReservationInMB) * MegaBytesToBytesMultiplier;
    }
    if (resourceGovPolicy.DiskQuotaInMB > 0)
    {
        config.HostConfig.DiskQuota = static_cast<uint64>(resourceGovPolicy.DiskQuotaInMB) * MegaBytesToBytesMultiplier;
    }
#if defined(PLATFORM_UNIX)
    // Note this is Linux specific, however, docker will NOT error out if specified.  It is simply ignored.
    if (resourceGovPolicy.ShmSizeInMB > 0)
    {
        config.HostConfig.ShmSize = static_cast<uint64>(resourceGovPolicy.ShmSizeInMB) * MegaBytesToBytesMultiplier;
    }

    // Note this is Linux specific and docker on windows will error out if this is set.
    if (resourceGovPolicy.KernelMemoryInMB > 0)
    {
        config.HostConfig.KernelMemory = static_cast<uint64>(resourceGovPolicy.KernelMemoryInMB) * MegaBytesToBytesMultiplier;
    }
#endif
    if (resourceGovPolicy.CpuShares > 0)
    {
        config.HostConfig.CpuShares = resourceGovPolicy.CpuShares;
    }
    if (!resourceGovPolicy.CpusetCpus.empty())
    {
        config.HostConfig.CpusetCpus = resourceGovPolicy.CpusetCpus;
    }
    if (resourceGovPolicy.NanoCpus > 0)
    {
        //for linux no need to adjust
        UNREFERENCED_PARAMETER(dockerVersion);
        config.HostConfig.NanoCpus = resourceGovPolicy.NanoCpus;
    }

    config.HostConfig.SecurityOptions = containerDescription.SecurityOptions;
    config.HostConfig.CpuPercent = resourceGovPolicy.CpuPercent;
    config.HostConfig.MaximumIOps = resourceGovPolicy.MaximumIOps;
    config.HostConfig.MaximumIOBytesps = resourceGovPolicy.MaximumIOBytesps;
    config.HostConfig.BlockIOWeight = resourceGovPolicy.BlockIOWeight;
    if (!containerDescription.AutoRemove)
    {
        config.HostConfig.AutoRemove = false;
    }
    if (containerDescription.RunInteractive)
    {
        config.Tty = true;
        config.OpenStdin = true;
    }
    config.HostConfig.Binds = binds;

    auto logConfig = containerDescription.LogConfig;
    if (!logConfig.Driver.empty())
    {
        config.HostConfig.LogConfig.Type = logConfig.Driver;

        error = ContainerConfigHelper::ParseDriverOptions(logConfig.DriverOpts, config.HostConfig.LogConfig.DriverOpts);

        if (!error.IsSuccess())
        {
            return error;
        }

    }
    config.Hostname = containerDescription.Hostname;

    return ErrorCodeValue::Success;
}

//we need to set DNS search option so DNS names for default services for different applications dont collide
wstring ContainerConfig::GetDnsSearchOptions(wstring const & appName)
{
    wstring searchOptions;
    if (!appName.empty())
    {
        wstring rootName = wformatString("{0}/", NamingUri::RootNamingUriString);
        searchOptions = appName;
        if (StringUtility::StartsWith(appName, rootName))
        {
            searchOptions.erase(0, rootName.size());
            vector<wstring> tokens;
            StringUtility::Split<wstring>(searchOptions, tokens, L"/");
            if (!tokens.empty())
            {
                auto it = tokens.rbegin();
                searchOptions = *it;
                it++;
                while (it != tokens.rend())
                {
                    searchOptions.append(wformatString(".{0}", *it));
                    it++;
                }
            }
        }
    }

    return searchOptions;
}

Common::WStringLiteral const CreateContainerResponse::IdParameter(L"Id");

CreateContainerResponse::CreateContainerResponse() : Id()
{
}

CreateContainerResponse::~CreateContainerResponse()
{}

Common::WStringLiteral const GetContainerResponse::NamesParameter(L"Names");

GetContainerResponse::GetContainerResponse()
{}

GetContainerResponse::~GetContainerResponse(){}

ContainerState::ContainerState()
{}

ContainerState::~ContainerState()
{}

Common::WStringLiteral const DockerResponse::MessageParameter(L"message");

DockerResponse::DockerResponse() : Message()
{
}

DockerResponse::~DockerResponse()
{
}

Common::WStringLiteral const IPAMConfig::IPv4AddressParameter(L"IPv4Address");
Common::WStringLiteral const IPAMConfig::IPv6AddressParameter(L"IPv6Address");

IPAMConfig::IPAMConfig() : IPv4Address(), IPv6Address()
{
}

IPAMConfig::~IPAMConfig()
{}

Common::WStringLiteral const UnderlayNetworkConfig::IPAMConfigParameter(L"IPAMConfig");

UnderlayNetworkConfig::UnderlayNetworkConfig() : IpamConfig()
{}

UnderlayNetworkConfig::~UnderlayNetworkConfig()
{}

Common::WStringLiteral const EndpointsConfig::UnderlayNetworkConfigParameter(L"servicefabric_network");

EndpointsConfig::EndpointsConfig() : UnderlayConfig()
{}

EndpointsConfig::~EndpointsConfig()
{
}

Common::WStringLiteral const NetworkingConfig::EndpointsConfigParameter(L"EndpointsConfig");

NetworkingConfig::NetworkingConfig() : EndpointConfig()
{
}

NetworkingConfig::~NetworkingConfig()
{}

Common::WStringLiteral const DockerVersionQueryResponse::VersionParameter(L"Version");
