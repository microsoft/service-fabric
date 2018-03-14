// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ProxyActions
        {
            void WriteToTextWriter(TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                    case OpenInstance:
                         w << L"OpenInstance"; return;
                    case CloseInstance:
                        w << L"CloseInstance"; return;
                    case OpenReplica:
                        w << L"OpenReplica"; return;
                    case ChangeReplicaRole:
                        w << L"ChangeReplicaRole"; return;
                    case CloseReplica:
                        w << L"CloseReplica"; return;
                    case OpenReplicator:
                        w << L"OpenReplicator"; return;
                    case ChangeReplicatorRole:
                        w << L"ChangeReplicatorRole"; return;
                    case CloseReplicator:
                        w << L"CloseReplicator"; return;
                    case AbortReplicator:
                        w << L"AbortReplicator"; return;
                    case AbortReplica:
                        w << L"AbortReplica"; return;
                    case AbortInstance:
                        w << L"AbortInstance"; return;
                    case GetReplicatorStatus:
                        w << L"GetReplicatorStatus"; return;
                    case ReplicatorPreWriteStatusRevokeUpdateConfiguration:
                        w << L"ReplicatorPreWriteStatusRevokeUpdateConfiguration"; return;
                    case ReplicatorUpdateCurrentConfiguration:
                        w << L"ReplicatorUpdateCurrentConfiguration"; return;
                    case ReplicatorUpdateCatchUpConfiguration:
                        w << L"ReplicatorUpdateCatchUpConfiguration"; return;
                    case ReplicatorUpdateConfiguration:
                        w << L"ReplicatorUpdateConfiguration"; return;
                    case CatchupReplicaSetQuorum:
                        w << L"CatchupReplicaSetQuorum"; return;
                    case PreWriteStatusRevokeCatchup:
                        w << L"PreWriteStatusRevokeCatchup"; return;
                    case CatchupReplicaSetAll:
                        w << L"CatchupReplicaSetAll"; return;
                    case CancelCatchupReplicaSet:
                        w << L"CancelCatchupReplicaSet"; return;
                    case BuildIdleReplica:
                        w << L"BuildIdleReplica"; return;
                    case RemoveIdleReplica:
                        w << L"RemoveIdleReplica"; return;
                    case ReportAnyDataLoss:
                        w << L"ReportAnyDataLoss"; return;
                    case UpdateEpoch:
                        w << L"UpdateEpoch"; return;
                    case ReconfigurationStarting:
                        w << L"ReconfigurationStarting"; return;
                    case ReconfigurationEnding:
                        w << L"ReconfigurationEnding"; return;
                    case GetReplicatorQuery:
                        w << L"GetReplicatorQuery"; return;
                    case UpdateReadWriteStatus:
                        w << L"UpdateReadWriteStatus"; return;
                    case UpdateServiceDescription:
                        w << L"UpdateServiceDescription"; return;
                    case OpenPartition:
                        w << L"OpenPartition"; return;
                    case ClosePartition:
                        w << L"ClosePartition"; return;
                    case FinalizeDemoteToSecondary:
                        w << L"FinalizeDemoteToSecondary"; return;
                    default: 
                        Common::Assert::CodingError("Unknown ProxyActions value");
                }
            }

            ENUM_STRUCTURED_TRACE(ProxyActions, OpenInstance, LastValidEnum);
        }
    }
}
