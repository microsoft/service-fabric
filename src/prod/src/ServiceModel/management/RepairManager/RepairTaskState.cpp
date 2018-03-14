// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Management
{
    namespace RepairManager
    {
        namespace RepairTaskState
        {
            BEGIN_FLAGS_TO_TEXT( RepairTaskState, Invalid )
                ADD_FLAGS_TO_TEXT_VALUE( Created )
                ADD_FLAGS_TO_TEXT_VALUE( Claimed )
                ADD_FLAGS_TO_TEXT_VALUE( Preparing )
                ADD_FLAGS_TO_TEXT_VALUE( Approved )
                ADD_FLAGS_TO_TEXT_VALUE( Executing )
                ADD_FLAGS_TO_TEXT_VALUE( Restoring )
                ADD_FLAGS_TO_TEXT_VALUE( Completed )
            END_FLAGS_TO_TEXT( RepairTaskFlags )

            FLAGS_STRUCTURED_TRACE( RepairTaskState, Invalid, Created, Completed );

            ErrorCode FromPublicApi(FABRIC_REPAIR_TASK_STATE publicValue, Enum & internalValue)
            {
                switch (publicValue)
                {
                case FABRIC_REPAIR_TASK_STATE_INVALID:
                    internalValue = Invalid;
                    break;
                case FABRIC_REPAIR_TASK_STATE_CREATED:
                    internalValue = Created;
                    break;
                case FABRIC_REPAIR_TASK_STATE_CLAIMED:
                    internalValue = Claimed;
                    break;
                case FABRIC_REPAIR_TASK_STATE_PREPARING:
                    internalValue = Preparing;
                    break;
                case FABRIC_REPAIR_TASK_STATE_APPROVED:
                    internalValue = Approved;
                    break;
                case FABRIC_REPAIR_TASK_STATE_EXECUTING:
                    internalValue = Executing;
                    break;
                case FABRIC_REPAIR_TASK_STATE_RESTORING:
                    internalValue = Restoring;
                    break;
                case FABRIC_REPAIR_TASK_STATE_COMPLETED:
                    internalValue = Completed;
                    break;
                default:
                    return ErrorCodeValue::InvalidArgument;
                }

                return ErrorCode::Success();
            }

            FABRIC_REPAIR_TASK_STATE ToPublicApi(Enum const internalValue)
            {
                switch (internalValue)
                {
                case Created:
                    return FABRIC_REPAIR_TASK_STATE_CREATED;
                case Claimed:
                    return FABRIC_REPAIR_TASK_STATE_CLAIMED;
                case Preparing:
                    return FABRIC_REPAIR_TASK_STATE_PREPARING;
                case Approved:
                    return FABRIC_REPAIR_TASK_STATE_APPROVED;
                case Executing:
                    return FABRIC_REPAIR_TASK_STATE_EXECUTING;
                case Restoring:
                    return FABRIC_REPAIR_TASK_STATE_RESTORING;
                case Completed:
                    return FABRIC_REPAIR_TASK_STATE_COMPLETED;
                default:
                    return FABRIC_REPAIR_TASK_STATE_INVALID;
                }
            }

            // TODO Replace with a macro, or generate a map
            bool TryParseValue(wstring const & input, Enum & value)
            {
                if (input == L"Invalid")
                {
                    value = Invalid;
                }
                else if (input == L"Created")
                {
                    value = Created;
                }
                else if (input == L"Claimed")
                {
                    value = Claimed;
                }
                else if (input == L"Preparing")
                {
                    value = Preparing;
                }
                else if (input == L"Approved")
                {
                    value = Approved;
                }
                else if (input == L"Executing")
                {
                    value = Executing;
                }
                else if (input == L"Restoring")
                {
                    value = Restoring;
                }
                else if (input == L"Completed")
                {
                    value = Completed;
                }
                else
                {
                    return false;
                }

                return true;
            }

            ErrorCode ParseStateMask(wstring const & input, Enum & value)
            {
                Enum result = Invalid;

                vector<wstring> tokens;
                StringUtility::Split(input, tokens, wstring(L"|"));

                for (auto it = tokens.begin(); it != tokens.end(); ++it)
                {
                    StringUtility::TrimWhitespaces(*it);

                    Enum tokenValue;
                    if (!TryParseValue(*it, tokenValue))
                    {
                        return ErrorCodeValue::InvalidArgument;
                    }

                    result |= tokenValue;
                }

                value = result;
                return ErrorCode::Success();
            }
        }
    }
}
