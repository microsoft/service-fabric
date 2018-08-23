// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Chaos.RandomActionGenerator;
    using System.Fabric.Common;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Text;

    /// <summary>
    /// This class defines all the test parameters to configure the ChaosTestScenario.
    /// </summary>
    [Serializable]
    public class ChaosParameters : ByteSerializable
    {
        private const string TraceType = "Chaos.ChaosParameters";

        private double requestTimeoutFactor;
        private readonly bool populated = false;
        private Dictionary<string, string> contextDictionary;

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosParameters" /> class.</para>
        /// </summary>
        /// <param name="maxConcurrentFaults">Maximum number of concurrent faults induced per iteration with the lowest being 1. The higher the concurrency the more aggressive the failovers
        /// thus inducing more complex series of failures to uncover bugs. using 2 or 3 for this is recommended.</param>
        /// <param name="timeToRun">Total time Chaos should run; maximum allowed value is TimeSpan.FromSeconds(uint.MaxValue)</param>
        /// <returns>The object containing the Chaos scenario parameters, typed as ChaosScenarioParameters</returns>
        public ChaosParameters(
            long maxConcurrentFaults,
            TimeSpan? timeToRun = null)
            : this(
                ChaosConstants.DefaultClusterStabilizationTimeout,
                maxConcurrentFaults,
                true,
                timeToRun ?? TimeSpan.FromSeconds(uint.MaxValue),
                null,
                true,
                ChaosConstants.WaitTimeBetweenIterationsDefault,
                ChaosConstants.WaitTimeBetweenFaultsDefault,
                ChaosConstants.ClusterHealthPolicyDefault)
            {
            }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosParameters" /> class.</para>
        /// </summary>
        /// <returns>The object containing the Chaos scenario parameters, typed as ChaosScenarioParameters</returns>
        public ChaosParameters()
            : this(
                ChaosConstants.DefaultClusterStabilizationTimeout,
                ChaosConstants.MaxConcurrentFaultsDefault,
                true,
                TimeSpan.FromSeconds(uint.MaxValue),
                null,
                true,
                ChaosConstants.WaitTimeBetweenIterationsDefault,
                ChaosConstants.WaitTimeBetweenFaultsDefault,
                ChaosConstants.ClusterHealthPolicyDefault)
            {
            }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosParameters" /> class.</para>
        /// </summary>
        /// <param name="maxClusterStabilizationTimeout">The maximum amount of time to wait for the entire cluster to stabilize after a fault iteration; cannot exceed TimeSpan.FromSeconds(uint.MaxValue)</param>
        /// <param name="maxConcurrentFaults">Maximum number of concurrent faults induced per iteration with the lowest being 1. The higher the concurrency the more aggressive the failovers
        /// thus inducing more complex series of failures to uncover bugs. using 2 or 3 for this is recommended.</param>
        /// <param name="enableMoveReplicaFaults">Enables or disables the MovePrimary and MoveSecondary faults.</param>
        /// <param name="timeToRun">Total time Chaos should run; maximum allowed value is TimeSpan.FromSeconds(uint.MaxValue)</param>
        /// <returns>The object containing the Chaos scenario parameters, typed as ChaosScenarioParameters</returns>
        public ChaosParameters(
            TimeSpan maxClusterStabilizationTimeout,
            long maxConcurrentFaults,
            bool enableMoveReplicaFaults,
            TimeSpan? timeToRun = null)
            : this(
                maxClusterStabilizationTimeout,
                maxConcurrentFaults,
                enableMoveReplicaFaults,
                timeToRun ?? TimeSpan.FromSeconds(uint.MaxValue),
                null,
                true,
                ChaosConstants.WaitTimeBetweenIterationsDefault,
                ChaosConstants.WaitTimeBetweenFaultsDefault,
                ChaosConstants.ClusterHealthPolicyDefault)
            {
            }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosParameters" /> class.</para>
        /// </summary>
        /// <param name="maxClusterStabilizationTimeout">The maximum amount of time to wait for the entire cluster to stabilize after a fault iteration; cannot exceed TimeSpan.FromSeconds(uint.MaxValue)</param>
        /// <param name="maxConcurrentFaults">Maximum number of concurrent faults induced per iteration with the lowest being 1. The higher the concurrency the more aggressive the failovers
        /// thus inducing more complex series of failures to uncover bugs. using 2 or 3 for this is recommended.</param>
        /// <param name="enableMoveReplicaFaults">Enables or disables the MovePrimary and MoveSecondary faults.</param>
        /// <param name="timeToRun">After running for this much time, Chaos will stop; cannot exceed TimeSpan.FromSeconds(uint.MaxValue)</param>
        /// <param name="context">This is a bag of (key, value) pairs. This can be used to record detailed context about why Chaos is being started for example.</param>
        /// <returns>The object containing the Chaos scenario parameters, typed as ChaosScenarioParameters</returns>
        public ChaosParameters(
            TimeSpan maxClusterStabilizationTimeout,
            long maxConcurrentFaults,
            bool enableMoveReplicaFaults,
            TimeSpan timeToRun,
            Dictionary<string, string> context)
            : this(
                maxClusterStabilizationTimeout,
                maxConcurrentFaults,
                enableMoveReplicaFaults,
                timeToRun,
                context,
                true,
                ChaosConstants.WaitTimeBetweenIterationsDefault,
                ChaosConstants.WaitTimeBetweenFaultsDefault,
                ChaosConstants.ClusterHealthPolicyDefault)
            {
            }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosParameters" /> class.</para>
        /// </summary>
        /// <param name="maxClusterStabilizationTimeout">The maximum amount of time to wait for the entire cluster to stabilize after a fault iteration; cannot exceed TimeSpan.FromSeconds(uint.MaxValue)</param>
        /// <param name="maxConcurrentFaults">Maximum number of concurrent faults induced per iteration with the lowest being 1. The higher the concurrency the more aggressive the failovers
        /// thus inducing more complex series of failures to uncover bugs. using 2 or 3 for this is recommended.</param>
        /// <param name="enableMoveReplicaFaults">Enables or disables the MovePrimary and MoveSecondary faults.</param>
        /// <param name="timeToRun">After running for this much time, Chaos will stop; cannot exceed TimeSpan.FromSeconds(uint.MaxValue)</param>
        /// <param name="context">This is a bag of (key, value) pairs. This can be used to record detailed context about why Chaos is being started for example.</param>
        /// <param name="waitTimeBetweenIterations">This is the amount of pause between two consecutive iterations of fault inducing. The more the pause, the less is the rate of faults over time; cannot exceed TimeSpan.FromSeconds(uint.MaxValue)</param>
        /// <param name="waitTimeBetweenFaults">This is the amount of pause between two consecutive faults in a single iteration -- the more the pause, the less the concurrency of faults; cannot exceed TimeSpan.FromSeconds(uint.MaxValue)</param>
        public ChaosParameters(
            TimeSpan maxClusterStabilizationTimeout,
            long maxConcurrentFaults,
            bool enableMoveReplicaFaults,
            TimeSpan timeToRun,
            Dictionary<string, string> context,
            TimeSpan waitTimeBetweenIterations,
            TimeSpan waitTimeBetweenFaults)
            : this(
                maxClusterStabilizationTimeout,
                maxConcurrentFaults,
                enableMoveReplicaFaults,
                timeToRun,
                context,
                true,
                waitTimeBetweenIterations,
                waitTimeBetweenFaults,
                ChaosConstants.ClusterHealthPolicyDefault)
            {
            }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosParameters" /> class.</para>
        /// </summary>
        /// <param name="maxClusterStabilizationTimeout">The maximum amount of time to wait for the entire cluster to stabilize after a fault iteration; cannot exceed TimeSpan.FromSeconds(uint.MaxValue)</param>
        /// <param name="maxConcurrentFaults">Maximum number of concurrent faults induced per iteration with the lowest being 1. The higher the concurrency the more aggressive the failovers
        /// thus inducing more complex series of failures to uncover bugs. using 2 or 3 for this is recommended.</param>
        /// <param name="enableMoveReplicaFaults">Enables or disables the MovePrimary and MoveSecondary faults.</param>
        /// <param name="timeToRun">After running for this much time, Chaos will stop; cannot exceed TimeSpan.FromSeconds(uint.MaxValue)</param>
        /// <param name="context">This is a bag of (key, value) pairs. This can be used to record detailed context about why Chaos is being started for example.</param>
        /// <param name="waitTimeBetweenIterations">This is the amount of pause between two consecutive iterations of fault inducing. The more the pause, the less is the rate of faults over time; cannot exceed TimeSpan.FromSeconds(uint.MaxValue)</param>
        /// <param name="waitTimeBetweenFaults">This is the amount of pause between two consecutive faults in a single iteration -- the more the pause, the less the concurrency of faults; cannot exceed TimeSpan.FromSeconds(uint.MaxValue)</param>
        /// <param name="clusterHealthPolicy">The cluster health policy that determines how healthy a cluster must be in order for Chaos to go on inducing faults.</param>
        public ChaosParameters(
            TimeSpan maxClusterStabilizationTimeout,
            long maxConcurrentFaults,
            bool enableMoveReplicaFaults,
            TimeSpan timeToRun,
            Dictionary<string, string> context,
            TimeSpan waitTimeBetweenIterations,
            TimeSpan waitTimeBetweenFaults,
            ClusterHealthPolicy clusterHealthPolicy)
            : this(
                maxClusterStabilizationTimeout,
                maxConcurrentFaults,
                enableMoveReplicaFaults,
                timeToRun,
                context,
                true,
                waitTimeBetweenIterations,
                waitTimeBetweenFaults,
                clusterHealthPolicy)
        {
        }

        internal ChaosParameters(
            TimeSpan maxClusterStabilizationTimeout,
            long maxConcurrentFaults,
            bool enableMoveReplicaFaults,
            TimeSpan timeToRun,
            Dictionary<string, string> context,
            bool disableStartStopNodeFaults,
            TimeSpan waitTimeBetweenIterations,
            TimeSpan waitTimeBetweenFaults,
            ClusterHealthPolicy clusterHealthPolicy)
        {
            this.ValidateArguments(maxConcurrentFaults, context, maxClusterStabilizationTimeout, timeToRun, waitTimeBetweenIterations, waitTimeBetweenFaults);

            this.MaxClusterStabilizationTimeout = maxClusterStabilizationTimeout;
            this.WaitTimeBetweenIterations = ChaosConstants.WaitTimeBetweenIterationsDefault;

            this.ActionGeneratorParameters = new ActionGeneratorParameters { MaxConcurrentFaults = maxConcurrentFaults };

            // Set default values for action generator parameters.

            this.EnableMoveReplicaFaults = enableMoveReplicaFaults;

            if (disableStartStopNodeFaults)
            {
                // This is disable start/stop node
                this.ActionGeneratorParameters.NodeFaultActionsParameters.RestartNodeFaultWeight = 1.0;
                this.ActionGeneratorParameters.NodeFaultActionsParameters.StartStopNodeFaultWeight = 0;
            }

            this.ActionGeneratorParameters.NodeFaultActionWeight = 40.0;
            this.ActionGeneratorParameters.ServiceFaultActionWeight = 60.0;

            this.ActionGeneratorParameters.ServiceFaultActionsParameters.RemoveReplicaFaultWeight = 100.0;
            this.ActionGeneratorParameters.ServiceFaultActionsParameters.RestartReplicaFaultWeight = 100.0;
            this.ActionGeneratorParameters.ServiceFaultActionsParameters.RestartCodePackageFaultWeight = 100.0;
            if (enableMoveReplicaFaults)
            {
                this.ActionGeneratorParameters.ServiceFaultActionsParameters.MovePrimaryFaultWeight = 100.0;
                this.ActionGeneratorParameters.ServiceFaultActionsParameters.MoveSecondaryFaultWeight = 100.0;
            }
            else
            {
                this.ActionGeneratorParameters.ServiceFaultActionsParameters.MovePrimaryFaultWeight = 0.0;
                this.ActionGeneratorParameters.ServiceFaultActionsParameters.MoveSecondaryFaultWeight = 0.0;
            }

            this.MaxConcurrentFaults = maxConcurrentFaults;

            this.TimeToRun = timeToRun;

            this.WaitTimeBetweenIterations = waitTimeBetweenIterations;
            this.WaitTimeBetweenFaults = waitTimeBetweenFaults;

            this.requestTimeoutFactor = ChaosConstants.RequestTimeoutFactorDefault;
            this.OperationTimeout = ChaosConstants.OperationTimeoutDefault;

            this.populated = true;

            this.Context = context;

            this.ClusterHealthPolicy = clusterHealthPolicy ?? ChaosConstants.ClusterHealthPolicyDefault;

            this.ApplyUpdatesFromContextIfAvailable(context);
        }

        internal ChaosParameters(
            ChaosParameters other)
        {
            this.ValidateArguments(other.MaxConcurrentFaults, other.Context, other.MaxClusterStabilizationTimeout, other.TimeToRun, other.WaitTimeBetweenIterations, other.WaitTimeBetweenFaults);

            this.MaxClusterStabilizationTimeout = other.MaxClusterStabilizationTimeout;

            this.WaitTimeBetweenIterations = other.WaitTimeBetweenIterations;

            this.ActionGeneratorParameters = new ActionGeneratorParameters { MaxConcurrentFaults = other.MaxConcurrentFaults };

            // Set default values for action generator parameters.

            this.EnableMoveReplicaFaults = other.EnableMoveReplicaFaults;

            // This is disable start/stop node
            this.ActionGeneratorParameters.NodeFaultActionsParameters.RestartNodeFaultWeight = other.ActionGeneratorParameters.NodeFaultActionsParameters.RestartNodeFaultWeight;
            this.ActionGeneratorParameters.NodeFaultActionsParameters.StartStopNodeFaultWeight = other.ActionGeneratorParameters.NodeFaultActionsParameters.StartStopNodeFaultWeight;

            this.ActionGeneratorParameters.NodeFaultActionWeight = other.ActionGeneratorParameters.NodeFaultActionWeight;
            this.ActionGeneratorParameters.ServiceFaultActionWeight = other.ActionGeneratorParameters.ServiceFaultActionWeight;

            this.ActionGeneratorParameters.ServiceFaultActionsParameters.RemoveReplicaFaultWeight = other.ActionGeneratorParameters.ServiceFaultActionsParameters.RemoveReplicaFaultWeight;
            this.ActionGeneratorParameters.ServiceFaultActionsParameters.RestartReplicaFaultWeight = other.ActionGeneratorParameters.ServiceFaultActionsParameters.RestartReplicaFaultWeight;
            this.ActionGeneratorParameters.ServiceFaultActionsParameters.RestartCodePackageFaultWeight = other.ActionGeneratorParameters.ServiceFaultActionsParameters.RestartCodePackageFaultWeight;

            this.ActionGeneratorParameters.ServiceFaultActionsParameters.MovePrimaryFaultWeight = other.ActionGeneratorParameters.ServiceFaultActionsParameters.MovePrimaryFaultWeight;
            this.ActionGeneratorParameters.ServiceFaultActionsParameters.MoveSecondaryFaultWeight = other.ActionGeneratorParameters.ServiceFaultActionsParameters.MoveSecondaryFaultWeight;

            this.MaxConcurrentFaults = other.MaxConcurrentFaults;

            this.TimeToRun = other.TimeToRun;

            this.WaitTimeBetweenIterations = other.WaitTimeBetweenIterations;
            this.WaitTimeBetweenFaults = other.WaitTimeBetweenFaults;

            this.requestTimeoutFactor = other.RequestTimeoutFactor;
            this.OperationTimeout = other.OperationTimeout;

            this.populated = true;

            this.Context = other.Context;

            this.ClusterHealthPolicy = other.ClusterHealthPolicy ?? ChaosConstants.ClusterHealthPolicyDefault;

            this.ApplyUpdatesFromContextIfAvailable(other.Context);

            this.ChaosTargetFilter = other.ChaosTargetFilter;
        }

        /// <summary>
        /// Wait time between two iterations for a random duration bound by this value.
        /// </summary>
        /// <value>
        /// The time-separation between two consecutive iterations of the ChaosTestScenario
        /// </value>
        [DataMember]
        public TimeSpan WaitTimeBetweenIterations { get; set; }

        internal ActionGeneratorParameters ActionGeneratorParameters { get; set; }

        /// <summary>
        /// The maximum amount of time to wait for the cluster to stabilize after a fault before failing the test.
        /// </summary>
        /// <value>
        /// After each iteration, the ChaosTestScenario waits for at most this amount of time for the cluster to become healthy
        /// </value>
        [DataMember]
        public TimeSpan MaxClusterStabilizationTimeout { get; set; }

        /// <summary>
        /// Maximum number of concurrent faults induced per iteration with the lowest being 1. The higer the concurrency the more aggressive the failovers;
        /// thus, inducing more complex series of failures to uncover bugs -- using 2 or 3 for this is recommended.
        /// </summary>
        [DataMember]
        public long MaxConcurrentFaults { get; set; }

        /// <summary>
        /// Enables or disables the MovePrimary and MoveSecondary faults.
        /// </summary>
        [DataMember]
        public bool EnableMoveReplicaFaults { get; set; }

        /// <summary>
        /// The maximum wait time between consecutive faults: the larger the value, the lower the concurrency (of faults).
        /// </summary>
        /// <value>
        /// Returns the maximum wait time between two consecutive faults as a TimeSpan
        /// </value>
        [DataMember]
        public TimeSpan WaitTimeBetweenFaults { get; set; }

        internal TimeSpan OperationTimeout { get; set; }

        internal double RequestTimeoutFactor
        {
            get
            {
                return this.requestTimeoutFactor <= 1 ? this.requestTimeoutFactor : ChaosConstants.RequestTimeoutFactorDefault;
            }
        }

        internal TimeSpan RequestTimeout
        {
            get
            {
                return TimeSpan.FromSeconds(this.requestTimeoutFactor * this.OperationTimeout.TotalSeconds);
            }
        }

        /// <summary>
        /// Gets the bag of (key, value) pairs that was passed while starting Chaos
        /// </summary>
        [DataMember]
        public Dictionary<string, string> Context
        {
            get
            {
                return this.contextDictionary;
            }

            internal set
            {
                this.contextDictionary = value ?? new Dictionary<string, string>();
            }
        }

        /// <summary>
        /// Total time for which the scenario will run before ending.
        /// </summary>
        /// <value>
        /// Returns the max run-time of the scenario as TimeSpan
        /// </value>
        [DataMember]
        public TimeSpan TimeToRun
        {
            get;
            internal set;
        }

        /// <summary>
        /// ClusterHealthPolicy determines the state of the health of the entities that Chaos
        /// ensures before going onto the next set of faults. Setting 'ConsiderWarningAsError' to false
        /// would let Chaos go onto the next set of faults while there are entities in the cluster with
        /// healthState == warning (although Chaos will skip the entities in warning while choosing
        /// faultable entities.)
        /// </summary>
        [DataMember]
        public ClusterHealthPolicy ClusterHealthPolicy
        {
            get;
            internal set;
        }

        /// <summary>
        /// List of cluster entities to target for Chaos faults.
        /// This filter can be used to target Chaos faults only to certain node types or only to certain applications.
        /// </summary>
        public ChaosTargetFilter ChaosTargetFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Returns a string representation of the class
        /// </summary>
        /// <returns>A string object</returns>
        public override string ToString()
        {
            if(!populated)
            {
                return string.Empty;
            }

            var description = new StringBuilder();
            description.AppendFormat("maxClusterStabilizationTimeout={0}, waitTimeBetweenFaults={1}, waitTimeBetweenIterations={2}, maxConcurrentFaults={3}, timeToRun={4}, enableMoveReplicas={5}",
                this.MaxClusterStabilizationTimeout,
                this.WaitTimeBetweenFaults,
                this.WaitTimeBetweenIterations,
                this.MaxConcurrentFaults,
                this.TimeToRun,
                this.EnableMoveReplicaFaults);

            if (this.Context != null && this.Context.Count > 0)
            {
                description.AppendLine(", Context:");
                for (int i = 0; i < this.Context.Count; ++i)
                {
                    var elem = this.Context.ElementAt(i);

                    description.AppendFormat("key={0}, value={1};", elem.Key, elem.Value);
                }
            }

            description.AppendLine(string.Format("ClusterHealthPolicy={0}", this.ClusterHealthPolicy));

            if (this.ChaosTargetFilter != null)
            {
                description.AppendLine(this.ChaosTargetFilter.ToString());
            }

            return description.ToString();
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeContextMap = new NativeTypes.FABRIC_EVENT_CONTEXT_MAP();

            if (this.Context != null && this.Context.Any())
            {
                int count = this.Context.ContainsKey(ChaosConstants.KeyForDummyContextEntry) ? this.Context.Count - 1 : this.Context.Count;
                var nativeContextArray = new NativeTypes.FABRIC_EVENT_CONTEXT_MAP_ITEM[count];

                for (int i = 0, j = 0; i < this.Context.Count; ++i)
                {
                    if(!this.Context.ElementAt(i).Key.Equals(ChaosConstants.KeyForDummyContextEntry, StringComparison.OrdinalIgnoreCase))
                    {
                        nativeContextArray[j].Key = pinCollection.AddObject(this.Context.ElementAt(i).Key);
                        nativeContextArray[j].Value = pinCollection.AddObject(this.Context.ElementAt(i).Value);

                        ++j;
                    }
                }

                nativeContextMap.Count = (uint)nativeContextArray.Length;
                nativeContextMap.Items = pinCollection.AddBlittable(nativeContextArray);
            }

            var ex1 = new NativeTypes.FABRIC_CHAOS_PARAMETERS_EX1
            {
                ClusterHealthPolicy = this.ClusterHealthPolicy.ToNative(pinCollection)
            };

            var ex2 = new NativeTypes.FABRIC_CHAOS_PARAMETERS_EX2
            {
                ChaosTargetFilter = this.ChaosTargetFilter == null ? IntPtr.Zero : this.ChaosTargetFilter.ToNative(pinCollection)
            };

            ex1.Reserved = pinCollection.AddBlittable(ex2);

            var nativeChaosParameters = new NativeTypes.FABRIC_CHAOS_PARAMETERS
            {
                MaxClusterStabilizationTimeoutInSeconds = Convert.ToUInt32(this.MaxClusterStabilizationTimeout.TotalSeconds),
                MaxConcurrentFaults = Convert.ToUInt32(this.MaxConcurrentFaults),
                EnableMoveReplicaFaults = NativeTypes.ToBOOLEAN(this.EnableMoveReplicaFaults),
                WaitTimeBetweenIterationsInSeconds = Convert.ToUInt32(this.WaitTimeBetweenIterations.TotalSeconds),
                WaitTimeBetweenFaultsInSeconds = Convert.ToUInt32(this.WaitTimeBetweenFaults.TotalSeconds),
                TimeToRunInSeconds = Convert.ToUInt32(this.TimeToRun.TotalSeconds),
                Context = nativeContextMap.Count > 0 ? pinCollection.AddBlittable(nativeContextMap) : IntPtr.Zero,
                Reserved = pinCollection.AddBlittable(ex1)
            };

            return pinCollection.AddBlittable(nativeChaosParameters);
        }

        /// <summary>
        /// Reads the state of this object from byte array.
        /// </summary>
        /// <param name="br">A BinaryReader object</param>
        /// <exception cref="EndOfStreamException">The end of the stream is reached. </exception>
        /// <exception cref="IOException">An I/O error occurs. </exception>
        public override void Read(BinaryReader br)
        {
            long timetoRunInTicks = br.ReadInt64();
            this.TimeToRun = new TimeSpan(timetoRunInTicks);

            long waitTimeBetweenIterationsInTicks = br.ReadInt64();
            this.WaitTimeBetweenIterations = new TimeSpan(waitTimeBetweenIterationsInTicks);

            long waitTimeBetweenFaultsInTicks = br.ReadInt64();
            this.WaitTimeBetweenFaults = new TimeSpan(waitTimeBetweenFaultsInTicks);

            long maxClusterStabilizationTimeoutInTicks = br.ReadInt64();
            this.MaxClusterStabilizationTimeout = new TimeSpan(maxClusterStabilizationTimeoutInTicks);

            bool enableReplicaMoveFaults = br.ReadBoolean();
            this.EnableMoveReplicaFaults = enableReplicaMoveFaults;

            long maxConcurrentFaults = br.ReadInt64();
            this.MaxConcurrentFaults = maxConcurrentFaults;

            // Context
            int kvpCount = br.ReadInt32();
            if (kvpCount > 0)
            {
                this.Context = new Dictionary<string, string>();
                for (int i = 0; i < kvpCount; ++i)
                {
                    string key = br.ReadString();
                    string val = br.ReadString();
                    if(!this.Context.ContainsKey(key))
                    {
                        this.Context.Add(key, val);
                    }
                }
            }

            // Extensions, not available in all versions, thus optional
            if (br.BaseStream.Position != br.BaseStream.Length)
            {
                this.ReadClusterHealthPolicy(br, this.ClusterHealthPolicy);
            }
            else
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "ChaosParameters.Read could not find ClusterHealthPolicy; probably because, it was trying to deserialize an older version of ChaosParameters.");
            }

            if (br.BaseStream.Position != br.BaseStream.Length)
            {
                bool hasChaosTargetFilter = br.ReadBoolean();

                if (hasChaosTargetFilter)
                {
                    this.ChaosTargetFilter = new ChaosTargetFilter();
                    this.ChaosTargetFilter.Read(br);
                }
            }
        }

        /// <summary>
        /// Writes the state of this object into a byte array.
        /// </summary>
        /// <param name="bw">A BinaryWriter object.</param>
        /// <exception cref="IOException">An I/O error occurs. </exception>
        public override void Write(BinaryWriter bw)
        {
            bw.Write(this.TimeToRun.Ticks);
            bw.Write(this.WaitTimeBetweenIterations.Ticks);
            bw.Write(this.WaitTimeBetweenFaults.Ticks);
            bw.Write(this.MaxClusterStabilizationTimeout.Ticks);
            bw.Write(this.EnableMoveReplicaFaults);
            bw.Write(this.MaxConcurrentFaults);

            // Context
            bw.Write((this.Context == null) ? 0 : this.Context.Count);
            if (this.Context != null && this.Context.Count > 0)
            {
                foreach (var kvp in this.Context)
                {
                    bw.Write(kvp.Key);   // key
                    bw.Write(kvp.Value); // value
                }
            }

            this.WriteClusterHealthPolicy(bw, this.ClusterHealthPolicy);

            bw.Write(this.ChaosTargetFilter != null);
            this.ChaosTargetFilter?.Write(bw);
        }

        /// <summary>
        /// Assumes the unique internal ctor is always called and populate the public properties that have not been set in that ctor
        /// </summary>
        /// <param name="source">The object from which to take the values of the ramaining public properties.</param>
        public void RehydratePropertiesNotInCtorFrom(ChaosParameters source)
        {
            if (source == null)
            {
                return;
            }

            var uniqueInternalConstructor = typeof(ChaosParameters).GetConstructors(BindingFlags.NonPublic | BindingFlags.Instance).FirstOrDefault();

            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.UnexpectedNumberOfInternalChaosParametersCtor_TelemetryId,
                uniqueInternalConstructor.Equals(default(ConstructorInfo)),
                StringResources.ChaosError_ChaosParametersContructionNotComplete);

            HashSet<string> constructorParameterNameSet = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            uniqueInternalConstructor.GetParameters()?.ForEach(p => constructorParameterNameSet.Add(p.Name));

            HashSet<PropertyInfo> publicPropertySet = new HashSet<PropertyInfo>();
            typeof(ChaosParameters).GetProperties(BindingFlags.Public | BindingFlags.Instance)?.ForEach(p => publicPropertySet.Add(p));

            var publicPropertiesNotSetFromCtor = new List<PropertyInfo>();
            publicPropertySet.Where(p => !constructorParameterNameSet.Contains(p.Name)).ForEach(p => publicPropertiesNotSetFromCtor.Add(p));

            foreach (var unsetProperty in publicPropertiesNotSetFromCtor)
            {
                this.SetPublicProperty(unsetProperty, source);
            }
        }

        internal static unsafe ChaosParameters CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_CHAOS_PARAMETERS native = *(NativeTypes.FABRIC_CHAOS_PARAMETERS*)nativeRaw;

            TimeSpan maxClusterStabilizationTimeout =
                TimeSpan.FromSeconds(native.MaxClusterStabilizationTimeoutInSeconds);

            var maxConcurrentFaults = native.MaxConcurrentFaults;

            TimeSpan waitTimeBetweenIterations = TimeSpan.FromSeconds(native.WaitTimeBetweenIterationsInSeconds);

            TimeSpan waitTimeBetweenFaults = TimeSpan.FromSeconds(native.WaitTimeBetweenFaultsInSeconds);

            TimeSpan timeToRun = TimeSpan.FromSeconds(native.TimeToRunInSeconds);

            var enabledMoveReplicaFaults = NativeTypes.FromBOOLEAN(native.EnableMoveReplicaFaults);

            var contextMap = new Dictionary<string, string>();

            if (native.Context != IntPtr.Zero)
            {
                var nativeMapPtr = (NativeTypes.FABRIC_EVENT_CONTEXT_MAP*)native.Context;
                var bytesPerItem = Marshal.SizeOf(typeof(NativeTypes.FABRIC_EVENT_CONTEXT_MAP_ITEM));
                for (int i = 0; i < nativeMapPtr->Count; ++i)
                {
                    var nativeItemPtr = nativeMapPtr->Items + (i * bytesPerItem);
                    var nativeMapItemPtr = (NativeTypes.FABRIC_EVENT_CONTEXT_MAP_ITEM*)nativeItemPtr;

                    contextMap.Add(
                        NativeTypes.FromNativeString(nativeMapItemPtr->Key),
                        NativeTypes.FromNativeString(nativeMapItemPtr->Value));
                }
            }

            var parameters = new ChaosParameters(
                maxClusterStabilizationTimeout,
                maxConcurrentFaults,
                enabledMoveReplicaFaults,
                timeToRun,
                contextMap,
                waitTimeBetweenIterations,
                waitTimeBetweenFaults);

            if (native.Reserved != IntPtr.Zero)
            {
                var ex1 = *((NativeTypes.FABRIC_CHAOS_PARAMETERS_EX1*)native.Reserved);

                if (ex1.ClusterHealthPolicy != IntPtr.Zero)
                {
                    parameters.ClusterHealthPolicy = ClusterHealthPolicy.FromNative(ex1.ClusterHealthPolicy);
                }

                if (ex1.Reserved != IntPtr.Zero)
                {
                    var ex2 = *(NativeTypes.FABRIC_CHAOS_PARAMETERS_EX2*)ex1.Reserved;
                    parameters.ChaosTargetFilter = ChaosTargetFilter.FromNative(ex2.ChaosTargetFilter);
                }
            }

            return parameters;
        }

        private void ReadClusterHealthPolicy(BinaryReader br, ClusterHealthPolicy policy)
        {
            policy.ConsiderWarningAsError = br.ReadBoolean();
            policy.MaxPercentUnhealthyNodes = br.ReadByte();
            policy.MaxPercentUnhealthyApplications = br.ReadByte();

            // Read application health policy map
            int kvpCount = br.ReadInt32();
            if (kvpCount > 0)
            {
                for (int i = 0; i < kvpCount; ++i)
                {
                    string key = br.ReadString();
                    byte value = br.ReadByte();

                    policy.ApplicationTypeHealthPolicyMap[key] = value;
                }
            }
        }

        private void WriteClusterHealthPolicy(BinaryWriter bw, ClusterHealthPolicy policy)
        {
            bw.Write(policy.ConsiderWarningAsError);
            bw.Write(policy.MaxPercentUnhealthyNodes);
            bw.Write(policy.MaxPercentUnhealthyApplications);

            bw.Write(policy.ApplicationTypeHealthPolicyMap.Count);
            foreach (var pair in policy.ApplicationTypeHealthPolicyMap)
            {
                bw.Write(pair.Key);
                bw.Write(pair.Value);
            }
        }

        private void ValidateArguments(
            long maxConcurrentFaults,
            Dictionary<string, string> context,
            TimeSpan maxClusterStabilizationTimeout,
            TimeSpan timeToRun,
            TimeSpan waitTimeBetweenIterations,
            TimeSpan waitTimeBetweenFaults)
        {
            if (maxConcurrentFaults < 0 || maxConcurrentFaults > UInt32.MaxValue)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "Expected value of MaxConcurrentFaults in ChaosParameters 0<=X<{0} Got: {1}", UInt32.MaxValue, maxConcurrentFaults);

                throw new ArgumentOutOfRangeException("maxConcurrentFaults", "Value must be a non-negative integer not-exceeding Uint32.MaxValue.");
            }

            if (context != null && context.Count > ChaosConstants.MaxNumberOfStartChaosContextEntries)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "Expected value of context.Count to be X<={0} Got: ",  ChaosConstants.MaxNumberOfStartChaosContextEntries, context.Count);

                throw new ArgumentOutOfRangeException("context", StringResources.ChaosError_ContextTooLarge);
            }

            if (maxClusterStabilizationTimeout.TotalSeconds > UInt32.MaxValue)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "Expected value of MaxClusterStabilizationTimeout.TotalSeconds to be X<={0} Got: {1}", UInt32.MaxValue, maxClusterStabilizationTimeout.TotalSeconds);

                throw new ArgumentOutOfRangeException("timeToRun", "maxClusterStabilizationTimeout.TotalSeconds cannot exceed UInt32.MaxValue.");
            }

            if (timeToRun.TotalSeconds > UInt32.MaxValue)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "Expected value of timeToRun.TotalSeconds to be X<={0} Got:{1}", UInt32.MaxValue, timeToRun.TotalSeconds);

                throw new ArgumentOutOfRangeException("timeToRun", "timeToRun.TotalSeconds cannot exceed UInt32.MaxValue.");
            }

            if (waitTimeBetweenIterations.TotalSeconds > UInt32.MaxValue)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "Expected value of waitTimeBetweenIterations.TotalSeconds to be X<={0} Got:{1}", UInt32.MaxValue, WaitTimeBetweenIterations.TotalSeconds);

                throw new ArgumentOutOfRangeException("timeToRun", "waitTimeBetweenIterations.TotalSeconds cannot exceed UInt32.MaxValue.");
            }

            if (waitTimeBetweenFaults.TotalSeconds > UInt32.MaxValue)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "Expected value of waitTimeBetweenFaults.TotalSeconds to be X<={0} Got: {1}", UInt32.MaxValue, WaitTimeBetweenFaults.TotalSeconds);

                throw new ArgumentOutOfRangeException("timeToRun", "waitTimeBetweenFaults.TotalSeconds cannot exceed UInt32.MaxValue.");
            }
        }

        private void ApplyUpdatesFromContextIfAvailable(Dictionary<string, string> context)
        {
            if (context != null)
            {
                foreach (var kvp in context)
                {
                    if (kvp.Key.StartsWith(ChaosConstants.ServiceFabricChaosInternalContextKeyPrefix))
                    {
                        ChaosUtility.VisitObject(this, kvp.Key.Substring(ChaosConstants.ServiceFabricChaosInternalContextKeyPrefix.Length), kvp.Value, ChaosUtility.ObjectVisitMode.Update);
                    }
                }
            }
        }
    }
}