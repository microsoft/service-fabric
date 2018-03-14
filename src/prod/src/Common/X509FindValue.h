// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class X509FindValue
    {
    public:
        typedef std::shared_ptr<X509FindValue> SPtr;

        static ErrorCode Create(X509FindType::Enum type, LPCWSTR value, _Out_ SPtr & result);
        static ErrorCode Create(X509FindType::Enum type, std::wstring const & value, _Out_ SPtr & result);

        static ErrorCode Create(
            FABRIC_X509_FIND_TYPE type,
            LPCVOID value,
            LPCVOID secondaryValue,
            _Out_ SPtr & result);

        static ErrorCode Create(
            X509FindType::Enum type,
            std::wstring const & value,
            std::wstring const & secondaryValue,
            _Out_ SPtr & result);

        static ErrorCode Create(
            std::wstring const & type,
            std::wstring const & value,
            std::wstring const & secondaryValue,
            _Out_ SPtr & result);

        virtual ~X509FindValue() = 0;

        virtual X509FindType::Enum Type() const = 0;
        virtual void const * Value() const = 0;
        virtual DWORD Flag() const { return 0; }

        virtual void* PrimaryValueToPublicPtr(Common::ScopedHeap & heap) const;

        bool PrimaryValueEqualsTo(X509FindValue const & other) const;
        bool operator == (X509FindValue const & other) const;
        bool operator != (X509FindValue const & other) const;

        std::pair<std::wstring, std::wstring> ToStrings() const;
        std::wstring PrimaryToString() const;
        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        SPtr const & Secondary() const;
        void SetSecondary(SPtr && secondary);

    private:
        // It is required that the output can be parsed back to X509FindValue
        virtual void OnWriteTo(Common::TextWriter & w, Common::FormatOptions const &) const = 0;
        virtual bool EqualsTo(X509FindValue const & other) const = 0;

        SPtr secondary_;
    };
}
