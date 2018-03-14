// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
#define RANDOMSESSION  \
    static_cast<RandomFabricTestSession>(FabricTestSession::GetFabricTestSession())    \

    class RandomTestApplicationHelper
    {
        DENY_COPY(RandomTestApplicationHelper)
    public:

        RandomTestApplicationHelper()
            : config_(),
            random_(),
            applicationTypeVersions_(),
            packageRGCapacities_()
        {
        }

        void SetConfig(std::shared_ptr<RandomSessionConfig> config)
        {
            config_ = config;
        }

        void UploadApplicationTypes();
        void ProvisionApplicationTypes();
        void CreateInitialApplicationType();
        void CreateUpgradeApplicationTypes();
        void GetAllServiceNames(size_t applicationTypeVersion, std::vector<std::wstring> &);
        void GetAllStatefulServiceNames(size_t applicationTypeVersion, std::vector<std::wstring> &);
        std::wstring GetVersionString(size_t applicationTypeVersion);
        bool IsStatefulService(size_t applicationTypeVersion, std::wstring const& serviceName);

        __declspec (property(get=get_PackageRGCapacities)) std::map<wstring, int>& PackageRGCapacities;
        std::map<wstring, int>& get_PackageRGCapacities() { return packageRGCapacities_; }

        static Common::GlobalWString AppTypeName;

    private:
        static Common::GlobalWString AppNamePrefix;

        class DefaultService
        {
        public:
            wstring ServiceName;
            wstring ServiceType;
            bool IsStateful;
            int PartitionCount;
            int ReplicaCount;
            ServiceModel::ServicePackageActivationMode::Enum ActivationMode;
        };

        class ServiceType
        {
        public:
            wstring TypeName;
            bool IsStateful;
            bool IsPersisted;
        };

        class CodePackage
        {
        public:
            wstring Name;
            vector<wstring> Types;
            wstring Version;
            bool SkipUpload;
            map<wstring, int> RgPolicies;
        };

        class ServicePackage
        {
        public:
            wstring Name;
            wstring Version;
            vector<ServiceType> Types;
            vector<CodePackage> CodePackages;
            vector<DefaultService> DefaultServices;
            bool SkipUpload;
            map<wstring, int> RgPolicies;
            bool HasMemoryPolicyPerCp;
            
            int GetServicePackageMemoryPolicy();
        };

        class ApplicationType
        {
        public:
            wstring TypeName;
            wstring Version;
            vector<ServicePackage> ServicePackages;
            wstring Operation;
        };

        bool AddServicePackage(ApplicationType &, std::wstring &);
        void GenerateServicePackageRgPolicies(ServicePackage &, bool);

        bool AddCodePackage(ApplicationType &, std::wstring &);
        bool AddCodePackage(ServicePackage &, std::wstring &);
        void GenerateCodePackageRgPolicies(CodePackage &, bool, int);

        bool AddServiceType(ApplicationType &, std::wstring &);
        bool AddServiceType(ServicePackage &, std::wstring &);

        wstring GetPackageRgPolicies(map<wstring, int>& policies, wstring separator = L",");

        bool RemoveServicePackage(ApplicationType &, std::wstring &);
        bool RemoveCodePackage(ApplicationType &, std::wstring &);
        bool RemoveServiceType(ApplicationType &, std::wstring &);
        bool UpgradeCodePackage(ApplicationType &, std::wstring &);
        bool UpgradeConfigPackage(ApplicationType &, std::wstring &);

        ServiceModel::ServicePackageActivationMode::Enum GetRandomServicePackageActivationMode();

        std::shared_ptr<RandomSessionConfig> config_;
        Random random_;
        std::vector<ApplicationType> applicationTypeVersions_;
        std::map<wstring, int> packageRGCapacities_;
    };
};
