// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Monitoring
{
    using System.Diagnostics;

    /// <summary>
    /// Encapsulates monitoring data for each api call
    /// </summary>
    internal class FabricMonitoringData
    {
        private long replicaId;
        private long replicaInstanceId;
        private NodeId nodeId;
        private Guid partitionid;
        private Stopwatch startTime;
        private string apiName;
        private string apiProperty;
        private ServiceContext context;

        public FabricMonitoringData(long replicaId, long replicaInstanceId, NodeId nodeId, Guid partitionId, string apiName, string apiProperty)
        {
            this.replicaId = replicaId;
            this.replicaInstanceId = replicaInstanceId;
            this.nodeId = nodeId;
            this.partitionid = partitionId;
            this.apiName = apiName;
            this.apiProperty = apiProperty;
            this.startTime = Stopwatch.StartNew();
        }

        public FabricMonitoringData(ServiceContext context, string apiName, string apiProperty)
        {
            this.nodeId = context.NodeContext.NodeId;
            this.partitionid = context.PartitionId;
            this.startTime = Stopwatch.StartNew();
            this.apiName = apiName;
            this.apiProperty = apiProperty;

            if (context is StatefulServiceContext)
            {
                var statefulContext = (StatefulServiceContext)context;
                this.replicaId = statefulContext.ReplicaId;
                this.replicaInstanceId = statefulContext.ReplicaOrInstanceId;
            }
            else
            {
                this.replicaId = context.ReplicaOrInstanceId;
            }

            this.context = context;
        }

        /// <summary>
        /// Partition Id
        /// </summary>
        public Guid Partitionid
        {
            get { return this.partitionid; }
        }

        /// <summary>
        /// ApiName being monitored
        /// </summary>
        public string ApiName
        {
            get { return this.apiName; }
        }

        /// <summary>
        /// Start time, set when FabricApiCallDescription is created
        /// </summary>
        public Stopwatch StartTime
        {
            get { return this.startTime; }
            set { this.startTime = value; }
        }
        
        /// <summary>
        /// Replica instance Id for Stateless services
        /// </summary>
        public long ReplicaInstanceId
        {
            get { return this.replicaInstanceId; }
        }

        /// <summary>
        /// Replica id 
        /// </summary>
        public long ReplicaId
        {
            get { return this.replicaId; }
        }

        /// <summary>
        /// Api Property being monitored
        /// </summary>
        public string ApiProperty
        {
            get { return this.apiProperty; }
        }

        /// <summary>
        /// ServiceContext. Optionally used to generate information in FabricMonitoringData constructor. 
        /// </summary>
        public ServiceContext Context
        {
            get { return this.context; }
            set { this.context = value; }
        }
    }
}