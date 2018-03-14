// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ConfigureContainerCertificateExportRequest : public Serialization::FabricSerializable
    {
    public:
        ConfigureContainerCertificateExportRequest();
        ConfigureContainerCertificateExportRequest(
            std::map<std::wstring, std::vector<ServiceModel::ContainerCertificateDescription>> const & certificateRef,
            std::wstring const & workDirectoryPath);

        __declspec(property(get=get_CertificateRef)) std::map<std::wstring, std::vector<ServiceModel::ContainerCertificateDescription>> const & CertificateRef;
        std::map<std::wstring, std::vector<ServiceModel::ContainerCertificateDescription>> const & get_CertificateRef() const { return certificateRef_; }

        __declspec(property(get = get_WorkDirectoryPath)) std::wstring const & WorkDirectoryPath;
        std::wstring const & get_WorkDirectoryPath() const { return workDirectoryPath_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(certificateRef_,workDirectoryPath_);

    private:
        std::map<std::wstring, std::vector<ServiceModel::ContainerCertificateDescription>> certificateRef_;
        std::wstring workDirectoryPath_;
    };
}
DEFINE_USER_MAP_UTILITY(std::wstring, std::vector<ServiceModel::ContainerCertificateDescription>)
