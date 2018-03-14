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

        namespace DataLossMode
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Partial:
                    w << L"Partial";
                    return;
                case Full:
                    w << L"Full";
                    return;
                case Invalid:
                    w << L"Invalid";
                    return;
                default:
                    w << Common::wformatString("Unknown DataLossMode value {0}", static_cast<int>(val));
                }
            }

            ErrorCode FromPublicApi(FABRIC_DATA_LOSS_MODE const & publicMode, __out Enum & mode)
            {
                switch (publicMode)
                {
                case FABRIC_DATA_LOSS_MODE_PARTIAL:
                    mode = Partial;
                    break;
                case FABRIC_DATA_LOSS_MODE_FULL:
                    mode = Full;
                    break;
                case FABRIC_DATA_LOSS_MODE_INVALID:
                    return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0}", FASResource::GetResources().DataLossModeInvalid));
                default:
                    return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0}", FASResource::GetResources().DataLossModeUnknown));
                }

                return ErrorCode(ErrorCodeValue::Success);
            }

            FABRIC_DATA_LOSS_MODE ToPublicApi(Enum const & mode)
            {
                switch (mode)
                {
                case Partial:
                    return FABRIC_DATA_LOSS_MODE_PARTIAL;
                case Full:
                    return FABRIC_DATA_LOSS_MODE_FULL;
                default:
                    return FABRIC_DATA_LOSS_MODE_INVALID;
                }
            }
        }
    }
}
