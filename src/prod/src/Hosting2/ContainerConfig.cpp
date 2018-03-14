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
Common::WStringLiteral const ContainerConfig::ExposedPortsParameter(L"ExposedPorts");
Common::WStringLiteral const ContainerConfig::NetworkingConfigParameter(L"NetworkingConfig");
Common::WStringLiteral const ContainerConfig::HostnameParameter(L"Hostname");
Common::WStringLiteral const ContainerConfig::TtyParameter(L"Tty");
Common::WStringLiteral const ContainerConfig::OpenStdinParameter(L"OpenStdin");

Common::WStringLiteral const GetContainerResponse::StateParameter(L"State");
Common::WStringLiteral const ContainerState::ErrorParameter(L"Error");

Common::WStringLiteral const ContainerState::PidParameter(L"Pid");
Common::WStringLiteral const ContainerState::IsDeadParameter(L"Dead");

Common::GlobalWString const ContainerConfig::FabricPackageFileNameEnvironment = make_global<wstring>(L"FabricPackageFileName");
// Starting from this version there is no need to limit cpu to 1% of total machine capacity
double const ContainerConfig::DockerVersionThreshold = 17.06;

ContainerConfig::ContainerConfig() : 
    Image(),
    Env(),
    Cmd(),
    EntryPoint(),
    ExposedPorts(),
    NetworkConfig(),
    Hostname(),
    Tty(false),
    OpenStdin(false)
{

}

ContainerConfig::~ContainerConfig()
{

}

ErrorCode ContainerConfig::CreateContainerConfig(
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
#if !defined(PLATFORM_UNIX)
            if (StringUtility::AreEqualCaseInsensitive(iter->first, *FabricPackageFileNameEnvironment))
            {
                packageFilePath = iter->second;
                containerPackageFilePath = Path::Combine(containerWFRoot, Path::GetFileName(iter->second));
                auto fabricPackageFileName = wformatString("{0}={1}", FabricPackageFileNameEnvironment, containerPackageFilePath);
                envVars.push_back(fabricPackageFileName);
            }
            else
            {
                wstring result = wformatString("{0}={1}", iter->first, iter->second);
                envVars.push_back(result);
            }
#else
            wstring result = wformatString("{0}={1}", iter->first, iter->second);
            envVars.push_back(result);
#endif
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

#if defined(PLATFORM_UNIX)
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

#else
    auto binRoot = wformatString("FabricCodePath={0}", HostingConfig::GetConfig().ContainerFabricBinRootFolder);
    envVars.push_back(binRoot);

    auto appDirectory = HostingConfig::GetConfig().GetContainerApplicationFolder(containerDescription.ApplicationId);
    auto deploymentFolderBind = wformatString("{0}:{1}", Path::NormalizePathSeparators(processDescription.AppDirectory), appDirectory);

    if (!packageFilePath.empty())
    {
        auto configDir = Path::GetDirectoryName(packageFilePath);
        auto configPath = wformatString("{0}:{1}:ro", Path::NormalizePathSeparators(configDir), HostingConfig::GetConfig().ContainerPackageRootFolder);
        binds.push_back(configPath);
    }

    binds.push_back(deploymentFolderBind);

    if (!fabricCodePath.empty())
    {
        auto binBinding = wformatString("{0}:{1}:ro", Path::NormalizePathSeparators(fabricCodePath), HostingConfig::GetConfig().ContainerFabricBinRootFolder);
        binds.push_back(binBinding);
    }

    if (!fabricContainerRoot.empty())
    {
        auto binBinding = wformatString("{0}:{1}", Path::NormalizePathSeparators(fabricContainerRoot), HostingConfig::GetConfig().ContainerFabricLogRootFolder);
        binds.push_back(binBinding);
    }

    auto logRoot = wformatString("FabricLogRoot={0}", HostingConfig::GetConfig().ContainerFabricLogRootFolder);
    envVars.push_back(logRoot);

    config.HostConfig.Isolation = ContainerIsolationMode::EnumToString(containerDescription.IsolationMode);

    // Mount the UT settings file
    // TODO: Bug#9728016 - Disable the bind until windows supports mounting file onto container
    // auto pathToContainerUnreliableTransportSettings = Path::Combine(HostingConfig::GetConfig().ContainerNodeWorkFolder, Transport::UnreliableTransportConfig::SettingsFileName);
    // auto utFileBind = wformatString("{0}:{1}:ro", Path::NormalizePathSeparators(pathToHostUnreliableTransportSettings), pathToContainerUnreliableTransportSettings);
    // binds.push_back(utFileBind);

#endif
    auto volumes = containerDescription.ContainerVolumes;
    for (auto it = volumes.begin(); it != volumes.end(); ++it)
    {
#if !defined(PLATFORM_UNIX)
        //Source of volume must be a valid path on host if no volume driver is specified
        if (it->Driver.empty() && !Directory::Exists(it->Source))
        {
            return ErrorCode(
                ErrorCodeValue::DirectoryNotFound,
                wformatString("{0} {1}", StringResource::Get(IDS_HOSTING_ContainerInvalidMountSource), it->Source));
        }
#endif
        if(it->IsReadOnly)
        {
             binds.push_back(wformatString("{0}:{1}:ro", it->Source, it->Destination));
        }
        else
        {
            binds.push_back(wformatString("{0}:{1}", it->Source, it->Destination));
        }
    }

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
#if defined(PLATFORM_UNIX)
    if (!processDescription.CgroupName.empty())
    {
        config.HostConfig.CgroupName = processDescription.CgroupName;
    }
#endif
    if (resourceGovPolicy.MemoryInMB > 0)
    {
        config.HostConfig.Memory = static_cast<uint64>(resourceGovPolicy.MemoryInMB) * 1024 * 1024;
    }
    if (resourceGovPolicy.MemorySwapInMB > 0)
    {
        //just a precaution here - in practise it does not matter whether we use int64 or uint64 
        //as that is a huge limit in any case
        uint64 swapMemory = static_cast<uint64>(resourceGovPolicy.MemorySwapInMB) * 1024 * 1024;
        if (swapMemory > INT64_MAX)
        {
            swapMemory = INT64_MAX;
        }
        config.HostConfig.MemorySwap = static_cast<int64> (swapMemory);
    }
    if (resourceGovPolicy.MemoryReservationInMB > 0)
    {
        config.HostConfig.MemoryReservation = static_cast<uint64>(resourceGovPolicy.MemoryReservationInMB) * 1024 * 1024;
    }
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
#if !defined(PLATFORM_UNIX)
        bool isNewVersion = !dockerVersion.empty();

        if (isNewVersion)
        {
            double currentVersion = 0.0;

            //docker version are of type MajorVersion.MinorVersion.Edition
            //we want to parse up to the second dot in order to get the version number
            auto truncatedVersion = StringUtility::LimitNumberOfCharacters(dockerVersion, L'.', 1);

            isNewVersion = isNewVersion && StringUtility::TryFromWString<double>(truncatedVersion, currentVersion);
            if (isNewVersion)
            {
                isNewVersion = currentVersion >= ContainerConfig::DockerVersionThreshold;
            }
        }

        if (!isNewVersion)
        {
            //we shall fetch this only once - static ensures thread safety
            static auto numberOfCpus = Environment::GetNumberOfProcessors();
            //for windows we are going to consider all the active processors from the system info
            //in order to scale the minimum number of NanoCpus to at least 1% of total cpu for older docker versions
            uint64 minimumNumberOfNanoCpus = numberOfCpus * Constants::DockerNanoCpuMultiplier / 100;
            config.HostConfig.NanoCpus = max(minimumNumberOfNanoCpus, resourceGovPolicy.NanoCpus);
        }
        else
        {
            config.HostConfig.NanoCpus = resourceGovPolicy.NanoCpus;
        }
#else
        //for linux no need to adjust
        UNREFERENCED_PARAMETER(dockerVersion);
        config.HostConfig.NanoCpus = resourceGovPolicy.NanoCpus;
#endif
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
