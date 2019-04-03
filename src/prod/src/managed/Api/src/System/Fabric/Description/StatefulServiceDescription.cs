// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Represents the extend <see cref="System.Fabric.Description.ServiceDescription" /> to provide additional information necessary to create stateful services.</para>
    /// </summary>
    /// <remarks>
    ///   <para />
    /// </remarks>
    public sealed class StatefulServiceDescription : ServiceDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.StatefulServiceDescription" /> class.</para>
        /// </summary>
        public StatefulServiceDescription()
            : base(ServiceDescriptionKind.Stateful)
        {
            this.TargetReplicaSetSize = 1;
            this.MinReplicaSetSize = 1;
        }

        internal StatefulServiceDescription(StatefulServiceDescription other)
            : base(other)
        {
            this.HasPersistedState = other.HasPersistedState;
            this.TargetReplicaSetSize = other.TargetReplicaSetSize;
            this.MinReplicaSetSize = other.MinReplicaSetSize;
            this.ReplicaRestartWaitDuration = other.ReplicaRestartWaitDuration;
            this.StandByReplicaKeepDuration = other.StandByReplicaKeepDuration;
            this.QuorumLossWaitDuration = other.QuorumLossWaitDuration;
        }

        /// <summary>
        /// <para>Gets or sets a value indicating whether this instance has persisted state.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the instance has persisted state; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        /// <remarks><para>When a <see cref="System.Fabric.FabricReplicator" /> at a secondary replica receives an operation 
        /// for a persistent service, it must wait for the service to acknowledge that the data has been persisted before it can send that acknowledgment 
        /// back to the primary. For non-persistent services, the operation can be acknowledged immediately upon receipt.</para>
        /// <para>When a 
        /// persistent service replica fails, the Service Fabric will not immediately consider that replica as lost because the persistent state for that replica 
        /// still exists. If the replica is recovered, it can be recreated using the persisted state. In contrast, beginning to build a replacement replica 
        /// immediately may be costly and unnecessary, especially when the failures are transient. To configure how long Service Fabric should wait for the 
        /// persistent replica to recover before building a new (replacement) replica from scratch, use the 
        /// <see cref="System.Fabric.Description.StatefulServiceDescription.ReplicaRestartWaitDuration" /> parameter. For non-persistent services (those 
        /// with <see cref="System.Fabric.Description.StatefulServiceDescription.HasPersistedState" /> set to <languageKeyword>false</languageKeyword>), Service Fabric 
        /// will immediately begin creating a new replica (since there is no persistent state to recover from, and hence no point in waiting for local recovery).</para></remarks>
        public bool HasPersistedState
        {
            get;
            set;
        }

        /// <summary>
        /// <para> Gets or sets the target size of the replica set.</para>
        /// </summary>
        /// <value>
        /// <para>The target size of the replica set.</para>
        /// </value>
        /// <remarks>
        /// <para>The number of replicas that the system creates and maintains for each partition of this service.</para>
        /// </remarks>
        public int TargetReplicaSetSize
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the minimum allowed replica set size for this service.</para>
        /// </summary>
        /// <value>
        /// <para>The minimum allowed replica set size for this service.</para>
        /// </value>
        /// <remarks>
        /// <para>
        /// Defines the minimum number of replicas that Service Fabric will keep in its view of the replica set for a given partition. For example, if 
        /// the <see cref="System.Fabric.Description.StatefulServiceDescription.TargetReplicaSetSize" /> is set to five, then normally (without failures) 
        /// there will be five replicas in the view of the replica set. However, this number will decrease during failures.
        /// For example, if the <see cref="System.Fabric.Description.StatefulServiceDescription.TargetReplicaSetSize" /> is five and the 
        /// <see cref="System.Fabric.Description.StatefulServiceDescription.MinReplicaSetSize" /> is three, then three concurrent failures will leave
        /// three replicas in the replica set's view (two up, one down). Service Fabric uses majority quorum on the number of replicas
        /// maintained in this view, which is two for this example. This means that the primary will continue to be able
        /// to replicate operations AND that the remaining secondary replica MUST apply the operation in order for the replica set to make progress.
        /// If the total number of replicas drops below the majority quorum of the <see cref="System.Fabric.Description.StatefulServiceDescription.MinReplicaSetSize" />,
        /// then further writes will be disallowed.
        /// </para>
        /// </remarks>
        public int MinReplicaSetSize
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the duration, in seconds, between when a replica goes down and when a new replica is created.</para>
        /// </summary>
        /// <value>
        /// <para>The duration as a <see cref="System.TimeSpan" /> object.</para>
        /// </value>
        /// <remarks>
        /// <para>When a persistent replica goes down, this timer starts.  When it expires Service Fabric will create a new replica on any node in the 
        /// cluster. This configuration is to reduce unnecessary state copies. When a persisted replica goes down, the system waits for it to come 
        /// back up for <see cref="System.Fabric.Description.StatefulServiceDescription.ReplicaRestartWaitDuration" /> seconds before creating a new 
        /// replica which will require a copy. Note that a replica that is down is not considered lost, yet.</para>
        /// <para>The default value is 300 (seconds).</para>
        /// </remarks>
        [JsonCustomization(IsIgnored = true)]
        public TimeSpan? ReplicaRestartWaitDuration
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the maximum duration, in seconds, for which a partition is allowed to be in a state of quorum loss.</para>
        /// </summary>
        /// <value>
        /// <para>The wait duration as a <see cref="System.TimeSpan" /> object.</para>
        /// </value>
        /// <remarks>
        /// <para>If the partition is still in quorum loss after this duration, the partition is recovered from quorum loss by considering the down 
        /// replicas as lost. Note that this can potentially incur data loss. The default value is Infinity and it is not recommended to change this value.</para>
        /// </remarks>
        [JsonCustomization(IsIgnored = true)]
        public TimeSpan? QuorumLossWaitDuration
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the definition on how long StandBy replicas should be maintained before being removed.</para>
        /// </summary>
        /// <value>
        /// <para>The definition on how long StandBy replicas should be maintained before being removed.</para>
        /// </value>
        /// <remarks>
        /// <para>Sometimes a replica will be down for longer than the <see cref="System.Fabric.Description.StatefulServiceDescription.ReplicaRestartWaitDuration" />. 
        /// In these cases a new replica will be built to replace it. Sometimes however the loss is not permanent and the persistent replica is eventually recovered. 
        /// This now constitutes a StandBy replica. StandBy replicas will preferentially be used in the case of subsequent failures or resource balancing actions, 
        /// since they represent persistent state that already exists and which can be used to expedite recovery. The 
        /// <see cref="System.Fabric.Description.StatefulServiceDescription.StandByReplicaKeepDuration" /> defines how long such StandBy replicas should be maintained 
        /// before being removed.</para>
        /// </remarks>
        [JsonCustomization(IsIgnored = true)]
        public TimeSpan? StandByReplicaKeepDuration
        {
            get;
            set;
        }

        [JsonCustomization(IsDefaultValueIgnored = true)]
        private int ReplicaRestartWaitDurationSeconds
        {
            get { return (int)(this.ReplicaRestartWaitDuration ?? TimeSpan.Zero).TotalSeconds; }
            set { this.ReplicaRestartWaitDuration = TimeSpan.FromTicks(value * TimeSpan.TicksPerSecond); }
        }

        [JsonCustomization(IsDefaultValueIgnored = true)]
        private int QuorumLossWaitDurationSeconds
        {
            get { return (int)(this.QuorumLossWaitDuration ?? TimeSpan.Zero).TotalSeconds; }
            set { this.QuorumLossWaitDuration = TimeSpan.FromTicks(value * TimeSpan.TicksPerSecond); }
        }

        [JsonCustomization(IsDefaultValueIgnored = true)]
        private int StandByReplicaKeepDurationSeconds
        {
            get { return (int)(this.StandByReplicaKeepDuration ?? TimeSpan.Zero).TotalSeconds; }
            set { this.StandByReplicaKeepDuration = TimeSpan.FromTicks(value * TimeSpan.TicksPerSecond); }
        }

        private NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS flags;

        // Rest has this property
        internal NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS Flags
        {
            get
            {
                var flags = NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS.FABRIC_STATEFUL_SERVICE_SETTINGS_NONE;
                flags |= this.ReplicaRestartWaitDuration.HasValue ? NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS.FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION : flags;
                flags |= this.QuorumLossWaitDuration.HasValue ? NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS.FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION : flags;
                flags |= this.StandByReplicaKeepDuration.HasValue ? NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS.FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION : flags;
                return flags;
            }
            set
            {
                this.flags = value;
            }
        }

        internal static new unsafe StatefulServiceDescription CreateFromNative(IntPtr native)
        {
            ReleaseAssert.AssertIfNot(native != IntPtr.Zero, StringResources.Error_NullNativePointer);

            var casted = (NativeTypes.FABRIC_STATEFUL_SERVICE_DESCRIPTION*)native;

            var description = new StatefulServiceDescription();

            description.ApplicationName = NativeTypes.FromNativeUri(casted->ApplicationName);
            description.ServiceName = NativeTypes.FromNativeUri(casted->ServiceName);
            description.ServiceTypeName = NativeTypes.FromNativeString(casted->ServiceTypeName);
            description.PartitionSchemeDescription = PartitionSchemeDescription.CreateFromNative(casted->PartitionScheme, casted->PartitionSchemeDescription);
            description.TargetReplicaSetSize = casted->TargetReplicaSetSize;
            description.MinReplicaSetSize = casted->MinReplicaSetSize;
            description.PlacementConstraints = NativeTypes.FromNativeString(casted->PlacementConstraints);
            description.HasPersistedState = NativeTypes.FromBOOLEAN(casted->HasPersistedState);
            description.InitializationData = NativeTypes.FromNativeBytes(casted->InitializationData, casted->InitializationDataSize);
            description.ParseCorrelations(casted->CorrelationCount, casted->Correlations);
            description.ParseLoadMetrics(casted->MetricCount, casted->Metrics);

            if (casted->Reserved == IntPtr.Zero)
            {
                return description;
            }

            var ex1 = (NativeTypes.FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1*)casted->Reserved;
            
            if (ex1->PolicyList != IntPtr.Zero)
            {
                var pList = (NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_LIST*)ex1->PolicyList;
                description.ParsePlacementPolicies(pList->PolicyCount, pList->Policies);
            }

            if (ex1->FailoverSettings != IntPtr.Zero)
            {
                var failoverSettings = (NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS*)ex1->FailoverSettings;

                if ((failoverSettings->Flags & (uint)NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS.FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION) != 0)
                {
                    description.ReplicaRestartWaitDuration = TimeSpan.FromSeconds(failoverSettings->ReplicaRestartWaitDurationSeconds);
                }

                if ((failoverSettings->Flags & (uint)NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS.FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION) != 0)
                {
                    description.QuorumLossWaitDuration = TimeSpan.FromSeconds(failoverSettings->QuorumLossWaitDurationSeconds);
                }

                if (failoverSettings->Reserved != IntPtr.Zero)
                {
                    var failoverSettingsEx1 = (NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_EX1*)failoverSettings->Reserved;
                    if (failoverSettingsEx1 == null)
                    {
                        throw new ArgumentException(StringResources.Error_UnknownReservedType);
                    }

                    if ((failoverSettings->Flags & (uint)NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS.FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION) != 0)
                    {
                        description.StandByReplicaKeepDuration = TimeSpan.FromSeconds(failoverSettingsEx1->StandByReplicaKeepDurationSeconds);
                    }
                }
            }

            if (ex1->Reserved == IntPtr.Zero)
            {
                return description;
            }

            var ex2 = (NativeTypes.FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX2*)ex1->Reserved;
            
            if (NativeTypes.FromBOOLEAN(ex2->IsDefaultMoveCostSpecified))
            {
                // This will correctly set the property IsDefaultMoveCostSpecified to true if move cost is valid.
                description.ParseDefaultMoveCost(ex2->DefaultMoveCost);
            }

            if (ex2->Reserved == IntPtr.Zero)
            {
                return description;
            }

            var ex3 = (NativeTypes.FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX3*)ex2->Reserved;
            
            description.ServicePackageActivationMode = InteropHelpers.FromNativeServicePackageActivationMode(ex3->ServicePackageActivationMode);
            description.ServiceDnsName = NativeTypes.FromNativeString(ex3->ServiceDnsName);

     	    if (ex3->Reserved == IntPtr.Zero)
            {
                return description;
            }

            var ex4 = (NativeTypes.FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX4*)ex3->Reserved;

            description.ParseScalingPolicies(ex4->ScalingPolicyCount, ex4->ServiceScalingPolicies);

            return description;
        }

        internal override IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND kind)
        {
            this.ValidateDefaultMetricValue();

            var nativeDescription = new NativeTypes.FABRIC_STATEFUL_SERVICE_DESCRIPTION[1];
            nativeDescription[0].ApplicationName = pin.AddObject(this.ApplicationName);
            nativeDescription[0].ServiceName = pin.AddObject(this.ServiceName);
            nativeDescription[0].ServiceTypeName = pin.AddBlittable(this.ServiceTypeName);
            nativeDescription[0].PartitionScheme = (NativeTypes.FABRIC_PARTITION_SCHEME)this.PartitionSchemeDescription.Scheme;
            nativeDescription[0].PartitionSchemeDescription = this.PartitionSchemeDescription.ToNative(pin);
            nativeDescription[0].TargetReplicaSetSize = this.TargetReplicaSetSize;
            nativeDescription[0].MinReplicaSetSize = this.MinReplicaSetSize;
            nativeDescription[0].PlacementConstraints = pin.AddBlittable(this.PlacementConstraints);
            nativeDescription[0].HasPersistedState = NativeTypes.ToBOOLEAN(this.HasPersistedState);

            var correlations = this.ToNativeCorrelations(pin);
            nativeDescription[0].CorrelationCount = correlations.Item1;
            nativeDescription[0].Correlations = correlations.Item2;

            var initializationData = NativeTypes.ToNativeBytes(pin, this.InitializationData);
            nativeDescription[0].InitializationDataSize = initializationData.Item1;
            nativeDescription[0].InitializationData = initializationData.Item2;

            var metrics = NativeTypes.ToNativeLoadMetrics(pin, this.Metrics);
            nativeDescription[0].MetricCount = metrics.Item1;
            nativeDescription[0].Metrics = metrics.Item2;

            var ex1 = new NativeTypes.FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1[1];

            if (this.PlacementPolicies != null)
            {
                var policies = this.ToNativePolicies(pin);
                var pList = new NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_LIST[1];

                pList[0].PolicyCount = policies.Item1;
                pList[0].Policies = policies.Item2;

                ex1[0].PolicyList = pin.AddBlittable(pList);
            }

            var failoverSettings = new NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS[1];
            var failoverSettingsEx1 = new NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_EX1[1];
            var flags = NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS.FABRIC_STATEFUL_SERVICE_SETTINGS_NONE;

            if (ReplicaRestartWaitDuration.HasValue)
            {
                flags |= NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS.FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION;
                failoverSettings[0].ReplicaRestartWaitDurationSeconds = (uint)this.ReplicaRestartWaitDuration.Value.TotalSeconds;
            }

            if (QuorumLossWaitDuration.HasValue)
            {
                flags |= NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS.FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION;
                failoverSettings[0].QuorumLossWaitDurationSeconds = (uint)this.QuorumLossWaitDuration.Value.TotalSeconds;
            }

            if (StandByReplicaKeepDuration.HasValue)
            {
                flags |= NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS.FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION;
                failoverSettingsEx1[0].StandByReplicaKeepDurationSeconds = (uint)this.StandByReplicaKeepDuration.Value.TotalSeconds;
            }

            if (flags != NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS.FABRIC_STATEFUL_SERVICE_SETTINGS_NONE)
            {
                failoverSettings[0].Flags = (uint)flags;

                if ((flags & NativeTypes.FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS.FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION) != 0)
                {
                    failoverSettings[0].Reserved = pin.AddBlittable(failoverSettingsEx1);
                }

                ex1[0].FailoverSettings = pin.AddBlittable(failoverSettings);
            }

            var ex2 = new NativeTypes.FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX2[1];

            ex2[0].DefaultMoveCost = this.ToNativeDefaultMoveCost();
            ex2[0].IsDefaultMoveCostSpecified = NativeTypes.ToBOOLEAN(this.IsDefaultMoveCostSpecified);

            var ex3 = new NativeTypes.FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX3[1];
            ex3[0].ServicePackageActivationMode = InteropHelpers.ToNativeServicePackageActivationMode(ServicePackageActivationMode);
            ex3[0].ServiceDnsName = pin.AddBlittable(this.ServiceDnsName);

            ex2[0].Reserved = pin.AddBlittable(ex3);
            ex1[0].Reserved = pin.AddBlittable(ex2);

            var ex4 = new NativeTypes.FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX4[1];
            if (ScalingPolicies != null)
            {
                var scalingPolicies = this.ToNativeScalingPolicies(pin);

                ex4[0].ScalingPolicyCount = scalingPolicies.Item1;
                ex4[0].ServiceScalingPolicies = scalingPolicies.Item2;
            }

            ex3[0].Reserved = pin.AddBlittable(ex4);
            
            nativeDescription[0].Reserved = pin.AddBlittable(ex1);

            kind = NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND.FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
            return pin.AddBlittable(nativeDescription);
        }

        internal void ValidateDefaultMetricValue()
        {
            foreach (StatefulServiceLoadMetricDescription metric in this.Metrics)
            {
                if ((metric.Name == "PrimaryCount" && (metric.PrimaryDefaultLoad != 1 || metric.SecondaryDefaultLoad != 0))
                || (metric.Name == "ReplicaCount" && (metric.PrimaryDefaultLoad != 1 || metric.SecondaryDefaultLoad != 1))
                || (metric.Name == "SecondaryCount" && (metric.PrimaryDefaultLoad != 0 || metric.SecondaryDefaultLoad != 1))
                || (metric.Name == "Count" && (metric.PrimaryDefaultLoad != 1 || metric.SecondaryDefaultLoad != 1))
                || (metric.Name == "InstanceCount"))
                {
                    throw new ArgumentException(StringResources.Error_InvalidDefaultMetricValue);
                }
            } 
        }
    }
}
