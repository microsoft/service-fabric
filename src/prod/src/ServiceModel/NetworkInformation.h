// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class NetworkInformation
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        NetworkInformation();

        NetworkInformation(NetworkInformation && other);

        NetworkInformation const & operator = (NetworkInformation && other);

        NetworkInformation(
            std::wstring const & networkName,
            std::wstring const & networkAddressPrefix,
            FABRIC_NETWORK_TYPE networkType,
            FABRIC_NETWORK_STATUS networkStatus);

        ~NetworkInformation() = default;

        Common::ErrorCode FromPublicApi(__in FABRIC_NETWORK_INFORMATION const & networkInformation);

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_NETWORK_INFORMATION & publicNetworkInformation) const;

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

        __declspec(property(get = get_NetworkType)) FABRIC_NETWORK_TYPE const &NetworkType;

        FABRIC_NETWORK_TYPE const& get_NetworkType() const { return networkType_; }

        __declspec(property(get = get_NetworkName)) std::wstring const &NetworkName;

        std::wstring const& get_NetworkName() const { return networkName_; }

        __declspec(property(get = get_NetworkAddressPrefix)) std::wstring const &NetworkAddressPrefix;

        std::wstring const& get_NetworkAddressPrefix() const { return networkAddressPrefix_; }

        __declspec(property(get = get_NetworkStatus)) FABRIC_NETWORK_STATUS const &NetworkStatus;

        FABRIC_NETWORK_STATUS const& get_NetworkStatus() const { return networkStatus_; }

        FABRIC_FIELDS_04(networkType_, networkName_, networkAddressPrefix_, networkStatus_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
        SERIALIZABLE_PROPERTY_ENUM(Constants::NetworkType, networkType_)
        SERIALIZABLE_PROPERTY(Constants::NetworkName, networkName_)
        SERIALIZABLE_PROPERTY(Constants::NetworkAddressPrefix, networkAddressPrefix_)
        SERIALIZABLE_PROPERTY_ENUM(Constants::NetworkStatus, networkStatus_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        FABRIC_NETWORK_TYPE networkType_;

        std::wstring networkName_;

        std::wstring networkAddressPrefix_;

        FABRIC_NETWORK_STATUS networkStatus_;
    };

    // Used to serialize results in REST
    QUERY_JSON_LIST(NetworkList, NetworkInformation)
}
