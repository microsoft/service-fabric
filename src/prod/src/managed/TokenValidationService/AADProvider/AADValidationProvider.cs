// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.TokenValidationService
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.AzureActiveDirectory.Server;
    using System.Fabric.Description;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class AADValidationProvider : ITokenValidationProvider
    {
        private readonly AADSettings settings;

        public AADValidationProvider(string configSection)
        {
            // Since we have the service's config section name here, it's
            // possible to load per-service configurations if needed.
            // Currently, all AAD settings are global so the config section
            // name is not used.

            this.settings = new AADSettings();
        }

        private ClaimDescriptionList ValidateToken(string authToken)
        {
            var issuer = string.Format(this.settings.TokenIssuerFormat, this.settings.TenantId);

            var validatedClaims = ServerUtility.Validate(
                issuer,
                this.settings.ClusterApplication,
                this.settings.RoleClaimKey,
                this.settings.AdminRoleClaimValue,
                this.settings.UserRoleClaimValue,
                string.Format(this.settings.CertEndpointFormat, this.settings.TenantId),
                TimeSpan.FromSeconds(this.settings.SigningCertRolloverCheckInterval).Ticks,
                authToken);

            var claimsList = new ClaimDescriptionList();
            var claims = new List<ClaimDescription>();

            var roleClaim = new ClaimDescription(
                this.settings.RoleClaimKey,
                issuer, // issuer
                issuer, // original issuer
                issuer, // subject
                validatedClaims.IsAdmin ? this.settings.AdminRoleClaimValue : this.settings.UserRoleClaimValue,
                "N/A"); // value type

            var expirationClaim = new ClaimDescription(
                ServerUtility.ExpirationClaim,
                issuer, // issuer
                issuer, // original issuer
                issuer, // subject
                validatedClaims.Expiration.TotalSeconds.ToString(),
                "N/A"); // value type

            claims.Add(roleClaim);
            claims.Add(expirationClaim);

            claimsList.AddClaims(claims);

            return claimsList;
        }

#region ITokenValidationProvider

        public Task<ClaimDescriptionList> ValidateTokenAsync(string authToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<ClaimDescriptionList>.Factory.StartNew(() => { return ValidateToken(authToken); });
        }

        public TokenServiceMetadata GetTokenServiceMetadata()
        {
            // AAD metadata is read by the Naming or HTTP gateway from cluster settings, not the service
            throw new NotImplementedException();
        }

#endregion
    }
}
