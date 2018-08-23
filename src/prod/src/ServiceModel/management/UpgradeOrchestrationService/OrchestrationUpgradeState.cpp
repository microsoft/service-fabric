// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace UpgradeOrchestrationService
    {
        using namespace Common;

        namespace OrchestrationUpgradeState
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Enum::Invalid:
                    w << L"Invalid";
                    break;
                case Enum::RollingBackInProgress:
                    w << L"RollingBackInProgress";
                    break;
                case Enum::RollingBackCompleted:
                    w << L"RollingBackCompleted";
                    break;
                case Enum::RollingForwardPending:
                    w << L"RollingForwardPending";
                    break;
                case Enum::RollingForwardInProgress:
                    w << L"RollingForwardInProgress";
                    break;
                case Enum::RollingForwardCompleted:
                    w << L"RollingForwardCompleted";
                    break;
                case Enum::Failed:
                    w << L"Failed";
                    break;
                case Enum::RollingBackPending:
                    w << L"RollingBackPending";
                    break;
                default:
                    w << Common::wformatString("Unknown OrchestrationUpgradeState value {0}", static_cast<int>(val));
                }
            }

            ErrorCode FromPublicApi(FABRIC_UPGRADE_STATE const & state, __out Enum & progress)
            {
                switch (state)
                {
                case FABRIC_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS:
                    progress = RollingBackInProgress;
                    break;
                case FABRIC_UPGRADE_STATE_ROLLING_BACK_COMPLETED:
                    progress = RollingBackCompleted;
                    break;
                case FABRIC_UPGRADE_STATE_ROLLING_FORWARD_PENDING:
                    progress = RollingForwardPending;
                    break;
                case FABRIC_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS:
                    progress = RollingForwardInProgress;
                    break;
                case FABRIC_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED:
                    progress = RollingForwardCompleted;
                    break;
                case FABRIC_UPGRADE_STATE_FAILED:
                    progress = Failed;
                    break;
                case FABRIC_UPGRADE_STATE_ROLLING_BACK_PENDING:
                    progress = RollingBackPending;
                    break;
                default:
                    return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("Unknown OrchestrationUpgradeState"));
                }

                return ErrorCode(ErrorCodeValue::Success);
            }

            FABRIC_UPGRADE_STATE ToPublicApi(Enum const & state)
            {
                switch (state)
                {
                case RollingBackInProgress:
                    return FABRIC_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS;
                case RollingBackCompleted:
                    return FABRIC_UPGRADE_STATE_ROLLING_BACK_COMPLETED;
                case RollingForwardPending:
                    return FABRIC_UPGRADE_STATE_ROLLING_FORWARD_PENDING;
                case RollingForwardInProgress:
                    return FABRIC_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS;
                case RollingForwardCompleted:
                    return FABRIC_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED;
                case Failed:
                    return FABRIC_UPGRADE_STATE_FAILED;
                case RollingBackPending:
                    return FABRIC_UPGRADE_STATE_ROLLING_BACK_PENDING;
                default:
                    return FABRIC_UPGRADE_STATE_INVALID;
                }
            }
        }
    }
}

