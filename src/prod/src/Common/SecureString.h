// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class SecureString
    {
        DEFAULT_COPY_CONSTRUCTOR(SecureString)

    public:
        static GlobalWString TracePlaceholder;

    public:
        SecureString();
        explicit SecureString(std::wstring const & secureValue);
        explicit SecureString(std::wstring && secureValue);
        ~SecureString();

        bool operator == (SecureString const & other) const;
        bool operator != (SecureString const & other) const;

        SecureString const & operator = (SecureString const & other);
        SecureString const & operator = (SecureString && other);

        std::wstring const & GetPlaintext() const;  

        bool IsEmpty() const;
        void Append(std::wstring const &);
        void Append(SecureString const &);
        void TrimTrailing(std::wstring const & trimChars);
        void Replace(std::wstring const & stringToReplace, std::wstring const & stringToInsert);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const;

    private:
        void Clear() const;

        std::wstring value_;
    };
}
