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

    internal interface IInfrastructureService 
    {
        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task<string> RunCommandAsync(bool isAdminCommand, string command, TimeSpan timeout, CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task ReportStartTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task ReportFinishTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task ReportTaskFailureAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken);
    }
}