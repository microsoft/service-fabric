// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Net.Security;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;

namespace System.Fabric.dSTSClient
{
    public class CertificateHandler
    {
        private static readonly string TraceType = "CertificateHandler";

        private string[] serverCommonNames;

        private string[] serverThumbprints;

        public CertificateHandler(string[] names, string[] thumbprints) { serverCommonNames = names; serverThumbprints = thumbprints; }

        public bool ValidateCertificate(object sender, X509Certificate certificate, X509Chain chain, SslPolicyErrors sslPolicyErrors)
        {
            string subjectName = "";
            string dnsName = "";
            string dnsFromAlternativeName = "";
            X509Certificate2 cert2 = certificate as X509Certificate2;

            if (sslPolicyErrors == SslPolicyErrors.RemoteCertificateNameMismatch) // Chain validation successful
            {
                if (cert2 != null)
                {
                    if (this.serverCommonNames != null)
                    {
                        subjectName = cert2.Subject;
                        dnsName = cert2.GetNameInfo(X509NameType.DnsName, false);
                        dnsFromAlternativeName = cert2.GetNameInfo(X509NameType.DnsFromAlternativeName, false);

                        foreach (string commonName in this.serverCommonNames)
                        {
                            if (String.Equals(subjectName, commonName, StringComparison.OrdinalIgnoreCase) ||
                                String.Equals(dnsName, commonName, StringComparison.OrdinalIgnoreCase) ||
                                String.Equals(dnsFromAlternativeName, commonName, StringComparison.OrdinalIgnoreCase))
                            {
                                return true;
                            }
                        }
                    }
                    
                    if (this.serverThumbprints != null)
                    {
                        foreach (string thumbprint in this.serverThumbprints)
                        {
                            if (String.Equals(thumbprint, cert2.Thumbprint, StringComparison.OrdinalIgnoreCase))
                            {
                                return true;
                            }
                        }
                    }
                }
            }
            else if ((sslPolicyErrors & SslPolicyErrors.RemoteCertificateChainErrors) != 0) // self signed certs
            {
                if (cert2 != null &&
                    this.serverThumbprints != null)
                {
                    foreach (string thumbprint in this.serverThumbprints)
                    {
                        if (String.Equals(thumbprint, cert2.Thumbprint, StringComparison.OrdinalIgnoreCase))
                        {
                            return true;
                        }
                    }
                }
            }
            else if (sslPolicyErrors == SslPolicyErrors.None)
            {
                return true;
            }
            else
            {
                dSTSClientImpl.TraceSource.WriteWarning(
                    TraceType,
                    "CertificateHandler: Unknown SSL Policy error: {0}",
                    sslPolicyErrors);
                return false;
            }

            dSTSClientImpl.TraceSource.WriteWarning(
                TraceType,
                "CertificateHandler: Failed to validate the server certificate with SubjectName: {0}, DnsName: {1}, DnsAltName: {2}",
                subjectName,
                dnsName,
                dnsFromAlternativeName);

            return false;
        }
    }
}
