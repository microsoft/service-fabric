// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

namespace Management
{
    namespace RepairManager
    {
        namespace NodeImpactLevel
        {
            BEGIN_ENUM_TO_TEXT( NodeImpactLevel )
                ADD_ENUM_TO_TEXT_VALUE ( Invalid )
                ADD_ENUM_TO_TEXT_VALUE ( None )
                ADD_ENUM_TO_TEXT_VALUE ( Restart )
                ADD_ENUM_TO_TEXT_VALUE ( RemoveData )
                ADD_ENUM_TO_TEXT_VALUE ( RemoveNode )
            END_ENUM_TO_TEXT( NodeImpactLevel )

            ENUM_STRUCTURED_TRACE( NodeImpactLevel, Invalid, static_cast<Enum>(LAST_ENUM_PLUS_ONE - 1) );

            ErrorCode FromPublicApi(FABRIC_REPAIR_NODE_IMPACT_LEVEL publicValue, Enum & internalValue)
            {
                switch (publicValue)
                {
                case FABRIC_REPAIR_NODE_IMPACT_LEVEL_INVALID:
                    internalValue = Invalid;
                    break;
                case FABRIC_REPAIR_NODE_IMPACT_LEVEL_NONE:
                    internalValue = None;
                    break;
                case FABRIC_REPAIR_NODE_IMPACT_LEVEL_RESTART:
                    internalValue = Restart;
                    break;
                case FABRIC_REPAIR_NODE_IMPACT_LEVEL_REMOVE_DATA:
                    internalValue = RemoveData;
                    break;
                case FABRIC_REPAIR_NODE_IMPACT_LEVEL_REMOVE_NODE:
                    internalValue = RemoveNode;
                    break;
                default:
                    return ErrorCodeValue::InvalidArgument;
                }

                return ErrorCode::Success();
            }

            FABRIC_REPAIR_NODE_IMPACT_LEVEL ToPublicApi(Enum const internalValue)
            {
                switch (internalValue)
                {
                case None:
                    return FABRIC_REPAIR_NODE_IMPACT_LEVEL_NONE;
                case Restart:
                    return FABRIC_REPAIR_NODE_IMPACT_LEVEL_RESTART;
                case RemoveData:
                    return FABRIC_REPAIR_NODE_IMPACT_LEVEL_REMOVE_DATA;
                case RemoveNode:
                    return FABRIC_REPAIR_NODE_IMPACT_LEVEL_REMOVE_NODE;
                default:
                    return FABRIC_REPAIR_NODE_IMPACT_LEVEL_INVALID;
                }
            }
        }
    }
}
