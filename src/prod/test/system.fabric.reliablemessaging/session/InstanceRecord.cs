// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ReliableSession.Test
{
    using System.Linq;

    /// <summary>
    /// Implements a service instance
    /// </summary>
    public class InstanceRecord
    {
        /// <summary>
        /// Service instance name
        /// </summary>
        public Uri InstanceName
        {
            get; 
            private set;
        }

        /// <summary>
        /// Gets or sets the number of partitions (replicas) for this service instance 
        /// </summary>
        public int CountOfPartitions
        {
            get
            {
                return null == this.Partitions ? 0 : this.Partitions.Count(); 
                
            } 
        }

        /// <summary>
        /// Gets or sets the number of partitions for this service instance
        /// </summary>
        public PartitionRecord[] Partitions
        {
            get; 
            private set;
        }

        /// <summary>
        /// Creates an instance of the service instance
        /// </summary>
        /// <param name="name">Name of the service instance</param>
        /// <param name="countOfPartitions">Number of partitions in this instance</param>
        public InstanceRecord(Uri name, int countOfPartitions)
        {
            this.InstanceName = name;
            this.Partitions = new PartitionRecord[countOfPartitions];
        }
    }
}