// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    typedef std::map<ServiceModel::ServicePackageIdentifier, ServiceModel::ServicePackageResourceGovernanceDescription> ServicePackageResourceGovernanceMap;
    typedef std::map<ServiceModel::ServicePackageIdentifier, ServiceModel::CodePackageContainersImagesDescription> CodePackageContainersImagesMap;

    class ApplicationUpgradeSpecification : public Serialization::FabricSerializable
    {
    public:
        ApplicationUpgradeSpecification();
        ApplicationUpgradeSpecification(
            ApplicationIdentifier const & applicationId, 
            ApplicationVersion const & applicationVersion, 
            std::wstring const & applicationName,
            uint64 instanceId, 
            UpgradeType::Enum upgradeType,
            bool isMonitored,
            bool isManual,
            std::vector<ServicePackageUpgradeSpecification> && packageUpgrades,
            std::vector<ServiceTypeRemovalSpecification> && removedTypes,
            ServicePackageResourceGovernanceMap && spRGSettings = ServicePackageResourceGovernanceMap(),
            CodePackageContainersImagesMap && cpContainersImages = CodePackageContainersImagesMap(),
            std::vector<std::wstring> && networks = std::vector<std::wstring>());

        ApplicationUpgradeSpecification(ApplicationUpgradeSpecification const & other);
        ApplicationUpgradeSpecification(ApplicationUpgradeSpecification && other);

        __declspec(property(get=get_ApplicationId, put=put_ApplicationId)) ApplicationIdentifier const & ApplicationId;
        ApplicationIdentifier const & get_ApplicationId() const { return applicationId_; }
        void put_ApplicationId(ApplicationIdentifier const & value) { applicationId_ = value; }

        __declspec(property(get=get_ApplicationVersion, put=put_ApplicationVersion)) ApplicationVersion const & AppVersion;
        ApplicationVersion const & get_ApplicationVersion() const { return applicationVersion_; }
        void put_ApplicationVersion(ApplicationVersion const & value) { applicationVersion_ = value; }

        __declspec(property(get=get_ApplicationName, put=put_ApplicationName)) std::wstring const & AppName;
        std::wstring const & get_ApplicationName() const { return applicationName_; }
        void put_ApplicationName(std::wstring const & value) { applicationName_ = value; }

        __declspec(property(get=get_InstanceId, put=put_InstanceId)) uint64 InstanceId;
        uint64 get_InstanceId() const { return instanceId_; }
        void put_InstanceId(uint64 value) { instanceId_ = value; }

        __declspec(property(get=get_UpgradeType)) UpgradeType::Enum UpgradeType;
        UpgradeType::Enum get_UpgradeType() const { return upgradeType_; }
        void SetUpgradeType(UpgradeType::Enum value) { upgradeType_ = value; }

        __declspec(property(get=get_IsMonitored)) bool IsMonitored;
        bool get_IsMonitored() const { return isMonitored_; }
        void SetIsMonitored(bool value) { isMonitored_ = value; }

        __declspec(property(get=get_IsManual)) bool IsManual;
        bool get_IsManual() const { return isManual_; }
        void SetIsManual(bool value) { isManual_ = value; }

        __declspec(property(get=get_PackageUpgrades)) std::vector<ServicePackageUpgradeSpecification> const & PackageUpgrades;
        std::vector<ServicePackageUpgradeSpecification> const & get_PackageUpgrades() const { return packageUpgrades_; }
        
        __declspec(property(get=get_RemovedTypes)) std::vector<ServiceTypeRemovalSpecification> const & RemovedTypes;
        std::vector<ServiceTypeRemovalSpecification> const & get_RemovedTypes() const { return removedTypes_; }
        
        __declspec(property(get=get_UpgradedRGSettings)) std::map<ServiceModel::ServicePackageIdentifier, ServiceModel::ServicePackageResourceGovernanceDescription> & UpgradedRGSettings;
        std::map<ServiceModel::ServicePackageIdentifier, ServiceModel::ServicePackageResourceGovernanceDescription> const& get_UpgradedRGSettings() const { return spRGSettings_; }

        __declspec(property(get = get_UpgradedCPContainersImages)) std::map<ServiceModel::ServicePackageIdentifier, ServiceModel::CodePackageContainersImagesDescription> & UpgradedCPContainersImages;
        std::map<ServiceModel::ServicePackageIdentifier, ServiceModel::CodePackageContainersImagesDescription> const& get_UpgradedCPContainersImages() const { return cpContainersImages_; }

        __declspec(property(get = get_Networks)) std::vector<std::wstring> const & Networks;
        std::vector<std::wstring> const & get_Networks() const { return networks_; }

        ApplicationUpgradeSpecification const & operator = (ApplicationUpgradeSpecification const & other);
        ApplicationUpgradeSpecification const & operator = (ApplicationUpgradeSpecification && other);

        bool GetUpgradeVersionForPackage(std::wstring const & packageName, RolloutVersion & result) const;
        bool GetUpgradeVersionForPackage(std::wstring const & packageName, ServicePackageVersionInstance & result) const;
        bool GetUpgradeVersionForServiceType(ServiceTypeIdentifier const & typeId, RolloutVersion & result) const;
        bool GetUpgradeVersionForServiceType(ServiceTypeIdentifier const & typeId, ServicePackageVersionInstance & result) const;
        bool IsServiceTypeBeingDeleted(ServiceTypeIdentifier const & typeId) const;
        bool IsCodePackageBeingUpgraded(ServiceTypeIdentifier const& typeId, std::wstring const& codePackageName) const;

        bool AddPackage(std::wstring const & packageName, RolloutVersion const & version, std::vector<std::wstring> const &);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_12(
            applicationId_,
            applicationVersion_,
            instanceId_,
            upgradeType_,
            isMonitored_,
            packageUpgrades_,
            applicationName_,
            removedTypes_,
            isManual_,
            spRGSettings_,
            cpContainersImages_,
            networks_);

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {
            std::string format = "UpgradeDescription: {Id={{0}};Version={1};AppName={2};InstanceId={3};UpgradeType={4};IsMonitored={5};IsManual={6};PackageUpgrades={7};RemovedTypes={8}}";

            size_t index = 0;
            traceEvent.AddEventField<ServiceModel::ApplicationIdentifier>(format, name + ".appId", index);
            traceEvent.AddEventField<ServiceModel::ApplicationVersion>(format, name + ".version", index);
            traceEvent.AddEventField<std::wstring>(format, name + ".appname", index);
            traceEvent.AddEventField<uint64>(format, name + ".instanceId", index);
            traceEvent.AddEventField<std::wstring>(format, name + ".upgradeType", index);
            traceEvent.AddEventField<bool>(format, name + ".isMonitored", index);
            traceEvent.AddEventField<bool>(format, name + ".isManual", index);
            traceEvent.AddEventField<std::wstring>(format, name + ".packageUpgrades", index);
            traceEvent.AddEventField<std::wstring>(format, name + ".removedTypes", index);
            
            return format;
        }

        void FillEventData(Common::TraceEventContext & context) const;


    private:
        ApplicationIdentifier applicationId_;
        ApplicationVersion applicationVersion_;
        std::wstring applicationName_;
        uint64 instanceId_;
        UpgradeType::Enum upgradeType_;
        bool isMonitored_;
        bool isManual_;
        std::vector<ServicePackageUpgradeSpecification> packageUpgrades_;
        std::vector<ServiceTypeRemovalSpecification> removedTypes_;
        std::map<ServiceModel::ServicePackageIdentifier, ServiceModel::ServicePackageResourceGovernanceDescription> spRGSettings_;
        std::map<ServiceModel::ServicePackageIdentifier, ServiceModel::CodePackageContainersImagesDescription> cpContainersImages_;
        std::vector<std::wstring> networks_;
    };
}