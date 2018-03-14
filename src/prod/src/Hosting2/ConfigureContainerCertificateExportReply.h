// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ConfigureContainerCertificateExportReply : public Serialization::FabricSerializable
    {
    public:
        ConfigureContainerCertificateExportReply();
        ConfigureContainerCertificateExportReply(
            Common::ErrorCode const & error,
            std::wstring const & errorMessage,
            map<std::wstring, std::wstring> certificatePaths,
            map<std::wstring, std::wstring> certificatePasswordPaths);

        __declspec(property(get = get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        __declspec(property(get = get_ErrorMessage)) std::wstring const & ErrorMessage;
        std::wstring const & get_ErrorMessage() const { return errorMessage_; }

        __declspec(property(get = get_CertificatePaths)) map<std::wstring, std::wstring> const & CertificatePaths;
        map<std::wstring, std::wstring> const & get_CertificatePaths() const { return certificatePaths_; }

        __declspec(property(get = get_CertificatePasswordPaths)) map<std::wstring, std::wstring> const & CertificatePasswordPaths;
        map<std::wstring, std::wstring> const & get_CertificatePasswordPaths() const { return certificatePasswordPaths_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_04(error_, errorMessage_, certificatePaths_, certificatePasswordPaths_);

    private:
        Common::ErrorCode error_;
        std::wstring errorMessage_;
        map<std::wstring, std::wstring> certificatePaths_;
        map<std::wstring, std::wstring> certificatePasswordPaths_;
    };
}
