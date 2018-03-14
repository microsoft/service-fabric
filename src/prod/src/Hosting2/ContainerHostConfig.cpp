// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ContainerHostConfig.h"

using namespace Hosting2;
using namespace Common;
using namespace ServiceModel;

Common::WStringLiteral const ContainerLogConfig::TypeParameter(L"Type");
Common::WStringLiteral const ContainerLogConfig::ConfigParameter(L"Config");

Common::WStringLiteral const ContainerVolumeConfig::NameParameter(L"Name");
Common::WStringLiteral const ContainerVolumeConfig::DriverParameter(L"Driver");
Common::WStringLiteral const ContainerVolumeConfig::DriverOptsParameter(L"DriverOpts");

Common::WStringLiteral const ContainerHostConfig::PublishAllPortsParameter(L"PublishAllPorts");
Common::WStringLiteral const ContainerHostConfig::NetworkModeParameter(L"NetworkMode");
Common::WStringLiteral const ContainerHostConfig::AutoRemoveParameter(L"AutoRemove");
Common::WStringLiteral const ContainerHostConfig::BindsParameter(L"Binds");
Common::WStringLiteral const ContainerHostConfig::PortBindingsParameter(L"PortBindings");
Common::WStringLiteral const ContainerHostConfig::MemoryParameter(L"Memory");
Common::WStringLiteral const ContainerHostConfig::MemorySwapParameter(L"MemorySwap");
Common::WStringLiteral const ContainerHostConfig::MemoryReservationParameter(L"MemoryReservation");
Common::WStringLiteral const ContainerHostConfig::CpuSharesParameter(L"CpuShares");
Common::WStringLiteral const ContainerHostConfig::CpuPercentParameter(L"CpuPercent");
Common::WStringLiteral const ContainerHostConfig::MaximumIOpsParameter(L"IOMaximumIOps");
Common::WStringLiteral const ContainerHostConfig::MaximumIOBytespsParameter(L"IOMaximumBandwidth");
Common::WStringLiteral const ContainerHostConfig::BlockIOWeightParameter(L"BlkioWeight");
Common::WStringLiteral const ContainerHostConfig::ContainerLogConfigParameter(L"LogConfig");
Common::WStringLiteral const ContainerHostConfig::IsolationParameter(L"Isolation");
Common::WStringLiteral const ContainerHostConfig::CpusetCpusParameter(L"CpusetCpus");
Common::WStringLiteral const ContainerHostConfig::DnsParameter(L"Dns");
Common::WStringLiteral const ContainerHostConfig::NanoCpusParameter(L"NanoCpus");
Common::WStringLiteral const ContainerHostConfig::DnsSearchParameter(L"DnsSearch");
Common::WStringLiteral const ContainerHostConfig::SecurityOptionsParameter(L"SecurityOpt");
Common::WStringLiteral const ContainerHostConfig::PrivilegedParameter(L"Privileged");
Common::WStringLiteral const ContainerHostConfig::IpcModeParameter(L"IpcMode");
Common::WStringLiteral const ContainerHostConfig::UTSModeParameter(L"UTSMode");
Common::WStringLiteral const ContainerHostConfig::UsernsModeParameter(L"UsernsMode");
Common::WStringLiteral const ContainerHostConfig::PidModeParameter(L"PidMode");
Common::WStringLiteral const ContainerHostConfig::CgroupNameParameter(L"CgroupParent");

ContainerHostConfig::ContainerHostConfig() :
PublishAllPorts(true),
NetworkMode(L"default"),
AutoRemove(true),
Binds(),
PortBindings(),
Memory(0),
MemoryReservation(0),
MemorySwap(0),
CpuShares(0),
CpuPercent(0),
MaximumIOps(0),
MaximumIOBytesps(0),
BlockIOWeight(0),
Isolation(),
CpusetCpus(),
Dns(),
NanoCpus(0),
DnsSearch(),
SecurityOptions(),
Privileged(false)
{
}

ContainerHostConfig::~ContainerHostConfig()
{
}

ErrorCode ContainerHostConfig::CreateVolumeConfig(
    vector<ContainerVolumeDescription> const & volumes,
    __out vector<ContainerVolumeConfig> & volumeConfigs)
{
    ErrorCode error(ErrorCodeValue::Success);

    for (auto it = volumes.begin(); it != volumes.end(); ++it)
    {
        if (!it->Driver.empty())
        {
            map<wstring, wstring> driverOpts;

            error = ContainerConfigHelper::ParseDriverOptions(it->DriverOpts, driverOpts);

            if (!error.IsSuccess())
            {
                return error;
            }

            ContainerVolumeConfig volumeConfig(it->Source, it->Driver, driverOpts);
            volumeConfigs.push_back(volumeConfig);
        }
    }

    return error;
}

wstring ContainerHostConfig::GetContainerNamespace(wstring const & containerName)
{
    return wformatString("container:{0}", containerName);
}
Common::WStringLiteral const PortBinding::HostPortParameter(L"HostPort");

PortBinding::PortBinding() : HostPort()
{
}

PortBinding::PortBinding(std::wstring const & hostPort) : HostPort(hostPort)
{
}

PortBinding::~PortBinding()
{
}

ContainerLogConfig::ContainerLogConfig():
    Type(),
    DriverOpts()
{
}

ContainerLogConfig::ContainerLogConfig(
    wstring const & type,
    map<wstring, wstring> const & driverOpts) :
    Type(type),
    DriverOpts(driverOpts)
{
}

ContainerLogConfig::~ContainerLogConfig()
{
}

ContainerVolumeConfig::ContainerVolumeConfig()
{
}

ContainerVolumeConfig::~ContainerVolumeConfig()
{
}

ContainerVolumeConfig::ContainerVolumeConfig(
    wstring const & volumeName,
    wstring const & driver,
    map<wstring, wstring> const & driverOpts) 
    : Name(volumeName),
    Driver(driver),
    DriverOpts(driverOpts)
{
}

