// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class PortBinding : public Common::IFabricJsonSerializable
    {
    public:
        PortBinding();
        PortBinding(std::wstring const &);
        ~PortBinding();
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(PortBinding::HostPortParameter, HostPort)
        END_JSON_SERIALIZABLE_PROPERTIES()
    public:
        std::wstring HostPort;
        static Common::WStringLiteral const HostPortParameter;
        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
    };

    class ContainerLogConfig : public Common::IFabricJsonSerializable
    {
    public:
        ContainerLogConfig();
        ContainerLogConfig(
            std::wstring const & type,
            std::map<std::wstring,std::wstring> const & driverOpts);
        ~ContainerLogConfig();

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ContainerLogConfig::TypeParameter, Type)
            SERIALIZABLE_PROPERTY_SIMPLE_MAP(ContainerLogConfig::ConfigParameter, DriverOpts)
        END_JSON_SERIALIZABLE_PROPERTIES()
    public:
        std::wstring Type;
        std::map<std::wstring, std::wstring> DriverOpts;
        static Common::WStringLiteral const TypeParameter;
        static Common::WStringLiteral const ConfigParameter;
        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
    };

    class ContainerVolumeConfig : public Common::IFabricJsonSerializable
    {
    public:
        ContainerVolumeConfig();
        ContainerVolumeConfig(
            std::wstring const & volumeName,
            std::wstring const & driver,
            std::map<std::wstring, std::wstring> const & driverOpts);
        ~ContainerVolumeConfig();
        
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ContainerVolumeConfig::NameParameter, Name)
            SERIALIZABLE_PROPERTY(ContainerVolumeConfig::DriverParameter, Driver)
            SERIALIZABLE_PROPERTY_SIMPLE_MAP(ContainerVolumeConfig::DriverOptsParameter, DriverOpts)
        END_JSON_SERIALIZABLE_PROPERTIES()
    public:
        std::wstring Name;
        std::wstring Driver;
        std::map<std::wstring, std::wstring> DriverOpts;

        static Common::WStringLiteral const NameParameter;
        static Common::WStringLiteral const DriverParameter;
        static Common::WStringLiteral const DriverOptsParameter;
        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
    };

    class ContainerHostConfig
        : public Common::IFabricJsonSerializable
    {
    public:
        ContainerHostConfig();

        ~ContainerHostConfig();

        static Common::ErrorCode CreateVolumeConfig(
            std::vector<ServiceModel::ContainerVolumeDescription> const & volumes,
            __out std::vector<ContainerVolumeConfig> & volumeConfig);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
          SERIALIZABLE_PROPERTY(ContainerHostConfig::PublishAllPortsParameter, PublishAllPorts)
          SERIALIZABLE_PROPERTY(ContainerHostConfig::NetworkModeParameter, NetworkMode)
          SERIALIZABLE_PROPERTY(ContainerHostConfig::AutoRemoveParameter, AutoRemove)
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::BindsParameter, Binds, !Binds.empty())
          SERIALIZABLE_PROPERTY_SIMPLE_MAP(ContainerHostConfig::PortBindingsParameter, PortBindings)
          SERIALIZABLE_PROPERTY_INT64_AS_NUM_IF(ContainerHostConfig::MemoryParameter, Memory, (Memory != 0))
          SERIALIZABLE_PROPERTY_INT64_AS_NUM_IF(ContainerHostConfig::MemorySwapParameter, MemorySwap, (MemorySwap != 0))
          SERIALIZABLE_PROPERTY_INT64_AS_NUM_IF(ContainerHostConfig::MemoryReservationParameter, MemoryReservation, (MemoryReservation != 0))
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::CpuSharesParameter, CpuShares, (CpuShares != 0 && NanoCpus == 0))
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::CpuPercentParameter, CpuPercent, (CpuPercent != 0 && NanoCpus == 0))
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::MaximumIOpsParameter, MaximumIOps, (MaximumIOps != 0))
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::MaximumIOBytespsParameter, MaximumIOBytesps, (MaximumIOBytesps != 0))
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::BlockIOWeightParameter, BlockIOWeight, (BlockIOWeight != 0))
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::DiskQuotaParameter, DiskQuota, (DiskQuota != 0))
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::KernelMemoryParameter, KernelMemory, (KernelMemory != 0))
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::ShmSizeParameter, ShmSize, (ShmSize != 0))
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::ContainerLogConfigParameter, LogConfig, (!LogConfig.Type.empty()))
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::IsolationParameter, Isolation, (!Isolation.empty()))
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::CpusetCpusParameter, CpusetCpus, (!CpusetCpus.empty() && NanoCpus == 0))
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::DnsParameter, Dns, !Dns.empty())
          SERIALIZABLE_PROPERTY_INT64_AS_NUM_IF(ContainerHostConfig::NanoCpusParameter, NanoCpus, (NanoCpus != 0))
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::DnsSearchParameter, DnsSearch, !DnsSearch.empty())
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::SecurityOptionsParameter, SecurityOptions, !SecurityOptions.empty())
          SERIALIZABLE_PROPERTY(ContainerHostConfig::PrivilegedParameter, Privileged)
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::PidModeParameter, PidMode, !PidMode.empty())
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::UTSModeParameter, UTSMode, !UTSMode.empty())
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::IpcModeParameter, IpcMode, !IpcMode.empty())
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::UsernsModeParameter, UsernsMode, !UsernsMode.empty())
          SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::CgroupNameParameter, CgroupName, !CgroupName.empty())
        END_JSON_SERIALIZABLE_PROPERTIES()

    public:
        bool PublishAllPorts;
        std::wstring NetworkMode;
        std::wstring Isolation;
        bool AutoRemove;
        std::vector<std::wstring> Binds;
        std::map<std::wstring, std::vector<PortBinding>> PortBindings;
        uint64 Memory;
        //docker treats this as -1 when unlimited so make it signed int
        int64 MemorySwap;
        uint64 MemoryReservation;
        uint CpuShares;
        uint CpuPercent;
        uint MaximumIOps;
        uint MaximumIOBytesps;
        uint BlockIOWeight;
        uint64 DiskQuota;
        uint64 KernelMemory;
        uint64 ShmSize;
        ContainerLogConfig LogConfig;
        std::wstring CpusetCpus;
        std::vector<std::wstring> Dns;
        //if we have this set we should not pass other cpu related info
        uint64 NanoCpus;
        std::vector<std::wstring> DnsSearch;
        std::vector<std::wstring> SecurityOptions;
        bool Privileged;

        std::wstring PidMode;
        std::wstring UTSMode;
        std::wstring IpcMode;
        std::wstring UsernsMode;

        std::wstring CgroupName;

        static std::wstring GetContainerNamespace(std::wstring const & containerName);

        static Common::WStringLiteral const PublishAllPortsParameter;
        static Common::WStringLiteral const IsolationParameter;
        static Common::WStringLiteral const NetworkModeParameter;
        static Common::WStringLiteral const AutoRemoveParameter;
        static Common::WStringLiteral const BindsParameter;
        static Common::WStringLiteral const PortBindingsParameter;
        static Common::WStringLiteral const MemoryParameter;
        static Common::WStringLiteral const MemorySwapParameter;
        static Common::WStringLiteral const MemoryReservationParameter;
        static Common::WStringLiteral const CpuSharesParameter;
        static Common::WStringLiteral const CpuPercentParameter;
        static Common::WStringLiteral const MaximumIOpsParameter;
        static Common::WStringLiteral const MaximumIOBytespsParameter;
        static Common::WStringLiteral const BlockIOWeightParameter;
        static Common::WStringLiteral const DiskQuotaParameter;
        static Common::WStringLiteral const KernelMemoryParameter;
        static Common::WStringLiteral const ShmSizeParameter;
        static Common::WStringLiteral const ContainerLogConfigParameter;
        static Common::WStringLiteral const CpusetCpusParameter;
        static Common::WStringLiteral const DnsParameter;
        static Common::WStringLiteral const NanoCpusParameter;
        static Common::WStringLiteral const DnsSearchParameter;
        static Common::WStringLiteral const SecurityOptionsParameter;
        static Common::WStringLiteral const PrivilegedParameter;
        static Common::WStringLiteral const PidModeParameter;
        static Common::WStringLiteral const UTSModeParameter;
        static Common::WStringLiteral const IpcModeParameter;
        static Common::WStringLiteral const UsernsModeParameter;
        static Common::WStringLiteral const CgroupNameParameter;
        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
    };

    class TestContainerHostConfig
        : public Common::IFabricJsonSerializable
    {
    public:
        TestContainerHostConfig();

        ~TestContainerHostConfig();

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_INT64_AS_NUM_IF(ContainerHostConfig::MemoryParameter, Memory, (Memory != 0))
            SERIALIZABLE_PROPERTY_INT64_AS_NUM_IF(ContainerHostConfig::NanoCpusParameter, NanoCpus, (NanoCpus != 0))
            SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::PidModeParameter, PidMode, !PidMode.empty())
            SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::UTSModeParameter, UTSMode, !UTSMode.empty())
            SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::IpcModeParameter, IpcMode, !IpcMode.empty())
            SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::UsernsModeParameter, UsernsMode, !UsernsMode.empty())
            SERIALIZABLE_PROPERTY_IF(ContainerHostConfig::CgroupNameParameter, CgroupName, !CgroupName.empty())
            END_JSON_SERIALIZABLE_PROPERTIES()

    public:
        uint64 Memory;
        uint64 NanoCpus;

        // used for PODS
        std::wstring PidMode;
        std::wstring UTSMode;
        std::wstring IpcMode;
        std::wstring UsernsMode;

        std::wstring CgroupName;
    };
}
