// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// <para>Represents a stream of replication or copy operations that are sent from the Primary to the Secondary replica.  </para>
    /// </summary>
    /// <exception cref="System.TimeoutException">
    /// <para>The exception that is thrown when the time allotted for a process or operation has expired.</para>
    /// </exception>
    /// <remarks>
    /// <para>The streams returned from <see cref="System.Fabric.IStateReplicator.GetCopyStream" /> and <see cref="System.Fabric.IStateReplicator.GetReplicationStream" /> 
    /// are objects that implement <see cref="System.Fabric.IOperationStream" />.</para>
    /// </remarks>
    public interface IOperationStream
    {
        /// <summary>
        /// <para>Gets the next object that implements <see cref="System.Fabric.IOperation" /> from the underlying <see cref="System.Fabric.IOperationStream" />.</para>
        /// </summary>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that 
        /// the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task{T}" /> of type <see cref="System.Fabric.IOperation" />.</para>
        /// </returns>
        Task<IOperation> GetOperationAsync(CancellationToken cancellationToken);
    }
}