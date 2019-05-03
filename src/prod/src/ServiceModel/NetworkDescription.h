// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class NetworkDescription
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        NetworkDescription() = default;

        NetworkDescription(std::wstring const & networkAddressPrefix);

        DEFAULT_MOVE_ASSIGNMENT(NetworkDescription)
        DEFAULT_MOVE_CONSTRUCTOR(NetworkDescription)
        DEFAULT_COPY_ASSIGNMENT(NetworkDescription)
        DEFAULT_COPY_CONSTRUCTOR(NetworkDescription)

        Common::ErrorCode FromPublicApi(__in FABRIC_NETWORK_DESCRIPTION const & networkDescription);

        __declspec(property(get = get_NetworkType)) FABRIC_NETWORK_TYPE const &NetworkType;

        FABRIC_NETWORK_TYPE const& get_NetworkType() const { return networkType_; }

        __declspec(property(get = get_NetworkAddressPrefix)) std::wstring const &NetworkAddressPrefix;

        std::wstring const& get_NetworkAddressPrefix() const { return networkAddressPrefix_; }

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(networkType_, networkAddressPrefix_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
        SERIALIZABLE_PROPERTY_ENUM(Constants::NetworkType, networkType_)
        SERIALIZABLE_PROPERTY(Constants::NetworkAddressPrefix, networkAddressPrefix_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        FABRIC_NETWORK_TYPE networkType_;

        std::wstring networkAddressPrefix_;
    };
}
