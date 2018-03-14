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
        namespace RepairTaskHealthCheckState
        {            
            ENUM_STRUCTURED_TRACE( RepairTaskHealthCheckState, NotStarted, TimedOut )

            ErrorCode FromPublicApi(FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE publicValue, Enum & internalValue)
            {
                switch (publicValue)
                {
                case FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_NOT_STARTED:
                    internalValue = NotStarted;
                    break;
                case FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_IN_PROGRESS:
                    internalValue = InProgress;
                    break;
                case FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_SUCCEEDED:
                    internalValue = Succeeded;
                    break;
                case FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_SKIPPED:
                    internalValue = Skipped;
                    break;
                case FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_TIMEDOUT:
                    internalValue = TimedOut;
                    break;
                default:
                    return ErrorCodeValue::InvalidArgument;
                }

                return ErrorCode::Success();
            }

            FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE ToPublicApi(Enum const internalValue)
            {
                switch (internalValue)
                {
                case InProgress:
                    return FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_IN_PROGRESS;
                case Succeeded:
                    return FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_SUCCEEDED;
                case Skipped:
                    return FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_SKIPPED;
                case TimedOut:
                    return FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_TIMEDOUT;
                default:
                    return FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_NOT_STARTED;
                }
            }
            
            void WriteToTextWriter(__in Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                case NotStarted: w << "NotStarted"; return;
                case InProgress: w << "InProgress"; return;
                case Succeeded: w << "Succeeded"; return;
                case Skipped: w << "Skipped"; return;
                case TimedOut: w << "TimedOut"; return;

                default:
                    // we should never hit this line. Not preferring to do an Assert::CodingError
                    // since this is just an enum-to-string dumping method				
                    w << "RepairTaskHealthCheckState(" << static_cast<ULONG>(e) << ')';
                }
            }
        }
    }
}
