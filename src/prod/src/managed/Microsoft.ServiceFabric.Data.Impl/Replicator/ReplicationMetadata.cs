// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Globalization;
    using System.Fabric.Common.Tracing;
    using Data;

    /// <summary>
    /// Metadata for state manager local state's replication.
    /// </summary>
    internal class ReplicationMetadata
    {
        /// <summary>
        ///  Initializes a new instance of the <see cref="ReplicationMetadata"/> class. 
        /// </summary>
        internal ReplicationMetadata(
            Uri name,
            Type type,
            byte[] initializationParameters,
            long stateProviderId,
            long parentStateProviderId,
            int version,
            StateManagerApplyType applyType)
        {
            Utility.Assert(name != null,
                "Name cannot be empty or null in replication metadata." +
                "SPID: {0}, parent SPID: {1}, version: {2}, apply type: {3}",
                stateProviderId,
                parentStateProviderId,
                version,
                applyType);

            // TODO #10279752: Separate redo out of ReplicationMetadata. Making
            // sure remove set null redo data in both native and managed.
            // In managed code, delete redo operation is not null.
            // In native code, delete redo operation is null.
            // This inconsistency would potentially result in a break in managed
            // to native migration where primary native replica replicates
            // serialization mode 0 data to secondary managed replica.
            Utility.Assert(
                type != null,
                "Inserted metadata type cannot be null" +
                "SPID: {0}, parent SPID: {1}, version: {2}, apply type: {3}",
                stateProviderId,
                parentStateProviderId,
                version,
                applyType);

            Utility.Assert(
                stateProviderId != DynamicStateManager.EmptyStateProviderId,
                "state provider id cannot be empty." +
                "SPID: {0}, parent SPID: {1}, version: {2}, apply type: {3}",
                stateProviderId,
                parentStateProviderId,
                version,
                applyType);

            // Parent Id can be empty so do not assert on it.
            this.Version = version;
            this.Name = name;
            this.Type = type;
            this.StateProviderId = stateProviderId;
            this.ParentStateProviderId = parentStateProviderId;
            this.StateManagerApplyType = applyType;
            this.InitializationContext = initializationParameters;
        }

        /// <summary>
        ///  Initializes a new instance of the <see cref="ReplicationMetadata"/> class. 
        /// </summary>
        // TODO #10279752: Separate redo out of ReplicationMetadata. Making
        // sure remove set null redo data in both native and managed.
        // In managed code, delete redo operation is not null.
        // In native code, delete redo operation is null.
        // This inconsistency would potentially result in a break in managed
        // to native migration where primary native replica replicates
        // serialization mode 0 data to secondary managed replica.
        internal ReplicationMetadata(
            Uri name,
            long stateProviderId,
            long parentStateProviderId,
            int version,
            StateManagerApplyType applyType)
        {
            Utility.Assert(name != null,
                "Name cannot be empty or null in replication metadata." +
                "SPID: {0}, parent SPID: {1}, version: {2}, apply type: {3}",
                stateProviderId,
                parentStateProviderId,
                version,
                applyType);


            Utility.Assert(
                stateProviderId != DynamicStateManager.EmptyStateProviderId,
                "state provider id cannot be empty." +
                "SPID: {0}, parent SPID: {1}, version: {2}, apply type: {3}",
                stateProviderId,
                parentStateProviderId,
                version,
                applyType);

            // Parent Id can be empty so do not assert on it.
            this.Version = version;
            this.Name = name;
            this.StateProviderId = stateProviderId;
            this.ParentStateProviderId = parentStateProviderId;
            this.StateManagerApplyType = applyType;
        }

        /// <summary>
        /// Gets or sets the initialization parameters.
        /// </summary>
        internal byte[] InitializationContext { get; private set; }

        /// <summary>
        /// Gets or sets the name.
        /// </summary>
        internal Uri Name { get; private set; }

        /// <summary>
        /// Gets or sets the guid associated with the state provider's parent.
        /// </summary>
        internal long ParentStateProviderId { get; set; }

        /// <summary>
        /// Gets or sets the state manager operation type.
        /// </summary>
        internal StateManagerApplyType StateManagerApplyType { get; set; }

        /// <summary>
        /// Gets or sets the state provider id.
        /// </summary>
        internal long StateProviderId { get; private set; }

        /// <summary>
        /// Gets or sets the type.
        /// </summary>
        internal Type Type { get; private set; }

        /// <summary>
        /// Gets or sets the version.
        /// </summary>
        internal int Version { get; private set; }

        /// <summary>
        /// Deserializes replication metadata and the redodata.
        /// </summary>
        public static ReplicationMetadata Deserialize(
            InMemoryBinaryReader metaDataReader,
            InMemoryBinaryReader redoDataReader,
            string traceType,
            int currentVersion)
        {
            try
            {
                // Read meta data first
                long stateProviderId;
                Uri name;
                StateManagerApplyType operationType;
                int version;

                DeserializeMetadata(
                    metaDataReader,
                    traceType,
                    currentVersion,
                    out stateProviderId,
                    out name,
                    out operationType,
                    out version);
                
                if (redoDataReader == null)
                {
                    return new ReplicationMetadata(
                        name,
                        stateProviderId,
                        DynamicStateManager.StateManagerId,
                        version,
                        operationType);
                }

                // Read Data Next
                string typeName;
                Type type = redoDataReader.ReadType(out typeName);
                Utility.Assert(type != null, SR.Assert_SM_TypeNotFound, traceType, typeName, name.ToString());

                byte[] initializationParameters = redoDataReader.ReadNullableBytes();
                long parentStateProviderId = redoDataReader.ReadInt64();

                // Since this is used in redo only scenarios, it is safe to assuem apply type is same as the original opeartion type.
                ReplicationMetadata replicationMetadata = new ReplicationMetadata(
                    name,
                    type,
                    initializationParameters,
                    stateProviderId,
                    parentStateProviderId,
                    version,
                    operationType);

                return replicationMetadata;
            }
            catch (Exception e)
            {
                StateManagerStructuredEvents.TraceException(
                    traceType,
                    "ReplicationMetadata.Deserialize",
                    string.Empty,
                    e);
                throw;
            }
        }

        /// <summary>
        /// Deserializes replication metadata.
        /// </summary>
        public static void DeserializeMetadataForUndoOperation(
            InMemoryBinaryReader metaDataReader,
            string traceType,
            int currentVersion,
            out long stateProviderId,
            out Uri stateProviderName,
            out StateManagerApplyType applyType)
        {
            try
            {
                // Read meta data.
                int version;
                DeserializeMetadata(
                    metaDataReader,
                    traceType,
                    currentVersion,
                    out stateProviderId,
                    out stateProviderName,
                    out applyType,
                    out version);

                Utility.Assert(applyType != StateManagerApplyType.Invalid, "Invalid apply type");
                if (applyType == StateManagerApplyType.Delete)
                {
                    applyType = StateManagerApplyType.Insert;
                }
                else
                {
                    applyType = StateManagerApplyType.Delete;
                }
            }
            catch (Exception e)
            {
                StateManagerStructuredEvents.TraceException(
                    traceType,
                    "ReplicationMetadata.Deserialize",
                    string.Empty,
                    e);
                throw;
            }
        }

        /// <summary>
        /// Serializes replication metadata.
        /// </summary>
        public static void SerializeMetaData(
            ReplicationMetadata replicationMetadata,
            string traceType,
            InMemoryBinaryWriter writer)
        {
            try
            {
                writer.Write(replicationMetadata.Version);
                writer.Write(replicationMetadata.StateProviderId);
                writer.Write(replicationMetadata.Name);
                writer.Write((byte) replicationMetadata.StateManagerApplyType);
            }
            catch (Exception e)
            {
                StateManagerStructuredEvents.TraceException(traceType, "ReplicationMetadata.Serialize", string.Empty, e);
                throw;
            }
        }

        public static void SerializeRedoData(
            ReplicationMetadata replicationMetadata,
            string traceType,
            InMemoryBinaryWriter writer)
        {
            try
            {
                writer.Write(replicationMetadata.Type);
                writer.WriteNullable(replicationMetadata.InitializationContext);
                writer.Write(replicationMetadata.ParentStateProviderId);
            }
            catch (Exception e)
            {
                StateManagerStructuredEvents.TraceException(traceType, "ReplicationMetadata.Serialize", string.Empty, e);
                throw;
            }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                @"Name: {0}, StateProviderId: {1}",
                this.Name.OriginalString,
                this.StateProviderId.ToString());
        }

        /// <summary>
        /// Deserializes replication metadata.
        /// </summary>
        private static void DeserializeMetadata(
            InMemoryBinaryReader metaDataReader,
            string traceType,
            int currentVersion,
            out long stateProviderId,
            out Uri stateProviderName,
            out StateManagerApplyType applyType,
            out int version)
        {
            try
            {
                version = metaDataReader.ReadInt32();
                if (version != currentVersion)
                {
                    var errorMessage = string.Format(
                        CultureInfo.InvariantCulture,
                        SR.Error_Unsupported_Version_Deserialization,
                        version,
                        currentVersion);
                    FabricEvents.Events.ReplicationMetadataDeserializeError(traceType, errorMessage);
                    throw new NotSupportedException(errorMessage);
                }

                stateProviderId = metaDataReader.ReadInt64();
                stateProviderName = metaDataReader.ReadUri();
                applyType = (StateManagerApplyType) metaDataReader.ReadByte();
                Utility.Assert(applyType != StateManagerApplyType.Invalid, "Invalid apply type");
            }
            catch (Exception e)
            {
                StateManagerStructuredEvents.TraceException(
                    traceType,
                    "ReplicationMetadata.Deserialize",
                    string.Empty,
                    e);
                throw;
            }
        }
    }
}