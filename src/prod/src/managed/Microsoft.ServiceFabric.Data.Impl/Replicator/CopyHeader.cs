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
    internal class CopyHeader
    {
        /// <summary>
        /// The version.
        /// </summary>
        private readonly int version;

        /// <summary>
        /// The copy stage.
        /// </summary>
        private readonly CopyStage copyStage;

        /// <summary>
        /// The primary replica id.
        /// </summary>
        private readonly long primaryReplicaId;

        /// <summary>
        /// Initializes a new instance of the <see cref="CopyHeader"/> class.
        /// </summary>
        /// <param name="version">
        /// The version.
        /// </param>
        /// <param name="copyStage">
        /// The copy stage.
        /// </param>
        /// <param name="primaryReplicaId">
        /// The primary replica id.
        /// </param>
        public CopyHeader(int version, CopyStage copyStage, long primaryReplicaId)
        {
            this.version = version;
            this.copyStage = copyStage;
            this.primaryReplicaId = primaryReplicaId;
        }

        /// <summary>
        /// Gets the version 1.
        /// </summary>
        public int Version
        {
            get
            {
                return this.version;
            }
        }

        /// <summary>
        /// Gets the stage.
        /// </summary>
        public CopyStage Stage
        {
            get
            {
                return this.copyStage;
            }
        }

        /// <summary>
        /// Gets the primary replica id.
        /// </summary>
        public long PrimaryReplicaId
        {
            get
            {
                return this.primaryReplicaId;
            }
        }

        /// <summary>
        /// The read from operation data.
        /// </summary>
        /// <param name="operationData">
        /// The operation data.
        /// </param>
        /// <returns>
        /// The <see cref="CopyHeader"/>.
        /// </returns>
        public static CopyHeader ReadFromOperationData(OperationData operationData)
        {
            var buffer = operationData[0].Array;

            using (var br = new BinaryReader(new MemoryStream(buffer, operationData[0].Offset, operationData[0].Count)))
            {
                var copyMetadataVersion = br.ReadInt32();
                var copyStage = (CopyStage)br.ReadInt32();
                var primaryReplicaId = br.ReadInt64();

                // Note that if version is 1, then the size must be exactly as expected.
                // Else the rest of the data is expected to be not required.
                Utility.Assert(
                    copyMetadataVersion != 1 || operationData[0].Array.Length == br.BaseStream.Position, 
                    "Unexpected copy state metadata size. Version {0} Length {1} Position {2}", 
                    copyMetadataVersion, operationData[0].Array.Length, br.BaseStream.Position);

                return new CopyHeader(copyMetadataVersion, copyStage, primaryReplicaId);
            }
        }
    }
}