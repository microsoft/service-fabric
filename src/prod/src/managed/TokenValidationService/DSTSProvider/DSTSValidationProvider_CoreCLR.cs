// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.TokenValidationService
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class DSTSValidationProvider : ITokenValidationProvider
    {
        public DSTSValidationProvider(string configSection)
        {
        }

#region ITokenValidationProvider

        public Task<ClaimDescriptionList> ValidateTokenAsync(string authToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public TokenServiceMetadata GetTokenServiceMetadata()
        {
            throw new NotSupportedException();
        }

#endregion
    }
}
