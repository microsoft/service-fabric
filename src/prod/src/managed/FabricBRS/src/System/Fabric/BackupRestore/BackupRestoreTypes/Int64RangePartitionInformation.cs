// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter;
    using System.Globalization;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;

    /// <summary>
    ///  Represents a Information for Partition
    /// </summary>
    [DataContract]
    public class Int64RangePartitionInformation : PartitionInformation 
    {
        /// <summary>
        /// <para>Specifies the minimum key value for this partition.</para>
        /// </summary>
        [DataMember]
        [JsonConverter(typeof(LongToStringConverter))]
        public long LowKey { get; set; }
        
        /// <summary>
        /// <para>Specifies the maximum key value for this partition.</para>
        /// </summary>
        [DataMember]
        [JsonConverter(typeof(LongToStringConverter))]
        public long HighKey { get; internal set; }

        /// <summary>
        /// Constructor
        /// </summary>
        public Int64RangePartitionInformation()
            : base(ServicePartitionKind.Int64Range)
        {
        }

        public override string ToString()
        {
            return string.Format( CultureInfo.InvariantCulture, "Kind: Int64RangePartition Highkey:{0} Lowkey:{1}", 
                this.HighKey,
                this.LowKey);
        }
    }
}