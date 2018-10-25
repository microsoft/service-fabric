// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Threading;
    using System.Threading.Tasks;

    internal interface ITokenValidationService 
    {
        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task<ClaimDescriptionList> ValidateTokenAsync(string authToken, TimeSpan timeout, CancellationToken cancellationToken);

        TokenServiceMetadata GetTokenServiceMetadata();
    }
}