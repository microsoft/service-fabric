// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class ApplicationDownloadSpecification : public Serialization::FabricSerializable
    {
    public:
        ApplicationDownloadSpecification();
        ApplicationDownloadSpecification(
            ServiceModel::ApplicationIdentifier const & applicationId, 
            ServiceModel::ApplicationVersion const & applicationVersion, 
            std::wstring const & applicationName,
            std::vector<ServicePackageDownloadSpecification> && packageDownloads);
        ApplicationDownloadSpecification(ApplicationDownloadSpecification const & other);
        ApplicationDownloadSpecification(ApplicationDownloadSpecification && other);

        __declspec(property(get=get_ApplicationId, put=put_ApplicationId)) ServiceModel::ApplicationIdentifier ApplicationId;
        ServiceModel::ApplicationIdentifier const & get_ApplicationId() const { return applicationId_; }
        void put_ApplicationId(ServiceModel::ApplicationIdentifier const & value) { applicationId_ = value; }

        __declspec(property(get=get_ApplicationVersion, put=put_ApplicationVersion)) ServiceModel::ApplicationVersion AppVersion;
        ServiceModel::ApplicationVersion const & get_ApplicationVersion() const { return applicationVersion_; }
        void put_ApplicationVersion(ServiceModel::ApplicationVersion const & value) { applicationVersion_ = value; }

        __declspec(property(get=get_ApplicationName, put=put_ApplicationName)) std::wstring const & AppName;
        std::wstring const & get_ApplicationName() const { return applicationName_; }
        void put_ApplicationName(std::wstring const & value) { applicationName_ = value; }

        __declspec(property(get=get_PackageDownloads)) std::vector<ServicePackageDownloadSpecification> & PackageDownloads;
        std::vector<ServicePackageDownloadSpecification> & get_PackageDownloads() const { return packageDownloads_; }
        
        ApplicationDownloadSpecification const & operator = (ApplicationDownloadSpecification const & other);
        ApplicationDownloadSpecification const & operator = (ApplicationDownloadSpecification && other);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

    private:
        ServiceModel::ApplicationIdentifier applicationId_;
        ServiceModel::ApplicationVersion applicationVersion_;
        std::wstring applicationName_;
        mutable std::vector<ServicePackageDownloadSpecification> packageDownloads_;
    };
}
