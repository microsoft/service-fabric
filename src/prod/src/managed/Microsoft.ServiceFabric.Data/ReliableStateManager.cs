// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Data.Notifications;

    /// <summary>
    /// <para>
    /// The ReliableStateManager class is responsible for managing <see cref="IReliableState"/> for a service replica.
    /// Each replica in a service has its own <see cref="IReliableState"/> and <see cref="ReliableStateManager"/>. 
    /// <see cref="IReliableState"/> can include <see cref="Microsoft.ServiceFabric.Data.Collections.IReliableDictionary{TKey, TValue}"/>,
    /// <see cref="Microsoft.ServiceFabric.Data.Collections.IReliableQueue{T}"/>,
    /// or any <see cref="Microsoft.ServiceFabric.Data.Collections.IReliableCollection{T}"/> types.
    /// </para>
    /// </summary>
    public class ReliableStateManager : IReliableStateManagerReplica2
    {
        private const string TraceType = "ReliableStateManager";

        /// <summary>
        /// Reliable State Manager Replica implementation.
        /// </summary>
        private readonly IReliableStateManagerReplica2 _impl;

        private bool useNativeReliableStateManager;

        private const string SectionName = "NativeRunConfiguration";
        private const string EnableNativeReliableStateManager = "EnableNativeReliableStateManager";
        private const string Test_LoadEnableNativeReliableStateManager = "Test_LoadEnableNativeReliableStateManager";

        #region TransactionChanged variables
        /// <summary>
        /// User's transaction changed event handler.
        /// </summary>
        private EventHandler<NotifyTransactionChangedEventArgs> userTransactionChangedEventHandler;

        /// <summary>
        /// Lock for the event handler registration.
        /// </summary>
        private readonly object eventHandlerRegistrationLock = new object();

        /// <summary>
        /// Indicates whether event handler is registered with the implementation.
        /// </summary>
        private bool isTransactionEventHandlerRegistered = false;
        #endregion

        #region StateManagerChanged variables
        /// <summary>
        /// User's state manager changed event handler.
        /// </summary>
        private EventHandler<NotifyStateManagerChangedEventArgs> userStateManagerChangedEventHandler;

        /// <summary>
        /// Indicates whether event handler is registered with implementation.
        /// </summary>
        private bool isStateManagerEventHandlerRegistered = false;

        /// <summary>
        /// Indicates whether the component is closing or aborting.
        /// </summary>
        private bool isClosingOrAborting = false;

        #endregion

        static ReliableStateManager()
        {
            AppDomain.CurrentDomain.AssemblyResolve += DataAssemblyResolver.OnAssemblyResolve;
        }

        internal IStateProviderReplica2 Replica
        {
            get
            {
                return this._impl;
            }
        }
		
        /// <summary>
        /// Create a new ReliableStateManager.
        /// </summary>
        /// <param name="serviceContext">A <see cref="StatefulServiceContext"/> that describes the service context.</param>
        /// <param name="configuration">Configuration parameters.</param>
        public ReliableStateManager(StatefulServiceContext serviceContext, ReliableStateManagerConfiguration configuration = null)
        {
            try
            {
                configuration = configuration ?? new ReliableStateManagerConfiguration();
                this.useNativeReliableStateManager = GetNativeRunConfigurationSetting(serviceContext, configuration);
            }
            catch (Exception ex)
            {
                DataTrace.Source.WriteErrorWithId(TraceType, serviceContext.PartitionId.ToString("B"), "{0}: GetNativeRunConfigurationSetting failed {1}", serviceContext.ReplicaId, ex);
                throw;
            }

            if (this.useNativeReliableStateManager)
            {
                DataTrace.Source.WriteInfoWithId(TraceType, serviceContext.PartitionId.ToString("B"), "{0}: Using native reliable statemanager", serviceContext.ReplicaId);
                this._impl = new Microsoft.ServiceFabric.Data.Extensions.ReliableStateManager(serviceContext, configuration);
            }

#if DotNetCoreClr
            else
            {
                var fabricCodePath = FabricEnvironment.GetCodePath();
                if (fabricCodePath != null)
                {
                    System.Reflection.Assembly dataImpl =
                        System.Reflection.Assembly.LoadFrom(
                            System.IO.Path.Combine(fabricCodePath, "NS", "Microsoft.ServiceFabric.Data.Impl.dll"));
                    this._impl =
                        (IReliableStateManagerReplica2)dataImpl.GetType("Microsoft.ServiceFabric.Data.EntryPoints")
                        .GetMethod("CreateReliableStateManager2")
                        .Invoke(
                            null,
                            new object[] { serviceContext, configuration });
			    }
            }
#else
            else
            {
                this._impl = EntryPoints.CreateReliableStateManager2(serviceContext, configuration);
            }
#endif
        }

        /// <summary>
        /// Occurs when a transaction changes.
        /// </summary>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        public event EventHandler<NotifyTransactionChangedEventArgs> TransactionChanged
        {
            add
            {
                // MCoskun: Order of the following operations is significant.
                // user event handler is set before the event handler is registered to ensure if a notification comes it can be plumbed.
                this.userTransactionChangedEventHandler += value;

                this.RegisterTransactionEventHandlerIfNecessary();

                ReleaseAssert.AssertIfNot(this.isTransactionEventHandlerRegistered, "transaction event handler must have been registered");
            }
            remove
            {
                this.userTransactionChangedEventHandler -= value;
            }
        }

        /// <summary>
        /// Occurs when the state manager changes.
        /// </summary>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        public event EventHandler<NotifyStateManagerChangedEventArgs> StateManagerChanged
        {
            add
            {
                // MCoskun: Order of the following operations is significant.
                // user event handler is set before the event handler is registered to ensure if a notification comes it can be plumbed.
                this.userStateManagerChangedEventHandler += value;

                this.RegisterStateManagerEventHandlerIfNecessary();


                ReleaseAssert.AssertIfNot(this.isStateManagerEventHandlerRegistered, "state manager event handler must have been registered");
            }
            remove
            {
                this.userStateManagerChangedEventHandler -= value;
            }
        }

#if !DotNetCoreClr
        internal long GetCurrentDataLossNumber() { return ((ReliableStateManagerImpl) this._impl).GetCurrentDataLossNumber(); }

        internal long GetLastCommittedSequenceNumber() { return ((ReliableStateManagerImpl)this._impl).GetLastCommittedSequenceNumber(); }
#endif

        /// <summary>
        /// Returns an enumerator that iterates through the collection.
        /// </summary>
        /// <returns>
        /// An <see cref="Microsoft.ServiceFabric.Data.IAsyncEnumerator{IReliableState}"/> object that can be used to iterate through the collection.
        /// </returns>
        /// <filterpriority>1</filterpriority>
        public IAsyncEnumerator<IReliableState> GetAsyncEnumerator() { return this._impl.GetAsyncEnumerator(); }

        /// <summary>
        /// Performs a full backup of all reliable state managed by this <see cref="IReliableStateManager"/>.
        /// </summary>
        /// <param name="backupCallback">Callback to be called when the backup folder has been created locally and is ready to be moved out of the node.</param>
        /// <returns>Task that represents the asynchronous backup operation.</returns>
        /// <remarks>
        /// A FULL backup will be performed with a one-hour timeout.
        /// Boolean returned by the backupCallback indicate whether the service was able to successfully move the backup folder to an external location.
        /// If false is returned, BackupAsync throws InvalidOperationException with the relevant message indicating backupCallback returned false.
        /// Also, backup will be marked as unsuccessful.
        /// </remarks>
        public Task BackupAsync(Func<BackupInfo, CancellationToken, Task<bool>> backupCallback) { return this._impl.BackupAsync(backupCallback); }

        /// <summary>
        /// Attempts to get an <see cref="IReliableState"/> of the given type <typeparamref name="T"/> and with the given name.
        /// </summary>
        /// <typeparam name="T">
        /// When specifying the type, you may ask for either a concrete type or an interface type. The retrieved object will
        /// be cast to the given type.
        /// </typeparam>
        /// <param name="name">
        /// The name of the <see cref="IReliableState"/>. This name must be unique in this <see cref="IReliableStateManager"/>
        /// across <see cref="IReliableState"/> types, including unrelated types.
        /// </param>
        /// <exception cref="ArgumentNullException"><paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">The <see cref="IReliableState"/> instance is not convertible to type <typeparamref name="T"/>.</exception>
        /// <exception cref="System.Fabric.FabricNotReadableException">
        /// Exception indicates that the State Manager cannot retrive a reliable collection.
        /// <see cref="FabricNotReadableException"/> can be thrown in all <see cref="ReplicaRole"/>s.
        /// For example, when a <see cref="ReplicaRole.Primary"/> or <see cref="ReplicaRole.ActiveSecondary"/> looses <see cref="IStatefulServicePartition.ReadStatus"/>.</exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is a tuple indicating whether the reliable state was found, and if so the instance.</returns>
        Task<ConditionalValue<T>> IReliableStateManager.TryGetAsync<T>(string name) { return this._impl.TryGetAsync<T>(name); }

        /// <summary>
        /// Attempts to get an <see cref="IReliableState"/> of the given type <typeparamref name="T"/> and with the given name.
        /// </summary>
        /// <typeparam name="T">
        /// When specifying the type, you may ask for either a concrete type or an interface type. The retrieved object will
        /// be cast to the given type.
        /// </typeparam>
        /// <param name="name">
        /// The name of the <see cref="IReliableState"/>. This name must be unique in this <see cref="IReliableStateManager"/>
        /// across <see cref="IReliableState"/> types, including unrelated types.
        /// </param>
        /// <exception cref="ArgumentNullException"><paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">The <see cref="IReliableState"/> instance is not convertible to type <typeparamref name="T"/>.</exception>
        /// <exception cref="System.Fabric.FabricNotReadableException">
        /// Exception indicates that the State Manager cannot retrive a reliable collection.
        /// <see cref="FabricNotReadableException"/> can be thrown in all <see cref="ReplicaRole"/>s.
        /// For example, when a <see cref="ReplicaRole.Primary"/> or <see cref="ReplicaRole.ActiveSecondary"/> looses <see cref="IStatefulServicePartition.ReadStatus"/>.</exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is a tuple indicating whether the reliable state was found, and if so the instance.</returns>
        Task<ConditionalValue<T>> IReliableStateManager.TryGetAsync<T>(Uri name) { return this._impl.TryGetAsync<T>(name); }

        /// <summary>
        /// Removes the <see cref="IReliableState"/> with the given name from this state manager. The state is
        /// permanently removed from persistent storage and all replicas.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. The <see cref="IReliableState"/> will be successfully removed along with all state
        /// or be left in-tact.
        /// </remarks>
        /// <param name="name">The name of the <see cref="IReliableState"/> to remove.</param>
        /// <exception cref="ArgumentNullException"><paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An <see cref="IReliableState"/> with the given name does not exist.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task IReliableStateManager.RemoveAsync(string name) { return this._impl.RemoveAsync(name); }

        /// <summary>
        /// Removes the <see cref="IReliableState"/> with the given name from this state manager. The state is
        /// permanently removed from persistent storage and all replicas.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. The <see cref="IReliableState"/> will be successfully removed along with all state
        /// or be left in-tact.
        /// </remarks>
        /// <param name="name">The name of the <see cref="IReliableState"/> to remove.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <exception cref="ArgumentNullException"><paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An <see cref="IReliableState"/> with the given name does not exist, or <paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task IReliableStateManager.RemoveAsync(string name, TimeSpan timeout) { return this._impl.RemoveAsync(name, timeout); }

        /// <summary>
        /// Removes the <see cref="IReliableState"/> with the given name from this state manager. The state is
        /// permanently removed from persistent storage and all replicas when the transaction is committed.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. The <see cref="IReliableState"/> will be successfully removed along with all state
        /// or be left in-tact. If this method throws an exception, the transaction must be aborted.
        /// </remarks>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="name">The name of the <see cref="IReliableState"/> to remove.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An <see cref="IReliableState"/> with the given name does not exist.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// For example, transaction used is already terminated: committed or aborted.
        /// </exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task IReliableStateManager.RemoveAsync(ITransaction tx, string name) { return this._impl.RemoveAsync(tx, name); }

        /// <summary>
        /// Removes the <see cref="IReliableState"/> with the given name from this state manager. The state is
        /// permanently removed from persistent storage and all replicas when the transaction is committed.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. The <see cref="IReliableState"/> will be successfully removed along with all state
        /// or be left in-tact. If this method throws an exception, the transaction must be aborted.
        /// </remarks>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="name">The name of the <see cref="IReliableState"/> to remove.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An <see cref="IReliableState"/> with the given name does not exist, or <paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// For example, transaction used is already terminated: committed or aborted.
        /// </exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task IReliableStateManager.RemoveAsync(ITransaction tx, string name, TimeSpan timeout) { return this._impl.RemoveAsync(tx, name, timeout); }

        /// <summary>
        /// Removes the <see cref="IReliableState"/> with the given name from this state manager. The state is
        /// permanently removed from persistent storage and all replicas.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. The <see cref="IReliableState"/> will be successfully removed along with all state
        /// or be left in-tact.
        /// </remarks>
        /// <param name="name">The name of the <see cref="IReliableState"/> to remove.</param>
        /// <exception cref="ArgumentNullException"><paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An <see cref="IReliableState"/> with the given name does not exist.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task IReliableStateManager.RemoveAsync(Uri name) { return this._impl.RemoveAsync(name); }

        /// <summary>
        /// Removes the <see cref="IReliableState"/> with the given name from this state manager. The state is
        /// permanently removed from persistent storage and all replicas.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. The <see cref="IReliableState"/> will be successfully removed along with all state
        /// or be left in-tact.
        /// </remarks>
        /// <param name="name">The name of the <see cref="IReliableState"/> to remove.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <exception cref="ArgumentNullException"><paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An <see cref="IReliableState"/> with the given name does not exist, or <paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task IReliableStateManager.RemoveAsync(Uri name, TimeSpan timeout) { return this._impl.RemoveAsync(name, timeout); }

        /// <summary>
        /// Removes the <see cref="IReliableState"/> with the given name from this state manager. The state is
        /// permanently removed from persistent storage and all replicas when the transaction is committed.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. The <see cref="IReliableState"/> will be successfully removed along with all state
        /// or be left in-tact. If this method throws an exception, the transaction must be aborted.
        /// </remarks>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="name">The name of the <see cref="IReliableState"/> to remove.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An <see cref="IReliableState"/> with the given name does not exist.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// For example, transaction used is already terminated: committed or aborted.
        /// </exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task IReliableStateManager.RemoveAsync(ITransaction tx, Uri name) { return this._impl.RemoveAsync(tx, name); }

        /// <summary>
        /// Removes the <see cref="IReliableState"/> with the given name from this state manager. The state is
        /// permanently removed from persistent storage and all replicas when the transaction is committed.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. The <see cref="IReliableState"/> will be successfully removed along with all state
        /// or be left in-tact. If this method throws an exception, the transaction must be aborted.
        /// </remarks>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="name">The name of the <see cref="IReliableState"/> to remove.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An <see cref="IReliableState"/> with the given name does not exist, or <paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// For example, transaction used is already terminated: committed or aborted.
        /// </exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task IReliableStateManager.RemoveAsync(ITransaction tx, Uri name, TimeSpan timeout) { return this._impl.RemoveAsync(tx, name, timeout); }

        /// <summary>
        /// Gets an <see cref="IReliableState"/> of the given type <typeparamref name="T"/> and with the given name if it exists, or creates one
        /// and returns it if it doesn't already exist.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. When an <see cref="IReliableState"/> needs to be created, it will either complete and return successfully
        /// or it will not be created.
        /// </remarks>
        /// <typeparam name="T">
        /// When specifying the <see cref="IReliableState"/> type, you may ask for either a class type or an interface type.
        /// <para>
        /// If specifying a class type, the system will attempt to return an instance of that type. 
        /// If an instance of that type cannot be instantiated (e.g., abstract class, no suitable constructor), an ArgumentException is thrown.
        /// </para>
        /// <para>
        /// If specifying an interface type, the manager will attempt to resolve the interface to a concrete type.
        /// If type mapping is specified by the user, this method will use the user-specified mapping to resolve the type (not yet supported).
        /// If type mapping is not specified by the user this method will select the default implementation for the interface given.
        /// If the given interface type does not have a default implementation, or a user-specified mapping for the type is not provided or the type
        /// is invalid, this method will throw ArgumentException.
        /// </para>
        /// </typeparam>
        /// <param name="name">
        /// The name of the <see cref="IReliableState"/>. This name must be unique in this <see cref="IReliableStateManager"/>
        /// across <see cref="IReliableState"/> types, including unrelated types.
        /// </param>
        /// <exception cref="ArgumentNullException"><paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An instance of the type <typeparamref name="T"/> cannot be created, or the existing <see cref="IReliableState"/> instance is not of type <typeparamref name="T"/>.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> IReliableStateManager.GetOrAddAsync<T>(string name) { return this._impl.GetOrAddAsync<T>(name); }

        /// <summary>
        /// Gets an <see cref="IReliableState"/> of the given type <typeparamref name="T"/> and with the given name if it exists, or creates one
        /// and returns it if it doesn't already exist.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. When an <see cref="IReliableState"/> needs to be created, it will either complete and return successfully
        /// or it will not be created.
        /// </remarks>
        /// <typeparam name="T">
        /// When specifying the <see cref="IReliableState"/> type, you may ask for either a class type or an interface type.
        /// <para>
        /// If specifying a class type, the system will attempt to return an instance of that type. 
        /// If an instance of that type cannot be instantiated (e.g., abstract class, no suitable constructor), an ArgumentException is thrown.
        /// </para>
        /// <para>
        /// If specifying an interface type, the manager will attempt to resolve the interface to a concrete type.
        /// If type mapping is specified by the user, this method will use the user-specified mapping to resolve the type (not yet supported).
        /// If type mapping is not specified by the user this method will select the default implementation for the interface given.
        /// If the given interface type does not have a default implementation, or a user-specified mapping for the type is not provided or the type
        /// is invalid, this method will throw ArgumentException.
        /// </para>
        /// </typeparam>
        /// <param name="name">
        /// The name of the <see cref="IReliableState"/>. This name must be unique in this <see cref="IReliableStateManager"/>
        /// across <see cref="IReliableState"/> types, including unrelated types.
        /// </param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <exception cref="ArgumentNullException"><paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An instance of the type <typeparamref name="T"/> cannot be created, or the existing <see cref="IReliableState"/> instance is not of type <typeparamref name="T"/>, or <paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> IReliableStateManager.GetOrAddAsync<T>(string name, TimeSpan timeout) { return this._impl.GetOrAddAsync<T>(name, timeout); }

        /// <summary>
        /// Gets an <see cref="IReliableState"/> of the given type <typeparamref name="T"/> and with the given name if it exists, or creates one
        /// and returns it if it doesn't already exist.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. When an <see cref="IReliableState"/> needs to be created, it will either complete and return successfully
        /// or it will not be created. If this method throws an exception, the transaction must be aborted.
        /// </remarks>
        /// <typeparam name="T">
        /// When specifying the <see cref="IReliableState"/> type, you may ask for either a class type or an interface type.
        /// <para>
        /// If specifying a class type, the system will attempt to return an instance of that type. 
        /// If an instance of that type cannot be instantiated (e.g., abstract class, no suitable constructor), an ArgumentException is thrown.
        /// </para>
        /// <para>
        /// If specifying an interface type, the manager will attempt to resolve the interface to a concrete type.
        /// If type mapping is specified by the user, this method will use the user-specified mapping to resolve the type (not yet supported).
        /// If type mapping is not specified by the user this method will select the default implementation for the interface given.
        /// If the given interface type does not have a default implementation, or a user-specified mapping for the type is not provided or the type
        /// is invalid, this method will throw ArgumentException.
        /// </para>
        /// </typeparam>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="name">
        /// The name of the <see cref="IReliableState"/>. This name must be unique in this <see cref="IReliableStateManager"/>
        /// across <see cref="IReliableState"/> types, including unrelated types.
        /// </param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An instance of the type <typeparamref name="T"/> cannot be created, or the existing <see cref="IReliableState"/> instance is not of type <typeparamref name="T"/>.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// For example, transaction used is already terminated: committed or aborted.
        /// </exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> IReliableStateManager.GetOrAddAsync<T>(ITransaction tx, string name) { return this._impl.GetOrAddAsync<T>(tx, name); }

        /// <summary>
        /// Gets an <see cref="IReliableState"/> of the given type <typeparamref name="T"/> and with the given name if it exists, or creates one
        /// and returns it if it doesn't already exist.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. When an <see cref="IReliableState"/> needs to be created, it will either complete and return successfully
        /// or it will not be created. If this method throws an exception, the transaction must be aborted.
        /// </remarks>
        /// <typeparam name="T">
        /// When specifying the <see cref="IReliableState"/> type, you may ask for either a class type or an interface type.
        /// <para>
        /// If specifying a class type, the system will attempt to return an instance of that type. 
        /// If an instance of that type cannot be instantiated (e.g., abstract class, no suitable constructor), an ArgumentException is thrown.
        /// </para>
        /// <para>
        /// If specifying an interface type, the manager will attempt to resolve the interface to a concrete type.
        /// If type mapping is specified by the user, this method will use the user-specified mapping to resolve the type (not yet supported).
        /// If type mapping is not specified by the user this method will select the default implementation for the interface given.
        /// If the given interface type does not have a default implementation, or a user-specified mapping for the type is not provided or the type
        /// is invalid, this method will throw ArgumentException.
        /// </para>
        /// </typeparam>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="name">
        /// The name of the <see cref="IReliableState"/>. This name must be unique in this <see cref="IReliableStateManager"/>
        /// across <see cref="IReliableState"/> types, including unrelated types.
        /// </param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An instance of the type <typeparamref name="T"/> cannot be created, or the existing <see cref="IReliableState"/> instance is not of type <typeparamref name="T"/>, or <paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// For example, transaction used is already terminated: committed or aborted.
        /// </exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> IReliableStateManager.GetOrAddAsync<T>(ITransaction tx, string name, TimeSpan timeout) { return this._impl.GetOrAddAsync<T>(tx, name, timeout); }

        /// <summary>
        /// Gets an <see cref="IReliableState"/> of the given type <typeparamref name="T"/> and with the given name if it exists, or creates one
        /// and returns it if it doesn't already exist.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. When an <see cref="IReliableState"/> needs to be created, it will either complete and return successfully
        /// or it will not be created.
        /// </remarks>
        /// <typeparam name="T">
        /// When specifying the <see cref="IReliableState"/> type, you may ask for either a class type or an interface type.
        /// <para>
        /// If specifying a class type, the system will attempt to return an instance of that type. 
        /// If an instance of that type cannot be instantiated (e.g., abstract class, no suitable constructor), an ArgumentException is thrown.
        /// </para>
        /// <para>
        /// If specifying an interface type, the manager will attempt to resolve the interface to a concrete type.
        /// If type mapping is specified by the user, this method will use the user-specified mapping to resolve the type (not yet supported).
        /// If type mapping is not specified by the user this method will select the default implementation for the interface given.
        /// If the given interface type does not have a default implementation, or a user-specified mapping for the type is not provided or the type
        /// is invalid, this method will throw ArgumentException.
        /// </para>
        /// </typeparam>
        /// <param name="name">
        /// The name of the <see cref="IReliableState"/>. This name must be unique in this <see cref="IReliableStateManager"/>
        /// across <see cref="IReliableState"/> types, including unrelated types.
        /// </param>
        /// <exception cref="ArgumentNullException"><paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An instance of the type <typeparamref name="T"/> cannot be created, or the existing <see cref="IReliableState"/> instance is not of type <typeparamref name="T"/>.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> IReliableStateManager.GetOrAddAsync<T>(Uri name) { return this._impl.GetOrAddAsync<T>(name); }

        /// <summary>
        /// Gets an <see cref="IReliableState"/> of the given type <typeparamref name="T"/> and with the given name if it exists, or creates one
        /// and returns it if it doesn't already exist.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. When an <see cref="IReliableState"/> needs to be created, it will either complete and return successfully
        /// or it will not be created.
        /// </remarks>
        /// <typeparam name="T">
        /// When specifying the <see cref="IReliableState"/> type, you may ask for either a class type or an interface type.
        /// <para>
        /// If specifying a class type, the system will attempt to return an instance of that type. 
        /// If an instance of that type cannot be instantiated (e.g., abstract class, no suitable constructor), an ArgumentException is thrown.
        /// </para>
        /// <para>
        /// If specifying an interface type, the manager will attempt to resolve the interface to a concrete type.
        /// If type mapping is specified by the user, this method will use the user-specified mapping to resolve the type (not yet supported).
        /// If type mapping is not specified by the user this method will select the default implementation for the interface given.
        /// If the given interface type does not have a default implementation, or a user-specified mapping for the type is not provided or the type
        /// is invalid, this method will throw ArgumentException.
        /// </para>
        /// </typeparam>
        /// <param name="name">
        /// The name of the <see cref="IReliableState"/>. This name must be unique in this <see cref="IReliableStateManager"/>
        /// across <see cref="IReliableState"/> types, including unrelated types.
        /// </param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <exception cref="ArgumentNullException"><paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An instance of the type <typeparamref name="T"/> cannot be created, or the existing <see cref="IReliableState"/> instance is not of type <typeparamref name="T"/>, or <paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> IReliableStateManager.GetOrAddAsync<T>(Uri name, TimeSpan timeout) { return this._impl.GetOrAddAsync<T>(name, timeout); }

        /// <summary>
        /// Gets an <see cref="IReliableState"/> of the given type <typeparamref name="T"/> and with the given name if it exists, or creates one
        /// and returns it if it doesn't already exist.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. When an <see cref="IReliableState"/> needs to be created, it will either complete and return successfully
        /// or it will not be created. If this method throws an exception, the transaction must be aborted.
        /// </remarks>
        /// <typeparam name="T">
        /// When specifying the <see cref="IReliableState"/> type, you may ask for either a class type or an interface type.
        /// <para>
        /// If specifying a class type, the system will attempt to return an instance of that type. 
        /// If an instance of that type cannot be instantiated (e.g., abstract class, no suitable constructor), an ArgumentException is thrown.
        /// </para>
        /// <para>
        /// If specifying an interface type, the manager will attempt to resolve the interface to a concrete type.
        /// If type mapping is specified by the user, this method will use the user-specified mapping to resolve the type (not yet supported).
        /// If type mapping is not specified by the user this method will select the default implementation for the interface given.
        /// If the given interface type does not have a default implementation, or a user-specified mapping for the type is not provided or the type
        /// is invalid, this method will throw ArgumentException.
        /// </para>
        /// </typeparam>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="name">
        /// The name of the <see cref="IReliableState"/>. This name must be unique in this <see cref="IReliableStateManager"/>
        /// across <see cref="IReliableState"/> types, including unrelated types.
        /// </param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An instance of the type <typeparamref name="T"/> cannot be created, or the existing <see cref="IReliableState"/> instance is not of type <typeparamref name="T"/>.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// For example, transaction used is already terminated: committed or aborted.
        /// </exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> IReliableStateManager.GetOrAddAsync<T>(ITransaction tx, Uri name) { return this._impl.GetOrAddAsync<T>(tx, name); }

        /// <summary>
        /// Gets an <see cref="IReliableState"/> of the given type <typeparamref name="T"/> and with the given name if it exists, or creates one
        /// and returns it if it doesn't already exist.
        /// </summary>
        /// <remarks>
        /// This is an atomic operation. When an <see cref="IReliableState"/> needs to be created, it will either complete and return successfully
        /// or it will not be created. If this method throws an exception, the transaction must be aborted.
        /// </remarks>
        /// <typeparam name="T">
        /// When specifying the <see cref="IReliableState"/> type, you may ask for either a class type or an interface type.
        /// <para>
        /// If specifying a class type, the system will attempt to return an instance of that type. 
        /// If an instance of that type cannot be instantiated (e.g., abstract class, no suitable constructor), an ArgumentException is thrown.
        /// </para>
        /// <para>
        /// If specifying an interface type, the manager will attempt to resolve the interface to a concrete type.
        /// If type mapping is specified by the user, this method will use the user-specified mapping to resolve the type (not yet supported).
        /// If type mapping is not specified by the user this method will select the default implementation for the interface given.
        /// If the given interface type does not have a default implementation, or a user-specified mapping for the type is not provided or the type
        /// is invalid, this method will throw ArgumentException.
        /// </para>
        /// </typeparam>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="name">
        /// The name of the <see cref="IReliableState"/>. This name must be unique in this <see cref="IReliableStateManager"/>
        /// across <see cref="IReliableState"/> types, including unrelated types.
        /// </param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="name"/> is null.</exception>
        /// <exception cref="ArgumentException">An instance of the type <typeparamref name="T"/> cannot be created, or the existing <see cref="IReliableState"/> instance is not of type <typeparamref name="T"/>, or <paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// For example, transaction used is already terminated: committed or aborted.
        /// </exception>
        /// <exception cref="FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> IReliableStateManager.GetOrAddAsync<T>(ITransaction tx, Uri name, TimeSpan timeout) { return this._impl.GetOrAddAsync<T>(tx, name, timeout); }

        /// <summary>
        /// Create and start a new transaction that can be used to group operations to be performed atomically.
        /// </summary>
        /// <remarks>
        /// Operations are added to the transaction by passing the <see cref="ITransaction"/> object in to reliable state methods.
        /// </remarks>
        /// <returns>A new transaction.</returns>
        ITransaction IReliableStateManager.CreateTransaction() { return this._impl.CreateTransaction(); }

        /// <summary>
        /// Adds a state serializer.
        /// Adds it for all reliable collection instances.
        /// </summary>
        /// <typeparam name="T">Type that will be serialized and de-serialized.</typeparam>
        /// <param name="stateSerializer">
        /// The state serializer to be added.
        /// </param>
        /// <returns>
        /// True if the serializer was added.
        /// False if a serailizer is already registered.
        /// </returns>
        /// <remarks>
        /// This method can only be called in InitializeStateSerializers.
        /// Instance specific state serializers always take precedence.
        /// </remarks>
        bool IReliableStateManager.TryAddStateSerializer<T>(IStateSerializer<T> stateSerializer)
        {
#pragma warning disable 0618
            return this._impl.TryAddStateSerializer(stateSerializer);
#pragma warning restore 0618
        }

        /// <summary>
        /// Restore a backup taken by <see cref="IStateProviderReplica.BackupAsync(System.Func{Microsoft.ServiceFabric.Data.BackupInfo,CancellationToken,System.Threading.Tasks.Task{bool}})"/> or 
        /// <see cref="IStateProviderReplica.BackupAsync(Microsoft.ServiceFabric.Data.BackupOption,System.TimeSpan,System.Threading.CancellationToken,System.Func{Microsoft.ServiceFabric.Data.BackupInfo,CancellationToken,System.Threading.Tasks.Task{bool}})"/>.
        /// </summary>
        /// <param name="backupFolderPath">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty or contain just whitespace. 
        /// UNC paths may also be provided.
        /// </param>
        /// <remarks>
        /// A safe backup will be performed, meaning the restore will only be completed if the data to restore is ahead of state of the current replica.
        /// </remarks>
        /// <returns>Task that represents the asynchronous restore operation.</returns>
        public Task RestoreAsync(string backupFolderPath) { return this._impl.RestoreAsync(backupFolderPath); }

        /// <summary>
        /// Restore a backup taken by <see cref="IStateProviderReplica.BackupAsync(System.Func{Microsoft.ServiceFabric.Data.BackupInfo,CancellationToken,System.Threading.Tasks.Task{bool}})"/> or 
        /// <see cref="IStateProviderReplica.BackupAsync(Microsoft.ServiceFabric.Data.BackupOption,System.TimeSpan,System.Threading.CancellationToken,System.Func{Microsoft.ServiceFabric.Data.BackupInfo,CancellationToken,System.Threading.Tasks.Task{bool}})"/>.
        /// </summary>
        /// <param name="restorePolicy">The restore policy.</param>
        /// <param name="backupFolderPath">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty or contain just whitespace. 
        /// UNC paths may also be provided.
        /// </param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous restore operation.</returns>
        public Task RestoreAsync(string backupFolderPath, RestorePolicy restorePolicy, CancellationToken cancellationToken)
        {
            return this._impl.RestoreAsync(backupFolderPath, restorePolicy, cancellationToken);
        }

        /// <summary>
        /// Performs a backup of all reliable state managed by this <see cref="IReliableStateManager"/>.
        /// </summary>
        /// <param name="option">The type of backup to perform.</param>
        /// <param name="timeout">The timeout for this operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <param name="backupCallback">Callback to be called when the backup folder has been created locally and is ready to be moved out of the node.</param>
        /// <returns>Task that represents the asynchronous backup operation.</returns>
        /// <remarks>
        /// Boolean returned by the backupCallback indicate whether the service was able to successfully move the backup folder to an external location.
        /// If false is returned, BackupAsync throws InvalidOperationException with the relevant message indicating backupCallback returned false.
        /// Also, backup will be marked as unsuccessful.
        /// </remarks>
        public Task BackupAsync(
            BackupOption option,
            TimeSpan timeout,
            CancellationToken cancellationToken,
            Func<BackupInfo, CancellationToken, Task<bool>> backupCallback)
        {
            return this._impl.BackupAsync(option, timeout, cancellationToken, backupCallback);
        }

        /// <summary>
        /// Set this property to receive notification when this <see cref="IStateProviderReplica"/> suspects data loss.
        /// </summary>
        /// <value>
        /// Function called as part of suspected data loss processing.
        /// </value>
        /// <remarks>
        /// <para>
        /// OnDataLossAsync function takes in CancellationToken and needs to return a Task that represents the asynchronous processing of the event.
        /// Returning true indicates that the reliable state manager's state has been restored.
        /// Returning false indicates that the reliable state manager's state has not been changed.
        /// </para>
        /// <para>
        /// The passed delegate should monitor the given cancellation token for cancellation requests.
        /// </para>
        /// </remarks>
        public Func<CancellationToken, Task<bool>> OnDataLossAsync
        {
            set { this._impl.OnDataLossAsync = value; }
        }

        /// <summary>
        /// Function called post restore has been performed on the replica.
        /// </summary>
        /// <value>
        /// Function called when the replica's state has been restored successfully by the framework
        /// </value>
        public Func<CancellationToken, Task> OnRestoreCompletedAsync
        {
            set { this._impl.OnRestoreCompletedAsync = value; }
        }

        /// <summary>
        /// Initialize the state provider replica using the service initialization information.
        /// </summary>
        /// <remarks>
        /// No complex processing should be done during Initialize. Expensive or long-running initialization should be done in OpenAsync.
        /// </remarks>
        /// <param name="initializationParameters">Service initialization information such as service name, partition id, replica id, and code package information.</param>
        void IStateProviderReplica.Initialize(StatefulServiceInitializationParameters initializationParameters)
        {
            this.Replica.Initialize(initializationParameters);
        }

        /// <summary>
        /// Open the state provider replica for use.
        /// </summary>
        /// <remarks>
        /// Extended state provider initialization tasks can be started at this time.
        /// </remarks>
        /// <param name="openMode">Indicates whether this is a new or existing replica.</param>
        /// <param name="partition">The partition this replica belongs to.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// Task that represents the asynchronous open operation. The result contains the replicator
        /// responsible for replicating state between other state provider replicas in the partition.
        /// </returns>
        Task<IReplicator> IStateProviderReplica.OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
        {
            return this.Replica.OpenAsync(openMode, partition, cancellationToken);
        }

        /// <summary>
        /// Notify the state provider replica that its role is changing, for example to Primary or Secondary.
        /// </summary>
        /// <param name="newRole">The new replica role, such as primary or secondary.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous change role operation.</returns>
        Task IStateProviderReplica.ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            return this.Replica.ChangeRoleAsync(newRole, cancellationToken);
        }

        /// <summary>
        /// Gracefully close the state provider replica.
        /// </summary>
        /// <remarks>
        /// This generally occurs when the replica's code is being upgrade, the replica is being moved
        /// due to load balancing, or a transient fault is detected.
        /// </remarks>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous close operation.</returns>
        Task IStateProviderReplica.CloseAsync(CancellationToken cancellationToken)
        {
            lock (this.eventHandlerRegistrationLock)
            {
                this.isClosingOrAborting = true;
            }

            this.CleanUpEventHandlers();
            return this.Replica.CloseAsync(cancellationToken);
        }

        /// <summary>
        /// Forcefully abort the state provider replica.
        /// </summary>
        /// <remarks>
        /// This generally occurs when a permanent fault is detected on the node, or when
        /// Service Fabric cannot reliably manage the replica's lifecycle due to internal failures.
        /// </remarks>
        void IStateProviderReplica.Abort()
        {
            lock (this.eventHandlerRegistrationLock)
            {
                this.isClosingOrAborting = true;
            }

            this.CleanUpEventHandlers();
            this.Replica.Abort();
        }

        /// <summary>
        /// Transaction changed forwarder.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="args">The notification.</param>
        private void OnTransactionChangedHandler(object sender, NotifyTransactionChangedEventArgs args)
        {
            var snappedUserDictionaryChangedEventHandler = this.userTransactionChangedEventHandler;

            if (snappedUserDictionaryChangedEventHandler != null)
            {
                snappedUserDictionaryChangedEventHandler.Invoke(this, args);
            }
        }

        /// <summary>
        /// State manager changed forwarder.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="args">The notification.</param>
        private void OnStateManagerChangedHandler(object sender, NotifyStateManagerChangedEventArgs args)
        {
            var snappedUserStateManagerChangedEventHandler = this.userStateManagerChangedEventHandler;

            if (snappedUserStateManagerChangedEventHandler != null)
            {
                snappedUserStateManagerChangedEventHandler.Invoke(this, args);
            }
        }

        private void RegisterTransactionEventHandlerIfNecessary()
        {
            if (this.isTransactionEventHandlerRegistered)
            {
                return;
            }

            lock (this.eventHandlerRegistrationLock)
            {
                this.ThrowIfClosingOrAbortingCallerHoldsLock();

                if (this.isTransactionEventHandlerRegistered == false)
                {
                    this._impl.TransactionChanged += this.OnTransactionChangedHandler;
                    this.isTransactionEventHandlerRegistered = true;
                }
            }
        }

        private void RegisterStateManagerEventHandlerIfNecessary()
        {
            if (this.isStateManagerEventHandlerRegistered)
            {
                return;
            }

            lock (this.eventHandlerRegistrationLock)
            {
                this.ThrowIfClosingOrAbortingCallerHoldsLock();

                if (this.isStateManagerEventHandlerRegistered == false)
                {
                    this._impl.StateManagerChanged += this.OnStateManagerChangedHandler;
                    this.isStateManagerEventHandlerRegistered = true;
                }
            }
        }

        private void UnRegisterTransactionEventHandlerIfNecessary()
        {
            ReleaseAssert.AssertIfNot(this.isClosingOrAborting, "Unregisteration only happens during close or abort.");

            // lock not required since close/abort is signaled, no registeration can come in.
            if (this.isTransactionEventHandlerRegistered)
            {
                this._impl.TransactionChanged -= this.OnTransactionChangedHandler;
                this.isTransactionEventHandlerRegistered = false;
            }
        }

        private void UnRegisterStateManagerEventHandlerIfNecessary()
        {
            ReleaseAssert.AssertIfNot(this.isClosingOrAborting, "Unregisteration only happens during close or abort.");

            // lock not required since close/abort is signaled, no registeration can come in.
            if (this.isStateManagerEventHandlerRegistered)
            {
                this._impl.StateManagerChanged -= this.OnStateManagerChangedHandler;
                this.isStateManagerEventHandlerRegistered = false;
            }
        }

        private void CleanUpEventHandlers()
        {
            this.UnRegisterTransactionEventHandlerIfNecessary();
            this.UnRegisterStateManagerEventHandlerIfNecessary();
        }

        private void ThrowIfClosingOrAbortingCallerHoldsLock()
        {
            if (this.isClosingOrAborting)
            {
                throw new FabricObjectClosedException("Reliable State Manager is closed.");
            }
        }

        private static string GetNativeRunConfigurationSettingFromConfigPackage(StatefulServiceContext serviceContext, ReliableStateManagerConfiguration configuration)
        {
            if (configuration == null || configuration.ConfigPackageName == null)
            {
                return null;
            }

            ConfigurationSection nativeConfigSection = null;
            string setting = null;

            var configPkg = serviceContext.CodePackageActivationContext.GetConfigurationPackageObject(configuration.ConfigPackageName);

            if (configPkg != null && configPkg.Settings != null && configPkg.Settings.Sections != null && configPkg.Settings.Sections.Contains(SectionName))
            {
                nativeConfigSection = configPkg.Settings.Sections[SectionName];
            }

            if (nativeConfigSection != null && nativeConfigSection.Parameters.Contains(Test_LoadEnableNativeReliableStateManager))
            {
                var value = nativeConfigSection.Parameters[Test_LoadEnableNativeReliableStateManager].Value;
                bool loadTestSetting = string.IsNullOrEmpty(value) ? false : bool.Parse(value);

                if (loadTestSetting && nativeConfigSection.Parameters.Contains(EnableNativeReliableStateManager))
                {
                    setting = nativeConfigSection.Parameters[EnableNativeReliableStateManager].Value;
                }
            }

            return setting;
        }

        private static string GetNativeRunConfigurationSettingFromClusterManifest()
        {
            var configStore = NativeConfigStore.FabricGetConfigStore();
            return configStore.ReadUnencryptedString(SectionName, EnableNativeReliableStateManager);
        }

        private static bool GetNativeRunConfigurationSetting(StatefulServiceContext serviceContext, ReliableStateManagerConfiguration configuration)
        {
            var value = GetNativeRunConfigurationSettingFromClusterManifest();

            if (configuration != null)
            {
                // override from configuration package
                value = GetNativeRunConfigurationSettingFromConfigPackage(serviceContext, configuration);
            }

            return string.IsNullOrEmpty(value) ? false : bool.Parse(value);
        }
    }
}