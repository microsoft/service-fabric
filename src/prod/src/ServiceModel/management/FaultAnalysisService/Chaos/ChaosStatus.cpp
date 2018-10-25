// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace FaultAnalysisService
    {
        using namespace Common;

        namespace ChaosStatus
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Running:
                    w << L"Running";
                    return;
                case Stopped:
                    w << L"Stopped";
                    return;
                default:
                    w << wformatString("Unknown ChaosStatus value {0}", static_cast<int>(val));
                }
            }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_STATUS const & publicStatus, __out Enum & status)
            {
                switch (publicStatus)
                {
                case FABRIC_CHAOS_STATUS_RUNNING:
                    status = Running;
                    break;
                case FABRIC_CHAOS_STATUS_STOPPED:
                    status = Stopped;
                    break;
                default:
                    return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0}", FASResource::GetResources().ChaosStatusUnknown));
                }

                return ErrorCode(ErrorCodeValue::Success);
            }

            FABRIC_CHAOS_STATUS ToPublicApi(Enum const & status)
            {
                switch (status)
                {
                case Running:
                    return FABRIC_CHAOS_STATUS_RUNNING;
                case Stopped:
                    return FABRIC_CHAOS_STATUS_STOPPED;
                default:
                    return FABRIC_CHAOS_STATUS_INVALID;
                }
            }

            wstring ChaosStatus::ToString(ChaosStatus::Enum const & val)
            {
                switch (val)
                {
                    case(ChaosStatus::Running):
                        return L"Running";
                    case(ChaosStatus::Stopped) :
                        return L"Stopped";
                    default:
                        return L"Invalid";
                }
            }
        }
    }
}
