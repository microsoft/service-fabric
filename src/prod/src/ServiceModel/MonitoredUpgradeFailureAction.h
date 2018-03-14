// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace MonitoredUpgradeFailureAction
    {
        enum Enum
        {
            Invalid = 0,
            Rollback = 1,
            Manual = 2,

            FirstValidEnum = Invalid,
            LastValidEnum = Manual
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val);

        MonitoredUpgradeFailureAction::Enum FromPublicApi(FABRIC_MONITORED_UPGRADE_FAILURE_ACTION const &);
        FABRIC_MONITORED_UPGRADE_FAILURE_ACTION ToPublicApi(MonitoredUpgradeFailureAction::Enum);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(Rollback)
            ADD_ENUM_VALUE(Manual)
        END_DECLARE_ENUM_SERIALIZER()

        DECLARE_ENUM_STRUCTURED_TRACE( MonitoredUpgradeFailureAction )
    };
}
