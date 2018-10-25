// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    /// <summary>
    /// Contains information necessary for recovery
    /// </summary>
    internal class RecoveryInformation
    {
        /// <summary>
        ///  Initializes a new instance of the <see cref="RecoveryInformation"/> class.
        /// </summary>
        internal RecoveryInformation(
            bool logCompleteCheckpointAfterRecovery,
            bool shouldRemoveLocalStateDueToIncompleteRestore)
        {
            this.LogCompleteCheckpointAfterRecovery = logCompleteCheckpointAfterRecovery;
            this.ShouldRemoveLocalStateDueToIncompleteRestore = shouldRemoveLocalStateDueToIncompleteRestore;
        }

        internal RecoveryInformation(bool shouldRemoveLocalStateDueToIncompleteRestore)
        {
            this.LogCompleteCheckpointAfterRecovery = false;
            this.ShouldSkipRecoveryDueToIncompleteChangeRoleNone = true;
            this.ShouldRemoveLocalStateDueToIncompleteRestore = shouldRemoveLocalStateDueToIncompleteRestore;
        }

        internal bool LogCompleteCheckpointAfterRecovery { get; private set; }

        /// <summary>
        /// Gets a value that indicates whether local state should be removed.
        /// If previous instance failed during restore, logging replicator might need the state manager
        /// to clean its state and start empty.
        /// </summary>
        internal bool ShouldRemoveLocalStateDueToIncompleteRestore { get; private set; }

        /// <summary>
        /// Gets a value that indicates whether perform recovery should be skipped.
        /// If the previous instance failed during change role to none, this will be true.
        /// It indicates that recovery should be skipped because Change Role to None will be retried by the Reliability subsystem.
        /// </summary>
        internal bool ShouldSkipRecoveryDueToIncompleteChangeRoleNone { get; private set; }
    }
}