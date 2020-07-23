// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Linq;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;

    /// <summary>
    /// certificate utilities
    /// </summary>
    internal static class X509CertificateUtility
    {
        private static readonly TraceType TraceType = new TraceType("X509CertificateUtility");

        /// <summary>
        /// try to find certificates under LocalMachine store location on the current machine.
        /// </summary>
        /// <param name="storeName">certificate store name</param>
        /// <param name="thumbprints">certificate thumbprints</param>
        /// <param name="foundCertificates">The found certificates. Empty collection indicates no cert is found.</param>
        /// <param name="traceLogger">trace logger</param>
        /// <returns>Unfound certificates. Empty collection indicates all certs are found.</returns>
        [SuppressMessage("StyleCop.CSharp.DocumentationRules", "SA1650:ElementDocumentationMustBeSpelledCorrectly", Justification = "Words like ip or fqdn are valid to use.")]
        public static IEnumerable<string> TryFindCertificate(
            StoreName storeName,
            IEnumerable<string> findValues,
            X509FindType findType,
            out IEnumerable<X509Certificate2> foundCertificates,
            ITraceLogger traceLogger = null)
        {
            return X509CertificateUtility.TryFindCertificate(string.Empty, storeName, findValues, findType, out foundCertificates, traceLogger);
        }

        /// <summary>
        /// try to find certificates under LocalMachine store location.
        /// </summary>
        /// <param name="machineIpOrFqdn">machine ip or fqdn. "localhost" for the local machine</param>
        /// <param name="storeName">certificate store name</param>
        /// <param name="thumbprints">certificate thumbprints</param>
        /// <param name="foundCertificates">The found certificates. Empty collection indicates no cert is found.</param>
        /// <param name="traceLogger">trace logger</param>
        /// <returns>Unfound certificates. Empty collection indicates all certs are found.</returns>
        [SuppressMessage("StyleCop.CSharp.DocumentationRules", "SA1650:ElementDocumentationMustBeSpelledCorrectly", Justification = "Words like ip or fqdn are valid to use.")]
        public static IEnumerable<string> TryFindCertificate(
            string machineIpOrFqdn,
            StoreName storeName,
            IEnumerable<string> findValues,
            X509FindType findType,
            out IEnumerable<X509Certificate2> foundCertificates,
            ITraceLogger traceLogger = null)
        {
            machineIpOrFqdn.MustNotBeNull("machineIpOrFqdn");

            // The validation on each thumbprint string will be covered by RDBug 7682650.
            findValues.ToArray().MustNotBeEmptyCollection("findValues");

            List<X509Certificate2> tmpFoundCertificates = new List<X509Certificate2>();
            foundCertificates = Enumerable.Empty<X509Certificate2>();
            List<string> result = new List<string>();
            X509Store store = null;

            string strStoreName = storeName.ToString();
            if (machineIpOrFqdn != string.Empty)
            {
                strStoreName = machineIpOrFqdn + "\\" + storeName;
            }

            try
            {
                try
                {
                    store = new X509Store(strStoreName, StoreLocation.LocalMachine);
                    store.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly);
                }
                catch (CryptographicException ex)
                {
                    // if the exception is access denied error, check if the read permission on the registry key HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\SecurePipeServers\winreg
                    // of the remote node machineIpOrFqdn has been granted to the caller account.
                    if (traceLogger != null)
                    {
                        traceLogger.WriteWarning(X509CertificateUtility.TraceType, ex.ToString());
                    }
                    
                    return findValues;
                }

                foreach (string findValue in findValues)
                {
                    X509Certificate2Collection certs = store.Certificates.Find(findType, findValue, validOnly: false);
                    if (certs != null && findType == X509FindType.FindBySubjectName)
                    {
                        certs.MatchExactSubject(findValue);
                    }

                    if (certs == null || certs.Count == 0)
                    {
                        result.Add(findValue);
                    }
                    else
                    {
                        tmpFoundCertificates.Add(findType == X509FindType.FindByThumbprint ? certs[0] : certs.LatestExpired());
                    }
                }

                foundCertificates = tmpFoundCertificates;
                return result;
            }
            finally
            {
                // In .net 4.6, X509Store is changed to implement IDisposable.
                // But unfortunately, SF today is built on .net 4.5
                if (store != null)
                {
                    store.Close();
                }
            }
        }

        /// <summary>
        /// validate a certificate
        /// </summary>
        /// <param name="certificate">certificate to validate</param>
        /// <returns>true if the certificate is effective and not expired</returns>
        public static bool IsCertificateValid(X509Certificate2 certificate)
        {
            certificate.MustNotBeNull("certificate");

            DateTime now = DateTime.Now;
            return now > Convert.ToDateTime(certificate.GetEffectiveDateString()) &&
                now < Convert.ToDateTime(certificate.GetExpirationDateString());
        }

        public static string FindIssuer(this X509Certificate2 cert, IEnumerable<string> issuerThumbprints)
        {
            X509Chain chain = new X509Chain();
            chain.ChainPolicy.RevocationMode = X509RevocationMode.NoCheck;
            chain.Build(cert);

            if (chain.ChainElements != null && chain.ChainElements.Count > 0)
            {
                foreach (X509ChainElement ce in chain.ChainElements)
                {
                    if (issuerThumbprints.Contains(ce.Certificate.Thumbprint, StringComparer.OrdinalIgnoreCase))
                    {
                        return ce.Certificate.Thumbprint;
                    }
                }
            }

            return null;
        }

        internal static void MatchExactSubject(this X509Certificate2Collection certs, string commonName)
        {
            List<X509Certificate2> certsToRemove = new List<X509Certificate2>();
            foreach (X509Certificate2 cert in certs)
            {
                if (cert.GetNameInfo(X509NameType.SimpleName, forIssuer: false) != commonName
                    && cert.Subject != commonName)
                {
                    certsToRemove.Add(cert);
                }
            }

            foreach (X509Certificate2 certToRemove in certsToRemove)
            {
                certs.Remove(certToRemove);
            }
        }

        internal static X509Certificate2 LatestExpired(this X509Certificate2Collection certs)
        {
            X509Certificate2 result = certs[0];
            for (int i = 1; i < certs.Count; i++)
            {
                if (Convert.ToDateTime(certs[i].GetExpirationDateString())
                    > Convert.ToDateTime(result.GetExpirationDateString()))
                {
                    result = certs[i];
                }
            }

            return result;
        }
    }
}