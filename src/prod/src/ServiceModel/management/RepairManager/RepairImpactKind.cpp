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
        namespace RepairImpactKind
        {
            BEGIN_ENUM_TO_TEXT( RepairImpactKind )
                ADD_ENUM_TO_TEXT_VALUE ( Invalid )
                ADD_ENUM_TO_TEXT_VALUE ( Node )
            END_ENUM_TO_TEXT( RepairImpactKind )

            ENUM_STRUCTURED_TRACE( RepairImpactKind, Invalid, static_cast<Enum>(LAST_ENUM_PLUS_ONE - 1) );

            ErrorCode FromPublicApi(FABRIC_REPAIR_IMPACT_KIND publicValue, Enum & internalValue)
            {
                switch (publicValue)
                {
                case FABRIC_REPAIR_IMPACT_KIND_INVALID:
                    internalValue = Invalid;
                    break;
                case FABRIC_REPAIR_IMPACT_KIND_NODE:
                    internalValue = Node;
                    break;
                default:
                    return ErrorCodeValue::InvalidArgument;
                }

                return ErrorCode::Success();
            }

            FABRIC_REPAIR_IMPACT_KIND ToPublicApi(Enum const internalValue)
            {
                switch (internalValue)
                {
                case Node:
                    return FABRIC_REPAIR_IMPACT_KIND_NODE;
                default:
                    return FABRIC_REPAIR_IMPACT_KIND_INVALID;
                }
            }
        }
    }
}
