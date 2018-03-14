// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ProxyActionsListTypes
        {
            void WriteToTextWriter(TextWriter & w, Enum const & val)
            {
                switch(val)
                {
                    case Empty:
                        w << L"Empty"; return;
                    case StatelessServiceOpen:
                        w << L"SOpen"; return;
                    case StatelessServiceClose:
                        w << L"SClose"; return;
                    case StatelessServiceAbort:
                        w << L"SAbort"; return;
                    case StatefulServiceOpenIdle:
                        w << L"SFOpenIdle"; return;
                    case StatefulServiceOpenPrimary:
                        w << L"SFOpenPrimary"; return;
                    case StatefulServiceClose:
                        w << L"SFClose"; return;
                    case StatefulServiceDrop:
                        w << L"SFDrop"; return;
                    case StatefulServiceAbort:
                        w << L"SFAbort"; return;
                    case StatefulServiceChangeRole:
                        w << L"SFChangeRole"; return;
                    case StatefulServicePromoteToPrimary:
                        w << L"SFPromoteToPrimary"; return;
                    case StatefulServiceDemoteToSecondary:
                        w << L"SFDemoteToSecondary"; return;
                    case StatefulServiceFinishDemoteToSecondary:
                        w << L"SFinishDemoteToSecondary"; return;
                    case StatefulServiceEndReconfiguration:
                        w << L"SFEndReconfiguration"; return;
                    case StatefulServiceReopen:
                        w << L"SFReopen"; return;
                    case ReplicatorBuildIdleReplica:
                        w << L"REBuildIdleReplica"; return;
                    case ReplicatorRemoveIdleReplica:
                        w << L"RERemoveIdleReplica"; return;
                    case ReplicatorGetStatus:
                        w << L"REGetStatus"; return;
                    case ReplicatorUpdateEpochAndGetStatus:
                        w << L"REUpdateEpochAndGetStatus"; return;
                    case ReplicatorUpdateReplicas:
                        w << L"REUpdateReplicas"; return;
                    case ReplicatorUpdateAndCatchupQuorum:
                        w << L"REUpdateAndCatchupQuorum"; return;
                    case CancelCatchupReplicaSet:
                        w << L"CancelCatchupReplicaSet"; return;
                    case ReplicatorGetQuery:
                        w << L"REGetQuery"; return;
                    case UpdateServiceDescription:
                        w << L"UpdateServiceDescription"; return;
                    case StatefulServiceFinalizeDemoteToSecondary:
                        w << "StatefulServiceFinalizeDemoteToSecondary"; return;
                    case PROXY_ACTION_LIST_TYPES_COUNT:
                        w << L"PROXY_ACTION_LIST_TYPES_COUNT"; return;
                    default: 
                        Common::Assert::CodingError("Unknown ProxyActionsListTypes value");
                }
            }

            ENUM_STRUCTURED_TRACE(ProxyActionsListTypes, Empty, PROXY_ACTION_LIST_TYPES_COUNT);
        }
    }
}
