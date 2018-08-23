// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Query;
    using System.Threading;
    using System.Threading.Tasks;

    internal interface IFaultAnalysisService 
    {
        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task InvokeDataLossAsync(
            InvokeDataLossDescription invokeDataLossDescription, 
            TimeSpan timeout, 
            CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task<PartitionDataLossProgress> GetInvokeDataLossProgressAsync(
            Guid operationId,
            TimeSpan timeout,
            CancellationToken cancellationToken);
            
        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task InvokeQuorumLossAsync(
            InvokeQuorumLossDescription invokeQuorumLossDescription, 
            TimeSpan timeout, 
            CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task<PartitionQuorumLossProgress> GetInvokeQuorumLossProgressAsync(
            Guid operationId,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task RestartPartitionAsync(
            RestartPartitionDescription restartPartitionDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task<PartitionRestartProgress> GetRestartPartitionProgressAsync(
            Guid operationId,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task<TestCommandQueryResult> GetTestCommandListAsync(
            TestCommandListDescription operationId,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task CancelTestCommandAsync(
            CancelTestCommandDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task<string> GetStoppedNodeListAsync(
        TimeSpan timeout,
        CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task StartNodeTransitionAsync(
            NodeTransitionDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task<NodeTransitionProgress> GetNodeTransitionProgressAsync(
            Guid operationId,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        #region Chaos
        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task StartChaosAsync(
            StartChaosDescription startChaosDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task StopChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task<ChaosReport> GetChaosReportAsync(
            GetChaosReportDescription getChaosReportDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken);
        #endregion

        Task<String> CallSystemService(
            string action,
            string inpubBlob,
            TimeSpan timeout,
            CancellationToken cancellationToken);
    }
}