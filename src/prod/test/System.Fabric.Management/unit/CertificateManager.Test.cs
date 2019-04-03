// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.CertificateManager.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.IO;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using WEX.TestExecution;
    using System.Security.Cryptography.X509Certificates;

    /// <summary>
    /// This is the class that tests the validation of the files that we deploy.
    /// </summary>
    [TestClass]
    internal class CertificateManagerTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        //Pass NULL for DNS, DateTime.MinValue for no datetime
        public void VerifyGenerateCertDefaultParam()
        {
            string subName = "TestCert_1";
            string store = "ROOT";
            StoreLocation storeLocation = StoreLocation.LocalMachine;
            DateTime dt = DateTime.MinValue;
            CertificateManager.GenerateSelfSignedCertAndImportToStore(subName, store, storeLocation, null, dt);
            bool result = VerifyCertExistsAndDelete(subName, store, storeLocation);
            Verify.IsTrue(result, string.Format("Generate Cert with parameters: {0}, {1}, {2}", subName, store, storeLocation));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void VerifyGenerateCertWithDNS()
        {
            var subName = "TestCert_2";
            var store = "ROOT";
            StoreLocation storeLocation = StoreLocation.LocalMachine;
            var dns = "client.sf.lrc.com";
            DateTime dt = DateTime.MinValue;
            CertificateManager.GenerateSelfSignedCertAndImportToStore(subName, store, storeLocation, dns, dt);
            bool result = VerifyCertExistsAndDelete(subName, store, storeLocation);
            Verify.IsTrue(result, string.Format("Generate Cert with parameters: {0}, {1}, {2}, {3}", subName, store, storeLocation, dns));
        }
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void VerifyGenerateCertWithDate()
        {
            var subName = "TestCert_3";
            var store = "ROOT";
            StoreLocation storeLocation = StoreLocation.LocalMachine;
            DateTime dt = new DateTime(2017, 1, 20);
            CertificateManager.GenerateSelfSignedCertAndImportToStore(subName, store, storeLocation, null, dt);
            bool result = VerifyCertExistsAndDelete(subName, store, storeLocation);
            Verify.IsTrue(result, string.Format("Generate Cert with parameters: {0}, {1}, {2}, {3}", subName, store, storeLocation, dt));
        }
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void VerifyGenerateCertWithDNSWithDate()
        {
            var subName = "TestCert_4";
            var store = "ROOT";
            StoreLocation storeLocation = StoreLocation.LocalMachine;
            var dns = "client.sf.lrc.com";
            DateTime dt = new DateTime(2017, 1, 20);
            CertificateManager.GenerateSelfSignedCertAndImportToStore(subName, store, storeLocation, dns, dt);
            bool result = VerifyCertExistsAndDelete(subName, store, storeLocation);
            Verify.IsTrue(result, string.Format("Generate Cert with parameters: {0}, {1}, {2}, {3}, {4}", subName, store, storeLocation, dns, dt));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void VerifyCertDeletionExactSubjectMatch()
        {
            var subName = "TestCert_5";
            var store = "ROOT";
            StoreLocation storeLocation = StoreLocation.LocalMachine;
            DateTime dt = DateTime.MinValue;
            CertificateManager.GenerateSelfSignedCertAndImportToStore(subName, store, storeLocation, null, dt);
            CertificateManager.DeleteCertificateFromStore(subName, store, storeLocation, true);
            bool result = VerifyCertExistsAndDelete(subName, store, storeLocation);
            Verify.IsFalse(result, string.Format("Delete Cert with parameters: {0}, {1}, {2}", subName, store, storeLocation));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void VerifyCertDeletionPrefixMatch()
        {
            var subName = "TestCert_6";
            var store = "ROOT";
            StoreLocation storeLocation = StoreLocation.LocalMachine;
            var delCertPrefix = "TestCert";
            DateTime dt = DateTime.MinValue;
            CertificateManager.GenerateSelfSignedCertAndImportToStore(subName, store, storeLocation, null, dt);
            CertificateManager.DeleteCertificateFromStore(delCertPrefix, store, storeLocation, false);
            bool result = VerifyCertExistsAndDelete(subName, store, storeLocation);
            Verify.IsFalse(result, string.Format("Delete Cert with parameters: {0}, {1}, {2}", subName, store, storeLocation));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void VerifyGenerateCertPFX()
        {
            string subName = "TestCert_7";
            string fileName = "TestCert_7.pfx";
            string pwd = "";
            DateTime dt = DateTime.MinValue;
            CertificateManager.GenerateSelfSignedCertAndSaveAsPFX(subName, fileName, pwd, null, dt);
            bool result = VerifyFileExists(fileName);
            Verify.IsTrue(result, string.Format("Generate Cert PFX with parameters: {0}, {1}, {2}", subName, fileName, pwd));
        }

        private static bool VerifyFileExists(string fileName)
        {
            if(File.Exists(fileName))
            {
                File.Delete(fileName);
                return true;
            }
            else
            {
                return false;
            }
        }
        private static bool VerifyCertExistsAndDelete(string subName, string store, StoreLocation storeLocation)
        {
            X509Store storeLoc;
            storeLoc = new X509Store(store, storeLocation);
            storeLoc.Open(OpenFlags.ReadWrite | OpenFlags.OpenExistingOnly);
            X509Certificate2Collection collection = (X509Certificate2Collection)storeLoc.Certificates;
            X509Certificate2Collection fcollection = (X509Certificate2Collection)collection.Find(X509FindType.FindBySubjectName, subName, false);
            if (fcollection.Count == 0)
            {
                return false;
            }
            else
            {
                foreach (X509Certificate2 x509 in fcollection)
                {
                    storeLoc.Remove(x509);
                }
                return true;
            }
        }
    }
}