// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            namespace UpgradeStateName
            {
                // All the states in the state machine
                // Cancelable indicates that an upgrade with a greater instance id 
                // will cause this one to stop and that to start
                enum Enum
                {
                    // This is the intiial state of the state machine
                    None,

                    // This state is reached once the entire state machine completes
                    Completed,

                    // Takes compensating actions for a previous fabric upgrade that closed the replicas
                    // It ensures that all the replicas have started reopening etc
                    FabricUpgrade_RollbackPreviousUpgrade,

                    // Cancellable state
                    // Calls BeginDownload on Hosting 
                    // Transitions to DownloadFailed on error
                    // Transitions to ReplicaCloseDecision on success
                    FabricUpgrade_Download,

                    // Timer state, Cancellable
                    // When expires transition to download
                    FabricUpgrade_DownloadFailed,

                    // non cancellable
                    // Determines whether replica close is required or not
                    // If the upgrade is happening at node up replica close is not required and transition to Upgrade
                    // Else ask hosting if replica close is required
                    //  If hosting says yes transition to SendCloseMessage
                    //  Else transition to Upgrade
                    FabricUpgrade_Validate,

                    // Timer state, Noncancellable
                    // When expires transitions to ReplicaCloseDecision
                    FabricUpgrade_ValidateFailed,

                    // NonCancellable
                    // Start closing all replicas on the node
                    FabricUpgrade_CloseReplicas,

                    // Noncancellable
                    // Update all entities that code upgrade is about to take place
                    FabricUpgrade_UpdateEntitiesOnCodeUpgrade,

                    // Noncancellable
                    // Issue BeginUpgade call to Hosting
                    // Transition to UpgradeFailed on error
                    // Transition to Sendreply on success
                    // In the case of a restart the node will be restarted before this returns
                    FabricUpgrade_Upgrade,

                    // Noncancellable
                    // When expires transition to upgrade
                    FabricUpgrade_UpgradeFailed,

                    // cancellable
                    // Enumerate FTs in the LFUM that are affected by the upgrade
                    ApplicationUpgrade_EnumerateFTs,

                    // cancellable
                    // Download updated bits for Application
                    ApplicationUpgrade_Download,

                    // Timer, Cancellable
                    // Switches back to ApplicationUpgrade_Download
                    ApplicationUpgrade_DownloadFailed,

                    // non cancellable
                    // Ask hosting to determine the runtimes affected by this upgrade
                    ApplicationUpgrade_Analyze,

                    // non cancellable
                    // Switches back to ApplicationUpgrade_Analyze
                    ApplicationUpgrade_AnalyzeFailed,

                    // NonCancellable
                    // Upgrades the application
                    ApplicationUpgrade_Upgrade,

                    // Noncancellable
                    // Switches back to ApplicationUpgrade_Upgrade
                    ApplicationUpgrade_UpgradeFailed,

                    // NonCancellable
                    // Updates the package version and closes replicas
                    ApplicationUpgrade_UpdateVersionsAndCloseIfNeeded,

                    // Asynchronous
                    // Transitions to ApplicationUpgrade_WaitForDropCompletionCheck
                    ApplicationUpgrade_WaitForCloseCompletion,

                    // Asynchronous
                    // Transitions to ApplicationUpgrade_ReplicaDownCompletionCheck
                    ApplicationUpgrade_WaitForDropCompletion,

                    // Timer
                    // Transitions to ApplicationUpgrade_ReplicaDownCompletionCheck
                    ApplicationUpgrade_WaitForReplicaDownCompletionCheck,

                    // Checks if replica downs are not pending
                    ApplicationUpgrade_ReplicaDownCompletionCheck,

                    // NonCancellable
                    ApplicationUpgrade_Finalize,

                    // Used by test code
                    Test1,
                    Test2,
                    Invalid,

                    LastValidEnum = Invalid
                };

                void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

                DECLARE_ENUM_STRUCTURED_TRACE(UpgradeStateName);
            }
        }
    }
}
