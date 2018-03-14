// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class StartRegisterApplicationHostReply : public Serialization::FabricSerializable
    {
    public:
        StartRegisterApplicationHostReply();
        StartRegisterApplicationHostReply(
            std::wstring const & nodeId,
            uint64 nodeInstanceId,
            std::wstring const & nodeName,
            DWORD nodeHostProcessId,
            std::wstring const & nodeType,
            std::wstring const & ipAddressOrFQDN,
            std::wstring const & clientConnectionAddress,
            std::wstring const & deploymentDirectory,
            HANDLE nodeLeaseHandle,
            Common::TimeSpan initialLeaseTTL,
            Common::TimeSpan const & nodeLeaseDuration,
            Common::ErrorCode const & errorCode,
            std::wstring const &nodeWorkFolder,
            std::map<wstring, wstring> const& logicalApplicationDirectories,
            std::map<wstring, wstring> const& logicalNodeDirectories,
            KtlLogger::SharedLogSettingsSPtr applicationSharedLogSettings,
            KtlLogger::SharedLogSettingsSPtr systemServicesSharedLogSettings);

        __declspec(property(get=get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        __declspec(property(get=get_NodeType)) std::wstring const & NodeType;
        std::wstring const & get_NodeType() const { return nodeType_; }

        __declspec(property(get=get_IPAddressOrFQDN)) std::wstring const & IPAddressOrFQDN;
        std::wstring const & get_IPAddressOrFQDN() const { return ipAddressOrFQDN_; }

        __declspec(property(get=get_NodeInstanceId)) uint64 NodeInstanceId;
        uint64 get_NodeInstanceId() { return nodeInstanceId_; }

        __declspec(property(get=get_ProcessId)) DWORD NodeHostProcessId;
        DWORD get_ProcessId() const { return nodeHostProcessId_; }

        __declspec(property(get=get_LeaseHandle)) HANDLE NodeLeaseHandle;
        HANDLE get_LeaseHandle() const { return reinterpret_cast<HANDLE>(nodeLeaseHandle_); }

        __declspec(property(get=get_LeaseDuration)) Common::TimeSpan NodeLeaseDuration;
        Common::TimeSpan get_LeaseDuration() const { return nodeLeaseDuration_; }

        __declspec(property(get=get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return errorCode_; }

        __declspec(property(get=get_ClientConnectionAddress)) std::wstring const & ClientConnectionAddress;
        std::wstring const & get_ClientConnectionAddress() const { return clientConnectionAddress_; }

        __declspec(property(get=get_DeploymentDirectory)) std::wstring const & DeploymentDirectory;
        std::wstring const & get_DeploymentDirectory() const { return deploymentDirectory_; }

        __declspec(property(get = get_NodeWorkFolder)) std::wstring const & NodeWorkFolder;
        std::wstring const & get_NodeWorkFolder() const { return nodeWorkFolder_; }

        __declspec(property(get = get_LogicalApplicationDirectories)) std::map<wstring, wstring> LogicalApplicationDirectories;
        inline std::map<wstring, wstring> const & get_LogicalApplicationDirectories() const { return logicalApplicationDirectories_; };

        __declspec(property(get = get_LogicalNodeDirectories)) std::map<wstring, wstring> LogicalNodeDirectories;
        inline std::map<wstring, wstring> const & get_LogicalNodeDirectories() const { return logicalNodeDirectories_; };

        Common::TimeSpan InitialLeaseTTL() const { return initialLeaseTTL_; }

        __declspec(property(get = get_ApplicationSharedLogSettings)) KtlLogger::SharedLogSettingsSPtr ApplicationSharedLogSettings;
        KtlLogger::SharedLogSettingsSPtr get_ApplicationSharedLogSettings() const { return applicationSharedLogSettings_; }

        __declspec(property(get = get_SystemServicesSharedLogSettings)) KtlLogger::SharedLogSettingsSPtr SystemServicesSharedLogSettings;
        KtlLogger::SharedLogSettingsSPtr get_SystemServicesSharedLogSettings() const { return systemServicesSharedLogSettings_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_17(
            nodeId_,
            nodeHostProcessId_,
            nodeLeaseHandle_,
            nodeLeaseDuration_,
            errorCode_,
            nodeInstanceId_,
            nodeName_,
            nodeType_,
            clientConnectionAddress_,
            deploymentDirectory_,
            ipAddressOrFQDN_,
            nodeWorkFolder_,
            initialLeaseTTL_,
            logicalApplicationDirectories_,
            logicalNodeDirectories_,
            applicationSharedLogSettings_,
            systemServicesSharedLogSettings_);

    private:
        std::wstring nodeId_;
        DWORD nodeHostProcessId_;
        uint64 nodeLeaseHandle_;
        uint64 nodeInstanceId_;
        std::wstring nodeName_;
        std::wstring nodeType_;
        std::wstring ipAddressOrFQDN_;
        std::wstring clientConnectionAddress_;
        std::wstring deploymentDirectory_;
        Common::TimeSpan nodeLeaseDuration_;
        Common::ErrorCode errorCode_;
        std::wstring nodeWorkFolder_;
        Common::TimeSpan initialLeaseTTL_; 
        KtlLogger::SharedLogSettingsSPtr applicationSharedLogSettings_;
        KtlLogger::SharedLogSettingsSPtr systemServicesSharedLogSettings_;
        std::wstring sharedLogFilePath_;
        std::wstring sharedLogFileId_;
        int sharedLogFileSizeInMB_;
        std::map<wstring, wstring> logicalApplicationDirectories_;
        std::map<wstring, wstring> logicalNodeDirectories_;
    };
}
