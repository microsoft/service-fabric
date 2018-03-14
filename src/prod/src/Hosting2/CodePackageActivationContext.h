// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class CodePackageActivationContext
        : public Common::ComponentRoot,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(CodePackageActivationContext);

    public:
        typedef std::map<std::wstring, Common::ConfigSettings> ConfigSettingsMap;

    public:

        static Common::ErrorCode CreateCodePackageActivationContext(
            CodePackageInstanceIdentifier const & codePackageInstanceId,
            ServiceModel::RolloutVersion const & applicationRolloutVersion,
            std::wstring const& deploymentRoot,
            Client::IpcHealthReportingClientSPtr & ipcHealthReportingClient,
            FabricNodeContextSPtr & nodeContext,
            bool isContainerHost,
            CodePackageActivationContextSPtr & codePackageActivationContextSPtr);

        __declspec(property(get = get_CodePackageInstanceId)) CodePackageInstanceIdentifier const & CodePackageInstanceId;
        CodePackageInstanceIdentifier const & get_CodePackageInstanceId() { return codePackageInstanceIdentifier_; }

        __declspec(property(get = get_ContextId)) std::wstring const & ContextId;
        std::wstring const & get_ContextId() { return contextId_; }

        __declspec(property(get = get_CodePackageName)) std::wstring const & CodePackageName;
        std::wstring const & get_CodePackageName() { return codePackageInstanceIdentifier_.CodePackageName; }

        __declspec(property(get = get_CodePackageVersion)) std::wstring const & CodePackageVersion;
        std::wstring const & get_CodePackageVersion() { return codePackageVersion_; }

        __declspec(property(get = get_ServicePackageName)) std::wstring const & ServicePackageName;
        std::wstring const & get_ServicePackageName()
        {
            return codePackageInstanceIdentifier_.ServicePackageInstanceId.ServicePackageName;
        }

        __declspec(property(get = get_WorkDirectory)) std::wstring const & WorkDirectory;
        std::wstring const & get_WorkDirectory() { return workingDirectory_; }

        __declspec(property(get = get_LogDirectory)) std::wstring const & LogDirectory;
        std::wstring const & get_LogDirectory() { return logDirectory_; }

        __declspec(property(get = get_TempDirectory)) std::wstring const & TempDirectory;
        std::wstring const & get_TempDirectory() { return tempDirectory_; }

        __declspec(property(get = get_ApplicationName)) std::wstring const & ApplicationName;
        std::wstring const & get_ApplicationName() { return applicationName_; }

        __declspec(property(get = get_ApplicationTypeName)) std::wstring const & ApplicationTypeName;
        std::wstring const & get_ApplicationTypeName() { return applicationTypeName_; }

        __declspec(property(get = get_ServicePublishAddress)) std::wstring const & ServicePublishAddress;
        std::wstring const & get_ServicePublishAddress() { return servicePublishAddress_; }

        __declspec(property(get = get_ServiceListenAddress)) std::wstring const & ServiceListenAddress;
        std::wstring const & get_ServiceListenAddress() { return serviceListenAddress_; }

        __declspec(property(get = get_EndpointResourceList)) Common::ReferencePointer<FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST> & EndpointResourceList;
        Common::ReferencePointer<FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST> & get_EndpointResourceList() { return endpointResourceList_; }

        __declspec(property(get = get_ServiceTypesList)) Common::ReferencePointer<FABRIC_SERVICE_TYPE_DESCRIPTION_LIST> & ServiceTypesList;
        Common::ReferencePointer<FABRIC_SERVICE_TYPE_DESCRIPTION_LIST> & get_ServiceTypesList() { return serviceTypeList_; }

        __declspec(property(get = get_ServiceGroupTypesList)) Common::ReferencePointer<FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST> & ServiceGroupTypesList;
        Common::ReferencePointer<FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST> & get_ServiceGroupTypesList() { return serviceGroupTypeList_; }

        __declspec(property(get = get_ApplicationPrincipals)) Common::ReferencePointer<FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION> & ApplicationPrincipals;
        Common::ReferencePointer<FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION> & get_ApplicationPrincipals() { return applicationPrincipals_; }

        void RegisterITentativeCodePackageActivationContext(std::wstring const& sourceId, ITentativeCodePackageActivationContext const &);
        void UnregisterITentativeCodePackageActivationContext(std::wstring const& sourceId);

        void OnServicePackageChanged();

        Common::ErrorCode GetServiceManifestDescription(__out std::shared_ptr<ServiceModel::ServiceManifestDescription> & serviceManifest);

        __declspec(property(get = get_Test_CodePackageId)) CodePackageInstanceIdentifier const & Test_CodePackageId;
        CodePackageInstanceIdentifier const & get_Test_CodePackageId() { return codePackageInstanceIdentifier_; }

        __declspec(property(get = get_Test_ConfigSettingsMap)) ConfigSettingsMap const & Test_ConfigSettingsMap;
        ConfigSettingsMap const & get_Test_ConfigSettingsMap() { return configSettingsMap_; }

        void GetCodePackageNames(Common::StringCollection &);
        void GetConfigurationPackageNames(Common::StringCollection &);
        void GetDataPackageNames(Common::StringCollection &);
        void GetServiceManifestName(std::wstring &);
        void GetServiceManifestVersion(std::wstring &);

        Common::ErrorCode GetCodePackage(std::wstring const& codePackageName, Common::ComPointer<IFabricCodePackage> &);
        Common::ErrorCode GetConfigurationPackage(std::wstring const& configPackageName, Common::ComPointer<IFabricConfigurationPackage> &);
        Common::ErrorCode GetDataPackage(std::wstring const& dataPackageName, Common::ComPointer<IFabricDataPackage> &);

        Common::ErrorCode ReportApplicationHealth(
            ServiceModel::HealthInformation && healthInfoObj,
            ServiceModel::HealthReportSendOptionsUPtr && sendOptions);
        Common::ErrorCode ReportDeployedApplicationHealth(
            ServiceModel::HealthInformation && healthInfoObj,
            ServiceModel::HealthReportSendOptionsUPtr && sendOptions);
        Common::ErrorCode ReportDeployedServicePackageHealth(
            ServiceModel::HealthInformation && healthInfoObj,
            ServiceModel::HealthReportSendOptionsUPtr && sendOptions);
        
        Common::ErrorCode GetDirectory(std::wstring const& logicalDirectoryName, std::wstring &);

        // ***********************************
        // Service package change notification
        // ***********************************

        LONGLONG RegisterCodePackageChangeHandler(
            Common::ComPointer<IFabricCodePackageChangeHandler> const &,
            std::wstring const & sourceId);
        void UnregisterCodePackageChangeHandler(LONGLONG);

        LONGLONG RegisterConfigurationPackageChangeHandler(
            Common::ComPointer<IFabricConfigurationPackageChangeHandler> const &,
            std::wstring const & sourceId);
        void UnregisterConfigurationPackageChangeHandler(LONGLONG);

        LONGLONG RegisterDataPackageChangeHandler(
            Common::ComPointer<IFabricDataPackageChangeHandler> const &,
            std::wstring const & sourceId);
        void UnregisterDataPackageChangeHandler(LONGLONG);

    private:

        CodePackageActivationContext(
            CodePackageInstanceIdentifier const & codePackageInstanceId,
            ServiceModel::RolloutVersion const & applicationRolloutVersion,
            std::wstring const& deploymentRoot,
            Client::IpcHealthReportingClientSPtr & ipcHealthReportingClient,
            FabricNodeContextSPtr & nodeContext,
            bool isContainerHost);

        Common::ErrorCode EnsureData();

        template <class TPackageInterface>
        class PackageChangeDescription
        {
        public:
            PackageChangeDescription(
                Common::ComPointer<TPackageInterface> && oldPackage,
                Common::ComPointer<TPackageInterface> && newPackage)
                : OldPackage(move(oldPackage))
                , NewPackage(move(newPackage))
            {
            }

            Common::ComPointer<TPackageInterface> OldPackage;
            Common::ComPointer<TPackageInterface> NewPackage;
        };

        Common::ErrorCode ReadData(
            __inout ServiceModel::ServicePackageDescription &,
            __inout ServiceModel::ServiceManifestDescription &,
            __inout ConfigSettingsMap &);

        Common::ErrorCode ReadData(
            __inout ServiceModel::ApplicationPackageDescription &,
            __inout ServiceModel::ServicePackageDescription &,
            __inout ServiceModel::ServiceManifestDescription &,
            __inout ConfigSettingsMap &);

        // ***********************************
        // Service package change notification
        // ***********************************

        template <class TPackageInterface, class TPackageDescription>
        std::vector<PackageChangeDescription<TPackageInterface>> ComputePackageChanges(
            std::vector<TPackageDescription> const & oldPackages,
            std::vector<TPackageDescription> const & newPackages,
            ConfigSettingsMap const & oldConfigSettings,
            ConfigSettingsMap const & newConfigSettings,
            std::wstring const& oldServiceManifestVersion,
            std::wstring const& newServiceManifestVersion);

        template <class TPackageInterface, class TPackageDigestedDescription>
        Common::ComPointer<TPackageInterface> CreateComPackage(TPackageDigestedDescription const &, std::wstring const& serviceManifestVersion, ConfigSettingsMap const &);

        template <>
        Common::ComPointer<IFabricCodePackage> CreateComPackage(ServiceModel::DigestedCodePackageDescription const &, std::wstring const& serviceManifestVersion, ConfigSettingsMap const &);

        template <>
        Common::ComPointer<IFabricConfigurationPackage> CreateComPackage(ServiceModel::DigestedConfigPackageDescription const &, std::wstring const& serviceManifestVersion, ConfigSettingsMap const &);

        template <>
        Common::ComPointer<IFabricDataPackage> CreateComPackage(ServiceModel::DigestedDataPackageDescription const &, std::wstring const& serviceManifestVersion, ConfigSettingsMap const &);

        void NotifyAllPackageChangeHandlers(
            std::vector<PackageChangeDescription<IFabricCodePackage>> const &,
            std::vector<PackageChangeDescription<IFabricConfigurationPackage>> const &,
            std::vector<PackageChangeDescription<IFabricDataPackage>> const &);

        template <class TPackageInterface, class THandlerInterface>
        void NotifyChangeHandlers(std::vector<PackageChangeDescription<TPackageInterface>> const & changedPackages);

        template <class THandlerInterface>
        std::vector<std::shared_ptr<PackageChangeHandler<THandlerInterface>>> CopyHandlers();

        template <>
        std::vector<std::shared_ptr<PackageChangeHandler<IFabricCodePackageChangeHandler>>> CopyHandlers();

        template <>
        std::vector<std::shared_ptr<PackageChangeHandler<IFabricConfigurationPackageChangeHandler>>> CopyHandlers();

        template <>
        std::vector<std::shared_ptr<PackageChangeHandler<IFabricDataPackageChangeHandler>>> CopyHandlers();

        // Locks and state
        Common::RwLock handlersLock_;
        Common::RwLock dataLock_;
        std::vector<std::shared_ptr<PackageChangeHandler<IFabricCodePackageChangeHandler>>> codeChangeHandlers_;
        std::vector<std::shared_ptr<PackageChangeHandler<IFabricConfigurationPackageChangeHandler>>> configChangeHandlers_;
        std::vector<std::shared_ptr<PackageChangeHandler<IFabricDataPackageChangeHandler>>> dataChangeHandlers_;
        std::map<std::wstring, Common::ReferencePointer<ITentativeCodePackageActivationContext>> sourcePointers_;
        LONGLONG nextHandlerId_;

        // This data does not change for a notification only upgrade
        Management::ImageModel::RunLayoutSpecification runLayout_;
        CodePackageInstanceIdentifier codePackageInstanceIdentifier_;
        std::wstring applicationRolloutVersion_;
        std::wstring codePackageVersion_;
        std::wstring applicationId_;
        std::wstring applicationName_;
        std::wstring applicationTypeName_;
        std::wstring contextId_;
        std::wstring workingDirectory_;
        std::wstring logDirectory_;
        std::wstring tempDirectory_;
        std::wstring serviceListenAddress_;
        std::wstring servicePublishAddress_;
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST> endpointResourceList_;
        Common::ReferencePointer<FABRIC_SERVICE_TYPE_DESCRIPTION_LIST> serviceTypeList_;
        Common::ReferencePointer<FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST> serviceGroupTypeList_;
        Common::ReferencePointer<FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION> applicationPrincipals_;

        // Mutable data can change on notification only upgrades
        std::shared_ptr<ServiceModel::ServicePackageDescription> servicePackage_;
        std::shared_ptr<ServiceModel::ServiceManifestDescription> serviceManifest_;
        ConfigSettingsMap configSettingsMap_;

        Client::IpcHealthReportingClientSPtr ipcHealthReportingClient_;
        FabricNodeContextSPtr nodeContext_;
        bool isContainerHost_;

        class WorkDirectories;
        std::unique_ptr<WorkDirectories> workDirectoriesUPtr_;

    }; // end class CodePackageActivationContext

    // templated functions

    template <class TPackageInterface, class TDigestedPackageDescription>
    std::vector<CodePackageActivationContext::PackageChangeDescription<TPackageInterface>> CodePackageActivationContext::ComputePackageChanges(
        std::vector<TDigestedPackageDescription> const & oldPackages,
        std::vector<TDigestedPackageDescription> const & newPackages,
        ConfigSettingsMap const & oldConfigSettings,
        ConfigSettingsMap const & newConfigSettings,
        std::wstring const& oldServiceManifestVersion,
        std::wstring const& newServiceManifestVersion)
    {
        vector<PackageChangeDescription<TPackageInterface>> changedPackages;

        for (auto iter = oldPackages.begin(); iter != oldPackages.end(); ++iter)
        {
            TDigestedPackageDescription const & oldPackage = *iter;
            auto findIter = find_if(newPackages.begin(), newPackages.end(), [&oldPackage](TDigestedPackageDescription const & newPackage)
            {
                return StringUtility::AreEqualCaseInsensitive(oldPackage.Name, newPackage.Name);
            });

            if (findIter != newPackages.end())
            {
                // existing package
                TDigestedPackageDescription const & newPackage = *findIter;

                if (oldPackage.RolloutVersionValue != newPackage.RolloutVersionValue)
                {
                    changedPackages.push_back(PackageChangeDescription<TPackageInterface>(
                        CreateComPackage<TPackageInterface, TDigestedPackageDescription>(oldPackage, oldServiceManifestVersion, oldConfigSettings),
                        CreateComPackage<TPackageInterface, TDigestedPackageDescription>(newPackage, newServiceManifestVersion, newConfigSettings)));
                }
            }
            else
            {
                // deleted package                
                Common::ComPointer<TPackageInterface> newPackage;
                changedPackages.push_back(PackageChangeDescription<TPackageInterface>(
                    CreateComPackage<TPackageInterface, TDigestedPackageDescription>(oldPackage, oldServiceManifestVersion, oldConfigSettings),
                    std::move(newPackage)));
            }
        }

        for (auto iter = newPackages.begin(); iter != newPackages.end(); ++iter)
        {
            TDigestedPackageDescription const & newPackage = *iter;
            auto findIter = find_if(oldPackages.begin(), oldPackages.end(), [&newPackage](TDigestedPackageDescription const & oldPackage)
            {
                return StringUtility::AreEqualCaseInsensitive(oldPackage.Name, newPackage.Name);
            });

            if (findIter == oldPackages.end())
            {
                // added package
                Common::ComPointer<TPackageInterface> oldPackage;
                changedPackages.push_back(PackageChangeDescription<TPackageInterface>(
                    std::move(oldPackage),
                    CreateComPackage<TPackageInterface, TDigestedPackageDescription>(newPackage, newServiceManifestVersion, newConfigSettings)));
            }
        }

        return std::move(changedPackages);
    }

    template <class TPackageInterface, class TDigestedPackageDescription>
    Common::ComPointer<TPackageInterface> CodePackageActivationContext::CreateComPackage(TDigestedPackageDescription const &, std::wstring const&, ConfigSettingsMap const &)
    {
        Common::Assert::CodingError("CreateComPackage(): Unsupported package interface and description combination");
    }

    template <class TPackageInterface, class THandlerInterface>
    void CodePackageActivationContext::NotifyChangeHandlers(std::vector<PackageChangeDescription<TPackageInterface>> const & changedPackages)
    {
        std::vector<std::shared_ptr<PackageChangeHandler<THandlerInterface>>> handlers = CopyHandlers<THandlerInterface>();

        for (auto iter = handlers.begin(); iter != handlers.end(); ++iter)
        {
            Common::ComPointer<THandlerInterface> const & handler = (*iter)->Handler;

            Common::ComPointer<ITentativeCodePackageActivationContext> comSource;

            wstring sourceId = (*iter)->SourceId;
            Common::ReferencePointer<ITentativeCodePackageActivationContext> snap;
            {
                Common::AcquireReadLock lock(handlersLock_);
                auto sourceIter = sourcePointers_.find(sourceId);
                if (sourceIter != sourcePointers_.end())
                {
                    snap = sourceIter->second;
                    if (snap.GetRawPointer() != NULL && snap->TryAddRef() > 0u)
                    {
                        comSource.SetNoAddRef(snap.GetRawPointer());
                    }
                }
            }

            if (comSource)
            {
                for (auto iter2 = changedPackages.begin(); iter2 != changedPackages.end(); ++iter2)
                {
                    if (iter2->OldPackage.GetRawPointer() == NULL)
                    {
                        handler->OnPackageAdded(
                            snap.GetRawPointer(),
                            iter2->NewPackage.GetRawPointer());
                    }
                    else if (iter2->NewPackage.GetRawPointer() == NULL)
                    {
                        handler->OnPackageRemoved(
                            snap.GetRawPointer(),
                            iter2->OldPackage.GetRawPointer());
                    }
                    else
                    {
                        handler->OnPackageModified(
                            snap.GetRawPointer(),
                            iter2->OldPackage.GetRawPointer(),
                            iter2->NewPackage.GetRawPointer());
                    }
                }
            }
        }
    }

    template <class THandlerInterface>
    std::vector<std::shared_ptr<PackageChangeHandler<THandlerInterface>>> CodePackageActivationContext::CopyHandlers()
    {
        Common::Assert::CodingError("CopyHandlers(): Unsupported change handler interface");
    }
}
