// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class NetworkIPAMConfig : public Common::IFabricJsonSerializable
    {
    public:
        NetworkIPAMConfig();

        NetworkIPAMConfig(
            std::wstring subnet,
            std::wstring gateway);

        ~NetworkIPAMConfig() {}

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_IF(NetworkIPAMConfig::Subnet, subnet_, !subnet_.empty())
            SERIALIZABLE_PROPERTY_IF(NetworkIPAMConfig::Gateway, gateway_, !gateway_.empty())
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring subnet_;
        std::wstring gateway_;

        static WStringLiteral const Subnet;
        static WStringLiteral const Gateway;
    };
}
