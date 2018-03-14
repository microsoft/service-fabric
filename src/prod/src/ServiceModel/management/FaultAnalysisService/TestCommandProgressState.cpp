// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace FaultAnalysisService
    {
        namespace TestCommandProgressState
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Invalid:
                    w << L"Invalid";
                    return;
                case Running:
                    w << L"Running";
                    return;
                case RollingBack:
                    w << L"RollingBack";
                    return;                        
                case Completed:
                    w << L"Completed";
                    return;
                case Faulted:
                    w << L"Faulted";
                    return;
                case Cancelled:
                    w << L"Cancelled";
                    return;
                case ForceCancelled:
                    w << L"ForceCancelled";
                    return;
                default:
                    w << Common::wformatString("Unknown TestCommandProgressState value {0}", static_cast<int>(val));
                }
            }

            TestCommandProgressState::Enum FromPublicApi(FABRIC_TEST_COMMAND_PROGRESS_STATE const & state)
            {
                switch (state)
                {
                case FABRIC_TEST_COMMAND_PROGRESS_STATE_RUNNING:
                    return TestCommandProgressState::Enum::Running;
                case FABRIC_TEST_COMMAND_PROGRESS_STATE_ROLLING_BACK:
                    return TestCommandProgressState::Enum::RollingBack;
                case FABRIC_TEST_COMMAND_PROGRESS_STATE_COMPLETED:
                    return TestCommandProgressState::Enum::Completed;
                case FABRIC_TEST_COMMAND_PROGRESS_STATE_FAULTED:
                    return TestCommandProgressState::Enum::Faulted;
                case FABRIC_TEST_COMMAND_PROGRESS_STATE_CANCELLED:
                    return TestCommandProgressState::Enum::Cancelled;
                case FABRIC_TEST_COMMAND_PROGRESS_STATE_FORCE_CANCELLED:
                    return TestCommandProgressState::Enum::ForceCancelled;
                default:
                    Common::Assert::CodingError("Unknown TestCommandProgressState");
                }
            }

            FABRIC_TEST_COMMAND_PROGRESS_STATE ToPublicApi(Enum const & state)
            {
                switch (state)
                {
                case Running:
                    return FABRIC_TEST_COMMAND_PROGRESS_STATE_RUNNING;
                case RollingBack:
                    return FABRIC_TEST_COMMAND_PROGRESS_STATE_ROLLING_BACK;
                case Completed:
                    return FABRIC_TEST_COMMAND_PROGRESS_STATE_COMPLETED;
                case Faulted:
                    return FABRIC_TEST_COMMAND_PROGRESS_STATE_FAULTED;
                case Cancelled:
                    return FABRIC_TEST_COMMAND_PROGRESS_STATE_CANCELLED;
                case ForceCancelled:
                    return FABRIC_TEST_COMMAND_PROGRESS_STATE_FORCE_CANCELLED;
                default:
                    Common::Assert::CodingError("Unknown TestCommandProgressState");
                }
            }
        }
    }
}

