// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;

    /// <summary>
    ///  Represents a Information for Partition
    /// </summary>
    [DataContract]
    [KnownType(typeof(Int64RangePartitionInformation))]
    [KnownType(typeof(NamedPartitionInformation))]
    [KnownType(typeof(SingletonPartitionInformation))]

    public abstract class PartitionInformation
    {
        /// <summary>
        /// Describes the type of partition.
        /// </summary>
        [JsonProperty(Order=-2)]
        [DataMember]
        [JsonConverter(typeof(StringEnumConverter))]
        public ServicePartitionKind ServicePartitionKind { get; set; }

        /// <summary>
        /// Unique Partition ID 
        /// </summary>
        [DataMember]
        public Guid Id { get; set; }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="servicePartitionKind">Service Partition Kind</param>
        protected PartitionInformation(ServicePartitionKind servicePartitionKind)
        {
            this.ServicePartitionKind = servicePartitionKind;
        }
    }
}