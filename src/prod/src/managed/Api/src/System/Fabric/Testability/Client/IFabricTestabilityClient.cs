// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client
{
    using System;
    using System.Fabric.Result;
    using System.Numerics;
    using System.Threading;
    using System.Threading.Tasks;

    public interface IFabricTestabilityClient
    {
        Task<OperationResult> StartNodeAsync(
            string nodeName,
            BigInteger nodeInstance,
            string ipAddressOrFQDN,
            int clusterConnectionPort,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task<OperationResult> StopNodeAsync(
            string nodeName,
            BigInteger nodeInstance,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task<OperationResult<RestartNodeResult>> RestartNodeAsync(
            string nodeName,
            BigInteger nodeInstance,
            bool collectFabricDump,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task<OperationResult<RestartNodeResult>> RestartNodeAsync(
            ReplicaSelector replicaSelector,
            bool collectFabricDump,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task<OperationResult> RestartDeployedCodePackageAsync(
            Uri applicationName,
            ReplicaSelector replicaSelector,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task<OperationResult> RestartDeployedCodePackageAsync(
            string nodeName,
            Uri applicationName,
            string serviceManifestName,
            string servicePackageActivationId,
            string codePackageName,
            long codePackageInstanceId,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task RestartReplicaAsync(
            ReplicaSelector replicaSelector,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task RestartReplicaAsync(
            string nodeName,
            Guid partitionId,
            long replicaId,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task RemoveReplicaAsync(
            ReplicaSelector replicaSelector,
            CompletionMode completionMode,
            bool forceRemove,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task RemoveReplicaAsync(
            string nodeName,
            Guid partitionId,
            long replicaId,
            CompletionMode completionMode,
            bool forceRemove,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task ValidateApplicationAsync(
            Uri applicationName,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task ValidateServiceAsync(
            Uri serviceName,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task InvokeDataLossAsync(
            PartitionSelector partitionSelector,
            DataLossMode datalossMode,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task RestartPartitionAsync(
            PartitionSelector partitionSelector,
            RestartPartitionMode restartPartitionMode,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task MovePrimaryReplicaAsync(
            string nodeName,
            PartitionSelector partitionSelector,
            bool ignoreConstraints,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task MoveSecondaryReplicaAsync(
            string currentNodeName,
            string newNodeName,
            PartitionSelector partitionSelector,
            bool ignoreConstraints,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task InvokeQuorumLossAsync(
            PartitionSelector partitionSelector,
            QuorumLossMode quorumlossMode,
            TimeSpan quorumlossDuration,
            TimeSpan operationTimeout,
            CancellationToken token);

        Task CleanTestStateAsync(
            TimeSpan operationTimeout,
            CancellationToken token);
    }
}


#pragma warning restore 1591