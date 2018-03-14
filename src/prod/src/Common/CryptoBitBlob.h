// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class CryptoBitBlob : public Serialization::FabricSerializable
    {
    public:
        typedef std::shared_ptr<CryptoBitBlob> SPtr;
        typedef std::unique_ptr<CryptoBitBlob> UPtr;

        CryptoBitBlob(uint unusedBits = 0);
        CryptoBitBlob(CRYPT_BIT_BLOB const & blob);
        CryptoBitBlob(ByteBuffer && data);
        CryptoBitBlob(CryptoBitBlob const &) = default;
        CryptoBitBlob(CryptoBitBlob &&) = default;

        ErrorCode Initialize(std::wstring const & hexString);
        ErrorCode InitializeAsSignatureHash(PCCertContext certContext);

        auto & Data() { return data_; }
        auto UnusedBits() const { return unusedBits_; }

        CryptoBitBlob & operator = (CryptoBitBlob const & other) = default;
        CryptoBitBlob & operator = (CryptoBitBlob && other) = default;

        bool operator == (CryptoBitBlob const & rhs) const;
        bool operator != (CryptoBitBlob const & rhs) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(data_, unusedBits_);

    private:
        ByteBuffer data_;
        uint unusedBits_ = 0;
    };
}
