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
        namespace RepairTaskFlags
        {
            BEGIN_FLAGS_TO_TEXT( RepairTaskFlags, None )
                ADD_FLAGS_TO_TEXT_VALUE( CancelRequested )
                ADD_FLAGS_TO_TEXT_VALUE( AbortRequested )
                ADD_FLAGS_TO_TEXT_VALUE( ForcedApproval )
            END_FLAGS_TO_TEXT( RepairTaskFlags )

            FLAGS_STRUCTURED_TRACE( RepairTaskFlags, None, CancelRequested, ForcedApproval );

            ErrorCode FromPublicApi(FABRIC_REPAIR_TASK_FLAGS publicValue, Enum & internalValue)
            {
                if ((publicValue & FABRIC_REPAIR_TASK_FLAGS_VALID_MASK) != publicValue)
                {
                    return ErrorCodeValue::InvalidArgument;
                }

                Enum result = None;

                if (publicValue & FABRIC_REPAIR_TASK_FLAGS_CANCEL_REQUESTED)
                {
                    result |= CancelRequested;
                }

                if (publicValue & FABRIC_REPAIR_TASK_FLAGS_ABORT_REQUESTED)
                {
                    result |= AbortRequested;
                }

                if (publicValue & FABRIC_REPAIR_TASK_FLAGS_FORCED_APPROVAL)
                {
                    result |= ForcedApproval;
                }

                internalValue = result;
                return ErrorCode::Success();
            }

            FABRIC_REPAIR_TASK_FLAGS ToPublicApi(Enum const internalValue)
            {
                int result = FABRIC_REPAIR_TASK_FLAGS_NONE;

                if (internalValue & CancelRequested)
                {
                    result |= FABRIC_REPAIR_TASK_FLAGS_CANCEL_REQUESTED;
                }

                if (internalValue & AbortRequested)
                {
                    result |= FABRIC_REPAIR_TASK_FLAGS_ABORT_REQUESTED;
                }

                if (internalValue & ForcedApproval)
                {
                    result |= FABRIC_REPAIR_TASK_FLAGS_FORCED_APPROVAL;
                }

                return static_cast<FABRIC_REPAIR_TASK_FLAGS>(result);
            }
        }
    }
}
