// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Indicates that the service is Singleton-partitioned, effectively non-partitioned or with one partition only, and uses the partition 
    /// scheme of a Singleton service.</para>
    /// </summary>
    [DataContract]
    public class SingletonPartitionInformation : PartitionInformation 
    {
        public override string ToString()
        {
            return "Kind: SingletonPartition ";
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public SingletonPartitionInformation() : base(ServicePartitionKind.Singleton)
        {
        }
    }
}