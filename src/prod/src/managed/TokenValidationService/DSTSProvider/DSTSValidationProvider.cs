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
    using Microsoft.IdentityModel.Claims;

    internal sealed class DSTSValidationProvider : ITokenValidationProvider
    {
        private DSTSObjectManager dstsManager;

        public DSTSValidationProvider(string configSection)
        {
            this.dstsManager = new DSTSObjectManager(configSection);
        }

        private ClaimDescriptionList ValidateToken(string authToken)
        {
            ClaimDescriptionList claimList = new ClaimDescriptionList();
            ClaimsIdentityCollection claimsResult = this.dstsManager.ValidateToken(authToken);
            List<ClaimDescription> claims = new List<ClaimDescription>();
            for (int i = 0; i < claimsResult.Count; i++)
            {
                IClaimsIdentity claimIdentity = claimsResult[i];
                foreach (Claim claim in claimIdentity.Claims)
                {
                    ClaimDescription claimDescription = new ClaimDescription(
                        claim.ClaimType,
                        claim.Issuer,
                        claim.OriginalIssuer,
                        claim.Subject.ToString(),
                        claim.Value,
                        claim.ValueType);
                    claims.Add(claimDescription);
                }
            }
            claimList.AddClaims(claims);
            return claimList;
        }

#region ITokenValidationProvider

        public Task<ClaimDescriptionList> ValidateTokenAsync(string authToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<ClaimDescriptionList>.Factory.StartNew(() => { return ValidateToken(authToken); });
        }

        public TokenServiceMetadata GetTokenServiceMetadata()
        {
            return this.dstsManager.GetDSTSMetadata();
        }

#endregion
    }
}