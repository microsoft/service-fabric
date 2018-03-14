// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EnumeratePropertiesToken 
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
    public:
        EnumeratePropertiesToken();

        EnumeratePropertiesToken(
            std::wstring const & lastEnumeratedPropertyName,
            _int64 propertiesVersion);

        static Common::ErrorCode Create(std::wstring const & escapedToken, __out EnumeratePropertiesToken & token);

        __declspec(property(get=get_PropertyName)) std::wstring const & LastEnumeratedPropertyName;
        std::wstring const & get_PropertyName() const { return lastEnumeratedPropertyName_; }

        __declspec(property(get=get_PropertiesVersion)) _int64 PropertiesVersion;
        _int64 get_PropertiesVersion() const { return propertiesVersion_; }

        __declspec(property(get=get_IsValid)) bool IsValid;
        bool get_IsValid() const { return isValid_; }

        Common::ErrorCode ToEscapedString(__out std::wstring & escapedToken) const;

        FABRIC_FIELDS_03(lastEnumeratedPropertyName_, propertiesVersion_, isValid_);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(lastEnumeratedPropertyName_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        static std::wstring const Delimiter;

        std::wstring lastEnumeratedPropertyName_;
        _int64 propertiesVersion_;
        bool isValid_;
    };
}
