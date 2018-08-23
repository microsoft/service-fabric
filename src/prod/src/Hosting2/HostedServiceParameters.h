// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class HostedServiceParameters : public Serialization::FabricSerializable
    {
    public:

        HostedServiceParameters();

        HostedServiceParameters(
            std::wstring const & serviceName,
            std::wstring const & exeName,
            std::wstring const & arguments,
            std::wstring const & workingDirectory,
            std::wstring const & environment,
            bool ctrlCSpecified,
            std::wstring const & serviceNodeName,
            std::wstring const & parentServiceName = std::wstring(),
            UINT port = 0,
            std::wstring const & protocol = std::wstring(),
            bool runasSpecified = false,
            std::wstring const & runasAccountName = std::wstring(),
            ServiceModel::SecurityPrincipalAccountType::Enum runasAccountType = ServiceModel::SecurityPrincipalAccountType::NetworkService,
            std::wstring const & runasPassword = std::wstring(),
            bool runasPasswordEncrypted = false,
            std::wstring const & sslCertificateFindValue = std::wstring(),
            std::wstring const & sslCertStoreLocation = std::wstring(),
            Common::X509FindType::Enum sslCertificateFindType = Common::X509FindType::FindByThumbprint,
            std::wstring const & cpusetCpus = std::wstring(),
            uint cpuShares = 0,
            uint memoryInMB = 0,
            uint memorySwapInMB = 0);

        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        std::wstring const & get_ServiceName() const { return serviceName_; }

        __declspec(property(get=get_ExeName)) std::wstring const & ExeName;
        std::wstring const & get_ExeName() const { return exeName_; }

        __declspec(property(get=get_Arguments)) std::wstring const & Arguments;
        std::wstring const & get_Arguments() const { return arguments_; }

        __declspec(property(get=get_WorkingDirectory)) std::wstring const & WorkingDirectory;
        std::wstring const & get_WorkingDirectory() const { return workingDirectory_; }

        __declspec(property(get=get_Environment)) std::wstring const & Environment;
        std::wstring const & get_Environment() const { return environment_; }

        __declspec(property(get=get_CtrlCSpecified)) bool CtrlCSpecified;
        bool get_CtrlCSpecified() const { return ctrlCSpecified_; }

        __declspec(property(get = get_ServiceNodeName)) std::wstring const & ServiceNodeName;
        std::wstring const & get_ServiceNodeName() const { return serviceNodeName_; }

        __declspec(property(get=get_ParentServiceName)) std::wstring const & ParentServiceName;
        std::wstring const & get_ParentServiceName() const { return parentServiceName_; } 

        __declspec(property(get=get_Port)) UINT Port;
        UINT get_Port() const { return port_; }

        __declspec(property(get = get_CpusetCpus)) std::wstring const & CpusetCpus;
        std::wstring const & get_CpusetCpus() const { return cpusetCpus_; }

        __declspec(property(get = get_CpuShares)) uint CpuShares;
        uint get_CpuShares() const { return cpuShares_; }

        __declspec(property(get = get_MemoryInMB)) uint MemoryInMB;
        uint get_MemoryInMB() const { return memoryInMB_; }

        __declspec(property(get = get_MemorySwapInMB)) uint MemorySwapInMB;
        uint get_MemorySwapInMB() const { return memorySwapInMB_; }

        __declspec(property(get=get_EndpointProtocol)) std::wstring const & EndpointProtocol;
        std::wstring const & get_EndpointProtocol() const { return protocol_; }

        __declspec(property(get=get_RunasSpecified)) bool RunasSpecified;
        bool get_RunasSpecified() const { return runasSpecified_; }

        __declspec(property(get=get_RunasAccountName)) std::wstring const & RunasAccountName;
        std::wstring const & get_RunasAccountName() const { return runasAccountName_; }

        __declspec(property(get=get_RunasAccountType)) ServiceModel::SecurityPrincipalAccountType::Enum RunasAccountType;
        ServiceModel::SecurityPrincipalAccountType::Enum get_RunasAccountType() const { return runasAccountType_; }

        __declspec(property(get=get_RunasPassword)) std::wstring const & RunasPassword;
        std::wstring const & get_RunasPassword() const { return runasPassword_; }

        __declspec(property(get=get_RunasPasswordEncrypted)) bool RunasPasswordEncrypted;
        bool get_RunasPasswordEncrypted() const { return runasPasswordEncrypted_; }

        __declspec(property(get=get_SslCertificateFindValue)) std::wstring const & SslCertificateFindValue;
        std::wstring const & get_SslCertificateFindValue() const { return sslCertificateFindValue_; }

        __declspec(property(get=get_SslCertificateStoreLocation)) std::wstring const & SslCertificateStoreLocation;
        std::wstring const & get_SslCertificateStoreLocation() const { return sslCertStoreLocation_; }

        __declspec(property(get=get_SslCertificateFindType)) Common::X509FindType::Enum SslCertificateFindType;
        Common::X509FindType::Enum get_SslCertificateFindType() const { return sslCertificateFindType_; }

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_22(
            serviceName_,
            exeName_,
            arguments_,
            workingDirectory_,
            environment_,
            ctrlCSpecified_,
            serviceNodeName_,
            parentServiceName_,
            port_,
            protocol_,
            runasSpecified_,
            runasAccountName_,
            runasAccountType_,
            runasPassword_,
            runasPasswordEncrypted_,
            sslCertificateFindValue_,
            sslCertStoreLocation_,
            sslCertificateFindType_,
            cpusetCpus_,
            cpuShares_,
            memoryInMB_,
            memorySwapInMB_);

    private:

        std::wstring serviceName_;
        std::wstring exeName_;
        std::wstring arguments_;
        std::wstring workingDirectory_;
        std::wstring environment_; 
        bool ctrlCSpecified_;
        std::wstring serviceNodeName_;
        std::wstring parentServiceName_;
        UINT port_;
        std::wstring protocol_;
        bool runasSpecified_;
        std::wstring runasAccountName_;
        ServiceModel::SecurityPrincipalAccountType::Enum runasAccountType_;
        std::wstring runasPassword_;
        bool runasPasswordEncrypted_;
        std::wstring sslCertificateFindValue_;
        std::wstring sslCertStoreLocation_;
        Common::X509FindType::Enum sslCertificateFindType_;
        std::wstring cpusetCpus_;
        uint cpuShares_;
        uint memoryInMB_;
        uint memorySwapInMB_;
    };
}
