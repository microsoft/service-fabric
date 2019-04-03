// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace ModelV2
    {
        class NetworkResourceDescriptionQueryResult
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
            , public Common::ISizeEstimator
        {
        public:
            NetworkResourceDescriptionQueryResult() = default;

            NetworkResourceDescriptionQueryResult(
                std::wstring const & networkName,
                FABRIC_NETWORK_TYPE networkType,
                std::wstring const & networkAddressPrefix,
                FABRIC_NETWORK_STATUS networkStatus);

            DEFAULT_MOVE_CONSTRUCTOR(NetworkResourceDescriptionQueryResult);
            DEFAULT_MOVE_ASSIGNMENT(NetworkResourceDescriptionQueryResult);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"name", networkName_)
                SERIALIZABLE_PROPERTY(L"properties", properties_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(networkName_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(properties_)
            END_DYNAMIC_SIZE_ESTIMATION()

            __declspec(property(get = get_NetworkType)) FABRIC_NETWORK_TYPE const &NetworkType;

            FABRIC_NETWORK_TYPE const& get_NetworkType() const { return properties_.NetworkType; }

            __declspec(property(get=get_NetworkName)) std::wstring const &NetworkName;
            std::wstring const& get_NetworkName() const { return networkName_; }

            __declspec(property(get = get_NetworkAddressPrefix)) std::wstring const &NetworkAddressPrefix;

            std::wstring const& get_NetworkAddressPrefix() const { return properties_.NetworkAddressPrefix; }

            __declspec(property(get = get_NetworkStatus)) FABRIC_NETWORK_STATUS const &NetworkStatus;

            FABRIC_NETWORK_STATUS const& get_NetworkStatus() const { return properties_.NetworkStatus; }

            FABRIC_FIELDS_02(networkName_, properties_);

            private:

            class NetworkProperties
                : public Serialization::FabricSerializable
                , public Common::IFabricJsonSerializable
                , public Common::ISizeEstimator
            {
                public:
                    NetworkProperties() = default;

                    NetworkProperties(
                        FABRIC_NETWORK_TYPE networkType,
                        std::wstring const & networkAddressPrefix,
                        FABRIC_NETWORK_STATUS networkStatus);

                    DEFAULT_COPY_CONSTRUCTOR(NetworkProperties);
                    DEFAULT_COPY_ASSIGNMENT(NetworkProperties);
                    DEFAULT_MOVE_CONSTRUCTOR(NetworkProperties);
                    DEFAULT_MOVE_ASSIGNMENT(NetworkProperties);

                    BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                        SERIALIZABLE_PROPERTY_ENUM(Constants::KindLowerCase, networkType_)
                        SERIALIZABLE_PROPERTY(Constants::NetworkAddressPrefixCamelCase, networkAddressPrefix_)
                        SERIALIZABLE_PROPERTY_ENUM(Constants::status, networkStatus_)
                        SERIALIZABLE_PROPERTY(Constants::statusDetails, networkStatusDetails_)
                    END_JSON_SERIALIZABLE_PROPERTIES()
    
                    BEGIN_DYNAMIC_SIZE_ESTIMATION()
                        DYNAMIC_SIZE_ESTIMATION_MEMBER(networkType_)
                        DYNAMIC_SIZE_ESTIMATION_MEMBER(networkAddressPrefix_)
                        DYNAMIC_SIZE_ESTIMATION_MEMBER(networkStatus_)
                        DYNAMIC_SIZE_ESTIMATION_MEMBER(networkStatusDetails_)
                    END_DYNAMIC_SIZE_ESTIMATION()

                    __declspec(property(get = get_NetworkType)) FABRIC_NETWORK_TYPE const &NetworkType;

                    FABRIC_NETWORK_TYPE const& get_NetworkType() const { return networkType_; }

                    __declspec(property(get = get_NetworkAddressPrefix)) std::wstring const &NetworkAddressPrefix;

                    std::wstring const& get_NetworkAddressPrefix() const { return networkAddressPrefix_; }

                    __declspec(property(get = get_NetworkStatus)) FABRIC_NETWORK_STATUS const &NetworkStatus;

                    FABRIC_NETWORK_STATUS const& get_NetworkStatus() const { return networkStatus_; }

                    __declspec(property(get = get_NetworkStatusDetails)) std::wstring const &NetworkStatusDetails;

                    std::wstring const& get_NetworkStatusDetails() const { return networkStatusDetails_; }

                    FABRIC_FIELDS_04(
                        networkType_,
                        networkAddressPrefix_,
                        networkStatus_,
                        networkStatusDetails_);

                private:
                    FABRIC_NETWORK_TYPE networkType_ = FABRIC_NETWORK_TYPE_INVALID;
                    std::wstring networkAddressPrefix_;
                    FABRIC_NETWORK_STATUS networkStatus_ = FABRIC_NETWORK_STATUS_INVALID;
                    std::wstring networkStatusDetails_;
            };
            
            std::wstring networkName_;
            NetworkProperties properties_;
        };

        QUERY_JSON_LIST(NetworkResourceDescriptionQueryResultList, NetworkResourceDescriptionQueryResult)
    }
}
