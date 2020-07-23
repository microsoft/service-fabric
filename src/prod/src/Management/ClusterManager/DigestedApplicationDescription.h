// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class DigestedApplicationDescription 
        {
        public:
            typedef std::map<ServiceModelPackageName, std::vector<ServiceModel::DigestedCodePackageDescription>> CodePackageDescriptionMap;
            typedef std::map<ServiceModelPackageName, std::vector<std::wstring>> CodePackageNameMap;

            DigestedApplicationDescription() { }

            DigestedApplicationDescription(
                ServiceModel::ApplicationInstanceDescription const & application,
                std::vector<StoreDataServicePackage> const & servicePackages,
                CodePackageDescriptionMap const & codePackages,
                std::vector<StoreDataServiceTemplate> const & serviceTemplates,
                std::vector<Naming::PartitionedServiceDescriptor> const & defaultServices,
                std::map<ServiceModel::ServicePackageIdentifier, ServiceModel::ServicePackageResourceGovernanceDescription> const & rgDescription,
                std::vector<wstring> const & networks);

            bool TryValidate(__out std::wstring & errorDetails);

            std::vector<StoreDataServicePackage> ComputeRemovedServiceTypes(DigestedApplicationDescription const & targetDescription);
            std::vector<StoreDataServicePackage> ComputeAddedServiceTypes(DigestedApplicationDescription const & targetDescription);

            Common::ErrorCode ComputeAffectedServiceTypes(
                DigestedApplicationDescription const & targetDescription, 
                __out std::vector<StoreDataServicePackage> & affectedTypes,
                __out std::vector<StoreDataServicePackage> & movedTypes,
                __out CodePackageNameMap & codePackages,
                __out bool & shouldRestartPackages);

            std::vector<StoreDataServiceTemplate> ComputeRemovedServiceTemplates(DigestedApplicationDescription const & targetDescription);
            std::vector<StoreDataServiceTemplate> ComputeModifiedServiceTemplates(DigestedApplicationDescription const & targetDescription);
            std::vector<StoreDataServiceTemplate> ComputeAddedServiceTemplates(DigestedApplicationDescription const & targetDescription);

            std::vector<ServiceModel::ServicePackageReference> ComputeAddedServicePackages(DigestedApplicationDescription const & targetDescription);

            CodePackageNameMap GetAllCodePackageNames();

            ServiceModel::ApplicationInstanceDescription Application;
            std::vector<StoreDataServicePackage> ServicePackages;
            std::vector<StoreDataServiceTemplate> ServiceTemplates;
            std::vector<Naming::PartitionedServiceDescriptor> DefaultServices;
            CodePackageDescriptionMap CodePackages;
            std::vector<wstring> Networks;

            std::map<ServiceModel::ServicePackageIdentifier, ServiceModel::ServicePackageResourceGovernanceDescription> ResourceGovernanceDescriptions;
            std::map<ServiceModel::ServicePackageIdentifier, ServiceModel::CodePackageContainersImagesDescription> CodePackageContainersImages;

        private:
            bool HasCodePackageChange(
                ServiceModelPackageName const &,
                std::vector<ServiceModel::DigestedCodePackageDescription> const & current, 
                std::vector<ServiceModel::DigestedCodePackageDescription> const & target,
                __inout CodePackageNameMap & codePackages);

            void AddAllCodePackages(
                ServiceModelPackageName const &,
                __inout CodePackageNameMap & codePackages);

            void AddCodePackage(
                __inout CodePackageNameMap & codePackages,
                ServiceModelPackageName const & servicePackageName,
                std::wstring const & codePackageName);
        };
    }
}
