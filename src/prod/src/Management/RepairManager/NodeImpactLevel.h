// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        namespace NodeImpactLevel
        {
            enum Enum
            {
                Invalid = 0,
                None,
                Restart,
                RemoveData,
                RemoveNode,
                LAST_ENUM_PLUS_ONE
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            DECLARE_ENUM_STRUCTURED_TRACE( NodeImpactLevel )

            Common::ErrorCode FromPublicApi(FABRIC_REPAIR_NODE_IMPACT_LEVEL, Enum &);
            FABRIC_REPAIR_NODE_IMPACT_LEVEL ToPublicApi(Enum const);

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(Invalid)
                ADD_ENUM_VALUE(None)
                ADD_ENUM_VALUE(Restart)
                ADD_ENUM_VALUE(RemoveData)
                ADD_ENUM_VALUE(RemoveNode)
                ADD_ENUM_VALUE(LAST_ENUM_PLUS_ONE)
            END_DECLARE_ENUM_SERIALIZER()
        };
    }
}
