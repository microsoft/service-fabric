// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Fabric;
    using System.Threading.Tasks;

    /// <summary>
    /// <para>Defines methods for manipulating Reliable Collections.</para>
    /// </summary>
    /// <remarks>
    /// <para>More information on Reliable Collections can be seen
    /// <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-reliable-services-reliable-collections">here</see>.</para>
    /// </remarks>
    /// <typeparam name="T">The type of the elements in the collection.</typeparam>
    public interface IReliableCollection<T> : IReliableState
    {
        /// <summary>
        /// <para>Gets the number of elements contained in the <see cref="IReliableCollection{T}"/>.</para>
        /// </summary>
        /// <param name="tx">
        /// The transaction to associate this operation with. See examples of
        /// transactions <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-work-with-reliable-collections">here</see>.
        /// </param>
        /// <exception cref="FabricNotReadableException">
        /// <para>Indicates that the IReliableCollection cannot serve reads at the moment.
        /// This exception can be thrown in all <see cref="ReplicaRole"/>s.
        /// One reason it may be thrown in the <see cref="ReplicaRole.Primary"/> role is loss of <see cref="IStatefulServicePartition.ReadStatus"/>.
        /// One reason it may be thrown in the <see cref="ReplicaRole.ActiveSecondary"/> role is that Reliable Collection's state is not yet consistent.</para>
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="FabricNotPrimaryException">
        /// <para>Thrown when attempting to perform this operation on a <see cref="IReliableCollection{T}"/> 
        /// that is not in the <see cref="ReplicaRole.Primary"/> role.
        /// In some instances, read operations, such as this one, can be performed from secondary replicas 
        /// depending on the implementation of the IReliableCollection used.</para>
        /// </exception>
        /// <returns>
        /// <para>A task that represents the asynchronous operation, indicating the number of elements.</para>
        /// </returns>
        Task<long> GetCountAsync(ITransaction tx);

        /// <summary>
        /// <para>Removes all state from the <see cref="IReliableCollection{T}"/>, including replicated and persisted state.</para>
        /// </summary>
        /// <exception cref="FabricNotPrimaryException">
        /// <para>Thrown when attempting to perform this operation
        /// on a <cref name="IReliableCollection{T}"/> that is not in the <see cref="ReplicaRole.Primary"/> role.</para>
        /// </exception>
        /// <exception cref="TimeoutException">
        /// <para>Indicates that this operation failed to complete within the given timeout.</para>
        /// </exception>
        /// <returns>
        /// <para>A task that represents the asynchronous clear operation.</para>
        /// </returns>
        Task ClearAsync();
    }
}