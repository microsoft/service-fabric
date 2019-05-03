// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class NetworkConfig : public Common::IFabricJsonSerializable
    {
    public:
        NetworkConfig();

        NetworkConfig(
            std::wstring name,
            std::wstring driver,
            bool checkDuplicate,
            Hosting2::NetworkIPAM networkIPAM);

        ~NetworkConfig() {}

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_IF(NetworkConfig::NetworkName, name_, !name_.empty())
            SERIALIZABLE_PROPERTY_IF(NetworkConfig::Driver, driver_, !driver_.empty())
            SERIALIZABLE_PROPERTY(NetworkConfig::CheckDuplicate, checkDuplicate_)
            SERIALIZABLE_PROPERTY(NetworkConfig::NetworkIPAM, networkIPAM_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring name_;
        std::wstring driver_;
        bool checkDuplicate_;
        NetworkIPAM networkIPAM_;

        static WStringLiteral const NetworkName;
        static WStringLiteral const Driver;
        static WStringLiteral const CheckDuplicate;
        static WStringLiteral const NetworkIPAM;
    };
}
