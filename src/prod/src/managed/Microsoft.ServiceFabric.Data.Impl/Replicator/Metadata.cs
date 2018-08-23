// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Globalization;
    using System.Threading;

    /// <summary>
    /// State of metadata that indicates if the state provider is active or deleted.
    /// </summary>
    internal enum MetadataMode : int
    {
        /// <summary>
        /// Metadata that corresponds to a state provider that is active.
        /// </summary>
        Active = 0,

        /// <summary>
        /// Metadata that corresponds to a state provider that has been soft-deleted.
        /// </summary>
        DelayDelete = 1,

        /// <summary>
        /// Metadata that corresponds to a state provider that needs to be cleaned up as a part of copy.
        /// </summary>
        FalseProgress = 2,
    }

    /// <summary>
    /// Metadata that represents a state provider in the state manager's local state.
    /// </summary>
    internal class Metadata
    {
        /// <summary>
        /// Flag that indicates if checkpoint should be called or not.
        /// </summary>
        private int checkpointFlag;

        /// <summary>
        /// Initializes a new instance of the <see cref="Metadata"/> class.
        /// </summary>
        internal Metadata(
            Uri name,
            Type type,
            IStateProvider2 stateProvider,
            byte[] initializationParameters,
            long stateProviderId,
            long parentStateProviderId,
            MetadataMode metadataMode,
            bool transientCreate)
        {
            Utility.Assert(name != null, "Name cannot be empty in metadata.");
            Utility.Assert(type != null, "type cannot be null.");
            Utility.Assert(
                stateProviderId != DynamicStateManager.EmptyStateProviderId,
                "state provider id cannot be empty.");

            // Parent Id can be empty so do not assert on it.
            this.Initialize(
                name,
                type,
                stateProvider,
                initializationParameters,
                stateProviderId,
                parentStateProviderId,
                -1,
                -1,
                metadataMode,
                transientCreate);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Metadata"/> class.
        /// </summary>
        internal Metadata(
            Uri name,
            Type type,
            IStateProvider2 stateProvider,
            byte[] initializationParameters,
            long stateProviderId,
            long parentId,
            long createLsn,
            long deleteLsn,
            MetadataMode metadataMode,
            bool transientCreate)
        {
            Utility.Assert(name != null, "Name cannot be empty in metadata.");
            Utility.Assert(type != null, "type cannot be null.");
            Utility.Assert(stateProvider != null, "state provider cannot be null.");
            Utility.Assert(
                stateProviderId != DynamicStateManager.EmptyStateProviderId,
                "state provider id cannot be empty.");

            // Parent Id can be empty so do not assert on it.
            this.Initialize(
                name,
                type,
                stateProvider,
                initializationParameters,
                stateProviderId,
                parentId,
                createLsn,
                deleteLsn,
                metadataMode,
                transientCreate);
        }

        /// <summary>
        /// Gets or sets the create lsn.
        /// </summary>
        internal long CreateLsn { get; set; }

        /// <summary>
        /// Gets or sets the delete lsn.
        /// </summary>
        internal long DeleteLsn { get; set; }

        /// <summary>
        /// Gets or sets initialization parameters.
        /// </summary>
        internal byte[] InitializationContext { get; private set; }

        /// <summary>
        /// Current status of state provider associated with metadata.
        /// </summary>
        internal MetadataMode MetadataMode { get; set; }

        /// <summary>
        /// Gets or sets metadata name.
        /// </summary>
        internal Uri Name { get; private set; }

        /// <summary>
        /// Gets or sets te parent id associated with the state provider.
        /// </summary>
        internal long ParentStateProviderId { get; set; }

        /// <summary>
        /// Gets or sets the state provider.
        /// </summary>
        internal IStateProvider2 StateProvider { get; set; }

        /// <summary>
        /// Gets or sets the state provider id.
        /// </summary>
        internal long StateProviderId { get; private set; }

        /// <summary>
        /// Gets transaction id associated with the metadata.
        /// </summary>
        internal long TransactionId { get; set; }

        /// <summary>
        /// Gets or sets the transient create state.
        /// </summary>
        internal bool TransientCreate { get; set; }

        /// <summary>
        /// Gets or sets the transient delete state.
        /// </summary>
        internal bool TransientDelete { get; set; }

        /// <summary>
        /// Gets or sets metadata type.
        /// </summary>
        internal Type Type { get; private set; }

        /// <summary>
        /// Sets the check point flag that indicates check point is needed for the state provider.
        /// </summary>
        public void SetCheckpointFlag()
        {
            Interlocked.CompareExchange(ref this.checkpointFlag, 1, 0);
        }

        /// <summary>
        /// Checks if check point should be called and reset the flag, if needed.
        /// </summary>
        public bool ShouldCheckpoint()
        {
            return Interlocked.CompareExchange(ref this.checkpointFlag, 0, 1) == 1;
        }

        /// <summary>
        /// Returns a string reprensentation of metadata.
        /// </summary>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                @"Name: {0}, StateProviderId: {1}, CreateLSN: {2}, DeleteLSN: {3} MetadataMode: {4} TransactionId: {5}",
                this.Name.OriginalString,
                this.StateProviderId.ToString(),
                this.CreateLsn,
                this.DeleteLsn,
                this.MetadataMode.ToString(),
                this.TransactionId);
        }

        /// <summary>
        /// Initializes metadata.
        /// </summary>
        private void Initialize(
            Uri name,
            Type type,
            IStateProvider2 stateProvider,
            byte[] initializationParameters,
            long stateProviderId,
            long parentId,
            long createLsn,
            long deleteLsn,
            MetadataMode metadataMode,
            bool transientCreate)
        {
            this.Name = name;
            this.Type = type;
            this.StateProvider = stateProvider;
            this.StateProviderId = stateProviderId;

            this.InitializationContext = initializationParameters;
            this.CreateLsn = createLsn;
            this.DeleteLsn = deleteLsn;
            this.TransientCreate = transientCreate;
            this.TransientDelete = false;
            this.checkpointFlag = 1;
            this.MetadataMode = metadataMode;
            this.TransactionId = 0;
            this.ParentStateProviderId = parentId;
        }
    }
}