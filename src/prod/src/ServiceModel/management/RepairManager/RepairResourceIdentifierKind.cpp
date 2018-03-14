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
        namespace RepairScopeIdentifierKind
        {
            BEGIN_ENUM_TO_TEXT( RepairScopeIdentifierKind )
                ADD_ENUM_TO_TEXT_VALUE ( Invalid )
                ADD_ENUM_TO_TEXT_VALUE ( Cluster )
            END_ENUM_TO_TEXT( RepairScopeIdentifierKind )

            ENUM_STRUCTURED_TRACE( RepairScopeIdentifierKind, Invalid, static_cast<Enum>(LAST_ENUM_PLUS_ONE - 1) );

            ErrorCode FromPublicApi(FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND publicValue, Enum & internalValue)
            {
                switch (publicValue)
                {
                case FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_INVALID:
                    internalValue = Invalid;
                    break;
                case FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER:
                    internalValue = Cluster;
                    break;
                default:
                    return ErrorCodeValue::InvalidArgument;
                }

                return ErrorCode::Success();
            }

            FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND ToPublicApi(Enum const internalValue)
            {
                switch (internalValue)
                {
                case Cluster:
                    return FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER;
                default:
                    return FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_INVALID;
                }
            }
        }
    }
}
