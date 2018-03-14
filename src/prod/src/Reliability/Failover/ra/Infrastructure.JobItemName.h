// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            namespace JobItemName
            {
                // Names of different job items in the RA
                // Used for tracing 
                enum Enum
                {
                    // Handles processing of FM/RAP/RA messages
                    MessageProcessing,

                    // FT Reconfiguration message retry 
                    ReconfigurationMessageRetry,
                    ReplicaCloseMessageRetry,
                    ReplicaOpenMessageRetry,

                    // Update Service Description
                    NodeUpdateService,

                    NodeActivationDeactivation,

                    Query,

                    // Replica Up Reply handler
                    ReplicaUpReply,

                    // FT deletion
                    StateCleanup,

                    // Update SD Retry
                    UpdateServiceDescriptionMessageRetry,

                    // Hosting
                    AppHostClosed,
                    RuntimeClosed,
                    ServiceTypeRegistered,

                    FabricUpgradeRollback,
                    FabricUpgradeUpdateEntity,
                    ApplicationUpgradeEnumerateFTs,
                    ApplicationUpgradeUpdateVersionsAndCloseIfNeeded,
                    ApplicationUpgradeReplicaDownCompletionCheck,
                    ApplicationUpgradeFinalizeFT,

                    // Used to read the LFUM during Generation Update
                    GetLfum,
                    GenerationUpdate,

                    NodeUpAck,

                    RAPQuery,

                    ClientReportFault,

                    CheckReplicaCloseProgressJobItem,
                    CloseFailoverUnitJobItem,
                    UpdateStateOnLFUMLoad,

                    UploadForReplicaDiscovery,

                    UpdateAfterFMMessageSend,

                    // Empty (has no trace
                    Empty,
                    // Used in test code
                    Test,

                    LastValidEnum = Test
                };

                void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

                DECLARE_ENUM_STRUCTURED_TRACE(JobItemName);
            }
        }
    }
}

