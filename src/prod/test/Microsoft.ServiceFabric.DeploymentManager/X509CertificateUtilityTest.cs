// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Security.Cryptography.X509Certificates;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    internal class X509CertificateUtilityTest
    {
        [TestInitialize]
        public void Initialize()
        {
#if !DotNetCoreClr
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;
#endif

            // in case somehow the certificates are left over from previous run
            this.Cleanup();

            Utility.InstallCertificates(Utility.installedCertificates.Select(p => p.Item2));
        }

        [TestCleanup]
        public void Cleanup()
        {
            Utility.UninstallCertificates(Utility.installedCertificates.Select(p => p.Item1));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TryFindCertificateTest()
        {
            IEnumerable<string> thumbprints = Utility.installedCertificates.Select(p => p.Item1).Union(Utility.uninstalledCertificates);
            this.InternalTryFindCertificateTest(string.Empty, thumbprints, X509FindType.FindByThumbprint);
            this.InternalTryFindCertificateTest("localhost", thumbprints, X509FindType.FindByThumbprint);
            this.InternalTryFindCertificateTest("127.0.0.1", thumbprints, X509FindType.FindByThumbprint);

            IEnumerable<string> cns = Utility.installedCertificates.Select(p => p.Item3).Union(Utility.uninstalledCertificates);
            this.InternalTryFindCertificateTest(string.Empty, cns, X509FindType.FindBySubjectName);
            this.InternalTryFindCertificateTest("localhost", cns, X509FindType.FindBySubjectName);
            this.InternalTryFindCertificateTest("127.0.0.1", cns, X509FindType.FindBySubjectName);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void IsCertificateValidTest()
        {
            IEnumerable<X509Certificate2> foundCerts;
            X509CertificateUtility.TryFindCertificate(
                "localhost",
                StoreName.My,
                Utility.installedCertificates.Select(p => p.Item1),
                X509FindType.FindByThumbprint,
                out foundCerts);

            Assert.AreEqual(Utility.installedCertificates.Count(), foundCerts.Count());
            Assert.IsTrue(foundCerts.All(cert => Utility.installedValidCertificates.Any(p => p.Item1 == cert.Thumbprint) == X509CertificateUtility.IsCertificateValid(cert)));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void FindIssuerTest()
        {
            IEnumerable<string> cns = Utility.installedCertificates.Select(p => p.Item3);
            IEnumerable<X509Certificate2> foundCerts;
            IEnumerable<string> unfoundCerts = X509CertificateUtility.TryFindCertificate(string.Empty, StoreName.My, cns, X509FindType.FindBySubjectName, out foundCerts);
            Assert.IsFalse(unfoundCerts.Any());
            Assert.AreEqual(cns.Count(), foundCerts.Count());

            X509Certificate2[] foundCertsResult = foundCerts.ToArray();
            for (int i = 0; i < foundCertsResult.Count(); i++)
            {
                string issuerThumbprint = foundCertsResult[i].FindIssuer(new string[] { Utility.installedCertificates[i].Item1 });
                Assert.AreEqual(Utility.installedCertificates[i].Item1, issuerThumbprint);
            }
        }

        internal void InternalTryFindCertificateTest(string ipAddress, IEnumerable<string> findValues, X509FindType findType)
        {
            IEnumerable<X509Certificate2> foundCerts;
            IEnumerable<string> unfoundCerts = X509CertificateUtility.TryFindCertificate(ipAddress, StoreName.My, findValues, findType, out foundCerts);

            bool isThumbprintTest = findType == X509FindType.FindByThumbprint;
            this.VerifyCollectionEquity(foundCerts.Select(c => isThumbprintTest ? c.Thumbprint : c.Subject), Utility.installedCertificates.Select(p => isThumbprintTest ? p.Item1 : "CN=" + p.Item3));
            this.VerifyCollectionEquity(unfoundCerts, Utility.uninstalledCertificates);
        }

        internal void VerifyCollectionEquity(IEnumerable<string> collection1, IEnumerable<string> collection2)
        {
            Assert.IsTrue(collection1.All(t => collection2.Contains(t, StringComparer.OrdinalIgnoreCase)));
            Assert.IsTrue(collection2.All(t => collection1.Contains(t, StringComparer.OrdinalIgnoreCase)));
        }
    }
}