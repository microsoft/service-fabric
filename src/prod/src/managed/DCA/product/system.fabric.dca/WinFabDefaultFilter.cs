// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Collections.Generic;

    internal static class WinFabDefaultFilter
    {
        private static readonly Dictionary<string, Dictionary<string, int>> Filters;

        static WinFabDefaultFilter()
        {
            // The filter below filters Windows Fabric traces at a level primarily intended
            // for debugging by Windows Fabric team members.
            Filters = new Dictionary<string, Dictionary<string, int>>()
                        {
                            {
                                "*",
                                new Dictionary<string, int>()
                                {
                                    { "*", 3 },
                                }
                            },

                            {
                                "Config",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "HM",
                                new Dictionary<string, int>()
                                {
                                    { "ProcessReport", 4 },
                                    { "CleanupTimer", 4 },
                                    { "ReadFailed", 4 },
                                    { "ReadComplete", 4 },
                                    { "CachePersistSS", 4 },
                                    { "Upgrade", 4 },
                                    { "ChildrenPerUd", 4 }
                                }
                            },

                            {
                                "Hosting",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "FabricNode",
                                new Dictionary<string, int>()
                                {
                                    { "State", 4 },
                                    { "NodeOpening", 4 },
                                    { "NodeOpened", 4 },
                                    { "NodeClosing", 4 },
                                    { "NodeClosed", 4 },
                                    { "NodeAborting", 4 },
                                    { "NodeAborted", 4 },
                                    { "ManagementSubsystem", 4 },
                                    { "ZombieModeState", 4 },
                                }
                            },

                            {
                                "LeaseAgent",
                                new Dictionary<string, int>()
                                {
                                    // Upload all info level traces, except the following which are not very useful
                                    { "*", 4 },

                                    // Disable per message info level traces
                                    { "TTL", 3 },
                                }
                            },

                            {
                                "RoutingTable",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "Token",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "Bootstrap",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "Join",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "JoinLock",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "Gap",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "SiteNode",
                                new Dictionary<string, int>()
                                {
                                    { "State", 4 },
                                }
                            },

                            {
                                "Arbitration",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "Reliability",
                                new Dictionary<string, int>()
                                {
                                    { "LifeCycle", 4 },
                                }
                            },

                            {
                                "RA",
                                new Dictionary<string, int>()
                                {
                                    { "LifeCycle", 4 },
                                    { "FT", 4 },
                                    { "Upgrade", 4 },
                                    { "ReplicaUp", 4 },
                                    { "ReplicaDown", 4 },
                                    { "SendRAP", 4 }
                                }
                            },

                            {
                                "RAP",
                                new Dictionary<string, int>()
                                {
                                    { "FT", 4 },
                                    { "UpdateReadStatus", 4 },
                                    { "UpdateWriteStatus", 4 },
                                    { "ApiSlow", 4 },
                                    { "ApiError", 4 },
                                    { "ApiReportFault", 4 }
                                }
                            },

                            {
                                "RE",
                                new Dictionary<string, int>()
                                {
                                    // Upload all info level traces, except the following which are not very useful
                                    { "*", 4 },

                                    // Disable per message info level traces
                                    { "PrimaryReplicate", 3 },
                                    { "PrimaryReplicateDone", 3 },
                                    { "PrimaryReplicateCancel", 3 },
                                    { "PrimaryQueueFull", 3 },
                                    { "ReplicatorDropMsgInvalid", 3 },
                                    { "ReplicatorDropMsgNoFromHeader", 3 },
                                    { "ReplicatorInvalidMessage", 3 },
                                }
                            },

                            {
                                "OQ",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 }
                                }
                            },

                            {
                                "FMM",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },

                                    { "Placement", 3 },
                                    { "Movement", 3 },
                                    { "SwapPrimary", 3 },
                                    { "Drop", 3 },
                                    { "Receive", 3 },
                                    { "FMReplica", 3 },
                                    { "FMFailoverUnit", 3 },
                                    { "StateMachineAction", 3 },
                                    { "FT", 3 },
                                    { "FTLockFailure", 3 },
                                    { "BuildReplicaSuccess", 3 },
                                    { "ReplicaUpdate", 3 },
                                    { "FTAction", 3 },
                                    { "FTSend", 3 }
                                }
                            },

                            {
                                "FM",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },

                                    { "Placement", 3 },
                                    { "Movement", 3 },
                                    { "SwapPrimary", 3 },
                                    { "Drop", 3 },
                                    { "Receive", 3 },
                                    { "FMReplica", 3 },
                                    { "FMFailoverUnit", 3 },
                                    { "StateMachineAction", 3 },
                                    { "FT", 3 },
                                    { "FTLockFailure", 3 },
                                    { "BuildReplicaSuccess", 3 },
                                    { "ReplicaUpdate", 3 },
                                    { "FTAction", 3 },
                                    { "FTSend", 3 }
                                }
                            },

                            {
                                "PLB",
                                new Dictionary<string, int>()
                                {
                                    { "PLBPeriodicalTrace", 4 },
                                    { "NodeLoads", 4 },
                                    { "UpdateNode", 4 },
                                    { "UpdateService", 4 },
                                    { "UpdateServiceDeleted", 4 },
                                    { "UpdateServiceType", 4 },
                                    { "UpdateMovementEnabled", 4 },
                                    { "SearcherUnsuccessfulPlacement", 4 },
                                    { "SearcherConstraintViolationNotCorrected", 4 },
                                    { "SearcherConstraintViolationPartiallyCorrected", 4 },
                                    { "PLBConstruct", 4 },
                                    { "PLBDispose", 4 },
                                    { "PlacementConstraintParsingError", 4 },
                                    { "PlacementConstraintEvaluationError", 4 },
                                    { "IgnoreLoadInvalidMetric", 4 },
                                    { "AffinityChainDetected", 4 },
                                    { "DecisionToken", 4 },
                                }
                            },

                            {
                                "Transport",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },

                                    // Disable per-message info level tracing
                                    { "EnqueueMessage", 3 },
                                    { "DispatchingMessage", 3 }
                                }
                            },

                            {
                                "FabricDCA",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "FabricWorkerProcess",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "SG",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "SGRE",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "SGSF",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "SGSL",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "SGSFM",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "SGSLM",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "SystemFabric",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "NamingGateway",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "NamingStoreService",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "CM",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "HttpGateway",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "InfrastructureService",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "RM",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "RepairService",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "General",
                                new Dictionary<string, int>()
                                {
                                    { "RepairService", 4 },
                                }
                            },

                            {
                                "TStore",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "Wave",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "Client",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "Lease",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },

                                    // Disable per-message info level tracing
                                    { "EnqueueMessageForSending", 3 },
                                    { "TransportSending", 3 },
                                    { "TransportReceived", 3 },
                                    { "TransportSendCompleted", 3 },
                                    { "LeaseRelationSendingLease", 3 },
                                    { "LeaseRelationRcvMsg", 3 },
                                    { "FindActiveRLAInTable", 3 }
                                }
                            },

                            {
                                "FabricGateway",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                            {
                                "EntreeServiceProxy",
                                new Dictionary<string, int>()
                                {
                                    { "*", 4 },
                                }
                            },

                             {
                                "Api",
                                new Dictionary<string, int>()
                                {
                                    { "Finish", 4 },
                                }
                            },

                            {
                                "CRM",
                                new Dictionary<string, int>()
                                {
                                    { "Decision", 4 },
                                    { "Operation", 4 },
                                }
                            },

                            {
                                "MasterCRM",
                                new Dictionary<string, int>()
                                {
                                    { "Decision", 4 },
                                    { "Operation", 4 },
                                }
                            },
                        };
            StringRepresentation = WinFabFilter.CreateStringRepresentation(Filters);
        }

        internal static string StringRepresentation { get; private set; }
    }
}