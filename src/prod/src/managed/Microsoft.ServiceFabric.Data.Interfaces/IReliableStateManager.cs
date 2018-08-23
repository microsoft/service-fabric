// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Fabric;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Data.Notifications;

    /// <summary>
    /// Manages all <see cref="IReliableState"/> for a service replica.
    /// Each replica in a service has its own state manager and thus its own set of <see cref="IReliableState"/>.
    /// </summary>
    public interface IReliableStateManager : IAsyncEnumerable<IReliableState>
    {
        /// <summary>
        /// Occurs when a transaction's state changes.
        /// For example, commit of a transaction.
        /// </summary>
        event EventHandler<NotifyTransactionChangedEventArgs> TransactionChanged;

        /// <summary>
        /// Occurs when State Manager's state changes.
        /// For example, creation or delete of reliable state or rebuild of the reliable state manager.
        /// </summary>
        event EventHandler<NotifyStateManagerChangedEventArgs> StateManagerChanged;

        /// <summary>
        /// Registers a custom serializer for all reliable collections.
        /// </summary>
        /// <typeparam name="T">Type that will be serialized and deserialized.</typeparam>
        /// <param name="stateSerializer">
        /// The state serializer to be added.
        /// </param>
        /// <returns>
        /// True if the custom serializer was added.
        /// False if a custom serializer for the given type already exists.
        /// </returns>
        /// <remarks>
        /// <para>
        /// When a reliable collection needs to serialize an object, it asks the state manager for a serializer for the given type.
        /// The state manager will first check if there is a custom serializer registered for the input type. If not, it will check if one of the built-in
        /// serializers can serialize the type. The state manager has built-in serializers for the following types: guid, bool, byte, sbyte, char, decimal, double,
        /// float, int, uint, long, ulong, short, ushort and string. If not, it will use <see cref="System.Runtime.Serialization.DataContractSerializer"/>.
        /// </para>
        /// <para>
        /// Serializers must be infinitely forwards and backwards compatible. For the types that are using built-in serializers, Service Fabric ensures
        /// forwards and backwards compatibility. However, if a custom serializer is added for a type with a built-in serializer, the custom serializer
        /// must be compatible with the built-in serialization format for that type.
        /// </para>
        /// <para>
        /// This method should be called from the constructor of the Stateful Service. 
        /// This ensures that the Reliable Collections have the necessary serializers before recovery of the persisted state begins.
        /// </para>
        /// </remarks>
        bool TryAddStateSerializer<T>(IStateSerializer<T> stateSerializer);

        /// <summary>
        /// Create and start a new transaction that can be used to group operations to be performed atomically.
        /// </summary>
        /// <remarks>
        /// Operations are added to the transaction by passing the <see cref="ITransaction"/> object in to reliable state methods.
        /// </remarks>
        /// <returns>A new transaction.</returns>
        ITransaction CreateTransaction();

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> GetOrAddAsync<T>(ITransaction tx, Uri name, TimeSpan timeout) where T : IReliableState;

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> GetOrAddAsync<T>(ITransaction tx, Uri name) where T : IReliableState;

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <exception cref="TransactionFaultedException">The operation has been internally faulted by the system. Retry the operation</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> GetOrAddAsync<T>(Uri name, TimeSpan timeout) where T : IReliableState;

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <exception cref="TransactionFaultedException">The operation has been internally faulted by the system. Retry the operation</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> GetOrAddAsync<T>(Uri name) where T : IReliableState;

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> GetOrAddAsync<T>(ITransaction tx, string name, TimeSpan timeout) where T : IReliableState;

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> GetOrAddAsync<T>(ITransaction tx, string name) where T : IReliableState;

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <exception cref="TransactionFaultedException">The operation has been internally faulted by the system. Retry the operation</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> GetOrAddAsync<T>(string name, TimeSpan timeout) where T : IReliableState;

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <exception cref="TransactionFaultedException">The operation has been internally faulted by the system. Retry the operation</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is the reliable state instance.</returns>
        Task<T> GetOrAddAsync<T>(string name) where T : IReliableState;

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task RemoveAsync(ITransaction tx, Uri name, TimeSpan timeout);

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task RemoveAsync(ITransaction tx, Uri name);

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <exception cref="TransactionFaultedException">The operation has been internally faulted by the system. Retry the operation</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task RemoveAsync(Uri name, TimeSpan timeout);

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <exception cref="TransactionFaultedException">The operation has been internally faulted by the system. Retry the operation</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task RemoveAsync(Uri name);

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task RemoveAsync(ITransaction tx, string name, TimeSpan timeout);

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task RemoveAsync(ITransaction tx, string name);

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <exception cref="TransactionFaultedException">The operation has been internally faulted by the system. Retry the operation</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task RemoveAsync(string name, TimeSpan timeout);

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
        /// <exception cref="System.Fabric.FabricNotPrimaryException">Thrown when the <see cref="Microsoft.ServiceFabric.Data.IReliableStateManager"/> is not in  <see cref="System.Fabric.ReplicaRole.Primary"/>.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <exception cref="TransactionFaultedException">The operation has been internally faulted by the system. Retry the operation</exception>
        /// <returns>Task that represents the asynchronous remove operation.</returns>
        Task RemoveAsync(string name);

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
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <exception cref="TransactionFaultedException">The operation has been internally faulted by the system. Retry the operation</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is a tuple indicating whether the reliable state was found, and if so the instance.</returns>
        Task<ConditionalValue<T>> TryGetAsync<T>(Uri name) where T : IReliableState;

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
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable State Manager is closed.</exception>
        /// <exception cref="TransactionFaultedException">The operation has been internally faulted by the system. Retry the operation</exception>
        /// <returns>Task that represents the asynchronous operation. The task result is a tuple indicating whether the reliable state was found, and if so the instance.</returns>
        Task<ConditionalValue<T>> TryGetAsync<T>(string name) where T : IReliableState;
    }
}