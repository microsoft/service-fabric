// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common 
{
    class X509FindSubjectAltName : public Common::X509FindValue
    {
        DENY_COPY(X509FindSubjectAltName);

    public:
        typedef std::shared_ptr<X509FindSubjectAltName> SPtr;

        static ErrorCode Create(std::wstring const & nameTypeAndValue, _Out_ SPtr & result);
        static ErrorCode Create(CERT_ALT_NAME_ENTRY const & certAltName, _Out_ SPtr & result);

        X509FindSubjectAltName();

        ErrorCode Initialize(std::wstring const & nameTypeAndValue);
        ErrorCode Initialize(CERT_ALT_NAME_ENTRY const & certAltName);

        X509FindType::Enum Type() const override;
        void const * Value() const override;
        bool PrimaryValueEqualsTo(X509FindSubjectAltName const & other) const;

        void* PrimaryValueToPublicPtr(Common::ScopedHeap & heap) const override;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    private:
        void OnWriteTo(Common::TextWriter & w, Common::FormatOptions const &) const override;
        bool EqualsTo(X509FindValue const & other) const override;

        CERT_ALT_NAME_ENTRY certAltName_;
        ByteBuffer nameBuffer_;
    };
}
