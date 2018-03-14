// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

namespace Reliability 
{ 
    namespace ReconfigurationAgentComponent 
    { 
        namespace Infrastructure
        {
            namespace JobItemName
            {
                void JobItemName::WriteToTextWriter(TextWriter & w, JobItemName::Enum const & e)
                {
                    switch (e)
                    {
                    case MessageProcessing:
                        w << "MessageProcessing";
                        break;

                    case ReconfigurationMessageRetry:
                        w << "ReconfigurationMessageRetry";
                        break;

                    case ReplicaCloseMessageRetry:
                        w << "ReplicaCloseMessageRetry";
                        break;

                    case ReplicaOpenMessageRetry:
                        w << "ReplicaOpenMessageRetry";
                        break;

                    case NodeActivationDeactivation:
                        w << "NodeActivationDeactivation";
                        break;

                    case NodeUpdateService:
                        w << "NodeUpdateService";
                        break;

                    case Query:
                        w << "Query";
                        break;

                    case ReplicaUpReply:
                        w << "ReplicaUpReply";
                        break;

                    case StateCleanup:
                        w << "StateCleanup";
                        break;

                    case UpdateServiceDescriptionMessageRetry:
                        w << "UpdateServiceDescriptionMessageRetry";
                        break;

                    case AppHostClosed:
                        w << "AppHostClosed";
                        break;

                    case RuntimeClosed:
                        w << "RuntimeClosed";
                        break;

                    case ServiceTypeRegistered:
                        w << "ServiceTypeRegistered";
                        break;

                    case FabricUpgradeRollback:
                        w << "FabricUpgradeRollback";
                        break;

                    case FabricUpgradeUpdateEntity:
                        w << "FabricUpgradeUpdateEntity";
                        break;

                    case ApplicationUpgradeEnumerateFTs:
                        w << "ApplicationUpgradeEnumerateFTs";
                        break;

                    case ApplicationUpgradeUpdateVersionsAndCloseIfNeeded:
                        w << "ApplicationUpgradeUpdateVersionsAndCloseIfNeeded";
                        break;

                    case ApplicationUpgradeFinalizeFT:
                        w << "ApplicationUpgradeFinalizeFT";
                        break;

                    case ApplicationUpgradeReplicaDownCompletionCheck:
                        w << "ApplicationUpgradeReplicaDownCompletionCheck";
                        break;

                    case GetLfum:
                        w << "GetLfum";
                        break;

                    case GenerationUpdate:
                        w << "GenerationUpdate";
                        break;

                    case NodeUpAck:
                        w << "NodeUpAck";
                        break;

                    case Empty:
                        w << " ";
                        break;

                    case Test:
                        w << "Test";
                        break;

                    case RAPQuery:
                        w << "RAPQuery";
                        break;

                    case ClientReportFault:
                        w << "ClientReportFault";
                        break;

                    case UploadForReplicaDiscovery:
                        w << "UploadForReplicaDiscovery";
                        break;

                    case CheckReplicaCloseProgressJobItem:
                        w << "CheckReplicaCloseProgressJobItem";
                        break;

                    case CloseFailoverUnitJobItem:
                        w << "CloseFailoverUnitJobItem";
                        break;

                    case UpdateStateOnLFUMLoad:
                        w << "UpdateStateOnLFUMLoad";
                        break;

                    case UpdateAfterFMMessageSend:
                        w << "UpdateAfterFMMessageSend";
                        break;

                    default:
                        Assert::CodingError("Invalid state for internal enum: {0}", static_cast<int>(e));
                    };
                }

                ENUM_STRUCTURED_TRACE(JobItemName, MessageProcessing, LastValidEnum);
            }
        }
    } 
}
