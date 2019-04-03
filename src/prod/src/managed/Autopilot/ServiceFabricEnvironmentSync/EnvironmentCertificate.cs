// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    using System;
    using System.IO;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;

    using Microsoft.Search.Autopilot.Security;

    internal abstract class EnvironmentCertificate
    {
        protected EnvironmentCertificate(X509Certificate2 certificate)
        {
            this.Certificate = certificate;
        }

        public bool HasPrivateKey { get { return this.Certificate != null ? this.Certificate.HasPrivateKey : false; } }

        public string Thumbprint { get { return this.Certificate != null ? this.Certificate.Thumbprint : string.Empty; } }

        public X509Certificate2 Certificate { get; private set; }
    }

    internal class PrivateKeyCertificate : EnvironmentCertificate
    {               
        private PrivateKeyCertificate(X509Certificate2 certificate) : base(certificate)
        {

        }

        public static bool TryLoadFromFile(string encryptedPrivateKeyFile, string encryptedPasswordFile, out PrivateKeyCertificate privateKeyCertificate)
        {            
            privateKeyCertificate = null;
            
            try
            {                
                byte[] encryptedPrivateKeyBytes = File.ReadAllBytes(encryptedPrivateKeyFile);
                byte[] encryptedPasswordBytes = File.ReadAllBytes(encryptedPasswordFile);                
                                
                byte[] decryptedPrivateKeyBlob;
                byte[] decryptedPasswordBlob;
                
                using (ApSecretProtection protect = new ApSecretProtection(ConfigurationManager.EnableSecretStorePersistentCaching))
                {
                    decryptedPrivateKeyBlob = protect.Decrypt(encryptedPrivateKeyBytes);
                    decryptedPasswordBlob = protect.Decrypt(encryptedPasswordBytes);
                }
            
                string decryptedPassword;
                using (MemoryStream stream = new MemoryStream(decryptedPasswordBlob))
                using (StreamReader reader = new StreamReader(stream))
                {
                    decryptedPassword = reader.ReadToEnd().Trim();
                }

                X509Certificate2 certificate = new X509Certificate2(decryptedPrivateKeyBlob, decryptedPassword, X509KeyStorageFlags.PersistKeySet | X509KeyStorageFlags.MachineKeySet);

                if (!certificate.HasPrivateKey)
                {
                    TextLogger.LogError("Certificate from encrypted private key file {0} should present a private key.", encryptedPrivateKeyFile);

                    return false;
                }

                if (!(certificate.PrivateKey is RSACryptoServiceProvider))
                {
                    TextLogger.LogError("Private key certificate from encrypted private key file {0} does not use a RSA based asymmetric algorithm implementation.", encryptedPrivateKeyFile);

                    return false;
                }

                privateKeyCertificate = new PrivateKeyCertificate(certificate);                                                

                return true;
            }
            catch (Exception e)
            {
                TextLogger.LogError("Failed to load private key certificate from encrypted private key file {0} / encrypted password file {1} : {2}", encryptedPrivateKeyFile, encryptedPasswordFile, e);

                return false;
            }            
        }
    }

    internal class PublicKeyCertificate : EnvironmentCertificate
    {
        private PublicKeyCertificate(X509Certificate2 certificate) : base(certificate)
        {

        }

        public static bool TryLoadFromFile(string publicKeyFile, out PublicKeyCertificate publicKeyCertificate)
        {        
            publicKeyCertificate = null;

            try
            {
                X509Certificate2 certificate = new X509Certificate2(publicKeyFile);

                if (certificate.HasPrivateKey)
                {
                    TextLogger.LogError("Certificate from public key file {0} should not present a private key.", publicKeyFile);

                    return false;
                }

                publicKeyCertificate = new PublicKeyCertificate(certificate);                                                      

                return true;
            }
            catch (Exception e)
            {
                TextLogger.LogError("Failed to load public key certificate from public key file {0} : {1}", publicKeyFile, e);

                return false;
            }            
        }
    }
}