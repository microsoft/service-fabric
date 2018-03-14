// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common 
{
    class SubjectName : public Common::X509FindValue
    {
        DENY_COPY(SubjectName);

    public:
        typedef std::shared_ptr<SubjectName> SPtr;
    
        static ErrorCode Create(std::wstring const & name, _Out_ SPtr & result);

        SubjectName(std::wstring const & name);

        ErrorCode Initialize();

        X509FindType::Enum Type() const override;
        void const * Value() const override;
        std::wstring const & Name() const;
        bool PrimaryValueEqualsTo(SubjectName const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    private:
        void OnWriteTo(Common::TextWriter & w, Common::FormatOptions const &) const override;
        bool EqualsTo(X509FindValue const & other) const override;

        std::wstring const name_;
#if defined(PLATFORM_UNIX)
        std::map<std::string, std::string> nameBlob_;
#else
        CERT_NAME_BLOB nameBlob_;
#endif
        std::vector<BYTE> nameBuffer_;
    };
}
