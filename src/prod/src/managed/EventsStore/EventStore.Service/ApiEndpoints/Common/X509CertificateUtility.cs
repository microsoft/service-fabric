// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Common
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Security.Cryptography.X509Certificates;

    internal static class X509CertificateUtility
    {
        private static readonly X509ChainStatusFlags AllowedChainStatusForIssuerThumbprintCheck =
            X509ChainStatusFlags.UntrustedRoot |
            X509ChainStatusFlags.OfflineRevocation |
            X509ChainStatusFlags.RevocationStatusUnknown;

        public static bool IsValid(this X509Certificate2 certificate)
        {
            DateTime now = DateTime.Now;
            return now > Convert.ToDateTime(certificate.GetEffectiveDateString()) &&
                now < Convert.ToDateTime(certificate.GetExpirationDateString());
        }

        internal static bool ValidateChainAndIssuerThumbprint(this X509Certificate2 cert, HashSet<string> issuerThumbprints)
        {
            X509Chain chain = new X509Chain();

            chain.ChainPolicy.RevocationMode = X509RevocationMode.NoCheck;

            var isValid = chain.Build(cert);
            if (issuerThumbprints.Count == 0)
            {
                return isValid;
            }

            if (chain.ChainElements == null || chain.ChainElements.Count == 0)
            {
                return false;
            }

            var chainStatus = AllowedChainStatusForIssuerThumbprintCheck;
            chain.ChainStatus.ForEach(s => chainStatus |= s.Status);

            // Only do issuer thumbprint check if there's no other errors other than the allowed list
            if (chainStatus == AllowedChainStatusForIssuerThumbprintCheck)
            {
                // For self-signed certificate there's only one element in the chain which is the certificate itself
                var issuer = chain.ChainElements.Count > 1 ? chain.ChainElements[1] : chain.ChainElements[0];
                return issuerThumbprints.Contains(issuer.Certificate.Thumbprint.ToUpper());
            }

            return false;
        }

        internal static bool MatchCommonName(this X509Certificate2 cert, string commonName)
        {
            if (cert.GetNameInfo(X509NameType.SimpleName, forIssuer: false) == commonName
                || cert.Subject == commonName)
            {
                return true;
            }

            return false;
        }
    }
}
