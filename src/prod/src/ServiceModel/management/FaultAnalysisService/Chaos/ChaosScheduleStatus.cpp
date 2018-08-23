//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace FaultAnalysisService
    {
        using namespace Common;

        namespace ChaosScheduleStatus
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Active:
                    w << L"Active";
                    return;
                case Expired:
                    w << "Expired";
                    return;
                case Pending:
                    w << "Pending";
                    return;
                case Stopped:
                    w << L"Stopped";
                    return;
                default:
                    w << wformatString("Unknown ChaosScheduleStatus value {0}", static_cast<int>(val));
                }
            }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_SCHEDULE_STATUS const & publicStatus, __out Enum & status)
            {
                switch (publicStatus)
                {
                case FABRIC_CHAOS_SCHEDULE_STATUS_ACTIVE:
                    status = Active;
                    break;
                case FABRIC_CHAOS_SCHEDULE_STATUS_EXPIRED:
                    status = Expired;
                    break;
                case FABRIC_CHAOS_SCHEDULE_STATUS_PENDING:
                    status = Pending;
                    break;
                case FABRIC_CHAOS_SCHEDULE_STATUS_STOPPED:
                    status = Stopped;
                    break;
                default:
                    return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0}", FASResource::GetResources().ChaosScheduleStatusUnknown));
                }

                return ErrorCode(ErrorCodeValue::Success);
            }

            FABRIC_CHAOS_SCHEDULE_STATUS ToPublicApi(Enum const & status)
            {
                switch (status)
                {
                case Active:
                    return FABRIC_CHAOS_SCHEDULE_STATUS_ACTIVE;
                case Expired:
                    return FABRIC_CHAOS_SCHEDULE_STATUS_EXPIRED;
                case Pending:
                    return FABRIC_CHAOS_SCHEDULE_STATUS_PENDING;
                case Stopped:
                    return FABRIC_CHAOS_SCHEDULE_STATUS_STOPPED;
                default:
                    return FABRIC_CHAOS_SCHEDULE_STATUS_INVALID;
                }
            }

            wstring ChaosScheduleStatus::ToString(ChaosScheduleStatus::Enum const & val)
            {
                switch (val)
                {
                    case(ChaosScheduleStatus::Active):
                        return L"Active";
                    case(ChaosScheduleStatus::Expired) :
                        return L"Expired";
                    case(ChaosScheduleStatus::Pending) :
                        return L"Pending";
                    case(ChaosScheduleStatus::Stopped) :
                        return L"Stopped";
                    default:
                        return L"Invalid";
                }
            }
        }
    }
}
