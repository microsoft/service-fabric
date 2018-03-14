// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class X509PubKey : public X509Identity
    {
    public:
        using UPtr = std::unique_ptr<X509PubKey>;

        X509PubKey(PCCertContext certContext);
#ifdef PLATFORM_UNIX
        ~X509PubKey();
#endif

        bool operator == (X509Identity const & rhs) const override;
        bool operator < (X509Identity const & rhs) const override;

        X509Identity::Type IdType() const override;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    private:
#ifdef PLATFORM_UNIX
        X509_ALGOR* algorithm_ = nullptr;
        ASN1_BIT_STRING* pubKey_ = nullptr;
#else
        std::string algObjId_;
        ByteBuffer paramBlob_;
        ByteBuffer keyBlob_;
#endif
    };
}
