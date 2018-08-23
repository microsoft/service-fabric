//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class PodPortMapping : public Serialization::FabricSerializable
    {
    public:
        enum Protocol {
            TCP = 0,
            UDP = 1
        };

        PodPortMapping()
        : HostIp(), Proto(Protocol::TCP), ContainerPort(0), HostPort(0)
        {
        }

        PodPortMapping(wstring hostIp, Protocol proto, ULONG containerPort, ULONG hostPort)
        : HostIp(hostIp), Proto(proto), ContainerPort(containerPort), HostPort(hostPort)
        {
        }

        wstring HostIp;
        Protocol Proto;
        ULONG ContainerPort;
        ULONG HostPort;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_04(HostIp, Proto, ContainerPort, HostPort);
    };

    class ContainerPodDescription : public Serialization::FabricSerializable
    {

    public:
        ContainerPodDescription();
        ContainerPodDescription(
            std::wstring const & hostName,
            ServiceModel::ContainerIsolationMode::Enum const & isolationMode,
            std::vector<PodPortMapping> const & portMappings);

        ContainerPodDescription(ContainerPodDescription const & other);

        ContainerPodDescription(ContainerPodDescription && other);
        virtual ~ContainerPodDescription();
        ContainerPodDescription const & operator = (ContainerPodDescription const & other);
        ContainerPodDescription const & operator = (ContainerPodDescription && other);
        
        __declspec(property(get = get_HostName)) std::wstring const & HostName;
        inline std::wstring const & get_HostName() const { return hostName_; };

        __declspec(property(get = get_IsolationMode)) ServiceModel::ContainerIsolationMode::Enum const & IsolationMode;
        inline ServiceModel::ContainerIsolationMode::Enum const & get_IsolationMode() const { return isolationMode_; };

        __declspec(property(get = get_PortMappings))  std::vector<PodPortMapping> const & PortMappings;
        inline std::vector<PodPortMapping> const & get_PortMappings() const { return portMappings_; };

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(hostName_, isolationMode_, portMappings_);

    private:
        std::wstring hostName_;
        ServiceModel::ContainerIsolationMode::Enum isolationMode_;
        std::vector<PodPortMapping> portMappings_;
    };
}
DEFINE_USER_ARRAY_UTILITY(Hosting2::PodPortMapping);

