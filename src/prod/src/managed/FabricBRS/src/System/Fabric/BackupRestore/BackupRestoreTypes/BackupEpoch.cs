// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    
    /// <summary>
    /// Represents the Backup version of the partition in Service Fabric.
    /// </summary>
    [DataContract]
    public class BackupEpoch
    {
        /// <summary>
        /// Representing an increasing value which is updated whenever data loss is suspected
        /// </summary>
        [JsonProperty(PropertyName="DataLossVersion")]
        [DataMember]
        [JsonConverter(typeof(LongToStringConverter))]
        public long DataLossNumber { get; set; }

        /// <summary>
        /// Representing an increasing value that is updated whenever the configuration of this replica set changes
        /// </summary>
        [JsonProperty(PropertyName="ConfigurationVersion")]
        [DataMember]
        [JsonConverter(typeof(LongToStringConverter))]
        public long ConfigurationNumber { get; set; }
    }
}