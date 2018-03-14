// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class CleanupContainerCertificateExportRequest : public Serialization::FabricSerializable
    {
    public:
        CleanupContainerCertificateExportRequest();
        CleanupContainerCertificateExportRequest(
            std::map<std::wstring, std::wstring> certificatePaths,
            std::map<std::wstring, std::wstring> certificatePasswordPaths);

        __declspec(property(get = get_CertificatePaths)) map<std::wstring, std::wstring> const & CertificatePaths;
        map<std::wstring, std::wstring> const & get_CertificatePaths() const { return certificatePaths_; }

        __declspec(property(get = get_CertificatePasswordPaths)) map<std::wstring, std::wstring> const & CertificatePasswordPaths;
        map<std::wstring, std::wstring> const & get_CertificatePasswordPaths() const { return certificatePasswordPaths_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(certificatePaths_, certificatePasswordPaths_);

    private:
        std::map<std::wstring, std::wstring> certificatePaths_;
        std::map<std::wstring, std::wstring> certificatePasswordPaths_;
    };
}
