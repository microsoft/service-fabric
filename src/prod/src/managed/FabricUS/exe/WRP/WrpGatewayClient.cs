// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Linq;
    using System.Net.Http;
    using System.Security.Cryptography.X509Certificates;
    using System.Threading;
    using System.Threading.Tasks;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using Newtonsoft.Json.Serialization;  
    using Constant = UpgradeService.Constants;

    internal class WrpGatewayClient
    {
        internal static readonly JsonSerializerSettings SerializerSettings = new JsonSerializerSettings()
        {
            NullValueHandling = NullValueHandling.Ignore,
            ContractResolver = new CamelCasePropertyNamesContractResolver(),
            Converters = new List<JsonConverter>
            {
                new StringEnumConverter { CamelCaseText = false },
            }
        };

        protected readonly IConfigStore ConfigStore = null;
        protected readonly string ConfigSectionName = null;
        private TraceType traceType = new TraceType("WrpGatewayClientBase");
        private readonly string clusterId;

        internal WrpGatewayClient(Uri baseUrl, string clusterId, IConfigStore configStore, string configSectionName)
        {
            baseUrl.ThrowIfNull("baseUrl");

            this.BaseUrl = baseUrl;
            this.clusterId = clusterId;
            this.ConfigStore = configStore;
            this.ConfigSectionName = configSectionName;
        }

        protected virtual TraceType TraceType
        {
            get { return this.traceType; }
        }

        protected Uri BaseUrl { get; set; }

        internal async Task<TResponse> GetWrpServiceResponseAsync<TRequest, TResponse>(
            Uri requestUri,
            HttpMethod httpMethod,
            TRequest request,
            CancellationToken token)
        {
            var requestBody = JsonConvert.SerializeObject(request, SerializerSettings);
            Trace.WriteInfo(TraceType, "PaasCoordinator:RequestBody: {0}", requestBody);
            string responseString = null;

            var clientCertificates = this.GetCertificates();

            responseString = await RestClientHelper.InvokeWithCertificatesAsync(requestUri, httpMethod, requestBody, clientCertificates, clusterId, token);            

            Trace.WriteInfo(TraceType, "PaasCoordinator:Response: {0}", responseString);
            return string.IsNullOrWhiteSpace(responseString) ? default(TResponse) : JsonConvert.DeserializeObject<TResponse>(responseString, SerializerSettings);
        }

        #region Cert 

        protected List<X509Certificate2> GetCertificates()
        {
            var storeName = this.ConfigStore.ReadUnencryptedString(this.ConfigSectionName, Constant.ConfigurationSection.X509StoreName);
            var storeLocation = this.ConfigStore.ReadUnencryptedString(this.ConfigSectionName, Constant.ConfigurationSection.X509StoreLocation);
            var findTypeStr = this.ConfigStore.ReadUnencryptedString(this.ConfigSectionName, Constant.ConfigurationSection.X509FindType);
            var findValue = this.ConfigStore.ReadUnencryptedString(this.ConfigSectionName, Constant.ConfigurationSection.X509FindValue);
            var secondaryFindValue = this.ConfigStore.ReadUnencryptedString(this.ConfigSectionName, Constant.ConfigurationSection.X509SecondaryFindValue);

            var certificates = new List<X509Certificate2>();
            if (string.IsNullOrWhiteSpace(storeName) ||
                string.IsNullOrWhiteSpace(storeLocation) ||
                string.IsNullOrWhiteSpace(findTypeStr) ||
                string.IsNullOrWhiteSpace(findValue))
            {
                Trace.WriteNoise(TraceType, "PaasCoordinator: No certificate configured");

                // Fall back to 'anonymous' self-signed certificate so that WRP's web host
                // does not reject the connection for lacking a cert. This certificate is
                // installed by the ServiceFabric extension when no cluster security is configured.
                // It is not trusted in any way, and only works when cert security is disabled for
                // the cluster resource.

                // CoreCLR Does not support StoreLocation.LocalMachine, hence using StoreLocation.CurrentUser
#if DotNetCoreClrLinux
                var store = new X509Store(StoreName.My, StoreLocation.CurrentUser);
#else
                var store = new X509Store(StoreName.My, StoreLocation.LocalMachine);
#endif
                try
                {
                    store.Open(OpenFlags.ReadOnly);

                    var certCollections = store.Certificates.Find(
                        X509FindType.FindBySubjectDistinguishedName,
                        Constant.PaasCoordinator.AnonymousCertDistiguishedName,
                        false /*load self-signed cert*/);

                    if (certCollections.Count > 0)
                    {
                        certificates.Add(certCollections[0]);
                    }

                }
                finally
                {
#if DotNetCoreClrLinux
                    store.Dispose();
#else
                    store.Close();
#endif
                }
            }
            else
            {
                X509FindType findType;
                if (!Enum.TryParse(findTypeStr, out findType))
                {
                    throw new ArgumentException("X509FindType in cluster manifest is not of proper enum value");
                }

                StoreLocation location;
                if (!Enum.TryParse(storeLocation, out location))
                {
                    throw new ArgumentException("X509FindType in cluster manifest is not of proper enum value");
                }

                // SFRP is generating ClusterManifests setting this value to StoreLocation.LocalMachine
                // Using hard-coded value of StoreLocation.CurrentUser for TP9 till SFRP is updated to set this value appropriately.
#if DotNetCoreClrLinux
               var store = new X509Store(storeName, StoreLocation.CurrentUser);
#else
               var store = new X509Store(storeName, location);
#endif
                try
                {
                    store.Open(OpenFlags.ReadOnly);

                    var findValues = new List<string>() { findValue };
                    if (!string.IsNullOrEmpty(secondaryFindValue))
                    {
                        findValues.Add(secondaryFindValue);
                    }

                    foreach (var value in findValues)
                    {
                        //// TODO native code try findvalue as complete subject name first, if no match, then as common name. 
                        //// .NET findbysubjectname is just a substring match
                        var certCollections = store.Certificates.Find(findType, value, false /*load self-signed cert*/);
                        if (certCollections.Count > 0)
                        {
                            Trace.WriteInfo(
                                TraceType,
                                "Certificates found: StoreName {0}, StoreLocation {1}, FindType {2}, FindValue {3}, Count {4}",
                                storeName,
                                storeLocation,
                                findType,
                                value,
                                certCollections.Count);

                            for (int i = 0; i < certCollections.Count; ++i)
                            {
                                Trace.WriteInfo(
                                    TraceType,
                                    "Certificate[{0}]: Thumbprint {1}, NotBefore {2}, NotAfter {3}, Subject {4}",
                                    i,                                    
                                    certCollections[i].Thumbprint,
                                    certCollections[i].NotBefore,
                                    certCollections[i].NotAfter,
                                    certCollections[i].Subject);
                            }

                            // Select a cert that is not expired and is the newest created.
                            // This should make it predictible if certificate is compromised and it needs to be replaced with a newer one.
                            var now = DateTime.UtcNow;
                            var selectedCert = certCollections.Cast<X509Certificate2>()
                                .Where(c => c.NotBefore <= now && now <= c.NotAfter)
                                .OrderByDescending(c => c.NotBefore)
                                .FirstOrDefault();

                            if (selectedCert != null)
                            {
                                Trace.WriteInfo(
                                    TraceType,
                                    "Selected certificate: Thumbprint {0}, NotBefore {1}, NotAfter {2}, Subject {3}",
                                    selectedCert.Thumbprint,
                                    selectedCert.NotBefore,
                                    selectedCert.NotAfter,
                                    selectedCert.Subject);

                                certificates.Add(selectedCert);
                            }
                            else
                            {
                                Trace.WriteInfo(
                                    TraceType,
                                    "No valid certificate found: StoreName {0}, StoreLocation {1}, FindType {2}, FindValue {3}, Count {4}",
                                    storeName,
                                    storeLocation,
                                    findType,
                                    value,
                                    certCollections.Count);
                            }
                        }
                        else
                        {
                            Trace.WriteWarning(
                                TraceType,
                                "Certificate not found: StoreName {0}, StoreLocation {1}, FindType {2}, FindValue {3}",
                                storeName,
                                storeLocation,
                                findType,
                                value);
                        }
                    }
                }
                finally
                {
#if DotNetCoreClrLinux
                    store.Dispose();
#else
                    store.Close();
#endif
                }

                if (!certificates.Any())
                {
                    throw new InvalidOperationException("Could not load primary and secondary certificate");
                }
            }

            return certificates;
        }
        #endregion
    }
}