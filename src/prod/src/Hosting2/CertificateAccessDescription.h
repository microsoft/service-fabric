// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class CertificateAccessDescription : public Serialization::FabricSerializable
    {

    public:
        CertificateAccessDescription();
        CertificateAccessDescription(
            ServiceModel::SecretsCertificateDescription const & certificate,
            std::vector<std::wstring> const & userSids,
            DWORD accessMask);
        CertificateAccessDescription(CertificateAccessDescription const & other);
        CertificateAccessDescription(CertificateAccessDescription && other);

        CertificateAccessDescription const & operator = (CertificateAccessDescription const & other);
        CertificateAccessDescription const & operator = (CertificateAccessDescription && other);

        __declspec(property(get=get_SecretsCertificate)) ServiceModel::SecretsCertificateDescription const & SecretsCertificate;
        ServiceModel::SecretsCertificateDescription const & get_SecretsCertificate() const { return certificate_; }

         __declspec(property(get=get_UserSids)) std::vector<std::wstring> const & UserSids;
        std::vector<std::wstring> const & get_UserSids() const { return userSids_; }

        __declspec(property(get=get_AccessMask)) DWORD AccessMask;
        DWORD get_AccessMask() const { return accessMask_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(certificate_, userSids_, accessMask_);

    private:
        ServiceModel::SecretsCertificateDescription certificate_;
        std::vector<std::wstring> userSids_;
        DWORD accessMask_;
    };
}
DEFINE_USER_ARRAY_UTILITY(Hosting2::CertificateAccessDescription);
