// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Security.Principal;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Azure.KeyVault;
using Microsoft.IdentityModel.Clients.ActiveDirectory;

namespace KeyVault.Wrapper
{
    static class CertificateStore
    {
        private static List<StoreLocation> defaultStoreLocationsToSearch = new List<StoreLocation>
        {
            StoreLocation.LocalMachine,
            StoreLocation.CurrentUser
        };

        public static X509Certificate2 GetCertificateByThumbprint(string certThumbprint)
        {
            foreach (StoreLocation current in CertificateStore.defaultStoreLocationsToSearch)
            {
                X509Certificate2 certificateByThumbprint = CertificateStore.GetCertificateByThumbprint(certThumbprint, current);
                if (certificateByThumbprint != null)
                {
                    return certificateByThumbprint;
                }
            }
            return null;
        }

        private static X509Certificate2 GetCertificateByThumbprint(string certThumbprint, StoreLocation storeLocation)
        {
            X509Certificate2 result = null;
            X509Store x509Store = new X509Store(StoreName.My, storeLocation);
            try
            {
                x509Store.Open(OpenFlags.IncludeArchived);
                X509Certificate2Collection x509Certificate2Collection = x509Store.Certificates.Find(X509FindType.FindByThumbprint, certThumbprint, false);
                if (x509Certificate2Collection != null && x509Certificate2Collection.Count != 0)
                {
                    result = x509Certificate2Collection[0];
                }
            }
            finally
            {
                x509Store.Close();
            }
            return result;
        }
    }

    class KeyVaultWrapper
    {
        private static Dictionary<string, object> processWideSyncDict = new Dictionary<string, object>();

        private object processWideSyncForCert;

        private bool threadInCriticalRegion;

        private ClientAssertionCertificate assertionCert;

        private KeyVaultClient kvc;

        private TokenCache tokenCache;

        private const string TestInfraClientId = "33e3af95-cf38-49be-9db9-909bb5b3cb2e";

        private const string TestInfraClientCertThumbprint = "6108D2170756888910B43787F0FE8C6DD9162FE7";

        private const string TestInfraVaultUri = "https://TestInfraKeyVault.vault.azure.net";

        private static readonly Random passwordRNG = new Random();

        public KeyVaultWrapper(string clientId, string appCertThumbprint) : this(clientId, CertificateStore.GetCertificateByThumbprint(appCertThumbprint))
        {
        }

        public KeyVaultWrapper(string clientId, X509Certificate2 cert)
        {
            this.assertionCert = new ClientAssertionCertificate(clientId, cert);
            this.kvc = new KeyVaultClient(new KeyVaultClient.AuthenticationCallback(this.GetAccessToken));
            this.tokenCache = new TokenCache();
            Dictionary<string, object> obj = KeyVaultWrapper.processWideSyncDict;
            lock (obj)
            {
                this.threadInCriticalRegion = false;
                if (!KeyVaultWrapper.processWideSyncDict.TryGetValue(cert.Thumbprint, out this.processWideSyncForCert))
                {
                    this.processWideSyncForCert = new object();
                    KeyVaultWrapper.processWideSyncDict.Add(cert.Thumbprint, this.processWideSyncForCert);
                }
            }
        }

        public static KeyVaultWrapper GetTestInfraKeyVaultWrapper()
        {
            return new KeyVaultWrapper("33e3af95-cf38-49be-9db9-909bb5b3cb2e", "6108D2170756888910B43787F0FE8C6DD9162FE7");
        }

        public static string GetTestInfraKeyVaultUri()
        {
            return "https://TestInfraKeyVault.vault.azure.net";
        }

        public string GetSecret(string vaultUri, string secretName, int timeoutMs = -1)
        {
            var task = this.kvc.GetSecretAsync(vaultUri, secretName);
            if (task.Wait(timeoutMs))
            {
                return task.Result.Value;
            }
            throw new TimeoutException(string.Format("Failed to retrieve secret {0} from vault {1} within the allowable timeout {2} in ms", secretName, vaultUri, timeoutMs));
        }

        private async Task<string> GetAccessToken(string authority, string resource, string scope)
        {
            AuthenticationContext authenticationContext = new AuthenticationContext(authority, this.tokenCache);
            bool flag = false;
            string accessToken;
            try
            {
                object obj = this.processWideSyncForCert;
                bool flag2 = false;
                try
                {
                    Monitor.Enter(obj, ref flag2);
                    while (this.threadInCriticalRegion)
                    {
                        Monitor.Wait(this.processWideSyncForCert);
                    }
                    this.threadInCriticalRegion = true;
                }
                finally
                {
                    if (flag2)
                    {
                        Monitor.Exit(obj);
                    }
                }
                var authResult = await authenticationContext.AcquireTokenAsync(resource, this.assertionCert);
                accessToken = authResult.AccessToken;
            }
            catch (AggregateException ex)
            {
                try
                {
                    IEnumerator<Exception> enumerator = ex.Flatten().InnerExceptions.GetEnumerator();
                    try
                    {
                        while (enumerator.MoveNext())
                        {
                            Exception current = enumerator.Current;
                            if (current is CryptographicException)
                            {
                                CryptographicException ex2 = (CryptographicException)current;
                                if (ex2.Message.Contains("KeySet does not exist"))
                                {
                                    throw ex2;
                                }
                            }
                        }
                    }
                    finally
                    {
                        if (enumerator != null)
                        {
                            enumerator.Dispose();
                        }
                    }
                    throw;
                }
                finally
                {
                    object obj = this.processWideSyncForCert;
                    bool flag2 = false;
                    try
                    {
                        Monitor.Enter(obj, ref flag2);
                        if (this.threadInCriticalRegion && !flag)
                        {
                            this.threadInCriticalRegion = false;
                            flag = true;
                            Monitor.PulseAll(this.processWideSyncForCert);
                        }
                    }
                    finally
                    {
                      Monitor.Exit(obj);
                    }
                }
            }
            finally
            {
                object obj = this.processWideSyncForCert;
                bool flag2 = false;
                try
                {
                    Monitor.Enter(obj, ref flag2);
                    if (this.threadInCriticalRegion && !flag)
                    {
                        this.threadInCriticalRegion = false;
                        flag = true;
                        Monitor.PulseAll(this.processWideSyncForCert);
                    }
                }
                finally
                {
                  Monitor.Exit(obj);
                }
            }
            return accessToken;
        }
    }
}
