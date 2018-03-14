// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageModel
    {
        class StoreLayoutSpecification
        {
            DENY_COPY(StoreLayoutSpecification)

        public:
            StoreLayoutSpecification();
            StoreLayoutSpecification(std::wstring const & storeRoot);

            std::wstring GetApplicationManifestFile(
                std::wstring const & applicationTypeName,
                std::wstring const & applicationTypeVersion) const;

            std::wstring GetApplicationInstanceFile(
                std::wstring const & applicationTypeName,
                std::wstring const & applicationId,
                std::wstring const & applicationInstanceVersion) const;            

            std::wstring GetApplicationPackageFile(
                std::wstring const & applicationTypeName,
                std::wstring const & applicationId,
                std::wstring const & applicationRolloutVersion) const;

            std::wstring GetServicePackageFile(
                std::wstring const & applicationTypeName,
                std::wstring const & applicationId,
                std::wstring const & servicePackageName,
                std::wstring const & servicePackageRolloutVersion) const;

            std::wstring GetServiceManifestFile(
                std::wstring const & applicationTypeName,
                std::wstring const & serviceManifestName,
                std::wstring const & serviceManifestVersion) const;

            std::wstring GetServiceManifestChecksumFile(
                std::wstring const & applicationTypeName,
                std::wstring const & serviceManifestName,
                std::wstring const & serviceManifestVersion) const;

            std::wstring GetCodePackageFolder(
                std::wstring const & applicationTypeName,
                std::wstring const & serviceManifestName,
                std::wstring const & codePackageName,
                std::wstring const & codePackageVersion) const;

            std::wstring GetCodePackageChecksumFile(
                std::wstring const & applicationTypeName,
                std::wstring const & serviceManifestName,
                std::wstring const & codePackageName,
                std::wstring const & codePackageVersion) const;

            std::wstring GetConfigPackageFolder(
                std::wstring const & applicationTypeName,
                std::wstring const & serviceManifestName,
                std::wstring const & configPackageName,
                std::wstring const & configPackageVersion) const;

            std::wstring GetConfigPackageChecksumFile(
                std::wstring const & applicationTypeName,
                std::wstring const & serviceManifestName,
                std::wstring const & configPackageName,
                std::wstring const & configPackageVersion) const;

            std::wstring GetDataPackageFolder(
                std::wstring const & applicationTypeName,
                std::wstring const & serviceManifestName,
                std::wstring const & dataPackageName,
                std::wstring const & dataPackageVersion) const;

            std::wstring GetDataPackageChecksumFile(
                std::wstring const & applicationTypeName,
                std::wstring const & serviceManifestName,
                std::wstring const & dataPackageName,
                std::wstring const & dataPackageVersion) const;

            std::wstring GetSettingsFile(
                std::wstring const & configPackageFolder) const;

            std::wstring GetApplicationInstanceFolder(
                std::wstring const & applicationTypeName, 
                std::wstring const & applicationId) const;

            std::wstring GetSubPackageArchiveFile(
                std::wstring const & packageFolder) const;

            std::wstring GetSubPackageExtractedFolder(
                std::wstring const & archiveFile) const;

            _declspec(property(get=get_Root, put=set_Root)) std::wstring & Root;
            std::wstring const & get_Root() const { return storeRoot_; }
            void set_Root(std::wstring const & value) { storeRoot_.assign(Common::Path::Combine(value, Store)); }

            std::wstring GetApplicationTypeFolder(std::wstring const & applicationTypeName) const;
            
        private:
            std::wstring storeRoot_;
            static Common::GlobalWString Store;
        };
    }
}
