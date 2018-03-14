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

        namespace QuorumLossMode
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case QuorumReplicas:
                    w << L"QuorumReplicas";
                    return;
                case AllReplicas:
                    w << L"AllReplicas";
                    return;
                case Invalid:
                    w << L"Invalid";
                    return;
                default:
                    w << Common::wformatString("Unknown QuorumLossMode value {0}", static_cast<int>(val));
                }
            }

            ErrorCode FromPublicApi(FABRIC_QUORUM_LOSS_MODE const & publicMode, __out Enum & mode)
            {
                switch (publicMode)
                {
                case FABRIC_QUORUM_LOSS_MODE_QUORUM_REPLICAS:
                    mode = QuorumReplicas;
                    break;
                case FABRIC_QUORUM_LOSS_MODE_ALL_REPLICAS:
                    mode = AllReplicas;
                    break;
                case FABRIC_QUORUM_LOSS_MODE_INVALID:
                    return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0}", FASResource::GetResources().QuorumLossModeInvalid));
                default:
                    return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0}", FASResource::GetResources().QuorumLossModeUnknown));
                }

                return ErrorCode(ErrorCodeValue::Success);
            }

            FABRIC_QUORUM_LOSS_MODE ToPublicApi(Enum const & mode)
            {
                switch (mode)
                {
                case QuorumReplicas:
                    return FABRIC_QUORUM_LOSS_MODE_QUORUM_REPLICAS;
                case AllReplicas:
                    return FABRIC_QUORUM_LOSS_MODE_ALL_REPLICAS;
                default:
                    return FABRIC_QUORUM_LOSS_MODE_INVALID;
                }
            }
        }
    }
}
