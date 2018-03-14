// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EnumeratePropertiesResult 
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
    public:
        EnumeratePropertiesResult()
            : enumerationStatus_()
            , properties_()
            , continuationToken_()
        {
        }

        EnumeratePropertiesResult(
            ::FABRIC_ENUMERATION_STATUS enumerationStatus,
            std::vector<NamePropertyResult> && properties,
            EnumeratePropertiesToken const & continuationToken)
            : enumerationStatus_(enumerationStatus)
            , properties_(std::move(properties))
            , continuationToken_(continuationToken)
        {
        }

        EnumeratePropertiesResult(
            EnumeratePropertiesResult const & other)
            : enumerationStatus_(other.enumerationStatus_)
            , properties_(other.properties_)
            , continuationToken_(other.continuationToken_)
        {
        }

        __declspec(property(get=get_FABRIC_ENUMERATION_STATUS)) ::FABRIC_ENUMERATION_STATUS FABRIC_ENUMERATION_STATUS;
        __declspec(property(get=get_Properties)) std::vector<NamePropertyResult> const & Properties;
        __declspec(property(get=get_ContinuationToken)) EnumeratePropertiesToken const & ContinuationToken;

        ::FABRIC_ENUMERATION_STATUS get_FABRIC_ENUMERATION_STATUS() const { return enumerationStatus_; }
        std::vector<NamePropertyResult> const & get_Properties() const { return properties_; }
        EnumeratePropertiesToken const & get_ContinuationToken() const { return continuationToken_; }

        std::vector<NamePropertyResult> && TakeProperties() { return std::move(properties_); };

        bool IsFinished() const { return (enumerationStatus_ & FABRIC_ENUMERATION_FINISHED_MASK) != 0; }

        static size_t EstimateSerializedSize(NamePropertyResult const &);
        static size_t EstimateSerializedSize(size_t uriLength, size_t propertyNameLength, size_t byteCount);

        FABRIC_FIELDS_03(enumerationStatus_, properties_, continuationToken_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(properties_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(enumerationStatus_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(continuationToken_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        ::FABRIC_ENUMERATION_STATUS enumerationStatus_;
        std::vector<NamePropertyResult> properties_;
        EnumeratePropertiesToken continuationToken_;
    };
}
