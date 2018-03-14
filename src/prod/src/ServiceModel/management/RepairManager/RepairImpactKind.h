// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        namespace RepairImpactKind
        {
            enum Enum
            {
                Invalid = 0,
                Node = 1,
                LAST_ENUM_PLUS_ONE = 2,
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            DECLARE_ENUM_STRUCTURED_TRACE( RepairImpactKind )

            Common::ErrorCode FromPublicApi(FABRIC_REPAIR_IMPACT_KIND, Enum &);
            FABRIC_REPAIR_IMPACT_KIND ToPublicApi(Enum const);

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(Invalid)
                ADD_ENUM_VALUE(Node)
                ADD_ENUM_VALUE(LAST_ENUM_PLUS_ONE)
            END_DECLARE_ENUM_SERIALIZER()
        };
    }
}
