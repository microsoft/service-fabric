// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define HASH_SIZE_DEFAULT 20 // default to SHA1
namespace Common
{
    class Thumbprint : public X509FindValue, public X509Identity
    {
    public:
        typedef std::shared_ptr<Thumbprint> SPtr;

        static ErrorCode Create(std::wstring const & str, _Out_ SPtr & result);
        static ErrorCode Create(PCCertContext certContext, _Out_ SPtr & result);

        Thumbprint();
        Thumbprint(Thumbprint const & other);
        Thumbprint(Thumbprint && other);

        ErrorCode Initialize(std::wstring const & hashValue);
        ErrorCode Initialize(PCCertContext certContext);

        X509FindType::Enum Type() const override;
        void const * Value() const override; // returns address of CRYPT_HASH_BLOB
        BYTE const * Hash() const; // returns hash value
        DWORD HashSizeInBytes() const;

        bool CertChainShouldBeVerified() const;

        bool PrimaryValueEqualsTo(Thumbprint const & other) const;
        Thumbprint & operator = (Thumbprint const & other);
        Thumbprint & operator = (Thumbprint && other);

        bool operator == (Thumbprint const & rhs) const;
        bool operator != (Thumbprint const & rhs) const;
        bool operator < (Thumbprint const & rhs) const;

        bool operator == (X509Identity const & rhs) const override; // override X509Identity
        bool operator < (X509Identity const & rhs) const override; // override X509Identity

        X509Identity::Type IdType() const override;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    private:
        void SetBlobValue();
        void OnWriteTo(Common::TextWriter & w, Common::FormatOptions const &) const override;
        bool EqualsTo(X509FindValue const & other) const override; // override X509FindValue

        ByteBuffer buffer_;
        CRYPT_HASH_BLOB value_;
        bool certChainShouldBeVerified_;
    };
}
