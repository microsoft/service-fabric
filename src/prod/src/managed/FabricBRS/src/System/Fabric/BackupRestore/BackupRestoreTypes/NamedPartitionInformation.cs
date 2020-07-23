// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Globalization;
    using System.Runtime.Serialization;

    /// <summary>
    ///Describes the partition information for the name as a string that is based on partition schemes.
    /// </summary>
    [DataContract]
    public class NamedPartitionInformation : PartitionInformation 
    {
        /// <summary>
        /// <para>Specifies the minimum key value for this partition.</para>
        /// </summary>
        [DataMember]
        public string Name { get; set; }

        /// <summary>
        /// Constructor
        /// </summary>
        public NamedPartitionInformation()
            : base(ServicePartitionKind.Named)
        {
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Kind: NamedPartition Name:{0}",  this.Name);
        }
    }
}