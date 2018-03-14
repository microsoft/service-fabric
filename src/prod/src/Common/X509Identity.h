// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class X509Identity
    {
    public:
        typedef std::shared_ptr<X509Identity> SPtr;

        enum Type
        {
            None,
            Thumbprint,
            PublicKey
        };

        X509Identity();
        virtual ~X509Identity();

        virtual bool operator == (X509Identity const & rhs) const = 0;
        bool operator != (X509Identity const & rhs) const;

        virtual bool operator < (X509Identity const & rhs) const = 0;

        virtual void WriteTo(TextWriter & w, FormatOptions const &) const = 0;

        virtual Type IdType() const = 0;
    };

    class X509IdentitySet
    {
    public:
        ErrorCode SetToThumbprints(std::wstring const & thumbprints);

        ErrorCode AddThumbprint(std::wstring const & thumbprint);
        void AddPublicKey(PCCertContext certContext);

        void Add(X509Identity::SPtr && x509Identity);
        void Add(X509Identity::SPtr const & x509Identity);
        void Add(X509IdentitySet const & other);

        bool operator==(X509IdentitySet const & rhs) const;
        bool operator!=(X509IdentitySet const & rhs) const;

        bool Contains(X509Identity::SPtr const & x509Identity) const;
        bool Contains(X509IdentitySet const & rhs) const;

        size_t Size() const;
        bool IsEmpty() const;

        void WriteTo(TextWriter & w, FormatOptions const &) const;
        std::wstring ToString() const;
        std::vector<std::wstring> ToStrings() const;

    private:
        struct X509IdentitySPtrLess
        {
            bool operator()(X509Identity::SPtr const & lhs, X509Identity::SPtr const & rhs) const
            {
                return (*lhs) < (*rhs);
            }
        };
        std::set<X509Identity::SPtr, X509IdentitySPtrLess> set_;
    };
}
