// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class Claim : public Serialization::FabricSerializable
    {
    public:
        Claim();
            
        Claim(
            std::wstring const& claimType,
            std::wstring const& issuer,
            std::wstring const& originalIssuer,
            std::wstring const& subject,
            std::wstring const& value,
            std::wstring const& valueType);

        __declspec(property(get=get_ClaimType, put=set_ClaimType)) std::wstring const & ClaimType;
        std::wstring const & get_ClaimType() const { return claimType_; }
        void set_ClaimType(std::wstring const & value) { claimType_ = value; }

        __declspec(property(get=get_Issuer, put=set_Issuer)) std::wstring const & Issuer;
        std::wstring const & get_Issuer() const { return issuer_; }
        void set_Issuer(std::wstring const & value) { issuer_ = value; }

        __declspec(property(get=get_OriginalIssuer, put=set_OriginalIssuer)) std::wstring const & OriginalIssuer;
        std::wstring const & get_OriginalIssuer() const { return originalIssuer_; }
        void set_OriginalIssuer(std::wstring const & value) { originalIssuer_ = value; }

        __declspec(property(get=get_Subject, put=set_Subject)) std::wstring const & Subject;
        std::wstring const & get_Subject() const { return subject_; }
        void set_Subject(std::wstring const & value) { claimType_ = value; }

        __declspec(property(get=get_Value, put=set_Value)) std::wstring const & Value;
        std::wstring const & get_Value() const { return value_; }
        void set_Value(std::wstring const & value) { value_ = value; }

        __declspec(property(get=get_ValueType, put=set_ValueType)) std::wstring const & ValueType;
        std::wstring const & get_ValueType() const { return valueType_; }
        void set_ValueType(std::wstring const & value) { valueType_ = value; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_06(claimType_, issuer_, originalIssuer_, subject_, value_, valueType_);

    private:

        std::wstring claimType_;
        std::wstring issuer_;
        std::wstring originalIssuer_;
        std::wstring subject_;
        std::wstring value_;
        std::wstring valueType_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::Claim);
