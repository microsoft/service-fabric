// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Fabric;
    using System.IO;

    /// <summary>
    /// The copy metadata.
    /// </summary>
    internal class CopyMetadata
    {
        /// <summary>
        /// The copy state metadata version.
        /// </summary>
        private readonly int copyStateMetadataVersion;

        /// <summary>
        /// The progress vector.
        /// </summary>
        private readonly ProgressVector progressVector;

        /// <summary>
        /// The starting epoch.
        /// </summary>
        private readonly Epoch startingEpoch;

        /// <summary>
        /// The starting logical sequence number.
        /// </summary>
        private readonly LogicalSequenceNumber startingLogicalSequenceNumber;

        /// <summary>
        /// The checkpoint lsn.
        /// </summary>
        private readonly LogicalSequenceNumber checkpointLsn;

        /// <summary>
        /// The upto lsn.
        /// </summary>
        private readonly LogicalSequenceNumber uptoLsn;

        /// <summary>
        /// The highest state provider copied lsn.
        /// </summary>
        private readonly LogicalSequenceNumber highestStateProviderCopiedLsn;

        /// <summary>
        /// Gets the highest state provider copied lsn.
        /// </summary>
        internal LogicalSequenceNumber HighestStateProviderCopiedLsn
        {
            get
            {
                return this.highestStateProviderCopiedLsn;
            }
        }

        /// <summary>
        /// Gets the upto lsn.
        /// </summary>
        public LogicalSequenceNumber UptoLsn
        {
            get
            {
                return this.uptoLsn;
            }
        }

        /// <summary>
        /// Gets the checkpoint lsn.
        /// </summary>
        public LogicalSequenceNumber CheckpointLsn
        {
            get
            {
                return this.checkpointLsn;
            }
        }

        /// <summary>
        /// Gets the starting logical sequence number.
        /// </summary>
        public LogicalSequenceNumber StartingLogicalSequenceNumber
        {
            get
            {
                return this.startingLogicalSequenceNumber;
            }
        }

        /// <summary>
        /// Gets the starting epoch.
        /// </summary>
        public Epoch StartingEpoch
        {
            get
            {
                return this.startingEpoch;
            }
        }

        /// <summary>
        /// Gets the progress vector.
        /// </summary>
        public ProgressVector ProgressVector
        {
            get
            {
                return this.progressVector;
            }
        }

        /// <summary>
        /// Gets the copy state metadata version.
        /// </summary>
        public int CopyStateMetadataVersion
        {
            get
            {
                return this.copyStateMetadataVersion;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="CopyMetadata"/> class.
        /// </summary>
        /// <param name="copyStateMetadataVersion">
        /// The copy state metadata version.
        /// </param>
        /// <param name="progressVector">
        /// The progress vector.
        /// </param>
        /// <param name="startingEpoch">
        /// The starting epoch.
        /// </param>
        /// <param name="startingLogicalSequenceNumber">
        /// The starting logical sequence number.
        /// </param>
        /// <param name="checkpointLsn">
        /// The checkpoint lsn.
        /// </param>
        /// <param name="uptoLsn">
        /// The upto lsn.
        /// </param>
        /// <param name="highestStateProviderCopiedLsn">
        /// The highest state provider copied lsn.
        /// </param>
        public CopyMetadata(
            int copyStateMetadataVersion, 
            ProgressVector progressVector, 
            Epoch startingEpoch, 
            LogicalSequenceNumber startingLogicalSequenceNumber, 
            LogicalSequenceNumber checkpointLsn, 
            LogicalSequenceNumber uptoLsn, 
            LogicalSequenceNumber highestStateProviderCopiedLsn)
        {
            this.copyStateMetadataVersion = copyStateMetadataVersion;
            this.progressVector = progressVector;
            this.startingEpoch = startingEpoch;
            this.startingLogicalSequenceNumber = startingLogicalSequenceNumber;
            this.checkpointLsn = checkpointLsn;
            this.uptoLsn = uptoLsn;
            this.highestStateProviderCopiedLsn = highestStateProviderCopiedLsn;
        }

        /// <summary>
        /// The read from operation data.
        /// </summary>
        /// <param name="operationData">
        /// The operation data.
        /// </param>
        /// <returns>
        /// The <see cref="CopyMetadata"/>.
        /// </returns>
        public static CopyMetadata ReadFromOperationData(OperationData operationData)
        {
            var copiedProgressVector = new ProgressVector();

            using (var br = new BinaryReader(new MemoryStream(operationData[0].Array, operationData[0].Offset, operationData[0].Count)))
            {
                var copyStateMetadataVersion = br.ReadInt32();
                copiedProgressVector.Read(br, false);
                var startingEpoch = new Epoch(br.ReadInt64(), br.ReadInt64());
                var startingLsn = new LogicalSequenceNumber(br.ReadInt64());
                var checkpointLsn = new LogicalSequenceNumber(br.ReadInt64());
                var uptoLsn = new LogicalSequenceNumber(br.ReadInt64());
                var highestStateProviderCopiedLsn = new LogicalSequenceNumber(br.ReadInt64());

                // Note that if version is 1, then the size must be exactly as expected.
                // Else the rest of the data is expected to be not required.
                Utility.Assert(
                    copyStateMetadataVersion != 1 || operationData[0].Array.Length == br.BaseStream.Position, 
                    "Unexpected copy state metadata size. Version {0} logMetadata.Length {1} Position {2}", 
                    copyStateMetadataVersion, operationData[0].Array.Length, br.BaseStream.Position);

                return new CopyMetadata(
                    copyStateMetadataVersion, 
                    copiedProgressVector, 
                    startingEpoch, 
                    startingLsn, 
                    checkpointLsn, 
                    uptoLsn, 
                    highestStateProviderCopiedLsn);
            }
        }
    }
}