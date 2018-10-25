// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.TokenValidationService
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Threading;
    using System.Threading.Tasks;

    internal interface ITokenValidationProvider
    {
        Task<ClaimDescriptionList> ValidateTokenAsync(string authToken, TimeSpan timeout, CancellationToken cancellationToken);
        TokenServiceMetadata GetTokenServiceMetadata();
    }
}