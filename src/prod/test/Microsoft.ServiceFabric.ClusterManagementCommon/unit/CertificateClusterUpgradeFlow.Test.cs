// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ClusterManagementCommon;

    /// <summary>
    /// test 
    /// </summary>
    [TestClass]
    public class CertificateClusterUpgradeFlowTest
    {
        private const string CertThumbprint1 = "6A61E1A1A2FF01586BB5F4898438D211D1A39C62";

        private const string CertThumbprint2 = "9D02AA8535E5F0E3D8F638CC87D160548C73BD97";

        private const string CertCn1 = "mycn";

        private const string CertCn2 = "Mycn";

        private const string IssuerThumbprint1 = "tp1,tp2";

        private const string IssuerThumbprint2 = "tp3,tp4";

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void AddCertTest()
        {
            X509 currentCert;
            X509 targetCert;
            List<CertificateClusterUpgradeStep> flow;

            GetCerts_AddThumbprint(out currentCert, out targetCert, addPrimary: true);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_AddThumbprint(currentCert.ClusterCertificate, targetCert.ClusterCertificate, flow);

            GetCerts_AddThumbprint(out currentCert, out targetCert, addPrimary: false);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_AddThumbprint(currentCert.ClusterCertificate, targetCert.ClusterCertificate, flow);

            GetCerts_AddCn(out currentCert, out targetCert, addCn1: true);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_AddCn(currentCert.ClusterCertificateCommonNames, targetCert.ClusterCertificateCommonNames, flow);

            GetCerts_AddCn(out currentCert, out targetCert, addCn1: false);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_AddCn(currentCert.ClusterCertificateCommonNames, targetCert.ClusterCertificateCommonNames, flow);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RemoveCertTest()
        {
            X509 currentCert;
            X509 targetCert;
            List<CertificateClusterUpgradeStep> flow;

            GetCerts_RemoveThumbprint(out currentCert, out targetCert, removePrimary: true);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_RemoveThumbprint(currentCert.ClusterCertificate, targetCert.ClusterCertificate, flow);

            GetCerts_RemoveThumbprint(out currentCert, out targetCert, removePrimary: false);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_RemoveThumbprint(currentCert.ClusterCertificate, targetCert.ClusterCertificate, flow);

            GetCerts_RemoveCn(out currentCert, out targetCert, removeCn1: true);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_RemoveCn(currentCert.ClusterCertificateCommonNames, targetCert.ClusterCertificateCommonNames, flow);

            GetCerts_RemoveCn(out currentCert, out targetCert, removeCn1: false);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_RemoveCn(currentCert.ClusterCertificateCommonNames, targetCert.ClusterCertificateCommonNames, flow);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ReplaceCertTest()
        {
            X509 currentCert;
            X509 targetCert;
            List<CertificateClusterUpgradeStep> flow;

            GetCerts_ReplaceThumbprint(out currentCert, out targetCert);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_ReplaceThumbprint(currentCert.ClusterCertificate, targetCert.ClusterCertificate, flow);

            GetCerts_ReplaceCn(out currentCert, out targetCert);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_ReplaceCn(currentCert.ClusterCertificateCommonNames, targetCert.ClusterCertificateCommonNames, flow);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void SwapCertTest()
        {
            X509 currentCert;
            X509 targetCert;
            List<CertificateClusterUpgradeStep> flow;

            GetCerts_Swap(out currentCert, out targetCert);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_Swap(currentCert.ClusterCertificate, targetCert.ClusterCertificate, flow);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CertTypeChangeTest()
        {
            X509 currentCert;
            X509 targetCert;
            List<CertificateClusterUpgradeStep> flow;

            // 1 thumbprint -> 1 cn
            GetCerts_TypeChange(out currentCert, out targetCert, 1, 0, 0, 1);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_TypeChange(currentCert, targetCert, flow);

            // 1 thumbprint -> 2 cns
            GetCerts_TypeChange(out currentCert, out targetCert, 1, 0, 0, 2);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_TypeChange(currentCert, targetCert, flow);

            // 2 thumbprint -> 1 cn
            GetCerts_TypeChange(out currentCert, out targetCert, 2, 0, 0, 1);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_TypeChange(currentCert, targetCert, flow);

            // 2 thumbprint -> 2 cns
            GetCerts_TypeChange(out currentCert, out targetCert, 2, 0, 0, 2);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_TypeChange(currentCert, targetCert, flow);

            // 1 cn -> 1 thumbprint
            GetCerts_TypeChange(out currentCert, out targetCert, 0, 1, 1, 0);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_TypeChange(currentCert, targetCert, flow);

            // 1 cn -> 2 thumbprints
            GetCerts_TypeChange(out currentCert, out targetCert, 0, 1, 2, 0);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_TypeChange(currentCert, targetCert, flow);

            // 2 cns -> 1 thumbprint
            GetCerts_TypeChange(out currentCert, out targetCert, 0, 2, 1, 0);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_TypeChange(currentCert, targetCert, flow);

            // 2 cns -> 2 thumbprints
            GetCerts_TypeChange(out currentCert, out targetCert, 0, 2, 2, 0);
            flow = CertificateClusterUpgradeFlow.GetUpgradeFlow(currentCert, targetCert);
            VerifyFlow_TypeChange(currentCert, targetCert, flow);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void GetAddedCnsAndIssuersTest()
        {
            Dictionary<string, string> currentList = new Dictionary<string, string>()
            {
                { "cn1", "tp1,tp2" },
                { "cn2", null },
                { "cn3", "tp3" },
                { "cn4", null}
            };
            Dictionary<string, string> newList = new Dictionary<string, string>()
            {
                { "cn1", "tp2,tp4" },
                { "cn2", null },
                { "cn4", "tp5" },
                { "cn5", null },
            };

            ServerCertificateCommonNames currentCns = ConstructCertCommonNames(currentList);
            ServerCertificateCommonNames newCns = ConstructCertCommonNames(newList);
            Dictionary<string, string> actualResult = CertificateClusterUpgradeFlow.GetAddedCnsAndIssuers(currentCns, newCns);
            Dictionary<string, string> expectedResult = new Dictionary<string, string>()
            {
                { "cn1", "tp4" },
                { "cn4", "tp5" },
                { "cn5", null },
            };

            Assert.AreEqual(expectedResult.Count, actualResult.Count);
            Assert.IsTrue(actualResult.All(p => p.Value == expectedResult[p.Key]));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void MergeCnsAndIssuersTest()
        {
            Dictionary<string, string> current = new Dictionary<string, string>()
            {
                { "cn1", "tp1,tp2" },
                { "cn2", null },
            };
            Dictionary<string, string> added = new Dictionary<string, string>();

            // no added
            Dictionary<string, string> actualResult = CertificateClusterUpgradeFlow.MergeCnsAndIssuers(current, added);
            Dictionary<string, string> expectedResult = current;
            Assert.AreEqual(expectedResult.Count, actualResult.Count);
            Assert.IsTrue(actualResult.All(p => p.Value == expectedResult[p.Key]));

            // have added cn and issuers
            added = new Dictionary<string, string>()
            {
                { "cn1", "tp2,tp3" },
                { "cn3", "tp4" },
            };
            actualResult = CertificateClusterUpgradeFlow.MergeCnsAndIssuers(current, added);
            expectedResult = new Dictionary<string, string>()
            {
                { "cn1", "tp1,tp2,tp3" },
                { "cn2", null },
                { "cn3", "tp4" },
            };
            Assert.AreEqual(expectedResult.Count, actualResult.Count);

            foreach (string key in actualResult.Keys)
            {
                string actualValue = actualResult[key];
                string expectedValue = expectedResult[key];

                Assert.AreEqual(actualValue == null, expectedValue == null);
                if (actualValue != null)
                {
                    string[] actualThumbprints = actualValue.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
                    string[] expectedThumbprints = expectedValue.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
                    Assert.IsTrue(actualThumbprints.Any());
                    Assert.IsTrue(expectedThumbprints.Any());
                    Assert.IsTrue(actualThumbprints.All(p => expectedThumbprints.Contains(p)) && expectedThumbprints.All(p => actualThumbprints.Contains(p)));
                }
            }
        }

        internal ServerCertificateCommonNames ConstructCertCommonNames(Dictionary<string, string> commonNames)
        {
            List<CertificateCommonNameBase> cnList = new List<CertificateCommonNameBase>();
            if (commonNames != null)
            {
                foreach (var commonName in commonNames)
                {
                    cnList.Add(new CertificateCommonNameBase()
                    {
                        CertificateCommonName = commonName.Key,
                        CertificateIssuerThumbprint = commonName.Value
                    });
                }
            }

            return new ServerCertificateCommonNames()
            {
                CommonNames = cnList
            };
        }

        internal static X509 ConstructCertByThumbprint(string thumbprint1, string thumbprint2 = null)
        {
            return new X509()
            {
                ClusterCertificate = new CertificateDescription()
                {
                    Thumbprint = thumbprint1,
                    ThumbprintSecondary = thumbprint2
                }
            };
        }

        internal static X509 ConstructCertByCn(string cn1, string issuers1, string cn2 = null, string issuers2 = null)
        {
            X509 result = new X509()
            {
                ClusterCertificateCommonNames = new ServerCertificateCommonNames()
                {
                    CommonNames = new List<CertificateCommonNameBase>()
                    {
                        new CertificateCommonNameBase()
                        {
                            CertificateCommonName = cn1,
                            CertificateIssuerThumbprint = issuers1
                        }
                    }
                }
            };

            if (cn2 != null)
            {
                result.ClusterCertificateCommonNames.CommonNames.Add(new CertificateCommonNameBase() { CertificateCommonName = cn2, CertificateIssuerThumbprint = issuers2 });
            }

            return result;
        }

        internal void GetCerts_AddThumbprint(out X509 currentCert, out X509 targetCert, bool addPrimary)
        {
            currentCert = ConstructCertByThumbprint(CertThumbprint1);

            targetCert = ConstructCertByThumbprint(
                addPrimary ? CertThumbprint2 : CertThumbprint1,
                addPrimary ? CertThumbprint1 : CertThumbprint2);
        }

        internal void GetCerts_AddCn(out X509 currentCert, out X509 targetCert, bool addCn1)
        {
            currentCert = ConstructCertByCn(CertCn1, IssuerThumbprint1);

            targetCert = ConstructCertByCn(
                addCn1 ? CertCn2 : CertCn1,
                addCn1 ? IssuerThumbprint2 : IssuerThumbprint1,
                addCn1 ? CertCn1 : CertCn2,
                addCn1 ? IssuerThumbprint1 : IssuerThumbprint2);
        }

        internal void GetCerts_RemoveThumbprint(out X509 currentCert, out X509 targetCert, bool removePrimary)
        {
            currentCert = ConstructCertByThumbprint(CertThumbprint1, CertThumbprint2);

            targetCert = ConstructCertByThumbprint(removePrimary ? CertThumbprint2 : CertThumbprint1);
        }

        internal void GetCerts_RemoveCn(out X509 currentCert, out X509 targetCert, bool removeCn1)
        {
            currentCert = ConstructCertByCn(CertCn1, IssuerThumbprint1, CertCn2, IssuerThumbprint2);

            targetCert = ConstructCertByCn(
                removeCn1 ? CertCn2 : CertCn1,
                removeCn1 ? IssuerThumbprint2 : IssuerThumbprint1);
        }

        internal void GetCerts_ReplaceThumbprint(out X509 currentCert, out X509 targetCert)
        {
            currentCert = ConstructCertByThumbprint(CertThumbprint1);

            targetCert = ConstructCertByThumbprint(CertThumbprint2);
        }

        internal void GetCerts_ReplaceCn(out X509 currentCert, out X509 targetCert)
        {
            currentCert = ConstructCertByCn(CertCn1, IssuerThumbprint1);

            targetCert = ConstructCertByCn(CertCn2, IssuerThumbprint2);
        }

        internal void GetCerts_Swap(out X509 currentCert, out X509 targetCert)
        {
            currentCert = ConstructCertByThumbprint(CertThumbprint1, CertThumbprint2);

            targetCert = ConstructCertByThumbprint(CertThumbprint2, CertThumbprint1);
        }

        internal void GetCerts_TypeChange(out X509 currentCert, out X509 targetCert, int srcThumbprintCount, int srcCnCount, int targetThumbprintCount, int targetCnCount)
        {
            currentCert = null;
            targetCert = null;

            if (srcThumbprintCount > 0)
            {
                currentCert = ConstructCertByThumbprint(CertThumbprint1, srcThumbprintCount > 1 ? CertThumbprint2 : null);
            }

            if (srcCnCount > 0)
            {
                currentCert = ConstructCertByCn(CertCn1, IssuerThumbprint1, srcCnCount > 1 ? CertCn2 : null, srcCnCount > 1 ? IssuerThumbprint2 : null);
            }

            if (targetThumbprintCount > 0)
            {
                targetCert = ConstructCertByThumbprint(CertThumbprint1, targetThumbprintCount > 1 ? CertThumbprint2 : null);
            }

            if (targetCnCount > 0)
            {
                targetCert = ConstructCertByCn(CertCn1, IssuerThumbprint1, targetCnCount > 1 ? CertCn2 : null, targetCnCount > 1 ? IssuerThumbprint2 : null);
            }
        }

        internal void VerifyFlow_AddThumbprint(CertificateDescription currentCert, CertificateDescription targetCert, List<CertificateClusterUpgradeStep> steps)
        {
            Assert.AreEqual(2, steps.Count);

            CertificateClusterUpgradeStep step = steps[0];
            Assert.AreEqual(2, step.ThumbprintWhiteList.Count);
            Assert.IsTrue(step.ThumbprintWhiteList.Contains(targetCert.Thumbprint) && step.ThumbprintWhiteList.Contains(targetCert.ThumbprintSecondary));
            Assert.AreEqual(currentCert.Thumbprint, step.ThumbprintLoadList.Thumbprint);
            Assert.IsNull(step.ThumbprintLoadList.ThumbprintSecondary);
            Assert.AreEqual(currentCert.Thumbprint, step.ThumbprintFileStoreSvcList.Thumbprint);
            Assert.IsNull(step.ThumbprintFileStoreSvcList.ThumbprintSecondary);

            step = steps[1];
            Assert.AreEqual(2, step.ThumbprintWhiteList.Count);
            Assert.IsTrue(step.ThumbprintWhiteList.Contains(targetCert.Thumbprint) && step.ThumbprintWhiteList.Contains(targetCert.ThumbprintSecondary));
            Assert.AreEqual(targetCert.Thumbprint, step.ThumbprintLoadList.Thumbprint);
            Assert.AreEqual(targetCert.ThumbprintSecondary, step.ThumbprintLoadList.ThumbprintSecondary);
            Assert.IsNotNull(step.ThumbprintLoadList.ThumbprintSecondary);
            Assert.AreEqual(targetCert.Thumbprint, step.ThumbprintFileStoreSvcList.Thumbprint);
            Assert.AreEqual(targetCert.ThumbprintSecondary, step.ThumbprintFileStoreSvcList.ThumbprintSecondary);
            Assert.IsNotNull(step.ThumbprintFileStoreSvcList.ThumbprintSecondary);
        }

        internal void VerifyFlow_AddCn(ServerCertificateCommonNames currentCert, ServerCertificateCommonNames targetCert, List<CertificateClusterUpgradeStep> steps)
        {
            Assert.AreEqual(2, steps.Count);

            CertificateClusterUpgradeStep step = steps[0];
            Assert.AreEqual(2, step.CommonNameWhiteList.Count);
            Assert.IsTrue(step.CommonNameWhiteList.Keys.Contains(targetCert.CommonNames[0].CertificateCommonName) && step.CommonNameWhiteList.Keys.Contains(targetCert.CommonNames[1].CertificateCommonName));
            Assert.IsFalse(step.CommonNameWhiteList.Values.Any(p => string.IsNullOrWhiteSpace(p)));
            Assert.IsTrue(step.CommonNameWhiteList.Values.Contains(targetCert.CommonNames[0].CertificateIssuerThumbprint) && step.CommonNameWhiteList.Values.Contains(targetCert.CommonNames[1].CertificateIssuerThumbprint));
            Assert.AreEqual(currentCert.CommonNames[0].CertificateCommonName, step.CommonNameLoadList.CommonNames[0].CertificateCommonName);
            Assert.AreEqual(1, step.CommonNameLoadList.CommonNames.Count);
            Assert.AreEqual(currentCert.CommonNames[0].CertificateCommonName, step.CommonNameFileStoreSvcList.CommonNames[0].CertificateCommonName);
            Assert.AreEqual(1, step.CommonNameFileStoreSvcList.CommonNames.Count);

            step = steps[1];
            Assert.AreEqual(2, step.CommonNameWhiteList.Count);
            Assert.IsTrue(step.CommonNameWhiteList.Keys.Contains(targetCert.CommonNames[0].CertificateCommonName) && step.CommonNameWhiteList.Keys.Contains(targetCert.CommonNames[1].CertificateCommonName));
            Assert.IsFalse(step.CommonNameWhiteList.Values.Any(p => string.IsNullOrWhiteSpace(p)));
            Assert.IsTrue(step.CommonNameWhiteList.Values.Contains(targetCert.CommonNames[0].CertificateIssuerThumbprint) && step.CommonNameWhiteList.Values.Contains(targetCert.CommonNames[1].CertificateIssuerThumbprint));
            Assert.AreEqual(targetCert.CommonNames[0].CertificateCommonName, step.CommonNameLoadList.CommonNames[0].CertificateCommonName);
            Assert.AreEqual(targetCert.CommonNames[1].CertificateCommonName, step.CommonNameLoadList.CommonNames[1].CertificateCommonName);
            Assert.AreEqual(2, step.CommonNameLoadList.CommonNames.Count);
            Assert.AreEqual(targetCert.CommonNames[0].CertificateCommonName, step.CommonNameFileStoreSvcList.CommonNames[0].CertificateCommonName);
            Assert.AreEqual(targetCert.CommonNames[1].CertificateCommonName, step.CommonNameFileStoreSvcList.CommonNames[1].CertificateCommonName);
            Assert.AreEqual(2, step.CommonNameFileStoreSvcList.CommonNames.Count);
        }

        internal void VerifyFlow_RemoveThumbprint(CertificateDescription currentCert, CertificateDescription targetCert, List<CertificateClusterUpgradeStep> steps)
        {
            Assert.AreEqual(2, steps.Count);

            CertificateClusterUpgradeStep step = steps[0];
            Assert.AreEqual(2, step.ThumbprintWhiteList.Count);
            Assert.IsTrue(step.ThumbprintWhiteList.Contains(currentCert.Thumbprint) && step.ThumbprintWhiteList.Contains(currentCert.ThumbprintSecondary));
            Assert.AreEqual(targetCert.Thumbprint, step.ThumbprintLoadList.Thumbprint);
            Assert.IsNull(step.ThumbprintLoadList.ThumbprintSecondary);
            Assert.AreEqual(currentCert.Thumbprint, step.ThumbprintFileStoreSvcList.Thumbprint);
            Assert.AreEqual(currentCert.ThumbprintSecondary, step.ThumbprintFileStoreSvcList.ThumbprintSecondary);

            step = steps[1];
            Assert.AreEqual(1, step.ThumbprintWhiteList.Count);
            Assert.AreEqual(targetCert.Thumbprint, step.ThumbprintWhiteList[0]);
            Assert.AreEqual(targetCert.Thumbprint, step.ThumbprintLoadList.Thumbprint);
            Assert.IsNull(step.ThumbprintLoadList.ThumbprintSecondary);
            Assert.AreEqual(targetCert.Thumbprint, step.ThumbprintFileStoreSvcList.Thumbprint);
            Assert.AreEqual(targetCert.ThumbprintSecondary, step.ThumbprintFileStoreSvcList.ThumbprintSecondary);
            Assert.IsNull(step.ThumbprintFileStoreSvcList.ThumbprintSecondary);
        }

        internal void VerifyFlow_RemoveCn(ServerCertificateCommonNames currentCert, ServerCertificateCommonNames targetCert, List<CertificateClusterUpgradeStep> steps)
        {
            Assert.AreEqual(2, steps.Count);

            CertificateClusterUpgradeStep step = steps[0];
            Assert.AreEqual(2, step.CommonNameWhiteList.Count);
            Assert.IsTrue(step.CommonNameWhiteList.Keys.Contains(currentCert.CommonNames[0].CertificateCommonName) && step.CommonNameWhiteList.Keys.Contains(currentCert.CommonNames[1].CertificateCommonName));
            Assert.IsFalse(step.CommonNameWhiteList.Values.Any(p => string.IsNullOrWhiteSpace(p)));
            Assert.IsTrue(step.CommonNameWhiteList.Values.Contains(currentCert.CommonNames[0].CertificateIssuerThumbprint) && step.CommonNameWhiteList.Values.Contains(currentCert.CommonNames[1].CertificateIssuerThumbprint));
            Assert.AreEqual(targetCert.CommonNames[0].CertificateCommonName, step.CommonNameLoadList.CommonNames[0].CertificateCommonName);
            Assert.AreEqual(1, step.CommonNameLoadList.CommonNames.Count);
            Assert.AreEqual(currentCert.CommonNames[0].CertificateCommonName, step.CommonNameFileStoreSvcList.CommonNames[0].CertificateCommonName);
            Assert.AreEqual(currentCert.CommonNames[1].CertificateCommonName, step.CommonNameFileStoreSvcList.CommonNames[1].CertificateCommonName);

            step = steps[1];
            Assert.AreEqual(1, step.CommonNameWhiteList.Count);
            Assert.AreEqual(targetCert.CommonNames[0].CertificateCommonName, step.CommonNameWhiteList.First().Key);
            Assert.IsNotNull(step.CommonNameWhiteList.First().Value);
            Assert.AreEqual(targetCert.CommonNames[0].CertificateIssuerThumbprint, step.CommonNameWhiteList.First().Value);
            Assert.AreEqual(targetCert.CommonNames[0].CertificateCommonName, step.CommonNameLoadList.CommonNames[0].CertificateCommonName);
            Assert.AreEqual(1, step.CommonNameLoadList.CommonNames.Count);
            Assert.AreEqual(targetCert.CommonNames[0].CertificateCommonName, step.CommonNameFileStoreSvcList.CommonNames[0].CertificateCommonName);
            Assert.AreEqual(1, step.CommonNameFileStoreSvcList.CommonNames.Count);
        }

        internal void VerifyFlow_ReplaceThumbprint(CertificateDescription currentCert, CertificateDescription targetCert, List<CertificateClusterUpgradeStep> steps)
        {
            Assert.AreEqual(3, steps.Count);

            CertificateClusterUpgradeStep step = steps[0];
            Assert.AreEqual(2, step.ThumbprintWhiteList.Count);
            Assert.IsTrue(step.ThumbprintWhiteList.Contains(currentCert.Thumbprint) && step.ThumbprintWhiteList.Contains(targetCert.Thumbprint));
            Assert.AreEqual(currentCert.Thumbprint, step.ThumbprintLoadList.Thumbprint);
            Assert.IsNull(step.ThumbprintLoadList.ThumbprintSecondary);
            Assert.AreEqual(currentCert.Thumbprint, step.ThumbprintFileStoreSvcList.Thumbprint);
            Assert.IsNull(step.ThumbprintFileStoreSvcList.ThumbprintSecondary);

            step = steps[1];
            Assert.AreEqual(2, step.ThumbprintWhiteList.Count);
            Assert.IsTrue(step.ThumbprintWhiteList.Contains(currentCert.Thumbprint) && step.ThumbprintWhiteList.Contains(targetCert.Thumbprint));
            Assert.AreEqual(targetCert.Thumbprint, step.ThumbprintLoadList.Thumbprint);
            Assert.IsNull(step.ThumbprintLoadList.ThumbprintSecondary);
            Assert.AreEqual(currentCert.Thumbprint, step.ThumbprintFileStoreSvcList.Thumbprint);
            Assert.AreEqual(targetCert.Thumbprint, step.ThumbprintFileStoreSvcList.ThumbprintSecondary);

            step = steps[2];
            Assert.AreEqual(1, step.ThumbprintWhiteList.Count);
            Assert.AreEqual(targetCert.Thumbprint, step.ThumbprintWhiteList[0]);
            Assert.AreEqual(targetCert.Thumbprint, step.ThumbprintLoadList.Thumbprint);
            Assert.IsNull(step.ThumbprintLoadList.ThumbprintSecondary);
            Assert.AreEqual(targetCert.Thumbprint, step.ThumbprintFileStoreSvcList.Thumbprint);
            Assert.IsNull(step.ThumbprintFileStoreSvcList.ThumbprintSecondary);
        }

        internal void VerifyFlow_ReplaceCn(ServerCertificateCommonNames currentCert, ServerCertificateCommonNames targetCert, List<CertificateClusterUpgradeStep> steps)
        {
            Assert.AreEqual(3, steps.Count);

            CertificateClusterUpgradeStep step = steps[0];
            Assert.AreEqual(2, step.CommonNameWhiteList.Count);
            Assert.IsTrue(step.CommonNameWhiteList.Keys.Contains(currentCert.CommonNames[0].CertificateCommonName) && step.CommonNameWhiteList.Keys.Contains(targetCert.CommonNames[0].CertificateCommonName));
            Assert.IsFalse(step.CommonNameWhiteList.Values.Any(p => string.IsNullOrWhiteSpace(p)));
            Assert.IsTrue(step.CommonNameWhiteList.Values.Contains(currentCert.CommonNames[0].CertificateIssuerThumbprint) && step.CommonNameWhiteList.Values.Contains(targetCert.CommonNames[0].CertificateIssuerThumbprint));
            Assert.AreEqual(currentCert.CommonNames[0].CertificateCommonName, step.CommonNameLoadList.CommonNames[0].CertificateCommonName);
            Assert.AreEqual(1, step.CommonNameLoadList.CommonNames.Count);
            Assert.AreEqual(currentCert.CommonNames[0].CertificateCommonName, step.CommonNameFileStoreSvcList.CommonNames[0].CertificateCommonName);
            Assert.AreEqual(1, step.CommonNameFileStoreSvcList.CommonNames.Count);

            step = steps[1];
            Assert.AreEqual(2, step.CommonNameWhiteList.Count);
            Assert.IsTrue(step.CommonNameWhiteList.Keys.Contains(currentCert.CommonNames[0].CertificateCommonName) && step.CommonNameWhiteList.Keys.Contains(targetCert.CommonNames[0].CertificateCommonName));
            Assert.IsFalse(step.CommonNameWhiteList.Values.Any(p => string.IsNullOrWhiteSpace(p)));
            Assert.IsTrue(step.CommonNameWhiteList.Values.Contains(currentCert.CommonNames[0].CertificateIssuerThumbprint) && step.CommonNameWhiteList.Values.Contains(targetCert.CommonNames[0].CertificateIssuerThumbprint));
            Assert.AreEqual(targetCert.CommonNames[0].CertificateCommonName, step.CommonNameLoadList.CommonNames[0].CertificateCommonName);
            Assert.AreEqual(1, step.CommonNameLoadList.CommonNames.Count);
            Assert.AreEqual(currentCert.CommonNames[0].CertificateCommonName, step.CommonNameFileStoreSvcList.CommonNames[0].CertificateCommonName);
            Assert.AreEqual(targetCert.CommonNames[0].CertificateCommonName, step.CommonNameFileStoreSvcList.CommonNames[1].CertificateCommonName);

            step = steps[2];
            Assert.AreEqual(1, step.CommonNameWhiteList.Count);
            Assert.AreEqual(targetCert.CommonNames[0].CertificateCommonName, step.CommonNameWhiteList.First().Key);
            Assert.IsFalse(step.CommonNameWhiteList.Values.Any(p => string.IsNullOrWhiteSpace(p)));
            Assert.IsTrue(step.CommonNameWhiteList.Values.Contains(targetCert.CommonNames[0].CertificateIssuerThumbprint));
            Assert.AreEqual(targetCert.CommonNames[0].CertificateCommonName, step.CommonNameLoadList.CommonNames[0].CertificateCommonName);
            Assert.AreEqual(1, step.CommonNameLoadList.CommonNames.Count);
            Assert.AreEqual(targetCert.CommonNames[0].CertificateCommonName, step.CommonNameFileStoreSvcList.CommonNames[0].CertificateCommonName);
            Assert.AreEqual(1, step.CommonNameFileStoreSvcList.CommonNames.Count);
        }

        internal void VerifyFlow_Swap(CertificateDescription currentCert, CertificateDescription targetCert, List<CertificateClusterUpgradeStep> steps)
        {
            Assert.AreEqual(2, steps.Count);

            CertificateClusterUpgradeStep step = steps[0];
            Assert.AreEqual(2, step.ThumbprintWhiteList.Count);
            Assert.IsTrue(step.ThumbprintWhiteList.Contains(targetCert.Thumbprint) && step.ThumbprintWhiteList.Contains(targetCert.ThumbprintSecondary));
            Assert.AreEqual(targetCert.Thumbprint, step.ThumbprintLoadList.Thumbprint);
            Assert.AreEqual(targetCert.ThumbprintSecondary, step.ThumbprintLoadList.ThumbprintSecondary);
            Assert.AreEqual(currentCert.Thumbprint, step.ThumbprintFileStoreSvcList.Thumbprint);
            Assert.AreEqual(targetCert.ThumbprintSecondary, step.ThumbprintFileStoreSvcList.ThumbprintSecondary);

            step = steps[1];
            Assert.AreEqual(2, step.ThumbprintWhiteList.Count);
            Assert.IsTrue(step.ThumbprintWhiteList.Contains(targetCert.Thumbprint) && step.ThumbprintWhiteList.Contains(targetCert.ThumbprintSecondary));
            Assert.AreEqual(targetCert.Thumbprint, step.ThumbprintLoadList.Thumbprint);
            Assert.AreEqual(targetCert.ThumbprintSecondary, step.ThumbprintLoadList.ThumbprintSecondary);
            Assert.AreEqual(targetCert.Thumbprint, step.ThumbprintFileStoreSvcList.Thumbprint);
            Assert.AreEqual(targetCert.ThumbprintSecondary, step.ThumbprintFileStoreSvcList.ThumbprintSecondary);
        }

        internal void VerifyFlow_TypeChange(X509 currentCert, X509 targetCert, List<CertificateClusterUpgradeStep> steps)
        {
            Assert.AreEqual(3, steps.Count);

            int srcThumbprintCount = currentCert.ClusterCertificate == null ? 0 : (currentCert.ClusterCertificate.ThumbprintSecondary != null ? 2 : 1);
            int srcCnCount = currentCert.ClusterCertificateCommonNames == null ? 0 : (currentCert.ClusterCertificateCommonNames.CommonNames.Count > 1 ? 2: 1);
            int targetThumbprintCount = targetCert.ClusterCertificate == null ? 0 : (targetCert.ClusterCertificate.ThumbprintSecondary != null ? 2 : 1);
            int targetCnCount = targetCert.ClusterCertificateCommonNames == null ? 0 : (targetCert.ClusterCertificateCommonNames.CommonNames.Count > 1 ? 2 : 1);
            int totalThumbprintCount = srcThumbprintCount + targetThumbprintCount;
            int totalCnCount = srcCnCount + targetCnCount;
            int totalCount = totalThumbprintCount + totalCnCount;
            List<string> srcThumbprints = srcThumbprintCount == 0 ? new List<string>() : currentCert.ClusterCertificate.ToThumbprintList();
            List<string> srcCns = srcCnCount == 0 ? new List<string>() : currentCert.ClusterCertificateCommonNames.CommonNames.Select(p => p.CertificateCommonName).ToList();
            List<string> targetThumbprints = targetThumbprintCount == 0 ? new List<string>() : targetCert.ClusterCertificate.ToThumbprintList();
            Dictionary<string, string> targetCns = targetCnCount == 0 ? new Dictionary<string, string>() : targetCert.ClusterCertificateCommonNames.CommonNames.ToDictionary(p => p.CertificateCommonName, p=> p.CertificateIssuerThumbprint);
            List<string> allThumbprints = srcThumbprints.Concat(targetThumbprints).ToList();
            List<string> allCns = srcCns.Concat(targetCns.Keys).ToList();

            CertificateClusterUpgradeStep step = steps[0];

            Assert.AreEqual(totalCount, step.ThumbprintWhiteList.Count + step.CommonNameWhiteList.Count);
            Assert.AreEqual(totalThumbprintCount, step.ThumbprintWhiteList.Count);
            Assert.IsTrue(allThumbprints.All(p => step.ThumbprintWhiteList.Contains(p)));
            Assert.AreEqual(totalCnCount, step.CommonNameWhiteList.Count);
            Assert.IsTrue(allCns.All(p => step.CommonNameWhiteList.Keys.Contains(p)));
            Assert.IsFalse(step.CommonNameWhiteList.Values.Any(p => p == null));

            Assert.AreSame(currentCert.ClusterCertificate, step.ThumbprintLoadList);
            Assert.AreSame(currentCert.ClusterCertificateCommonNames, step.CommonNameLoadList);

            Assert.AreSame(currentCert.ClusterCertificate, step.ThumbprintFileStoreSvcList);
            Assert.AreSame(currentCert.ClusterCertificateCommonNames, step.CommonNameFileStoreSvcList);

            step = steps[1];

            Assert.AreEqual(totalCount, step.ThumbprintWhiteList.Count + step.CommonNameWhiteList.Count);
            Assert.AreEqual(totalThumbprintCount, step.ThumbprintWhiteList.Count);
            Assert.IsTrue(allThumbprints.All(p => step.ThumbprintWhiteList.Contains(p)));
            Assert.AreEqual(totalCnCount, step.CommonNameWhiteList.Count);
            Assert.IsTrue(allCns.All(p => step.CommonNameWhiteList.Keys.Contains(p)));
            Assert.IsFalse(step.CommonNameWhiteList.Values.Any(p => p == null));

            Assert.AreSame(targetCert.ClusterCertificate, step.ThumbprintLoadList);
            Assert.AreSame(targetCert.ClusterCertificateCommonNames, step.CommonNameLoadList);

            Assert.AreEqual(totalThumbprintCount, step.ThumbprintFileStoreSvcList.ToThumbprintList().Count);
            Assert.IsTrue(allThumbprints.All(p => step.ThumbprintFileStoreSvcList.ToThumbprintList().Contains(p)));
            Assert.AreEqual(totalCnCount, step.CommonNameFileStoreSvcList.CommonNames.Count);
            Assert.IsTrue(allCns.All(p => step.CommonNameFileStoreSvcList.CommonNames.Select(q => q.CertificateCommonName).Contains(p)));

            step = steps[2];

            List<string> finalThumbprintWhiteList = step.ThumbprintWhiteList == null ? new List<string>() : step.ThumbprintWhiteList;
            Dictionary<string, string> finalCnWhiteList = step.CommonNameWhiteList == null ? new Dictionary<string, string>() : step.CommonNameWhiteList;

            Assert.AreEqual(targetThumbprintCount + targetCnCount, finalThumbprintWhiteList.Count + finalCnWhiteList.Count);
            Assert.AreEqual(targetThumbprintCount, finalThumbprintWhiteList.Count);
            Assert.IsTrue(targetThumbprints.All(p => finalThumbprintWhiteList.Contains(p)));
            Assert.AreEqual(targetCnCount, finalCnWhiteList.Count);
            Assert.IsTrue(targetCns.Keys.All(p => finalCnWhiteList.Keys.Contains(p)));
            Assert.IsTrue(targetCns.Values.All(p => finalCnWhiteList.Values.Contains(p)));
            Assert.IsFalse(targetCns.Values.Any(p => p == null));

            Assert.AreSame(targetCert.ClusterCertificate, step.ThumbprintLoadList);
            Assert.AreSame(targetCert.ClusterCertificateCommonNames, step.CommonNameLoadList);

            Assert.AreSame(targetCert.ClusterCertificate, step.ThumbprintFileStoreSvcList);
            Assert.AreSame(targetCert.ClusterCertificateCommonNames, step.CommonNameFileStoreSvcList);
        }
    }
}