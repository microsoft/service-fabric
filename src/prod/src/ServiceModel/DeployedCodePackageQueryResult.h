// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class DeployedCodePackageQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        DeployedCodePackageQueryResult();
        DeployedCodePackageQueryResult(
            std::wstring const & codePackageName,
            std::wstring const & codePackageVersion,
            std::wstring const & serviceManifestName,
            std::wstring const & servicePackageActivationId,
            HostType::Enum hostType,
            HostIsolationMode::Enum hostIsolationMode,
            DeploymentStatus::Enum deployedCodePackageStatus,
            ULONG runFrequencyInterval,
            bool hasSetupEntryPoint,
            ServiceModel::CodePackageEntryPoint && mainEntryPoint,
            ServiceModel::CodePackageEntryPoint && setupEntryPoint,
            std::wstring const & serviceNameInternalUseOnly);

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM & publicDeployedCodePackageQueryResult) const ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        Common::ErrorCode FromPublicApi(__in FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM const & publicDeployedCodePackageQueryResult);

        __declspec(property(get=get_CodePackageName)) std::wstring const &CodePackageName;
        std::wstring const& get_CodePackageName() const { return codePackageName_; }

         __declspec(property(get=get_CodePackageVersion)) std::wstring const &CodePackageVersion;
        std::wstring const& get_CodePackageVersion() const { return codePackageVersion_; }

        __declspec(property(get=get_ServiceManifestName)) std::wstring const & ServiceManifestName;
        std::wstring const& get_ServiceManifestName() const { return serviceManifestName_; }

        __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & ServicePackageActivationId;
        std::wstring const & get_ServicePackageActivationId() const { return servicePackageActivationId_; }

        __declspec(property(get = get_HostType)) HostType::Enum HostType;
        HostType::Enum get_HostType() const { return hostType_; }

        __declspec(property(get = get_HostIsolationMode)) HostIsolationMode::Enum HostIsolationMode;
        HostIsolationMode::Enum get_HostIsolationMode() const { return hostIsolationMode_; }

        __declspec(property(get=get_DeploymentStatus)) DeploymentStatus::Enum DeployedCodePackageStatus;
        DeploymentStatus::Enum get_DeploymentStatus() const { return deployedCodePackageStatus_; }

        __declspec(property(get=get_RunFrequencyInterval)) ULONG RunFrequencyInterval;
        ULONG get_RunFrequencyInterval() const { return runFrequencyInterval_; }

        __declspec(property(get=get_EntryPoint)) CodePackageEntryPoint const &EntryPoint;
        CodePackageEntryPoint const& get_EntryPoint() const { return mainEntryPoint_; }

        __declspec(property(get=get_SetupEntryPoint)) CodePackageEntryPoint const &SetupEntryPoint;
        CodePackageEntryPoint const& get_SetupEntryPoint() const { return setupEntryPoint_; }        

        __declspec(property(get=get_HasSetupEntryPoint)) bool HasSetupEntryPoint;
        bool get_HasSetupEntryPoint() const { return hasSetupEntryPoint_; }        

        __declspec(property(get=get_NodeName, put=put_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }
        void put_NodeName(std::wstring const & value) { nodeName_ = value; }

        __declspec(property(get = get_ServiceNameInternalUseOnly)) std::wstring const &ServiceNameInternalUseOnly;
        std::wstring const& get_ServiceNameInternalUseOnly() const { return serviceNameInternalUseOnly_; }

        FABRIC_FIELDS_13(
            codePackageName_, 
            codePackageVersion_, 
            serviceManifestName_, 
            deployedCodePackageStatus_, 
            runFrequencyInterval_, 
            setupEntryPoint_, 
            mainEntryPoint_,
            hasSetupEntryPoint_, 
            servicePackageActivationId_, 
            hostType_, 
            hostIsolationMode_,
            nodeName_,
            serviceNameInternalUseOnly_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, codePackageName_)
            SERIALIZABLE_PROPERTY(Constants::Version, codePackageVersion_)
            SERIALIZABLE_PROPERTY(Constants::ServiceManifestName, serviceManifestName_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::Status, deployedCodePackageStatus_)
            SERIALIZABLE_PROPERTY(Constants::RunFrequencyInterval, runFrequencyInterval_)
            SERIALIZABLE_PROPERTY(Constants::SetupEntryPoint, setupEntryPoint_)
            SERIALIZABLE_PROPERTY(Constants::MainEntryPoint, mainEntryPoint_)
            SERIALIZABLE_PROPERTY(Constants::HasSetupEntryPoint, hasSetupEntryPoint_)
            SERIALIZABLE_PROPERTY(Constants::ServicePackageActivationId, servicePackageActivationId_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HostType, hostType_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HostIsolationMode, hostIsolationMode_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring codePackageName_;
        std::wstring codePackageVersion_;
        std::wstring serviceManifestName_;
        std::wstring servicePackageActivationId_;
        HostType::Enum hostType_;
        HostIsolationMode::Enum hostIsolationMode_;
        DeploymentStatus::Enum deployedCodePackageStatus_;
        ULONG runFrequencyInterval_; 
        CodePackageEntryPoint setupEntryPoint_;
        CodePackageEntryPoint mainEntryPoint_;
        bool hasSetupEntryPoint_;
        std::wstring nodeName_;

        std::wstring serviceNameInternalUseOnly_; // Do not include it in JSON serialization.
    };
}
