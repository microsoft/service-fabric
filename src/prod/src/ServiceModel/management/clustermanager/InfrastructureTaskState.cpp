// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace ClusterManager
    {
        namespace InfrastructureTaskState
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                    case Invalid: w << "Invalid"; return;

                    case PreProcessing: w << "PreProcessing"; return;
                    case PreAckPending: w << "PreAckPending"; return;
                    case PreAcked: w << "PreAcked"; return;

                    case PostProcessing: w << "PostProcessing"; return;
                    case PostAckPending: w << "PostAckPending"; return;
                    case PostAcked: w << "PostAcked"; return;
                };

                w << "InfrastructureTaskState(" << static_cast<int>(e) << ')';
            }

            ENUM_STRUCTURED_TRACE( InfrastructureTaskState, Invalid, static_cast<Enum>(LAST_ENUM_PLUS_ONE - 1) );

            Enum FromPublicApi(FABRIC_INFRASTRUCTURE_TASK_STATE publicType)
            {
                switch (publicType)
                {
                case FABRIC_INFRASTRUCTURE_TASK_STATE_PRE_PROCESSING:
                    return PreProcessing;
                case FABRIC_INFRASTRUCTURE_TASK_STATE_PRE_ACK_PENDING:
                    return PreAckPending;
                case FABRIC_INFRASTRUCTURE_TASK_STATE_PRE_ACKED:
                    return PreAcked;
                case FABRIC_INFRASTRUCTURE_TASK_STATE_POST_PROCESSING:
                    return PostProcessing;
                case FABRIC_INFRASTRUCTURE_TASK_STATE_POST_ACK_PENDING:
                    return PostAckPending;
                case FABRIC_INFRASTRUCTURE_TASK_STATE_POST_ACKED:
                    return PostAcked;
                default:
                    return Invalid;
                }
            }

            FABRIC_INFRASTRUCTURE_TASK_STATE ToPublicApi(Enum const nativeType)
            {
                switch (nativeType)
                {
                case PreProcessing:
                    return FABRIC_INFRASTRUCTURE_TASK_STATE_PRE_PROCESSING;
                case PreAckPending:
                    return FABRIC_INFRASTRUCTURE_TASK_STATE_PRE_ACK_PENDING;
                case PreAcked:
                    return FABRIC_INFRASTRUCTURE_TASK_STATE_PRE_ACKED;
                case PostProcessing:
                    return FABRIC_INFRASTRUCTURE_TASK_STATE_POST_PROCESSING;
                case PostAckPending:
                    return FABRIC_INFRASTRUCTURE_TASK_STATE_POST_ACK_PENDING;
                case PostAcked:
                    return FABRIC_INFRASTRUCTURE_TASK_STATE_POST_ACKED;
                default:
                    return FABRIC_INFRASTRUCTURE_TASK_STATE_INVALID;
                }
            }
        }
    }
}


