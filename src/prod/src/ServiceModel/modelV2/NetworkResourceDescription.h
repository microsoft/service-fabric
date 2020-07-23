// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class NetworkProperties
            : public ModelType
        {
        public:
            NetworkProperties() = default;

            NetworkProperties(std::wstring const & networkAddressPrefix);

            bool operator==(NetworkProperties const &) const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_ENUM(Constants::KindLowerCase, networkType_)
                SERIALIZABLE_PROPERTY(Constants::NetworkAddressPrefixCamelCase, networkAddressPrefix_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_02(networkType_, networkAddressPrefix_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(networkType_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(networkAddressPrefix_)
            END_DYNAMIC_SIZE_ESTIMATION()

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const;

            std::wstring GetNetworkTypeString() const;

            FABRIC_NETWORK_TYPE networkType_ = FABRIC_NETWORK_TYPE_INVALID;
            std::wstring networkAddressPrefix_;
        };

        class NetworkResourceDescription
            : public ResourceDescription
        {
        public:

            NetworkResourceDescription() = default;

            NetworkResourceDescription(std::wstring const & networkName, ServiceModel::NetworkDescription const & networkDescription);

            bool operator==(NetworkResourceDescription const & other) const;

            __declspec(property(get = get_NetworkType)) FABRIC_NETWORK_TYPE const &NetworkType;

            FABRIC_NETWORK_TYPE const& get_NetworkType() const { return Properties.networkType_; }

            __declspec(property(get = get_NetworkAddressPrefix)) std::wstring const &NetworkAddressPrefix;

            std::wstring const& get_NetworkAddressPrefix() const { return Properties.networkAddressPrefix_; }

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            void WriteTo(__in Common::TextWriter &, Common::FormatOptions const &) const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(Constants::properties, Properties)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(Properties)
            END_DYNAMIC_SIZE_ESTIMATION()

            FABRIC_FIELDS_01(Properties)

        public:
            NetworkProperties Properties;
        };
    }
}
