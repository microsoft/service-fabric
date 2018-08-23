// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if DotNetCoreClr
using Microsoft.IdentityModel.Tokens;
using System.IdentityModel.Tokens.Jwt;
using System.Net.Http;
using System.Xml.Linq;
#else
using System.IdentityModel.Metadata;
using System.IdentityModel.Tokens;
using System.ServiceModel.Security;
#endif

using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Security.Claims;
using System.Security.Cryptography.X509Certificates;
using System.Threading.Tasks;
using System.Threading;
using System.Xml;
using System;
using System.Fabric.Common.Tracing;

namespace System.Fabric.AzureActiveDirectory.Server
{
    internal class TokenSigningCertificateManager
    {
        private static readonly TimeSpan certRefreshBuffer = TimeSpan.Zero;

        private const string XmlNamespace = "http://www.w3.org/2000/xmlns/";
        private const string XmlDSig = "http://www.w3.org/2000/09/xmldsig#";
        private const string TraceType = "AAD.Server";

        private static readonly FabricEvents.ExtensionsEvents TraceSource = FabricEvents.ExtensionsEvents.GetEventSource(FabricEvents.Tasks.SystemFabric);

        private volatile Timer refreshTimer;
        private readonly object refreshTimerLock = new object();

        private string certEndpoint;
        private List<X509SecurityKey> certCache = new List<X509SecurityKey>();
        private readonly RwLock certCacheLock = new RwLock();

        private string issuer;
        private readonly RwLock issuerLock = new RwLock();

        public List<X509SecurityKey> CertCache
        {
            get
            {
                using (var r = certCacheLock.AcquireReadLock())
                {
                    return certCache;
                }
            }
            set
            {
                using (var w = certCacheLock.AcquireWriteLock())
                {
                    certCache = value;
                }
            }
        }

        public string Issuer
        {
            get
            {
                using (var r = issuerLock.AcquireReadLock())
                {
                    return issuer;
                }
            }
            set
            {
                using (var w = issuerLock.AcquireWriteLock())
                {
                    issuer = value;
                }
            }
        }

        public void Start(TimeSpan certRolloverInterval, string certEndpoint, string expectedIssuer)
        {
            if (refreshTimer == null)
            {
                lock (refreshTimerLock)
                {
                    if (refreshTimer == null)
                    {
                        this.certEndpoint = certEndpoint;
                        this.Issuer = expectedIssuer;

                        WriteInfo("Start refresh token-signing cert timer: rollover interval = {0}", certRolloverInterval);

                        RefreshCertCacheSync();

                        refreshTimer = new Timer((arg) => { RefreshCertCacheAsync().Wait(); }, null, certRolloverInterval, certRolloverInterval);
                                                
                        WriteInfo("Refresh token-signing cert timer scheduled.");
                    }
                }
            }
        }

        public List<X509SecurityKey> GetTokenSigningCerts(string certEndpoint)
        {
            var certs = GetValidCerts();

            if (certs.Count == 0)
            {
                // Attempt to fetch token-signing certs if all cached ones expire
                RefreshCertCacheSync();

                certs = GetValidCerts();

                if (certs.Count == 0)
                {
                    throw new SecurityTokenValidationException(string.Format(
                        "No token-signing certificates in cache: certEndpoint = {0}", 
                        certEndpoint));
                }
            }

            return certs;
        }

        private List<X509SecurityKey> GetValidCerts()
        {
            var certs = new List<X509SecurityKey>();

            certs.AddRange(this.CertCache.Where(t => !IsExpired(t)));

            return certs;
        }

        private static bool IsExpired(X509SecurityKey cert)
        {
            var now = DateTime.Now;

            return (now < cert.Certificate.NotBefore || now > cert.Certificate.NotAfter.Subtract(certRefreshBuffer));
        }

        private void RefreshCertCacheSync()
        {
            RefreshCertCacheAsync(true).Wait(); // throw exceptions in addition to logging
        }

        private async Task RefreshCertCacheAsync(bool shouldThrow = false)
        {
            try
            {
                WriteInfo("Start to refresh token-signing certificate cache.");

#if DotNetCoreClr
                var refreshedCerts = await FetchAndParseCerts_CoreCLRAsync();
#else
                var refreshedCerts = await FetchAndParseCerts_WIFAsync();
#endif

                if (refreshedCerts == null || refreshedCerts.Count == 0)
                {
                    throw new SecurityTokenValidationException(string.Format(CultureInfo.InvariantCulture,
                        "No refreshed certs found from {0}",
                        certEndpoint));
                }
                else
                {
                    CertCache = refreshedCerts;

                    WriteInfo("Refresh of token-signing certificate cache complete.");
                }
            }
            catch (Exception ex)
            {
                WriteException(ex);

                if (shouldThrow)
                {
                    throw;
                }
            }
        }

#if DotNetCoreClr
        private readonly HttpClient httpClient = new HttpClient();

        private async Task<List<X509SecurityKey>> FetchAndParseCerts_CoreCLRAsync()
        {
            var results = new List<X509SecurityKey>();
            
            var response = await httpClient.GetAsync(certEndpoint, HttpCompletionOption.ResponseContentRead);

            if (!response.IsSuccessStatusCode)
            {
                throw new SecurityTokenValidationException(string.Format(CultureInfo.InvariantCulture,
                    "Failed to fetch metadata from {0}: status={1} details={2}",
                    certEndpoint,
                    response.StatusCode,
                    response.ReasonPhrase));
            }

            var certElements = new List<XElement>();

            using (var stream = await response.Content.ReadAsStreamAsync())
            {
                var xml = XDocument.Load(stream);

                var rootNamespace = xml.Root.Attribute("xmlns").Value;

                foreach (var roleDescriptor in xml.Root.Descendants(string.Format("{{{0}}}RoleDescriptor", rootNamespace)))
                {
                    var typeNamespace = roleDescriptor.Attribute(string.Format("{{{0}}}xsi", XmlNamespace)).Value;

                    foreach (var attr in roleDescriptor.Attributes(string.Format("{{{0}}}type", typeNamespace)))
                    {
                        if (attr.Value.Equals("fed:SecurityTokenServiceType"))
                        {
                            certElements.AddRange(roleDescriptor.Descendants(string.Format("{{{0}}}X509Certificate", XmlDSig)));
                        }
                    }
                }

                var entityID = xml.Root.Attribute("entityID").Value;
                if (!string.IsNullOrEmpty(issuer))
                {
                    this.Issuer = entityID;

                    WriteInfo("Updated issuer from federation metadata: {0}", this.Issuer);
                }
                else
                {
                    WriteWarning("Issuer can't be extracted from federation metadata, keeping config value.");
                }
            }

            results.AddRange(certElements.Select(el => new X509SecurityKey(new X509Certificate2(Convert.FromBase64String(el.Value)))));

            return results;
        }

#else // !DotNetCoreClr

        // 
        // The convenience methods used here to parse WS-Federation documents are 
        // provided by Windows Identity Foundation. As of version 6.1.7600.16394, 
        // the Microsoft.IdentityModel NuGet package is not .NET Core compatible.
        //
#pragma warning disable 1998 // No actual async calls
        private async Task<List<X509SecurityKey>> FetchAndParseCerts_WIFAsync()
        {
            var refreshedCerts = new List<X509SecurityKey>();
            var xmlResolver = new XmlSecureResolver(new XmlUrlResolver(), certEndpoint);
            var xmlSettings = new XmlReaderSettings() { XmlResolver = xmlResolver };

            using (var metadataReader = XmlReader.Create(certEndpoint, xmlSettings))
            {
                var serializer = new MetadataSerializer()
                {
                    // Token-signing certificate used by AAD is self-signed to facilitate easy rollover.
                    // The certificate endpoint is authenticated, so no need to validate the token-signing cert.
                    //
                    CertificateValidationMode = X509CertificateValidationMode.None
                };

                var metadata = serializer.ReadMetadata(metadataReader) as EntityDescriptor;
                if (metadata == null)
                {
                    throw new SecurityTokenValidationException(
                        string.Format(CultureInfo.InvariantCulture,
                            "Invalid Federation Metadata document from {0}", 
                            certEndpoint));
                }

                string entityID = metadata.EntityId.Id;
                if (!string.IsNullOrEmpty(entityID))
                {
                    this.Issuer = entityID;

                    WriteInfo("Updated issuer from federation metadata: {0}", this.Issuer);
                }
                else
                {
                    WriteWarning("Issuer can't be extracted from federation metadata, keeping config value.");
                }

                var stsd = metadata.RoleDescriptors.OfType<SecurityTokenServiceDescriptor>().FirstOrDefault();
                if (stsd == null)
                {
                    throw new SecurityTokenValidationException(
                        string.Format(CultureInfo.InvariantCulture,
                            "There is no RoleDescriptor of type SecurityTokenServiceType in the metadata from {0}",
                            certEndpoint));
                }

                var x509DataClauses = stsd.Keys.Where(
                    key => key.KeyInfo != null && (key.Use == KeyType.Signing || key.Use == KeyType.Unspecified)).
                        Select(key => key.KeyInfo.OfType<X509RawDataKeyIdentifierClause>().First());

                refreshedCerts.AddRange(x509DataClauses.Select(cert => new X509SecurityKey(new X509Certificate2(cert.GetX509RawData()))));
            } // using XmlReader.Create

            return refreshedCerts;
        }
#pragma warning restore 1998
#endif

        private static void WriteInfo(string format, params object[] args)
        {
            TraceSource.WriteInfo(TraceType, format, args);
        }

        private static void WriteWarning(string format, params object[] args)
        {
            TraceSource.WriteWarning(TraceType, format, args);
        }

        private static void WriteException(Exception ex)
        {
            TraceSource.WriteError(TraceType, "{0}", ex);
        }
    }
}
