// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageModel
    {
        class BuildLayoutSpecification
        {
            DENY_COPY(BuildLayoutSpecification)

        public:
            BuildLayoutSpecification();
            BuildLayoutSpecification(std::wstring const & buildRoot);

            std::wstring GetApplicationManifestFile() const;

            std::wstring GetSubPackageArchiveFile(
                std::wstring const & packageFolder) const;

            std::wstring GetSubPackageExtractedFolder(
                std::wstring const & archiveFile) const;

            bool IsArchiveFile(std::wstring const & fileName) const;

            std::wstring GetServiceManifestFile(
                std::wstring const & serviceManifestName) const;

            std::wstring GetServiceManifestChecksumFile(
                std::wstring const & serviceManifestName) const;

            std::wstring GetCodePackageFolder(
                std::wstring const & serviceManifestName,
                std::wstring const & codePackageName) const;

            std::wstring GetCodePackageChecksumFile(
                std::wstring const & serviceManifestName,
                std::wstring const & codePackageName) const;

            std::wstring GetConfigPackageFolder(
                std::wstring const & serviceManifestName,
                std::wstring const & configPackageName) const;

            std::wstring GetConfigPackageChecksumFile(
                std::wstring const & serviceManifestName,
                std::wstring const & configPackageName) const;

            std::wstring GetDataPackageFolder(
                std::wstring const & serviceManifestName,
                std::wstring const & dataPackageName) const;

            std::wstring GetDataPackageChecksumFile(
                std::wstring const & serviceManifestName,
                std::wstring const & dataPackageName) const;

            std::wstring GetSettingsFile(
                std::wstring const & configPackageFolder) const;

            std::wstring GetChecksumFile(
                std::wstring const & fileOrDirectoryName) const;

            _declspec(property(get=get_Root, put=set_Root)) std::wstring & Root;
            std::wstring const & get_Root() const { return buildRoot_; }
            void set_Root(std::wstring const & value) { buildRoot_.assign(value); }

        private:
            std::wstring buildRoot_;
            std::wstring GetServiceManifestFolder(
                std::wstring const & serviceManifestName) const;
        };
    }
}
