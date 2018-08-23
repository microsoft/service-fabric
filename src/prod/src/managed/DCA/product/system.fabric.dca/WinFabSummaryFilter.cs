// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Collections.Generic;

    internal static class WinFabSummaryFilter
    {
        private static readonly Dictionary<string, Dictionary<string, int>> Filters;

        static WinFabSummaryFilter()
        {
            // The filter below filters Windows Fabric traces at a level primarily intended
            // for debugging by customers who develop their services on Windows Fabric.
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
                                "FabricNode",
                                new Dictionary<string, int>()
                                {
                                    { "NodeOpening", 4 },
                                    { "NodeOpened", 4 },
                                    { "NodeClosing", 4 },
                                    { "NodeClosed", 4 },
                                    { "NodeAborting", 4 },
                                    { "NodeAborted", 4 },
                                }
                            },

                            {
                                "RAP",
                                new Dictionary<string, int>()
                                {
                                    { "ApiSlow", 4 },
                                    { "ApiError", 4 },
                                    { "ApiReportFault", 4 }
                                }
                            },

                            {
                                "FMM",
                                new Dictionary<string, int>()
                                {
                                    { "NodeAddedToGFUM", 4 },
                                    { "NodeRemovedFromGFUM", 4 },
                                    { "ServiceCreated", 4 },
                                    { "ServiceDeleted", 4 },
                                    { "ReconfigurationStarted", 4 },
                                    { "ReconfigurationCompleted", 4 },
                                    { "PartitionDeleted", 4 },
                                }
                            },

                            {
                                "FM",
                                new Dictionary<string, int>()
                                {
                                    { "Counters", 4 },
                                    { "NodeAddedToGFUM", 4 },
                                    { "NodeRemovedFromGFUM", 4 },
                                    { "ServiceCreated", 4 },
                                    { "ServiceDeleted", 4 },
                                    { "ServiceLocationUpdated", 4 },
                                    { "ReconfigurationStarted", 4 },
                                    { "ReconfigurationCompleted", 4 },
                                    { "PartitionDeleted", 4 },
                                }
                            },

                            {
                                "CM",
                                new Dictionary<string, int>()
                                {
                                    { "ApplicationCreated", 4 },
                                    { "ApplicationDeleted", 4 },
                                    { "ApplicationUpgradeStart", 4 },
                                    { "ApplicationUpgradeComplete", 4 },
                                    { "ApplicationUpgradeRollback", 4 },
                                    { "ApplicationUpgradeRollbackComplete", 4 },
                                    { "ApplicationUpgradeDomainComplete", 4 },
                                }
                            },

                            {
                                "RA",
                                new Dictionary<string, int>()
                                {
                                    { "ReconfigurationCompleted", 4 },
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
                                "PLB",
                                new Dictionary<string, int>()
                                {
                                    { "DecisionToken", 4 },
                                }
                            },

                            {
                                "PLBM",
                                new Dictionary<string, int>()
                                {
                                    { "DecisionToken", 4 },
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