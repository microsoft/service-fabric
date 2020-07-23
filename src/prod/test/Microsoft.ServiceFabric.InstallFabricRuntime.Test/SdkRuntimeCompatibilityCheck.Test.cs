// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.InstallFabricRuntime.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.SdkRuntimeCompatibilityCheck;
    using System;
    using System.IO;
    using System.Reflection;

    [TestClass]
    public class SdkRuntimeCompatibilityCheckTest
    {
        private static string BaseDir = new FileInfo(Assembly.GetExecutingAssembly().Location).DirectoryName;

        static readonly string emptyJson = "CompatibilityCheckModel_empty.json";
        static readonly string corruptedJson = "CompatibilityCheckModel_corruptedjson.json";
        static readonly string incompleteJson = "CompatibilityCheckModel_incomplete.json";
        static readonly string wrongVersionJson = "CompatibilityCheckModel_wrongversion.json";
        static readonly string corruptedVersionJson = "CompatibilityCheckModel_corruptedversion.json";
        static readonly string correctJson = "CompatibilityCheckModel_correct.json";


        internal bool SdkRuntimeCompatibilityCheckTest_Utility(string jsonFileName) 
        {
            string jsonFilePath = Path.Combine(SdkRuntimeCompatibilityCheckTest.BaseDir, jsonFileName);
            Console.WriteLine("BaseDir: {0}", SdkRuntimeCompatibilityCheckTest.BaseDir);
            Console.WriteLine("Running {0}", jsonFilePath);
            string jsonString = File.ReadAllText(jsonFilePath);
            string errorMessage;
            SdkRuntimeCompatibilityModel model = SdkRuntimeCompatibilityModel.TryGetSdkRuntimeCompatibilityModel(jsonString, out errorMessage);
            return SdkRuntimeCompatibilityModel.IsValid(model, out errorMessage);
        }

        [TestMethod]
        [Owner("taxi")]
        public void SdkRuntimeCompatibilityCheckTest_CheckNull()
        {
            Assert.IsFalse(SdkRuntimeCompatibilityCheckTest_Utility(emptyJson));
        }

        [TestMethod]
        [Owner("taxi")]
        public void SdkRuntimeCompatibilityCheckTest_CheckIncorrectContent()
        {
            Assert.IsFalse(SdkRuntimeCompatibilityCheckTest_Utility(corruptedJson));
        }

        [TestMethod]
        [Owner("taxi")]
        public void SdkRuntimeCompatibilityCheckTest_CheckIncompleteModel()
        {
            Assert.IsFalse(SdkRuntimeCompatibilityCheckTest_Utility(incompleteJson));
        }

        [TestMethod]
        [Owner("taxi")]
        public void SdkRuntimeCompatibilityCheckTest_CheckWrongVersionFormat()
        {
            Assert.IsFalse(SdkRuntimeCompatibilityCheckTest_Utility(wrongVersionJson));
        }

        [TestMethod]
        [Owner("taxi")]
        public void SdkRuntimeCompatibilityCheckTest_CheckCorruptedVersionFormat()
        {
            Assert.IsFalse(SdkRuntimeCompatibilityCheckTest_Utility(corruptedVersionJson));
        }

        [TestMethod]
        [Owner("taxi")]
        public void SdkRuntimeCompatibilityCheckTest_CheckCorrect()
        {
            Assert.IsTrue(SdkRuntimeCompatibilityCheckTest_Utility(correctJson));
        }        
    }
}