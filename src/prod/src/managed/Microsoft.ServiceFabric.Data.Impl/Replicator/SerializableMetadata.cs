// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;

    /// <summary>
    /// Metadata for state manager local state
    /// </summary>
    internal class SerializableMetadata
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SerializableMetadata"/> class. 
        /// </summary>
        public SerializableMetadata(
            Uri name,
            Type type,
            byte[] initializationParameters,
            long stateProviderId,
            long parentStateProviderId,
            MetadataMode metadataMode,
            long createLsn,
            long deleteLsn) : this(name, initializationParameters, stateProviderId, parentStateProviderId, metadataMode, createLsn, deleteLsn)
        {
            Utility.Assert(type != null, "type cannot be null.");
            this.Type = type;
            this.TypeString = null;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SerializableMetadata"/> class. 
        /// </summary>
        public SerializableMetadata(
            Uri name,
            string type,
            byte[] initializationParameters,
            long stateProviderId,
            long parentStateProviderId,
            MetadataMode metadataMode,
            long createLsn,
            long deleteLsn) : this(name, initializationParameters, stateProviderId, parentStateProviderId, metadataMode, createLsn, deleteLsn)
        {
            Utility.Assert(type != null, "type cannot be null.");
            this.TypeString = type;
            this.Type = null;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SerializableMetadata"/> class. 
        /// </summary>
        internal SerializableMetadata(
            Uri name,
            byte[] initializationParameters,
            long stateProviderId,
            long parentStateProviderId,
            MetadataMode metadataMode,
            long createLsn,
            long deleteLsn)
        {
            Utility.Assert(name != null, "Name cannot be empty in replication metadata.");

            Utility.Assert(stateProviderId != DynamicStateManager.EmptyStateProviderId, "state provider id cannot be empty.");

            this.Name = name;

            this.CreateLsn = createLsn;
            this.DeleteLsn = deleteLsn;

            this.InitializationContext = initializationParameters;
            this.StateProviderId = stateProviderId;
            this.ParentStateProviderId = parentStateProviderId;
            this.MetadataMode = metadataMode;
        }

        /// <summary>
        /// Gets or sets the create sequence number.
        /// </summary>
        internal long CreateLsn { get; private set; }

        /// <summary>
        /// Gets or sets the delete sequence number.
        /// </summary>
        internal long DeleteLsn { get; set; }

        /// <summary>
        /// Gets or sets the initialization parameters.
        /// </summary>
        internal byte[] InitializationContext { get; private set; }

        /// <summary>
        /// Gets or sets the metadata mode.
        /// </summary>
        internal MetadataMode MetadataMode { get; set; }

        /// <summary>
        /// Gets or sets the name.
        /// </summary>
        internal Uri Name { get; private set; }

        /// <summary>
        /// Gets or sets the id for this state provider's parent (or Guid.Empty if it has no parent).
        /// </summary>
        internal long ParentStateProviderId { get; private set; }

        /// <summary>
        /// Gets or sets the state provider id.
        /// </summary>
        internal long StateProviderId { get; private set; }

        /// <summary>
        /// Gets or sets the type.
        /// </summary>
        internal Type Type { get; private set; }

        /// <summary>
        /// Gets or sets the type string.
        /// </summary>
        internal string TypeString { get; private set; }
    }
}