// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace RollingUpgradeMode
    {
        enum Enum
        {
            Invalid = 0,
            UnmonitoredAuto = 1,
            UnmonitoredManual = 2,
            Monitored = 3,

            FirstValidEnum = Invalid,
            LastValidEnum = Monitored
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val);

        DECLARE_ENUM_STRUCTURED_TRACE( RollingUpgradeMode )

        Enum FromPublicApi(FABRIC_ROLLING_UPGRADE_MODE const &);
        FABRIC_ROLLING_UPGRADE_MODE ToPublicApi(Enum const &);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(UnmonitoredAuto)
            ADD_ENUM_VALUE(UnmonitoredManual)
            ADD_ENUM_VALUE(Monitored)
        END_DECLARE_ENUM_SERIALIZER()
    };
}
