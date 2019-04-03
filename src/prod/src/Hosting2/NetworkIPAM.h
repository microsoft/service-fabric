// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class NetworkIPAM : public Common::IFabricJsonSerializable
    {
    public:
        NetworkIPAM();

        NetworkIPAM(std::vector<Hosting2::NetworkIPAMConfig> ipamConfig);

        ~NetworkIPAM() {}

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_IF(NetworkIPAM::Config, ipamConfig_, !ipamConfig_.empty())
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::vector<Hosting2::NetworkIPAMConfig> ipamConfig_;

        static WStringLiteral const Config;
    };
}
