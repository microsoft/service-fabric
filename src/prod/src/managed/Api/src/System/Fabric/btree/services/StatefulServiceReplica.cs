// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Services
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Data.Replicator;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Net;
    using System.Linq;

    /// <summary>
    /// Abstract class for stateful service replica functionality.
    /// </summary>
    public abstract class StatefulServiceReplica : IReplicatedService, IStatefulServiceReplica
    {
        #region Instance Members

        /// <summary>
        /// Used to read the replicator settings from the service manifest.
        /// </summary>
        private const string ReplicatorEndpointTag = "ReplicatorEndpoint";

        /// <summary>
        /// Replica initialization parameters.
        /// </summary>
        private StatefulServiceInitializationParameters initializationParameters;

        /// <summary>
        /// Partition object for this replica.
        /// </summary>
        private IStatefulServicePartition servicePartition;

        /// <summary>
        /// Partition object for this replica.
        /// </summary>
        private IStatefulServicePartitionEx servicePartitionEx;

        /// <summary>
        /// Partition object for this replica when hosted inside a service group.
        /// </summary>
        private IServiceGroupPartition serviceGroupPartition;

        /// <summary>
        /// State provider hosted by this replica.
        /// </summary>
        private IStateProvider stateProvider;
        private IAtomicGroupStateProvider atomicGroupStateProvider;
        private IAtomicGroupStateProviderEx atomicGroupStateProviderEx;

        /// <summary>
        /// Fabric Replicator associated with this replica.
        /// </summary>
        private IStateReplicator stateReplicator;
        private IAtomicGroupStateReplicator atomicGroupStateReplicator;
        private IAtomicGroupStateReplicatorEx atomicGroupStateReplicatorEx;

        /// <summary>
        /// Used to forward the replica lifetime and change role calls.
        /// </summary>
        private IStateProviderBroker stateProviderBroker;

        /// <summary>
        /// List of replica states.
        /// </summary>
        private enum ReplicaState
        {
            Opened,
            Closed,
            Aborted,
        }

        /// <summary>
        /// Current replica state.
        /// </summary>
        private ReplicaState replicaState = ReplicaState.Closed;

        /// <summary>
        /// Current replica role.
        /// </summary>
        private ReplicaRole replicaRole = ReplicaRole.Unknown;

        #endregion

        #region Properties

        /// <summary>
        /// Initialization data.
        /// </summary>
        protected StatefulServiceInitializationParameters ServiceInitializationParameters
        {
            get { return this.initializationParameters; }
        }

        /// <summary>
        /// v1 partition.
        /// </summary>
        protected IStatefulServicePartition ServicePartition
        {
            get { return this.servicePartition; }
        }
        
        [SuppressMessage("Microsoft.Naming", "CA1711:IdentifiersShouldNotHaveIncorrectSuffix")]
        /// <summary>
        /// v2 partition.
        /// </summary>
        protected IStatefulServicePartitionEx ServicePartitionEx
        {
            get { return this.servicePartitionEx; }
        }

        /// <summary>
        /// Service group partition.
        /// </summary>
        protected IServiceGroupPartition ServiceGroupPartition
        {
            get { return this.serviceGroupPartition; }
        }

        /// <summary>
        /// v1 replicator.
        /// </summary>
        protected IStateReplicator StateReplicator
        {
            get { return this.stateReplicator; }
        }

        /// <summary>
        /// v1 Service group atomic group replicator.
        /// </summary>
        protected IAtomicGroupStateReplicator AtomicGroupStateReplicator
        {
            get { return this.atomicGroupStateReplicator; }
        }

        [SuppressMessage("Microsoft.Naming", "CA1711:IdentifiersShouldNotHaveIncorrectSuffix")]
        /// <summary>
        /// v2 replicator.
        /// </summary>
        protected IAtomicGroupStateReplicatorEx AtomicGroupStateReplicatorEx
        {
            get { return this.atomicGroupStateReplicatorEx; }
        }

        /// <summary>
        /// Retrieves current replica role.
        /// </summary>
        protected ReplicaRole CurrentReplicaRole
        {
            get
            {
                return this.replicaRole;
            }
        }

        #endregion

        #region IStatefulServiceReplica Members

        /// <summary>
        /// Initializes the replica object.
        /// </summary>
        /// <param name="initializationParameters">Service initialization parameters.</param>
        public virtual void Initialize(StatefulServiceInitializationParameters initializationParameters)
        {
            //
            // Check arguments.
            //
            if (null == initializationParameters)
            {
                AppTrace.TraceSource.WriteError("StatefulServiceReplica.Initialize", "{0}", this.GetHashCode());
                throw new ArgumentNullException("initializationParameters");
            }

            //
            // Perform local initialization.
            //
            this.initializationParameters = initializationParameters;
            AppTrace.TraceSource.WriteNoise("StatefulServiceReplica.Initialize", "{0}", this.ToString());

            //
            // Perform custom initialization.
            //
            this.OnInitialize(initializationParameters);
        }

        /// <summary>
        /// Opens the replica.
        /// </summary>
        /// <param name="openMode">Replica open mode (new or existent).</param>
        /// <param name="partition">Stateful partition object.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public virtual async Task<IReplicator> OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == partition)
            {
                AppTrace.TraceSource.WriteError("StatefulServiceReplica.Open", "{0}", this.ToString());
                throw new ArgumentNullException("partition");
            }

            //
            // Perform local open functionality.
            //
            AppTrace.TraceSource.WriteNoise("StatefulServiceReplica.Open", "{0}", this.ToString());

            //
            // Set partition related members.
            // 
            this.servicePartition = partition;
            this.serviceGroupPartition = partition as IServiceGroupPartition;
            this.servicePartitionEx = partition as IStatefulServicePartitionEx;

            //
            // Create an implementation of the state provider broker.
            //
            this.stateProviderBroker = this.CreateStateProviderBroker();
            if (null == this.stateProviderBroker)
            {
                AppTrace.TraceSource.WriteError("StatefulServiceReplica.Open", "{0} invalid state provider broker", this.ToString());
                throw new InvalidOperationException();
            }

            //
            // Extract state providers.
            //
            this.stateProvider = this.stateProviderBroker as IStateProvider;
            this.atomicGroupStateProvider = this.stateProviderBroker as IAtomicGroupStateProvider;
            this.atomicGroupStateProviderEx = this.stateProviderBroker as IAtomicGroupStateProviderEx;
            if (null == this.stateProvider && null == this.atomicGroupStateProvider && null == this.atomicGroupStateProviderEx)
            {
                AppTrace.TraceSource.WriteError("StatefulServiceReplica.Open", "{0} invalid state providers", this.ToString());
                throw new InvalidOperationException();
            }

            //
            // Create replicator settings (replication and log). 
            // For service groups, these settings are specified in the service manifest.
            //
            ReplicatorSettings replicatorSettings = null;
            if (null != this.serviceGroupPartition)
            {
                replicatorSettings = this.ReplicatorSettings;
            }

            ReplicatorLogSettings replicatorLogSettings = null;
            if (null != this.atomicGroupStateProviderEx && null != this.serviceGroupPartition)
            {
                replicatorLogSettings = this.ReplicatorLogSettings;
            }

            //
            // Create replicator.
            //
            FabricReplicator replicator = null;
            FabricReplicatorEx replicatorEx = null;
            if (null == this.atomicGroupStateProviderEx)
            {
                AppTrace.TraceSource.WriteInfo("StatefulServiceReplica.Open", "{0} creating replicator", this.ToString());
                //
                // v1 replicator.
                //
                replicator = this.servicePartition.CreateReplicator(this.stateProvider, replicatorSettings);
                this.stateReplicator = replicator.StateReplicator;
                this.atomicGroupStateReplicator = this.stateReplicator as IAtomicGroupStateReplicator;
            }
            else
            {
                AppTrace.TraceSource.WriteInfo("StatefulServiceReplica.Open", "{0} creating atomic group replicator", this.ToString());
                //
                // v2 replicator.
                //
                replicatorEx = this.servicePartitionEx.CreateReplicatorEx(this.atomicGroupStateProviderEx, replicatorSettings, replicatorLogSettings);
                this.atomicGroupStateReplicatorEx = replicatorEx.StateReplicator;
            }

            //
            // Perform local open functionality. Initialize and open state provider broker.
            //
            this.stateProviderBroker.Initialize(this.initializationParameters);
            await this.stateProviderBroker.OpenAsync(
                openMode,
                this.servicePartitionEx,
                this.atomicGroupStateReplicatorEx,
                cancellationToken);

            //
            // Perform custom open functionality.
            //
            await this.OnOpenAsync(openMode, cancellationToken);

            //
            // Change current replica state.
            //
            this.replicaState = ReplicaState.Opened;
            AppTrace.TraceSource.WriteNoise("StatefulServiceReplica.State", "{0} is {1}", this.ToString(), this.replicaState);

            //
            // Done.
            //
            return (null != replicator) ? replicator as IReplicator : replicatorEx as IReplicator;
        }

        /// <summary>
        /// Called on the replica when the role is changing.
        /// </summary>
        /// <param name="newRole">New role for the replica.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public virtual async Task<string> ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            ReplicaRole currentRole = this.replicaRole;
            AppTrace.TraceSource.WriteNoise("StatefulServiceReplica.ChangeRole", "{0} starting role change from {1} to {2}", this.ToString(), currentRole, newRole);

            //
            // Perform local change role functionality.Change role in the state provider broker.
            //
            await this.stateProviderBroker.ChangeRoleAsync(newRole, cancellationToken);
            
            //
            // Perform custom change role functionality.
            //
            await this.OnChangeRoleAsync(newRole, cancellationToken);

            //
            // Retrieve endpoint.
            //
            string endpoint = await this.GetEndpointAsync(newRole, cancellationToken);

            AppTrace.TraceSource.WriteNoise("StatefulServiceReplica.ChangeRole", "{0} completed role change from {1} to {2}", this.ToString(), currentRole, newRole);
            this.replicaRole = newRole;

            return endpoint;
        }

        /// <summary>
        /// Called on the replica when it is being closed.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public virtual async Task CloseAsync(CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise("StatefulServiceReplica.Close", "{0}", this.ToString());

            //
            // Perform local close functionality.Close state provider broker.
            //
            await this.stateProviderBroker.CloseAsync(cancellationToken);

            //
            // Perform custom close functionality.
            //
            await this.OnCloseAsync(cancellationToken);

            //
            // Change current replica state.
            //
            this.replicaState = ReplicaState.Closed;
            AppTrace.TraceSource.WriteNoise("StatefulServiceReplica.State", "{0} is {1}", this.ToString(), this.replicaState);
        }

        /// <summary>
        /// Called on the replica when it is being aborted.
        /// </summary>
        public virtual void Abort()
        {
            AppTrace.TraceSource.WriteNoise("StatefulServiceReplica.Abort", "{0}", this.ToString());

            //
            // Perform local abort functionality. Abort state provider broker.
            //
            this.stateProviderBroker.Abort();

            //
            // Perform custom abort functionality.
            //
            this.OnAbort();

            //
            // Change current replica state.
            //
            this.replicaState = ReplicaState.Aborted;
            AppTrace.TraceSource.WriteNoise("StatefulServiceReplica.State", "{0} is {1}", this.ToString(), this.replicaState);
        }

        #endregion

        #region IReplicatedService Members

        /// <summary>
        /// Return ReplicatorSettings in this method.
        /// It is guaranteed that when this is called InitializationParameters and the Partition are available.
        /// </summary>
        /// <returns></returns>
        protected override ReplicatorSettings ReplicatorSettings 
        {
            get
            {
                var endpoint = this.initializationParameters.CodePackageActivationContext.GetEndpoints()[StatefulServiceReplica.ReplicatorEndpointTag];
                return new ReplicatorSettings
                {
                    ReplicatorAddress = string.Format(CultureInfo.InvariantCulture, "{0}:{1}", StatefulServiceReplica.GetIPAddressOrHostName(true), endpoint.Port)
                };
            }
        }

        /// <summary>
        /// Return ReplicatorLogSettings in this method.
        /// It is guaranteed that when this is called InitializationParameters and the Partition are available.
        /// </summary>
        /// <returns></returns>
        protected override ReplicatorLogSettings ReplicatorLogSettings 
        {
            get
            {
                return null;
            }
        }

        /// <summary>
        /// Return the endpoint for the fabric client to resolve. Depeding on the replica role
        /// the endpoint may or may not be empty.
        /// </summary>
        /// <param name="role">Replica role used to determine if an endpoint should be opened.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        protected override Task<string> GetEndpointAsync(ReplicaRole role, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise("StatefulServiceReplica.GetEndpoint", "{0} role {1}", this.ToString(), role);
            //
            // No endpoint returned.
            //
            var tcs = new TaskCompletionSource<string>();
            tcs.SetResult(string.Empty);
            return tcs.Task;
        }

        /// <summary>
        /// Called during replica initialization.
        /// </summary>
        /// <param name="initializationParameters">Service intialization parameters.</param>
        protected override void OnInitialize(StatefulServiceInitializationParameters initializationParameters)
        {
            AppTrace.TraceSource.WriteNoise("StatefulServiceReplica.OnInitialize", "{0}", this.ToString());
        }

        /// <summary>
        /// Called during OpenAsync.
        /// The derived class should perform its initialization in this method.
        /// <param name="openMode">Replica open mode (new or existent).</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        protected override Task OnOpenAsync(ReplicaOpenMode openMode, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise("StatefulServiceReplica.OnOpen", "{0}", this.ToString());

            //
            // Do nothing.
            //
            var tcs = new TaskCompletionSource<bool>();
            tcs.SetResult(true);
            return tcs.Task;
        }

        /// <summary>
        /// Called during ChangeRoleAsync.
        /// </summary>
        /// <param name="newRole">New replica role.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        protected override Task OnChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise("StatefulServiceReplica.OnChangeRole", "{0}", this.ToString());

            //
            // Do nothing.
            //
            var tcs = new TaskCompletionSource<bool>();
            tcs.SetResult(true);
            return tcs.Task;
        }

        /// <summary>
        /// Called during CloseAsync.
        /// The derived class should gracefully cleanup its resources.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        protected override Task OnCloseAsync(CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise("StatefulServiceReplica.OnClose", "{0}", this.ToString());

            //
            // Do nothing.
            //
            var tcs = new TaskCompletionSource<bool>();
            tcs.SetResult(true);
            return tcs.Task;
        }

        /// <summary>
        /// Called during Abort.
        /// The derived class should attempt to immediately cleanup its resources.
        /// </summary>
        protected override void OnAbort()
        {
            AppTrace.TraceSource.WriteNoise("StatefulServiceReplica.OnAbort", "{0}", this.ToString());
        }

        #endregion

        #region Object Overrides

        /// <summary>
        /// Returns a string that represents the current object.
        /// </summary>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Name: {0}. Uri: {1}. PartitionId: {2}. ReplicaId: {3}", 
                this.initializationParameters.ServiceTypeName,
                this.initializationParameters.ServiceName,
                this.initializationParameters.PartitionId,
                this.initializationParameters.ReplicaId);
        }

        #endregion

        #region Helper Routines

        /// <summary>
        /// Returns the IP address or host name of this machine.
        /// </summary>
        /// <returns></returns>
        public static string GetIPAddressOrHostName()
        {
            return StatefulServiceReplica.GetIPAddressOrHostName(false);
        }

        /// <summary>
        /// Returns the IP address or host name of this machine. IPv4 is preferred.
        /// </summary>
        /// <param name="isFabricReplicatorAddress">True, if the call is made for composing the replicator address.</param>
        /// <returns></returns>
        private static string GetIPAddressOrHostName(bool isFabricReplicatorAddress)
        {
            IPHostEntry entry = Dns.GetHostEntry(Dns.GetHostName());
            IPAddress ipv4Address = entry.AddressList.FirstOrDefault<IPAddress>(
                ipAddress => (System.Net.Sockets.AddressFamily.InterNetwork == ipAddress.AddressFamily) && !IPAddress.IsLoopback(ipAddress));

            if (ipv4Address != null)
            {
                return ipv4Address.ToString();
            }

            IPAddress ipv6Address = entry.AddressList.FirstOrDefault<IPAddress>(
                ipAddress => (System.Net.Sockets.AddressFamily.InterNetworkV6 == ipAddress.AddressFamily) && !IPAddress.IsLoopback(ipAddress));

            if (ipv6Address != null)
            {
                if (isFabricReplicatorAddress)
                {
                    return string.Format(CultureInfo.InvariantCulture, "[{0}]", ipv6Address);
                }
                else
                {
                    return ipv6Address.ToString();
                }
            }
            else
            {
                return entry.HostName;
            }
        }
        
        #endregion
    }
}