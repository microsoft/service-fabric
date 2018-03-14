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

        namespace RestartPartitionMode
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case AllReplicasOrInstances:
                    w << L"AllReplicasOrInstances";
                    return;
                case OnlyActiveReplicas:
                    w << L"OnlyActiveReplicas";
                    return;
                case Invalid:
                    w << L"Invalid";
                    return;
                default:
                    w << Common::wformatString("Unknown RestartPartitionMode value {0}", static_cast<int>(val));
                }
            }

            ErrorCode FromPublicApi(FABRIC_RESTART_PARTITION_MODE const & publicMode, __out Enum & mode)
            {
                switch (publicMode)
                {
                case FABRIC_RESTART_PARTITION_MODE_ALL_REPLICAS_OR_INSTANCES:
                    mode = AllReplicasOrInstances;
                    break;
                case FABRIC_RESTART_PARTITION_MODE_ONLY_ACTIVE_SECONDARIES:
                    mode = OnlyActiveReplicas;
                    break;
                case FABRIC_RESTART_PARTITION_MODE_INVALID:
                    return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0}", FASResource::GetResources().RestartPartitionModeInvalid));
                default:
                    return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0}", FASResource::GetResources().RestartPartitionModeUnknown));
                }

                return ErrorCode(ErrorCodeValue::Success);
            }

            FABRIC_RESTART_PARTITION_MODE ToPublicApi(Enum const & mode)
            {
                switch (mode)
                {
                case OnlyActiveReplicas:
                    return FABRIC_RESTART_PARTITION_MODE_ONLY_ACTIVE_SECONDARIES;
                case AllReplicasOrInstances:
                    return FABRIC_RESTART_PARTITION_MODE_ALL_REPLICAS_OR_INSTANCES;
                default:
                    return FABRIC_RESTART_PARTITION_MODE_INVALID;
                }
            }
        }
    }
}
