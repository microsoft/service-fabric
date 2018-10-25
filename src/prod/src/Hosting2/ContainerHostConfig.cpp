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
Common::WStringLiteral const ContainerHostConfig::DiskQuotaParameter(L"DiskQuota");
Common::WStringLiteral const ContainerHostConfig::KernelMemoryParameter(L"KernelMemory");
Common::WStringLiteral const ContainerHostConfig::ShmSizeParameter(L"ShmSize");
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
Privileged(false),
DiskQuota(0),
KernelMemory(0),
ShmSize(0)
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

void ContainerHostConfig::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerHostConfig { ");
    w.Write("PublishAllPorts = {0}", PublishAllPorts);
    w.Write("NetworkMode = {0}", NetworkMode);
    w.Write("Isolation = {0}", Isolation);
    w.Write("AutoRemove = {0}", AutoRemove);
    w.Write("Binds = {0}", Binds);    
    w.Write("PortBindings = {0}", PortBindings);
    w.Write("Memory = {0}", Memory);
    w.Write("MemorySwap = {0}", MemorySwap);
    w.Write("MemoryReservation = {0}", MemoryReservation);  
    w.Write("CpuShares = {0}", CpuShares);
    w.Write("CpuPercent = {0}", CpuPercent);
    w.Write("MaximumIOps = {0}", MaximumIOps);
    w.Write("MaximumIOBytesps = {0}", MaximumIOBytesps);
    w.Write("BlockIOWeight = {0}", BlockIOWeight);
    w.Write("LogConfig = {0}", LogConfig);
    w.Write("CpusetCpus = {0}", CpusetCpus);
    w.Write("Dns = {0}", Dns);
    w.Write("NanoCpus = {0}", NanoCpus);
    w.Write("DnsSearch = {0}", DnsSearch);
    w.Write("SecurityOptions = {0}", SecurityOptions);
    w.Write("Privileged = {0}", Privileged);
    w.Write("PidMode = {0}", PidMode);
    w.Write("UTSMode = {0}", UTSMode);
    w.Write("IpcMode = {0}", IpcMode);
    w.Write("UsernsMode = {0}", UsernsMode);
    w.Write("CgroupName = {0}", CgroupName);
    w.Write("}");
}

TestContainerHostConfig::TestContainerHostConfig() :
    Memory(0),
    NanoCpus(0)
{
}

TestContainerHostConfig::~TestContainerHostConfig()
{
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


void PortBinding::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("PortBinding { ");
    w.Write("HostPort = {0}", HostPort);
    w.Write("}");
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

void ContainerLogConfig::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerLogConfig { ");
    w.Write("Type = {0}", Type);
    w.Write("DriverOpts = {0}", DriverOpts);
    w.Write("}");
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

void ContainerVolumeConfig::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerVolumeConfig { ");
    w.Write("Name = {0}", Name);
    w.Write("Driver = {0}", Driver);
    w.Write("DriverOpts = {0}", DriverOpts);
    w.Write("}");
}

