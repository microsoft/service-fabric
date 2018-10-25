// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.IO;

    /// <summary>
    /// Properties are key-value pairs.
    /// </summary>
    internal sealed class StateManagerFileProperties : FileProperties
    {
        private const string BlocksHandlePropertyName = "blocks";

        private const string MetadataHandlePropertyName = "metadata";

        private const string PartitionIdPropertyName = "partitionid";

        private const string ReplicaIdPropertyName = "replicaid";

        private const string RootStateProviderCountPropertyName = "roots";

        private const string StateProviderCountPropertyName = "count";

        private const string PrepareCheckpointLSNPropertyName = "checkpointlsn";

        /// <summary>
        /// The handle to the block sizes block.
        /// </summary>
        private BlockHandle blocksHandle;

        /// <summary>
        /// The handle to the state provider metadata block.
        /// </summary>
        private BlockHandle metadataHandle;

        /// <summary>
        /// The partition id for the state manager.
        /// </summary>
        private Guid partitionId;

        /// <summary>
        /// The replica id for the state manager.
        /// </summary>
        private long replicaId;

        /// <summary>
        /// The number of root state providers (state providers with no parents).
        /// </summary>
        private uint rootStateProviderCount = 0;

        /// <summary>
        /// The number of state providers.
        /// </summary>
        private uint stateProviderCount = 0;

        /// <summary>
        /// The PrepareCheckpointLSN value.
        /// If the file does not have PrepareCheckpointLSN property, PrepareCheckpointLSN will be InvalidLSN.
        /// </summary>
        private long prepareCheckpointLSN = StateManagerConstants.InvalidLSN;

        /// Testing purpose only.
        /// <summary>
        /// In the change of adding the prepareCheckpointLSN property, we need to verify the file with prepareCheckpointLSN
        /// property can be read from the code version does not have the property, and the file without prepareCheckpointLSN
        /// property can be read from the code version which include the property
        /// </summary>
        private bool test_Ignore = false;

        private readonly bool doNotWritePrepareCheckpointLSN = false;

        /// <summary>
        /// Used for read path only
        /// </summary>
        public StateManagerFileProperties() : this(false)
        {
        }

        public StateManagerFileProperties(bool doNotWritePrepareCheckpointLSN)
        {
            this.doNotWritePrepareCheckpointLSN = doNotWritePrepareCheckpointLSN;
        }

        public BlockHandle BlocksHandle
        {
            get { return this.blocksHandle; }

            set { this.blocksHandle = value; }
        }

        public BlockHandle MetadataHandle
        {
            get { return this.metadataHandle; }

            set { this.metadataHandle = value; }
        }

        public Guid PartitionId
        {
            get { return this.partitionId; }

            set { this.partitionId = value; }
        }

        public long ReplicaId
        {
            get { return this.replicaId; }

            set { this.replicaId = value; }
        }

        public uint RootStateProviderCount
        {
            get { return this.rootStateProviderCount; }

            set { this.rootStateProviderCount = value; }
        }

        public uint StateProviderCount
        {
            get { return this.stateProviderCount; }

            set { this.stateProviderCount = value; }
        }

        public long PrepareCheckpointLSN
        {
            get { return this.prepareCheckpointLSN; }

            set { this.prepareCheckpointLSN = value; }
        }

        public bool Test_Ignore
        {
            set { this.test_Ignore = value; }
        }

        /// <summary>
        /// Serialize the <see cref="StateManagerFileProperties"/> into the given stream.
        /// </summary>
        /// <param name="writer">Stream to write to.</param>
        public override void Write(BinaryWriter writer)
        {
            // Allow the base class to write first.
            base.Write(writer);

            // 'metadata' - BlockHandle
            writer.Write(MetadataHandlePropertyName);
            VarInt.Write(writer, (int) BlockHandle.SerializedSize);
            this.MetadataHandle.Write(writer);

            // 'blocks' - BlockHandle
            writer.Write(BlocksHandlePropertyName);
            VarInt.Write(writer, (int) BlockHandle.SerializedSize);
            this.BlocksHandle.Write(writer);

            // 'count' - uint
            writer.Write(StateProviderCountPropertyName);
            VarInt.Write(writer, (int) sizeof(uint));
            writer.Write(this.StateProviderCount);

            // 'roots' - uint
            writer.Write(RootStateProviderCountPropertyName);
            VarInt.Write(writer, (int) sizeof(uint));
            writer.Write(this.RootStateProviderCount);

            // 'partitionid' - Guid
            writer.Write(PartitionIdPropertyName);
            var partitionIdBytes = this.PartitionId.ToByteArray();
            VarInt.Write(writer, (int) partitionIdBytes.Length);
            writer.Write(partitionIdBytes);

            // 'replicaid' - long
            writer.Write(ReplicaIdPropertyName);
            VarInt.Write(writer, (int) sizeof(long));
            writer.Write(this.ReplicaId);

            // #12249219: Without the "allowPrepareCheckpointLSNToBeInvalid" being set to true in the backup code path, 
            // It is possible for all backups before the first checkpoint after the upgrade to fail.
            if (this.doNotWritePrepareCheckpointLSN == false && this.test_Ignore == false)
            {
                // 'checkpointlsn' - long
                writer.Write(PrepareCheckpointLSNPropertyName);
                VarInt.Write(writer, (int)sizeof(long));
                writer.Write(this.PrepareCheckpointLSN);
            }
        }

        /// <summary>
        /// Read the given property value.
        /// </summary>
        /// <param name="reader">Stream to read from.</param>
        /// <param name="property">Property to read.</param>
        /// <param name="valueSize">The size in bytes of the value.</param>
        protected override void ReadProperty(BinaryReader reader, string property, int valueSize)
        {
            switch (property)
            {
                case MetadataHandlePropertyName:
                    this.MetadataHandle = BlockHandle.Read(reader);
                    break;

                case BlocksHandlePropertyName:
                    this.BlocksHandle = BlockHandle.Read(reader);
                    break;

                case StateProviderCountPropertyName:
                    this.StateProviderCount = reader.ReadUInt32();
                    break;

                case RootStateProviderCountPropertyName:
                    this.RootStateProviderCount = reader.ReadUInt32();
                    break;

                case PartitionIdPropertyName:
                    this.PartitionId = new Guid(reader.ReadBytes(valueSize));
                    break;

                case ReplicaIdPropertyName:
                    this.ReplicaId = reader.ReadInt64();
                    break;

                case PrepareCheckpointLSNPropertyName:
                    if (this.test_Ignore)
                    {
                        // If test ignore set to true, we need to jump over the value block.
                        base.ReadProperty(reader, property, valueSize);
                        break;
                    }

                    this.PrepareCheckpointLSN = reader.ReadInt64();
                    Utility.Assert(
                        PrepareCheckpointLSN >= StateManagerConstants.ZeroLSN,
                        "StateManagerFileProperties::ReadProperty If the file has PrepareCheckpointLSN property, it must larger than or equal to 0, PrepareCheckpointLSN: {0}.",
                        prepareCheckpointLSN);
                    break;

                default:
                    base.ReadProperty(reader, property, valueSize);
                    break;
            }
        }
    }
}