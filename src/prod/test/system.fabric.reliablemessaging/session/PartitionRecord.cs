// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ReliableSession.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.ReliableMessaging.Session;

    /// <summary>
    /// PartitionKey types
    /// </summary>
    public enum PartitionKeyType
    {
        Singleton = 1, 
        StringKey = 2, 
        Int64Key = 3
    }
   
    /// <summary>
    /// Represents a Service Partition
    /// </summary>
    public class PartitionRecord
    {
        /// <summary>
        /// Name of the service instance
        /// </summary>
        public Uri InstanceName
        {
            get; 
            private set;
        }

        /// <summary>
        /// The unique partition key
        /// </summary>
        public string PartitionKey
        {
            get
            {
               switch (this.PartitionType)
                {
                    case PartitionKeyType.Singleton:
                        return InstanceName + ":" + "0";

                    case PartitionKeyType.StringKey:
                        return InstanceName + ":" + this.StringPartitionPartitionKey;

                    case PartitionKeyType.Int64Key:
                        return InstanceName + ":" + this.Int64PartitionPartitionKey.IntegerKeyLow + ":" + this.Int64PartitionPartitionKey.IntegerKeyHigh;

                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }
        }

        /// <summary>
        /// The type of the partition
        /// </summary>
        public PartitionKeyType PartitionType
        {
            get; 
            private set;
        }

        /// <summary>
        /// The string name of the PartitionKey if Named partition type
        /// </summary>
        public string StringPartitionPartitionKey
        {
            get; 
            private set;
        }

        /// <summary>
        /// The int64(type) partition Key if range partition type
        /// </summary>
        public IntegerPartitionKeyRange Int64PartitionPartitionKey
        {
            get; 
            private set;
        }

        /// <summary>
        /// Gets or Sets Session Manager of the partition Instance
        /// </summary>
        public IReliableSessionManager SessionManager
        {
            get; 
            set;
        }

        /// <summary>
        /// Gets or sets the end point of the partition instance
        /// </summary>
        public string Endpoint
        {
            get; 
            set; 
        }

        /// <summary>
        /// Gets or sets the inbound sessions for this partition instance
        /// Key = ServiceInstanceName + ":" + PartitionKey(0 in case of Singletone)
        /// </summary>
        public Dictionary<string, IReliableMessagingSession> InboundSessions
        {
            get; 
            private set; 
        }

        /// <summary>
        /// Creates an instance of the partition instance
        /// </summary>
        /// <param name="name">Name of the partition instance</param>
        /// <param name="type">Type of the partition</param>
        /// <param name="stringPartitionKey">Name of the partition if Named partition type</param>
        /// <param name="intPartitionKey">Range of the partition if range partition type</param>
        public PartitionRecord(Uri name, PartitionKeyType type, string stringPartitionKey, IntegerPartitionKeyRange intPartitionKey)
        {
            this.InstanceName = name;
            this.PartitionType = type;
            this.StringPartitionPartitionKey = stringPartitionKey;
            this.Int64PartitionPartitionKey = intPartitionKey;
            this.InboundSessions = new Dictionary<string, IReliableMessagingSession>();
        }
    }
}