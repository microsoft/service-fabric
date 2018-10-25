// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if DotNetCoreClr
using Microsoft.IdentityModel.Tokens;
using System.IdentityModel.Tokens.Jwt;
#else
using System.IdentityModel.Metadata;
using System.IdentityModel.Tokens;
using System.ServiceModel.Security;
using System.Net;
#endif

using System.Collections.Generic;
using System.Fabric.Common.Tracing;
using System.Globalization;
using System.Linq;
using System.Security.Claims;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using System;

namespace System.Fabric.AzureActiveDirectory.Server
{
    internal sealed class ServerUtility
    {
        public const string ExpirationClaim = "exp";

        private const string TraceType = "AAD.Server";
        private static readonly FabricEvents.ExtensionsEvents TraceSource = FabricEvents.ExtensionsEvents.GetEventSource(FabricEvents.Tasks.SystemFabric);
        
        static volatile TokenSigningCertificateManager tokenSigningCertificateManager = null;
        private static readonly object tokenSigningCertificateManagerLock = new object();

        static TokenSigningCertificateManager TokenSigningCertificateManager
        {
            get
            {
                if (tokenSigningCertificateManager == null)
                {
                    lock (tokenSigningCertificateManagerLock)
                    {
                        if (tokenSigningCertificateManager == null)
                        {
                            tokenSigningCertificateManager = new TokenSigningCertificateManager();
                        }
                    }
                }
                return tokenSigningCertificateManager;
            }
        }

        static ServerUtility()
        {
            TraceConfig.InitializeFromConfigStore();

#if !DotNetCoreClr
            // "System.Net.WebException: The request was aborted: Could not create SSL/TLS secure channel" 
            // may be encountered when making HttpWebRequest calls to an https endpoint if the cipher suites 
            // have been modified according to "https://requirements.azurewebsites.net/Requirements/Details/6417"
            //
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;
#endif
        }

        public static ValidatedClaims Validate(
            string expectedIssuer,
            string expectedAudience,
            string expectedRoleClaimKey,
            string expectedAdminRoleValue,
            string expectedUserRoleValue,
            string certEndpoint,
            long certRolloverIntervalTicks,
            string jwt)
        {
            TokenSigningCertificateManager.Start(new TimeSpan(certRolloverIntervalTicks), certEndpoint, expectedIssuer);

            var claimsPrincipal = ValidateJwt(
                TokenSigningCertificateManager.Issuer,
                expectedAudience, 
                certEndpoint, 
                jwt);

            bool roleKeyFound = false;
            bool isAdmin = false;
            TimeSpan expiration = TimeSpan.Zero;

            foreach (var claim in claimsPrincipal.Claims)
            {
                WriteInfo("Claim: {0}", claim);

                if (claim.Type == expectedRoleClaimKey)
                {
                    roleKeyFound = true;

                    if (claim.Value == expectedAdminRoleValue)
                    {
                        isAdmin = true;
                    }
                    else if (claim.Value == expectedUserRoleValue)
                    {
                        // no-op: do not reset isAdmin from true back to false
                    }
                    else
                    {
                        throw new SecurityTokenValidationException(string.Format(CultureInfo.InvariantCulture,
                            "Invalid role: {0}={1}", 
                            claim.Type, 
                            claim.Value));
                    }
                }
                else if (claim.Type == ExpirationClaim)
                {
                    var seconds = int.Parse(claim.Value);

                    // 1970-01-01T00:00:00Z UTC (from JWT specification)
                    //
                    var baseDate = new DateTime(1970, 01, 01, 0, 0, 0, DateTimeKind.Utc);
                    var expirationDate = baseDate.Add(TimeSpan.FromSeconds(seconds));

                    expiration = expirationDate.Subtract(DateTime.UtcNow);
                }
            }

            if (!roleKeyFound)
            {
                throw new SecurityTokenValidationException("Role claim not found");
            }

            WriteInfo("Result: IsAdmin={0} Expiration={1}", isAdmin, expiration);

            return new ValidatedClaims(isAdmin, expiration);
        }

        private static ClaimsPrincipal ValidateJwt(
            string expectedIssuer,
            string expectedAudience,
            string certEndpoint,
            string jwt)
        {
            var signingCerts = TokenSigningCertificateManager.GetTokenSigningCerts(certEndpoint);

            foreach (var cert in signingCerts)
            {
                WriteInfo(
                    "Cached signing cert: thumbprint={0} valid=[{1}, {2}]", 
                    cert.Certificate.Thumbprint,
                    cert.Certificate.NotBefore,
                    cert.Certificate.NotAfter);
            }

            var validationParams = new TokenValidationParameters()
            {
                ValidIssuer = expectedIssuer,
                ValidAudience = expectedAudience,
                IssuerSigningKeys = signingCerts
            };

            SecurityToken validatedToken = null;

            var claimsPrincipal = new JwtSecurityTokenHandler().ValidateToken(
                jwt,
                validationParams,
                out validatedToken);

            return claimsPrincipal;
        }
        
        private static void WriteInfo(string format, params object[] args)
        {
            TraceSource.WriteInfo(TraceType, format, args);
        }

        //
        // Inner Classes
        //

        public sealed class ValidatedClaims
        {
            public bool IsAdmin { get; private set; }

            public TimeSpan Expiration { get; private set; }

            internal ValidatedClaims(
                bool isAdmin,
                TimeSpan expiration)
            {
                this.IsAdmin = isAdmin;
                this.Expiration = expiration;
            }
        };
    }
}
