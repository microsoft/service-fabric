// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Threading;
    using System.Threading.Tasks;

    internal interface IGatewayResourceManager 
    {
        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task<string> UpdateOrCreateAsync(string resourceDescription, TimeSpan timeout, CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task<List<string>> QueryAsync(string queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task DeleteAsync(string resourceName, TimeSpan timeout, CancellationToken cancellationToken);
    }
}
