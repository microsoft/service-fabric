// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.DataStructures;
using System.IO;
using System.Runtime.Serialization;

namespace System.Fabric.BackupRestore
{
    [Serializable]
    internal class BackupMetadata : ICloneable
    {
        internal int Version = 1;

        internal Guid PartitionId { get; set; }

        internal BackupPolicy Policy { get; set; }

        internal DateTime PolicyUpdateTime { get; set; }

        internal RecoveryPointInformation LastRecoveryPoint { get; set; }

        internal int NumberOfBackupsInChain { get; set; }

        [NonSerialized]
        internal int JitterInMillisUsedInLastBackup;

        /// <summary>
        /// Does a shallow copy of the BackupMetadata object
        /// </summary>
        /// <returns>Cloned object</returns>
        public object Clone()
        {
            var obj = new BackupMetadata
            {
                Version = this.Version,
                PartitionId = this.PartitionId,
                Policy = this.Policy,
                PolicyUpdateTime = this.PolicyUpdateTime,
                LastRecoveryPoint = this.LastRecoveryPoint,
                NumberOfBackupsInChain = this.NumberOfBackupsInChain,
                JitterInMillisUsedInLastBackup = this.JitterInMillisUsedInLastBackup
            };

            return obj;
        }

        /// <summary>
        /// Provides a shallow copy of object
        /// </summary>
        /// <param name="copyFromObj">Object from which to copy the settings from</param>
        internal void CopyFrom(BackupMetadata copyFromObj)
        {
            this.Version = copyFromObj.Version;
            this.PartitionId = copyFromObj.PartitionId;
            this.Policy = copyFromObj.Policy;
            this.PolicyUpdateTime = copyFromObj.PolicyUpdateTime;
            this.LastRecoveryPoint = copyFromObj.LastRecoveryPoint;
            this.NumberOfBackupsInChain = copyFromObj.NumberOfBackupsInChain;
            this.JitterInMillisUsedInLastBackup = copyFromObj.JitterInMillisUsedInLastBackup;
        }

        internal byte[] Serialize()
        {
            using (var stream = new MemoryStream())
            {
                new DataContractSerializer(typeof(BackupMetadata)).WriteObject(stream, this);
                return stream.ToArray();
            }
        }

        internal static BackupMetadata Deserialize(byte[] serializedBytes)
        {
            using (var stream = new MemoryStream(serializedBytes))
            {
                return (BackupMetadata)new DataContractSerializer(typeof(BackupMetadata)).ReadObject(stream);
            }
        }
    }
}