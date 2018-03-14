// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common 
{
    class CommonName : public Common::X509FindValue
    {
        DENY_COPY(CommonName);

    public:
        typedef std::shared_ptr<CommonName> SPtr;

        static ErrorCode Create(std::wstring const & name, _Out_ SPtr & result);

        CommonName(std::wstring const & name);

        X509FindType::Enum Type() const override;
        void const * Value() const override;
        DWORD Flag() const override;
        bool PrimaryValueEqualsTo(CommonName const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    private:
        void Initialize();
        void OnWriteTo(Common::TextWriter & w, Common::FormatOptions const &) const override;
        bool EqualsTo(X509FindValue const & other) const override;

        std::wstring const name_;
        CERT_RDN_ATTR certRdnAttr_;
        CERT_RDN certRdn_;
    };
}
