// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// <para> An <see cref="System.Fabric.IOperationDataStream" /> 
    /// encapsulates a stream of <see cref="System.Fabric.OperationData" /> objects that are exchanged between Primary replica and Secondary replica.
    /// Objects that implement <see cref="System.Fabric.IOperationDataStream" /> are used during the copy process.
    /// Both the copy context <see cref="System.Fabric.IStateProvider.GetCopyContext" /> method that is sent from the Secondary replica to the Primary replica 
    /// and the copy state <see cref="System.Fabric.IStateProvider.GetCopyState(System.Int64,System.Fabric.IOperationDataStream)" /> method implement the <see cref="System.Fabric.IOperationDataStream" /> 
    /// interface.</para>
    /// </summary>
    public interface IOperationDataStream
    {
        /// <summary>
        /// <para>Gets the next <see cref="System.Fabric.OperationData" /> object from the <see cref="System.Fabric.IOperationDataStream" />.</para>
        /// </summary>
        /// <param name="cancellationToken">
        /// <para>Provides a mechanism to cancel the asynchronous operation.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Threading.Tasks.Task{T}" /> of type <see cref="System.Fabric.OperationData" />.</para>
        /// </returns>
        /// <remarks>
        /// <para>
        ///     Returning null indicates to the system that the transfer is complete.</para>
        /// </remarks>
        [SuppressMessage(FxCop.Category.MSInternal, FxCop.Rule.UseApprovedGenericsForPrecompiledAssemblies, Justification = "Not precompiled assembly.")]
        Task<OperationData> GetNextAsync(CancellationToken cancellationToken);
    }
}