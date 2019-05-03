// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Internal.EventSource
{
    using System.Diagnostics.CodeAnalysis;
    using System.Diagnostics.Tracing;
    using System.Fabric.Health;
    using System.Threading.Tasks;
    using FabricMonSvc;

    [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1306:FieldNamesMustBeginWithLowerCaseLetter", Justification = "Method parameters define the names of column names in MDS table.")]
    [EventSource(Name = "Microsoft-ServiceFabric-Monitoring-Health")]
    internal sealed class MonitoringEventSource : EventSource
    {
        public static readonly MonitoringEventSource Current = new MonitoringEventSource();
        
        // The current schemaVerison is 1. Incrementing the schemaVersion to 2 
        // to indicate transition from MdsTableWriter to EventSource based implementation.
        private const int SchemaVersion = 2;

        // These event ids must be unique and 0 < eventId < 2^16
        private const int ClusterHealthStateEventId = 1;
        private const int ApplicationHealthStateEventId = 2;
        private const int NodeHealthStateEventId = 3;
        private const int ServiceHealthStateEventId = 4;
        private const int PartitionHealthStateEventId = 5;
        private const int ReplicaHealthStateEventId = 6;
        private const int DeployedApplicationHealthStateEventId = 7;
        private const int DeployedServicePackageHealthStateEventId = 8;

        private const int ClusterHealthEventEventId = 9;
        private const int ApplicationHealthEventEventId = 10;
        private const int NodeHealthEventEventId = 11;
        private const int ServiceHealthEventEventId = 12;
        private const int PartitionHealthEventEventId = 13;
        private const int ReplicaHealthEventEventId = 14;
        private const int DeployedApplicationHealthEventEventId = 15;
        private const int DeployedServicePackageHealthEventEventId = 16;

        static MonitoringEventSource()
        {
            // A workaround for the problem where ETW activities do not get tracked until Tasks infrastructure is initialized.
            // This problem will be fixed in .NET Framework 4.6.2.
            Task.Run(() => { });
        }

        // Instance constructor is private to enforce singleton semantics
        private MonitoringEventSource() : base()
        {
        }

        #region Methods to log HealthState
        [Event(ClusterHealthStateEventId, Level = EventLevel.Informational, 
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogClusterHealthState(string ClusterName, string HealthState, string ReasonUnhealthy)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(ClusterHealthStateEventId, ClusterName, HealthState, ReasonUnhealthy);
            }
        }

        [Event(ApplicationHealthStateEventId, Level = EventLevel.Informational, 
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogApplicationHealthState(string ClusterName, string ApplicationName, string HealthState, string ReasonUnhealthy)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(ApplicationHealthStateEventId, ClusterName, ApplicationName, HealthState, ReasonUnhealthy);
            }
        }

        [Event(NodeHealthStateEventId, Level = EventLevel.Informational,
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogNodeHealthState(string ClusterName, string NodeName, string HealthState, string ReasonUnhealthy)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(NodeHealthStateEventId, ClusterName, NodeName, HealthState, ReasonUnhealthy);
            }
        }

        [Event(ServiceHealthStateEventId, Level = EventLevel.Informational,
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogServiceHealthState(string ClusterName, string ApplicationName, string ServiceName, string HealthState, string ReasonUnhealthy)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(ServiceHealthStateEventId, ClusterName, ApplicationName, ServiceName, HealthState, ReasonUnhealthy);
            }
        }

        [Event(PartitionHealthStateEventId, Level = EventLevel.Informational,
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogPartitionHealthState(string ClusterName, string ApplicationName, string ServiceName, string PartitionId, string HealthState, string ReasonUnhealthy)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(PartitionHealthStateEventId, ClusterName, ApplicationName, ServiceName, PartitionId, HealthState, ReasonUnhealthy);
            }
        }

        [Event(ReplicaHealthStateEventId, Level = EventLevel.Informational,
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogReplicaHealthState(string ClusterName, string ApplicationName, string ServiceName, string PartitionId, string ReplicaId, string HealthState, string ReasonUnhealthy)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(ReplicaHealthStateEventId, ClusterName, ApplicationName, ServiceName, PartitionId, ReplicaId, HealthState, ReasonUnhealthy);
            }
        }

        [Event(DeployedApplicationHealthStateEventId, Level = EventLevel.Informational,
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogDeployedApplicationHealthState(string ClusterName, string ApplicationName, string NodeName, string HealthState, string ReasonUnhealthy)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(DeployedApplicationHealthStateEventId, ClusterName, ApplicationName, NodeName, HealthState, ReasonUnhealthy);
            }
        }

        [Event(DeployedServicePackageHealthStateEventId, Level = EventLevel.Informational,
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogDeployedServicePackageHealthState(string ClusterName, string ApplicationName, string ServiceName, string NodeName, string HealthState, string ReasonUnhealthy)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(DeployedServicePackageHealthStateEventId, ClusterName, ApplicationName, ServiceName, NodeName, HealthState, ReasonUnhealthy);
            }
        }
        #endregion

        #region Methods to log HealthEvents
        [Event(ClusterHealthEventEventId, Level = EventLevel.Informational,
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogClusterHealthEvent(string ClusterName, string HealthState, string Description, string Property, string SequenceNumber, string SourceId, string IsExpired)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(ClusterHealthEventEventId, ClusterName, HealthState, Description, Property, SequenceNumber, SourceId, IsExpired);
            }
        }

        [Event(ApplicationHealthEventEventId, Level = EventLevel.Informational,
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogApplicationHealthEvent(string ClusterName, string ApplicationName, string HealthState, string Description, string Property, string SequenceNumber, string SourceId, string IsExpired)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(ApplicationHealthEventEventId, ClusterName, ApplicationName, HealthState, Description, Property, SequenceNumber, SourceId, IsExpired);
            }
        }

        [Event(NodeHealthEventEventId, Level = EventLevel.Informational,
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogNodeHealthEvent(string ClusterName, string NodeName, string HealthState, string Description, string Property, string SequenceNumber, string SourceId, string IsExpired)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(NodeHealthEventEventId, ClusterName, NodeName, HealthState, Description, Property, SequenceNumber, SourceId, IsExpired);
            }
        }

        [Event(ServiceHealthEventEventId, Level = EventLevel.Informational,
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogServiceHealthEvent(string ClusterName, string ApplicationName, string ServiceName, string HealthState, string Description, string Property, string SequenceNumber, string SourceId, string IsExpired)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(ServiceHealthEventEventId, ClusterName, ApplicationName, ServiceName, HealthState, Description, Property, SequenceNumber, SourceId, IsExpired);
            }
        }

        [Event(PartitionHealthEventEventId, Level = EventLevel.Informational,
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogPartitionHealthEvent(string ClusterName, string ApplicationName, string ServiceName, string PartitionId, string HealthState, string Description, string Property, string SequenceNumber, string SourceId, string IsExpired)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(PartitionHealthEventEventId, ClusterName, ApplicationName, ServiceName, PartitionId, HealthState, Description, Property, SequenceNumber, SourceId, IsExpired);
            }
        }

        [Event(ReplicaHealthEventEventId, Level = EventLevel.Informational,
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogReplicaHealthEvent(string ClusterName, string ApplicationName, string ServiceName, string PartitionId, string ReplicaId, string HealthState, string Description, string Property, string SequenceNumber, string SourceId, string IsExpired)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(ReplicaHealthEventEventId, ClusterName, ApplicationName, ServiceName, PartitionId, ReplicaId, HealthState, Description, Property, SequenceNumber, SourceId, IsExpired);
            }
        }

        [Event(DeployedApplicationHealthEventEventId, Level = EventLevel.Informational,
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogDeployedApplicationHealthEvent(string ClusterName, string ApplicationName, string NodeName, string HealthState, string Description, string Property, string SequenceNumber, string SourceId, string IsExpired)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(DeployedApplicationHealthEventEventId, ClusterName, ApplicationName, NodeName, HealthState, Description, Property, SequenceNumber, SourceId, IsExpired);
            }
        }

        [Event(DeployedServicePackageHealthEventEventId, Level = EventLevel.Informational,
            Keywords = Keywords.ServiceFabric | Keywords.Health | Keywords.Monitoring, Version = SchemaVersion)]
        public void LogDeployedServicePackageHealthEvent(string ClusterName, string ApplicationName, string ServiceName, string NodeName, string HealthState, string Description, string Property, string SequenceNumber, string SourceId, string IsExpired)
        {
            if (this.IsEnabled())
            {
                this.WriteEvent(DeployedServicePackageHealthEventEventId, ClusterName, ApplicationName, ServiceName, NodeName, HealthState, Description, Property, SequenceNumber, SourceId, IsExpired);
            }
        }
        #endregion

        public static class Keywords
        {
            public const EventKeywords ServiceFabric = (EventKeywords)2;
            public const EventKeywords Health = (EventKeywords)4;
            public const EventKeywords Monitoring = (EventKeywords)8;
        }
    }
}