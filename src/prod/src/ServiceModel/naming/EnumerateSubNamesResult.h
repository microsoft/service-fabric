// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EnumerateSubNamesResult 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DECLARE_SERIALIZATION_OVERHEAD_ESTIMATE()

    public:
        EnumerateSubNamesResult();

        EnumerateSubNamesResult(
            Common::NamingUri const & name,
            ::FABRIC_ENUMERATION_STATUS enumerationStatus,
            std::vector<std::wstring> && subNames,
            EnumerateSubNamesToken const & continuationToken);

        EnumerateSubNamesResult(
            EnumerateSubNamesResult const & other);

        __declspec(property(get=get_ParentName)) Common::NamingUri const & ParentName;
        __declspec(property(get=get_FABRIC_ENUMERATION_STATUS)) ::FABRIC_ENUMERATION_STATUS FABRIC_ENUMERATION_STATUS;
        __declspec(property(get=get_SubNames)) std::vector<std::wstring> const & SubNames;
        __declspec(property(get=get_ContinuationToken)) EnumerateSubNamesToken const & ContinuationToken;

        Common::NamingUri const & get_ParentName() const { return name_; }
        ::FABRIC_ENUMERATION_STATUS get_FABRIC_ENUMERATION_STATUS() const { return enumerationStatus_; }
        std::vector<std::wstring> const & get_SubNames() const { return subNames_; }
        EnumerateSubNamesToken const & get_ContinuationToken() const { return continuationToken_; }

        bool IsFinished() const { return (enumerationStatus_ & FABRIC_ENUMERATION_FINISHED_MASK) != 0; }

        static size_t EstimateSerializedSize(Common::NamingUri const & parentName, Common::NamingUri const & subName);
        static size_t EstimateSerializedSize(size_t uriLength);

        FABRIC_FIELDS_04(name_, enumerationStatus_, subNames_, continuationToken_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::SubNames, subNames_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        Common::NamingUri name_;
        ::FABRIC_ENUMERATION_STATUS enumerationStatus_;
        std::vector<std::wstring> subNames_;
        EnumerateSubNamesToken continuationToken_;
    };
}
