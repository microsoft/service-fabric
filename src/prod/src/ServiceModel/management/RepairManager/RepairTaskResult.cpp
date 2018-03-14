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
        namespace RepairTaskResult
        {
            BEGIN_FLAGS_TO_TEXT( RepairTaskResult, Invalid )
                ADD_FLAGS_TO_TEXT_VALUE ( Succeeded )
                ADD_FLAGS_TO_TEXT_VALUE ( Cancelled )
                ADD_FLAGS_TO_TEXT_VALUE ( Interrupted )
                ADD_FLAGS_TO_TEXT_VALUE ( Failed )
                ADD_FLAGS_TO_TEXT_VALUE ( Pending )
            END_FLAGS_TO_TEXT( RepairTaskResult )

            FLAGS_STRUCTURED_TRACE( RepairTaskResult, Invalid, Succeeded, Pending );

            ErrorCode FromPublicApi(FABRIC_REPAIR_TASK_RESULT publicValue, Enum & internalValue)
            {
                switch (publicValue)
                {
                case FABRIC_REPAIR_TASK_RESULT_INVALID:
                    internalValue = Invalid;
                    break;
                case FABRIC_REPAIR_TASK_RESULT_SUCCEEDED:
                    internalValue = Succeeded;
                    break;
                case FABRIC_REPAIR_TASK_RESULT_CANCELLED:
                    internalValue = Cancelled;
                    break;
                case FABRIC_REPAIR_TASK_RESULT_INTERRUPTED:
                    internalValue = Interrupted;
                    break;
                case FABRIC_REPAIR_TASK_RESULT_FAILED:
                    internalValue = Failed;
                    break;
                case FABRIC_REPAIR_TASK_RESULT_PENDING:
                    internalValue = Pending;
                    break;
                default:
                    return ErrorCodeValue::InvalidArgument;
                }

                return ErrorCode::Success();
            }

            FABRIC_REPAIR_TASK_RESULT ToPublicApi(Enum const internalValue)
            {
                switch (internalValue)
                {
                case Succeeded:
                    return FABRIC_REPAIR_TASK_RESULT_SUCCEEDED;
                case Cancelled:
                    return FABRIC_REPAIR_TASK_RESULT_CANCELLED;
                case Interrupted:
                    return FABRIC_REPAIR_TASK_RESULT_INTERRUPTED;
                case Failed:
                    return FABRIC_REPAIR_TASK_RESULT_FAILED;
                case Pending:
                    return FABRIC_REPAIR_TASK_RESULT_PENDING;
                default:
                    return FABRIC_REPAIR_TASK_RESULT_INVALID;
                }
            }
        }
    }
}
