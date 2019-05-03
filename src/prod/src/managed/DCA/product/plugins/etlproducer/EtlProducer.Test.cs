// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace DCAHostTest 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.IO;
    using System.Reflection;
    using System.Text;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using WEX.TestExecution;
    using Tools.EtlReader;

    [TestClass]
    class EtlProducerTest
    {
        // Members that are not test specific
        private TestContext testContext;
        public TestContext TestContext
        {
            get { return testContext; }
            set { testContext = value; }
        }

        private const string TraceType = "EtlProducerTest";
        private const string configFileName = "EtlProducerTest.cfg";
        private const string configFileContents =
        @"
[Trace/File]
Level = 5
Path = EtlProducer.Test
Option = m
        ";

        [ClassInitialize]
        public static void EtlProducerTestSetup(object testContext)
        {
            // Create the config file for the test
            string configFileFullPath = Path.Combine(
                                     Directory.GetCurrentDirectory(),
                                     configFileName);
            byte[] configFileBytes = Encoding.ASCII.GetBytes(configFileContents);
            File.WriteAllBytes(configFileFullPath, configFileBytes);
            Environment.SetEnvironmentVariable("FabricConfigFileName", configFileFullPath);

            // Initialize config store
            Utility.InitializeConfigStore((IConfigStoreUpdateHandler)null);

            Utility.InitializeTracing();
            Utility.TraceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            return;
        }

        private string currentFabricManifest;
        private string currentLeaseLayerManifest;
        private string currentKtlManifest;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void DefaultWindowsFabricManifestTest()
        {
            // Get the path to the ArtifactsSpecification file
            string assemblyLocation = Assembly.GetExecutingAssembly().Location;
            string artifactsSpecDirectory = Path.GetDirectoryName(assemblyLocation);
            string artifactsSpecFile = Path.Combine(
                                           artifactsSpecDirectory,
                                           "ArtifactsSpecification.csv");

            // Get the names of the default Windows Fabric manifests
            List<string> expectedManifests = new List<string>(WinFabricManifestManager.DefaultManifests);

            // Make sure each default Windows Fabric manifest is referenced in the artifacts specification
            using (StreamReader reader = new StreamReader(artifactsSpecFile))
            {
                for (; ; )
                {
                    string line = reader.ReadLine();
                    if (null == line)
                    {
                        // Reached end of file
                        break;
                    }

                    string manifestFound = null;
                    foreach (string expectedManifest in expectedManifests)
                    {
                        if (line.Contains(expectedManifest))
                        {
                            manifestFound = expectedManifest;
                            break;
                        }
                    }

                    if (null != manifestFound)
                    {
                        expectedManifests.Remove(manifestFound);
                        if (0 == expectedManifests.Count)
                        {
                            break;
                        }
                    }
                }

                foreach (string expectedManifest in expectedManifests)
                {
                    Verify.Fail(
                        String.Format(
                            "Unable to find default Windows Fabric manifest {0} in file {1}. If the manifest has been renamed or removed, please modify the {2} class accordingly.",
                            expectedManifest,
                            artifactsSpecFile,
                            typeof(WinFabricManifestManager)));
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [DataSource("Table:EtlProducerTestData.xml#EtlToManifestMap")]
        public void MapEtlToManifestTest()
        {
            this.currentFabricManifest = null;
            this.currentLeaseLayerManifest = null;
            this.currentKtlManifest = null;

            string[] manifests = testContext.DataRow["Manifests"] as string[];
            for (int i = 0; i < manifests.Length; i++)
            {
                manifests[i] = manifests[i].Trim();
            }

            WinFabricManifestManager manifestMgr = new WinFabricManifestManager(
                                                               manifests,
                                                               this.LoadManifest,
                                                               this.UnloadManifest);

            string[] mapping = testContext.DataRow["ExpectedMapping"] as string[];
            foreach (string etlToMan in mapping)
            {
                string[] etlToManParts = etlToMan.Split(',');
                Verify.IsTrue(etlToManParts.Length > 0);
                string etlFile = etlToManParts[0].Trim();

                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Processing ETL file {0}",
                    etlFile);

                bool exactMatch;
                manifestMgr.EnsureCorrectWinFabManifestVersionLoaded(etlFile, out exactMatch);

                List<string> expectedManifests = new List<string>();
                for (int i = 1; i < etlToManParts.Length; i++)
                {
                    expectedManifests.Add(etlToManParts[i].Trim());
                }

                string etlFileName = Path.GetFileName(etlFile);
                if (etlFileName.StartsWith(WinFabricManifestManager.FabricEtlPrefix))
                {
                    VerifyCurrentManifest(this.currentFabricManifest, expectedManifests);
                }
                else if (etlFileName.StartsWith(WinFabricManifestManager.LeaseLayerEtlPrefix))
                {
                    VerifyCurrentManifest(this.currentLeaseLayerManifest, expectedManifests);
                    VerifyCurrentManifest(this.currentKtlManifest, expectedManifests);
                }
                else
                {
                    Verify.Fail("Unexpected prefix in ETL file name");
                }

                foreach (string expectedManifest in expectedManifests)
                {
                    Utility.TraceSource.WriteError(
                        TraceType,
                        "Expected manifest {0} was not loaded.",
                        expectedManifest);
                    Verify.Fail("Expected manifest not loaded.");
                }
            }
        }

        private static void VerifyCurrentManifest(string currentManifest, List<string> expectedManifests)
        {
            if (null != currentManifest)
            {
                if (expectedManifests.Count == 0)
                {
                    Utility.TraceSource.WriteError(
                        TraceType,
                        "Manifest {0} is unexpectedly loaded",
                        currentManifest);
                    Verify.Fail("Manifest loaded when one expected");
                }
                else
                {
                    string matchingManifest = expectedManifests.Find(currentManifest.Equals);
                    if (null == matchingManifest)
                    {
                        Utility.TraceSource.WriteError(
                            TraceType,
                            "The currently loaded manifest {0} does not match any of the manifests we expected.",
                            currentManifest);
                        Verify.Fail("Unexpected manifest loaded.");
                    }

                    expectedManifests.Remove(matchingManifest);
                }
            }
        }

        private void LoadManifest(string manifestFile)
        {
            Utility.TraceSource.WriteInfo(
                TraceType,
                "Loading manifest file {0}",
                manifestFile);

            string manifestFileName = Path.GetFileName(manifestFile);
            if (WinFabricManifestManager.IsFabricManifestFileName(manifestFileName))
            {
                LoadManifest(manifestFile, ref this.currentFabricManifest);
            }
            else if (WinFabricManifestManager.IsLeaseLayerManifestFileName(manifestFileName))
            {
                LoadManifest(manifestFile, ref this.currentLeaseLayerManifest);
            }
            else if (WinFabricManifestManager.IsKtlManifestFileName(manifestFileName))
            {
                LoadManifest(manifestFile, ref this.currentKtlManifest);
            }
            else
            {
                Verify.Fail("Unexpected prefix in manifest file name");
            }
        }

        private static void LoadManifest(string manifestFile, ref string currentManifest)
        {
            if (null != currentManifest)
            {
                Utility.TraceSource.WriteError(
                    TraceType,
                    "Attempting to load manifest {0} while manifest {1} is already loaded.",
                    manifestFile,
                    currentManifest);
                Verify.Fail("We are attemping to load a manifest when another version is already loaded.");
            }
            currentManifest = manifestFile;
        }

        private void UnloadManifest(string manifestFile)
        {
            Utility.TraceSource.WriteInfo(
                TraceType,
                "Unloading manifest file {0}",
                manifestFile);

            string manifestFileName = Path.GetFileName(manifestFile);
            if (WinFabricManifestManager.IsFabricManifestFileName(manifestFileName))
            {
                UnloadManifest(manifestFile, ref this.currentFabricManifest);
            }
            else if (WinFabricManifestManager.IsLeaseLayerManifestFileName(manifestFileName))
            {
                UnloadManifest(manifestFile, ref this.currentLeaseLayerManifest);
            }
            else if (WinFabricManifestManager.IsKtlManifestFileName(manifestFileName))
            {
                UnloadManifest(manifestFile, ref this.currentKtlManifest);
            }
            else
            {
                Verify.Fail("Unexpected prefix in manifest file name");
            }
        }

        private static void UnloadManifest(string manifestFile, ref string currentManifest)
        {
            if (null == currentManifest)
            {
                Utility.TraceSource.WriteError(
                    TraceType,
                    "Attempting to unload manifest {0}, but there is no manifest currently loaded.",
                    manifestFile);
                Verify.Fail("We are attemping to unload a manifest, but no manifest is currently loaded.");
            }
            else if (false == manifestFile.Equals(currentManifest))
            {
                Utility.TraceSource.WriteError(
                    TraceType,
                    "Attempting to unload manifest {0}, but the currently loaded manifest is {1}.",
                    manifestFile,
                    currentManifest);
                Verify.Fail("We are attemping to unload a manifest that is different from the currently loaded manifest.");
            }
            else
            {
                currentManifest = null;
            }
        }
    }
}