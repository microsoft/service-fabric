// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageModel
    {
        class RunLayoutSpecification
        {
            DENY_COPY(RunLayoutSpecification)

        public:
            RunLayoutSpecification();
            RunLayoutSpecification(std::wstring const & runRoot);

            std::wstring GetApplicationFolder(
                std::wstring const & applicationId) const;

            std::wstring GetApplicationWorkFolder(
                std::wstring const & applicationId) const;

            std::wstring GetApplicationLogFolder(
                std::wstring const & applicationId) const;

            std::wstring GetApplicationTempFolder(
                std::wstring const & applicationId) const;

            std::wstring GetApplicationSettingsFolder(
                std::wstring const & applicationId) const;

            std::wstring GetApplicationPackageFile(
                std::wstring const & applicationId,
                std::wstring const & applicationRolloutVersion) const;

            std::wstring GetServicePackageFile(
                std::wstring const & applicationId,
                std::wstring const & servicePackageName,
                std::wstring const & servicePackageRolloutVersion) const;

            std::wstring GetCurrentServicePackageFile(
                std::wstring const & applicationId,
                std::wstring const & servicePackageName,
                std::wstring const & servicePackageActivationId) const;

            std::wstring GetEndpointDescriptionsFile(
                std::wstring const & applicationId,
                std::wstring const & servicePackageName,
                std::wstring const & servicePackageActivationId) const;

            std::wstring GetPrincipalSIDsFile(
                std::wstring const & applicationId) const;

            std::wstring GetServiceManifestFile(
                std::wstring const & applicationId,
                std::wstring const & serviceManifestName,
                std::wstring const & serviceManifestVersion) const;

            std::wstring GetCodePackageFolder(
                std::wstring const & applicationId,
                std::wstring const & servicePackageName,
                std::wstring const & codePackageName,
                std::wstring const & codePackageVersion,
                bool isShared) const;

            std::wstring GetConfigPackageFolder(
                std::wstring const & applicationId,
                std::wstring const & servicePackageName,
                std::wstring const & configPackageName,
                std::wstring const & configPackageVersion,
                bool isShared) const;

            std::wstring GetDataPackageFolder(
                std::wstring const & applicationId,
                std::wstring const & servicePackageName,
                std::wstring const & dataPackageName,
                std::wstring const & dataPackageVersion,
                bool isShared) const;

            std::wstring GetSettingsFile(
                std::wstring const & configPackageFolder) const;

            std::wstring GetSubPackageArchiveFile(
                std::wstring const & packageFolder) const;

            _declspec(property(get=get_Root, put=set_Root)) std::wstring & Root;
            std::wstring const & get_Root() const { return runRoot_; }
            void set_Root(std::wstring const & value) { runRoot_.assign(value); }

        private:
            std::wstring runRoot_;
            static Common::GlobalWString SharedFolderSuffix;
        };
    }
}
