// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ExposedPort : public Common::IFabricJsonSerializable
    {
    public:
        ExposedPort(){};
        ~ExposedPort(){};
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
        END_JSON_SERIALIZABLE_PROPERTIES()
    };

    class IPAMConfig : public Common::IFabricJsonSerializable
    {
    public:
        IPAMConfig();
        ~IPAMConfig();
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_IF(IPAMConfig::IPv4AddressParameter, IPv4Address, !IPv4Address.empty())
            SERIALIZABLE_PROPERTY_IF(IPAMConfig::IPv6AddressParameter, IPv6Address, !IPv6Address.empty())
        END_JSON_SERIALIZABLE_PROPERTIES()
    public:
        std::wstring IPv4Address;
        std::wstring IPv6Address;
        static Common::WStringLiteral const IPv4AddressParameter;
        static Common::WStringLiteral const IPv6AddressParameter;
        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.Write("IPAMConfig { ");
            w.Write("IPv4Address = {0}", IPv4Address);
            w.Write("IPv6Address = {0}", IPv6Address);
            w.Write("}");
        }
    };

    class UnderlayNetworkConfig : public Common::IFabricJsonSerializable
    {
    public:
        UnderlayNetworkConfig();
        ~UnderlayNetworkConfig();
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(UnderlayNetworkConfig::IPAMConfigParameter, IpamConfig)
        END_JSON_SERIALIZABLE_PROPERTIES()
    public:
        IPAMConfig IpamConfig;
        static Common::WStringLiteral const IPAMConfigParameter;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.Write("UnderlayNetworkConfig { ");
            w.Write("{0}", IpamConfig);
            w.Write("}");
        }
    };

    class EndpointsConfig : public Common::IFabricJsonSerializable
    {
    public:
        EndpointsConfig();
        ~EndpointsConfig();
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(EndpointsConfig::UnderlayNetworkConfigParameter, UnderlayConfig)
        END_JSON_SERIALIZABLE_PROPERTIES()
    public:
        UnderlayNetworkConfig UnderlayConfig;
        static Common::WStringLiteral const UnderlayNetworkConfigParameter;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.Write("EndpointsConfig { ");
            w.Write("{0}", UnderlayConfig);
            w.Write("}");
        }
    };

    class NetworkingConfig : public Common::IFabricJsonSerializable
    {
    public:
        NetworkingConfig();
        ~NetworkingConfig();
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(NetworkingConfig::EndpointsConfigParameter, EndpointConfig)
        END_JSON_SERIALIZABLE_PROPERTIES()
    public:
        EndpointsConfig EndpointConfig;
        static Common::WStringLiteral const EndpointsConfigParameter;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.Write("NetworkingConfig { ");
            w.Write("{0}", EndpointConfig);
            w.Write("}");
        }
    };

    class ContainerConfig
        : public Common::IFabricJsonSerializable
    {
    public:
        ContainerConfig();
        ContainerConfig(ContainerConfig &&);
        ContainerConfig const & operator =(ContainerConfig && other);

        ~ContainerConfig();

        static Common::ErrorCode CreateContainerConfig(
            std::vector<ServiceModel::ContainerVolumeDescription> const & localVolumes,
            std::vector<ServiceModel::ContainerVolumeDescription> const & mappedVolumes,
            ProcessDescription const & processDescription,
            ContainerDescription const & containerDescription,
            std::wstring const & dockerVersion,
            __out ContainerConfig & containerConfig);

        static std::wstring GetDnsSearchOptions(std::wstring const & appName);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
          SERIALIZABLE_PROPERTY(ContainerConfig::ImageParameter, Image)
          SERIALIZABLE_PROPERTY_IF(ContainerConfig::EnvParameter, Env, !Env.empty())
          SERIALIZABLE_PROPERTY_IF(ContainerConfig::CmdParameter, Cmd, !Cmd.empty())
          SERIALIZABLE_PROPERTY_IF(ContainerConfig::EntryPointParameter, EntryPoint, !EntryPoint.empty())
          SERIALIZABLE_PROPERTY(ContainerConfig::HostConfigParameter, HostConfig)
          SERIALIZABLE_PROPERTY_IF(ContainerConfig::HostnameParameter, Hostname, !Hostname.empty())
          SERIALIZABLE_PROPERTY_SIMPLE_MAP(ContainerConfig::LabelsParameter, Labels)
          SERIALIZABLE_PROPERTY_SIMPLE_MAP(ContainerConfig::ExposedPortsParameter, ExposedPorts)
          SERIALIZABLE_PROPERTY_IF(ContainerConfig::NetworkingConfigParameter, NetworkConfig, !(NetworkConfig.EndpointConfig.UnderlayConfig.IpamConfig.IPv4Address.empty()))
          SERIALIZABLE_PROPERTY(ContainerConfig::LogPathParameter, LogPath)
          SERIALIZABLE_PROPERTY_IF(ContainerConfig::TtyParameter, Tty, Tty)
          SERIALIZABLE_PROPERTY_IF(ContainerConfig::OpenStdinParameter, OpenStdin, OpenStdin)
        END_JSON_SERIALIZABLE_PROPERTIES()

    public:
        std::wstring Image;
        std::vector<std::wstring> Env;
        std::vector<std::wstring> Cmd;
        std::vector<std::wstring> EntryPoint;
        std::map<std::wstring, std::wstring> Labels;
        std::map<std::wstring, ExposedPort> ExposedPorts;
        ContainerHostConfig HostConfig;
        NetworkingConfig NetworkConfig;
        std::wstring Hostname;
        std::wstring LogPath;
        bool Tty;
        bool OpenStdin;

        static Common::WStringLiteral const ImageParameter;
        static Common::WStringLiteral const EnvParameter;
        static Common::WStringLiteral const CmdParameter;
        static Common::WStringLiteral const EntryPointParameter;
        static Common::WStringLiteral const HostConfigParameter;
        static Common::WStringLiteral const LabelsParameter;
        static Common::WStringLiteral const ExposedPortsParameter;
        static Common::WStringLiteral const NetworkingConfigParameter;
        static Common::WStringLiteral const HostnameParameter;
        static Common::GlobalWString const FabricPackageFileNameEnvironment;
        static Common::GlobalWString const RuntimeSslConnectionCertFilePath;
        static Common::WStringLiteral const LogPathParameter;
        static Common::WStringLiteral const TtyParameter;
        static Common::WStringLiteral const OpenStdinParameter;
        static Common::WStringLiteral const UserLogsDirectoryEnvVarName;
        static double const DockerVersionThreshold;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    private:
        void AddEnvironmentVariable(std::wstring const &name, std::wstring const &value);
        void AddBinding(std::wstring const &binding);
        Common::ErrorCode SetFabricRuntimeSettings(
            ProcessDescription const & processDescription,
            ContainerDescription const & containerDescription);
        static void AddVolumesToBinds(
            std::vector<std::wstring> & binds,
            std::vector<ServiceModel::ContainerVolumeDescription> const & volumeDescriptions);
    };

    class ContainerInspectResponse : public Common::IFabricJsonSerializable
    {
    public:
        ContainerInspectResponse() = default;
        ~ContainerInspectResponse() = default;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ContainerConfig::HostConfigParameter, HostConfig)
        END_JSON_SERIALIZABLE_PROPERTIES()

        TestContainerHostConfig HostConfig;
    };

    class CreateContainerResponse : public Common::IFabricJsonSerializable
    {
    public:
        CreateContainerResponse();
        ~CreateContainerResponse();

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(CreateContainerResponse::IdParameter, Id)
        END_JSON_SERIALIZABLE_PROPERTIES()

        std::wstring Id;
        static Common::WStringLiteral const IdParameter;
    };

    class ContainerState : public Common::IFabricJsonSerializable
    {
    public:
        ContainerState();
        ~ContainerState();
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ContainerState::ErrorParameter, Error)
            SERIALIZABLE_PROPERTY(ContainerState::PidParameter, Pid)
            SERIALIZABLE_PROPERTY(ContainerState::IsDeadParameter, IsDead)
        END_JSON_SERIALIZABLE_PROPERTIES()
    public:
        std::wstring Error;
        DWORD Pid;
        bool IsDead;
        static Common::WStringLiteral const ErrorParameter;
        static Common::WStringLiteral const PidParameter;
        static Common::WStringLiteral const IsDeadParameter;
    };

    class GetContainerResponse : public Common::IFabricJsonSerializable
    {
    public:
        GetContainerResponse();
        ~GetContainerResponse();
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(GetContainerResponse::NamesParameter, Names)
            SERIALIZABLE_PROPERTY(CreateContainerResponse::IdParameter, Id)
            SERIALIZABLE_PROPERTY(GetContainerResponse::StateParameter, State)
        END_JSON_SERIALIZABLE_PROPERTIES()
    public:
        std::vector<std::wstring> Names;
        std::wstring Id;
        std::wstring State;
        static Common::WStringLiteral const NamesParameter;
        static Common::WStringLiteral const StateParameter;
    };

    class DockerResponse : public Common::IFabricJsonSerializable
    {
    public:
        DockerResponse();
        ~DockerResponse();
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(DockerResponse::MessageParameter, Message)
        END_JSON_SERIALIZABLE_PROPERTIES()
    public:
        std::wstring Message;
        static Common::WStringLiteral const MessageParameter;
    };

    class DockerVersionQueryResponse : public Common::IFabricJsonSerializable
    {
    public:
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(DockerVersionQueryResponse::VersionParameter, Version)
        END_JSON_SERIALIZABLE_PROPERTIES()
    public:
        std::wstring Version;
        static Common::WStringLiteral const VersionParameter;
    };
}
