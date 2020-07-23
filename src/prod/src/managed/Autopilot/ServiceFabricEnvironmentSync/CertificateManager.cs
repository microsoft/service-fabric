// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Security.AccessControl;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using System.Security.Principal;

    using Microsoft.Search.Autopilot;    

    internal class EnvironmentCertificateManager
    {
        private string applicationDirectory;

        /*
         *  Note that for both AP security model and SF security model the security boundary is the set of machines. 
         *  Within machine boundary, the private keys are available directly from the certificate store and AP secret store would allow decryption of encrypted secrets.
         *  
         *  Since environment sync processes run within the security boundary, certificate manager would load and maintain the private key & public key certificates in memory.
         *  This would increase reliability in face of AP Secret Store unavailability.
         */
        private Dictionary<StoreName, Dictionary<string, EnvironmentCertificate>> environmentCertificates;

        public EnvironmentCertificateManager(string applicationDirectory)
        {
            this.applicationDirectory = applicationDirectory;

            this.environmentCertificates = new Dictionary<StoreName, Dictionary<string, EnvironmentCertificate>>();
            this.environmentCertificates[StoreName.My] = new Dictionary<string, EnvironmentCertificate>();
            this.environmentCertificates[StoreName.CertificateAuthority] = new Dictionary<string, EnvironmentCertificate>();
        }
        
        public bool EnsureEnvironmentCertificatesPresence()
        {                                                    
            foreach (var storeEnvironmentCertificates in this.environmentCertificates)
            {                
                StoreName storeName = storeEnvironmentCertificates.Key;

                TextLogger.LogInfo("Ensuring all environment certificates for certificate store {0}\\{1} are present in local environment.", StoreLocation.LocalMachine, storeName);

                int installedEnvironmentCertificates = 0;                
                X509Store store = null;                    
                try
                {
                    store = new X509Store(storeName, StoreLocation.LocalMachine);

                    store.Open(OpenFlags.OpenExistingOnly | OpenFlags.ReadWrite);
                        
                    foreach (var environmentCertificateEntry in storeEnvironmentCertificates.Value)
                    {
                        bool environmentCertificateInstalled = false;
                        X509Certificate2 environmentCertificate = environmentCertificateEntry.Value.Certificate;
                        if (store.Certificates != null)
                        {
                            X509Certificate2Collection certificateCollection = store.Certificates.Find(X509FindType.FindByThumbprint, environmentCertificate.Thumbprint, false);

                            environmentCertificateInstalled = certificateCollection.Count > 0;
                        }

                        if (!environmentCertificateInstalled)
                        {
                            TextLogger.LogInfo(
                                "{0} certificate with thumbprint {1}, subject {2} is being installed in certificate store {3}\\{4}.",
                                environmentCertificate.HasPrivateKey ? "Private key" : "Public key",
                                environmentCertificate.Thumbprint,
                                environmentCertificate.Subject,
                                StoreLocation.LocalMachine,
                                storeName);

                            store.Add(environmentCertificate);

                            if (environmentCertificate.HasPrivateKey)
                            {
                                TextLogger.LogInfo(
                                "Private key certificate with thumbprint {0}, subject {1} is being ACLed.",
                                environmentCertificate.Thumbprint,
                                environmentCertificate.Subject);

                                // ACL private key part of a private key certificate to network service.                            
                                RSACryptoServiceProvider cryptoServiceProvider = environmentCertificate.PrivateKey as RSACryptoServiceProvider;

                                CspParameters cspParameters = new CspParameters(cryptoServiceProvider.CspKeyContainerInfo.ProviderType, cryptoServiceProvider.CspKeyContainerInfo.ProviderName, cryptoServiceProvider.CspKeyContainerInfo.KeyContainerName)
                                {
                                    Flags = CspProviderFlags.UseExistingKey | CspProviderFlags.UseMachineKeyStore,

                                    CryptoKeySecurity = cryptoServiceProvider.CspKeyContainerInfo.CryptoKeySecurity
                                };

                                cspParameters.CryptoKeySecurity.AddAccessRule(new CryptoKeyAccessRule(new SecurityIdentifier(WellKnownSidType.NetworkServiceSid, null), CryptoKeyRights.FullControl | CryptoKeyRights.GenericRead, AccessControlType.Allow));

                                using (RSACryptoServiceProvider updatedCryptoServiceProvider = new RSACryptoServiceProvider(cspParameters))
                                {
                                    // Create a new RSACryptoServiceProvider with updated ACL rules to apply ACL changes.
                                }
                            }

                            installedEnvironmentCertificates++;
                        }
                    }                        
                }
                catch (Exception e)
                {
                    TextLogger.LogError("Failed to ensure all environment certificates for certificate store {0}\\{1} are present in local environment : {2}", StoreLocation.LocalMachine, storeName, e);

                    return false;
                }
                finally
                {
                    if (store != null)
                    {
                        store.Close();
                    }
                }

                TextLogger.LogInfo("Certificate store {0}\\{1} : Successfully installed {2} environment certificates. All {3} environment certificates referred to in current configurations for the certificate store have been installed.", StoreLocation.LocalMachine, storeName, installedEnvironmentCertificates, storeEnvironmentCertificates.Value.Count);
            }                
               
            return true;            
        }

        public bool GetEnvironmentCertificatesFromCurrentConfigurations()
        {
            return GetEnvironmentCertificatesFromCurrentConfigurations(StoreName.My) && GetEnvironmentCertificatesFromCurrentConfigurations(StoreName.CertificateAuthority);
        }

        /*
         * Private key certificate files (each private key certificate has an encrypted private key file (.pfx.encr) and an encrypted password file (.pfx.password.encr) ) as well as public key certificate files (i.e. cer files) are part of ServiceFabricEnvironmentSync application packages.
         * 
         * All certificates to be installed by ServiceFabricEnvironmentSync need to be listed in the form of "CertificateFriendlyName=CertificateDataDirectory" in certificate section of ServiceFabricEnvironmentSync configuration file.
         *      1) CertificateDataDirectory refers to a relative path from application package root directory. 
         *      2) For a public key certificate, CertificateFriendlyName.cer should be present in CertificateDataDirectory.
         *      3) For a private key certificate, CertificateFriendlyName.pfx.encr + CertificateFriendlyName.password.encr should be present in CertificateDataDirectory.
         * 
         * All certificates listed would be installed in LocalMachine\My or LocalMachine\CA certificate store. 
         * 
         * Sample certificate configuration sections
         * [ServiceFabricEnvironmentSync.Certificate.LocalMachine.My]
            certA=data\certificates
            certB=data\certificates            

           [ServiceFabricEnvironmentSync.Certificate.LocalMachine.CertificateAuthority]
            certC=data\certificates
            certD=data\certificates            
         */
        public bool GetEnvironmentCertificatesFromCurrentConfigurations(StoreName storeName)
        {
            try
            {
                if (storeName != StoreName.My && storeName != StoreName.CertificateAuthority)
                {
                    TextLogger.LogError("Certificate store name {0} is not supported. Supported store names : {1}, {2}.", storeName, StoreName.My, StoreName.CertificateAuthority);

                    return false;
                }

                string certificateListConfigurationSectionName = string.Format(CultureInfo.InvariantCulture, StringConstants.CertificateListConfigurationSectionNameTemplate, storeName);

                IConfiguration configuration = APConfiguration.GetConfiguration();

                if (configuration != null && configuration.SectionExists(certificateListConfigurationSectionName))
                {
                    string[] certificateFriendlyNames = configuration.GetSectionKeys(certificateListConfigurationSectionName);

                    if (certificateFriendlyNames != null)
                    {
                        foreach (string certificateFriendlyName in certificateFriendlyNames)
                        {
                            string certificateDataDirectoryName = configuration.GetStringValue(certificateListConfigurationSectionName, certificateFriendlyName);

                            string certificateDataDirectory = Path.Combine(this.applicationDirectory, certificateDataDirectoryName);

                            string encryptedPrivateKeyFile = Path.Combine(
                                certificateDataDirectory,
                                string.Format(
                                    CultureInfo.InvariantCulture,
                                    StringConstants.EncryptedPrivateKeyFileNameTemplate,
                                    certificateFriendlyName));

                            bool encryptedPrivateKeyFileExists = File.Exists(encryptedPrivateKeyFile);

                            string encryptedPasswordFile = Path.Combine(
                                certificateDataDirectory,
                                string.Format(
                                    CultureInfo.InvariantCulture,
                                    StringConstants.EncryptedPasswordFileNameTemplate,
                                    certificateFriendlyName));

                            bool encryptedPasswordFileExists = File.Exists(encryptedPasswordFile);

                            string publicKeyFile = Path.Combine(
                                certificateDataDirectory,
                                string.Format(
                                    CultureInfo.InvariantCulture,
                                    StringConstants.PublicKeyFileNameTemplate,
                                    certificateFriendlyName));

                            bool publicKeyFileExists = File.Exists(publicKeyFile);

                            // TODO: report health                               
                            if (!encryptedPrivateKeyFileExists && !encryptedPasswordFileExists && !publicKeyFileExists)
                            {
                                TextLogger.LogError("Neither encrypted private key & encrypted password files nor public key file exist for certificate {0} in certificate data directory {1}. A certificate to be installed needs to be either a public key certificate or a private key certificate.", certificateFriendlyName, certificateDataDirectory);

                                return false;
                            }

                            if ((encryptedPrivateKeyFileExists || encryptedPasswordFileExists) && publicKeyFileExists)
                            {
                                TextLogger.LogError("Both encrypted private key & encrypted password files and public key file exist for certificate {0} in certificate data directory {1}. A certificate to be installed needs to be either a public key certificate or a private key certificate but not both.", certificateFriendlyName, certificateDataDirectory);

                                return false;
                            }

                            if (encryptedPrivateKeyFileExists ^ encryptedPasswordFileExists)
                            {
                                TextLogger.LogError("Only one of encrypted private key file and encrypted password file exist for certificate {0} in certificate data directory {1}. Both are required to install a private key certificate.", certificateFriendlyName, certificateDataDirectory);

                                return false;
                            }

                            if (publicKeyFileExists)
                            {
                                PublicKeyCertificate publicKeyCertificate;
                                if (!PublicKeyCertificate.TryLoadFromFile(publicKeyFile, out publicKeyCertificate))
                                {
                                    return false;
                                }                                

                                if (this.environmentCertificates[storeName].ContainsKey(publicKeyCertificate.Thumbprint) && this.environmentCertificates[storeName][publicKeyCertificate.Thumbprint].HasPrivateKey)
                                {
                                    TextLogger.LogError("Certificate with thumbprint {0}, subject {1} are requested to be installed as both public key certificate and private key certificate in configuration under different entries.", publicKeyCertificate.Thumbprint, publicKeyCertificate.Certificate.Subject);

                                    return false;
                                }

                                this.environmentCertificates[storeName][publicKeyCertificate.Thumbprint] = publicKeyCertificate;

                                TextLogger.LogInfo("Public key certificate with thumbprint {0}, subject {1} needs to be present in certificate store {2}\\{3} in local environment based on current configurations.", publicKeyCertificate.Thumbprint, publicKeyCertificate.Certificate.Subject, StoreLocation.LocalMachine, storeName);
                            }
                            else
                            {
                                if (storeName == StoreName.CertificateAuthority)
                                {
                                    TextLogger.LogError("Private key certificate {0} cannot be installed in certificate store with name {1}.", certificateFriendlyName, storeName);

                                    return false;
                                }

                                PrivateKeyCertificate privateKeyCertificate;
                                if (!PrivateKeyCertificate.TryLoadFromFile(encryptedPrivateKeyFile, encryptedPasswordFile, out privateKeyCertificate))
                                {
                                    return false;
                                }                                

                                if (this.environmentCertificates[storeName].ContainsKey(privateKeyCertificate.Thumbprint) && !this.environmentCertificates[storeName][privateKeyCertificate.Thumbprint].HasPrivateKey)
                                {
                                    TextLogger.LogError("Certificate with thumbprint {0}, subject {1} are requested to be installed as both public key certificate and private key certificate in configuration under different entries.", privateKeyCertificate.Thumbprint, privateKeyCertificate.Certificate.Subject);

                                    return false;
                                }

                                this.environmentCertificates[storeName][privateKeyCertificate.Thumbprint] = privateKeyCertificate;

                                TextLogger.LogInfo("Private key certificate with thumbprint {0}, subject {1} needs to be present in certificate store {2}\\{3} in local environment based on current configurations.", privateKeyCertificate.Thumbprint, privateKeyCertificate.Certificate.Subject, StoreLocation.LocalMachine, storeName);
                            }
                        }
                    }
                }

                return true;
            }
            catch (Exception e)
            {
                TextLogger.LogError("Failed to get environment certificates based on current configurations : {0}", e);

                return false;
            }
        }        
    }
}