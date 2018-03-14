// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace NodeUpgradePhase
    {
        enum Enum
        {
            Invalid = 0,
            PreUpgradeSafetyCheck = 1,
            Upgrading = 2,
            PostUpgradeSafetyCheck = 3,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const& value);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(PreUpgradeSafetyCheck)
            ADD_ENUM_VALUE(Upgrading)
            ADD_ENUM_VALUE(PostUpgradeSafetyCheck)
        END_DECLARE_ENUM_SERIALIZER()
    }
}
