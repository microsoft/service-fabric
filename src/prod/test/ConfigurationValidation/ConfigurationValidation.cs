// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using System.Fabric.Test;
using System.IO;

namespace System.Fabric.Setup
{
    [TestClass]
    public class ConfigurationValidation
    {
        private string dataRoot = string.Empty;

        [TestMethod]
        public void ValidateConfiguration()
        {
            List<string> releaseFiles = new List<string>();
            dataRoot = Environment.GetEnvironmentVariable("_NTTREE");
            string currentPath = Path.Combine(dataRoot, "Configurations.csv");
            string fileList = Path.Combine(dataRoot, @"FabricUnitTests\FileList.xml"); 
            
            HashSet<SectionSetting> currentSettings = ValidationUtility.GetConfigsFromCSV(currentPath, true);
            HashSet<SectionSetting> releaseSettings = null;

            dataRoot = Path.Combine(dataRoot, @"FabricUnitTests");
            Dictionary<string, string> releases = ValidationUtility.GetUpgradeMatrix(dataRoot);
            List<SectionSetting> exceptionList = ValidationUtility.GetExceptionList();
            
            bool success = true;
            foreach (KeyValuePair<string, string> release in releases)
            {
                LogHelper.Log("Start to compare configuration with version {0}", release.Key);
                releaseSettings = ValidationUtility.GetConfigsFromCSV(release.Value, false);
                HashSet<SectionSetting> keys = ValidationUtility.GetDeletedKeys(releaseSettings, currentSettings);

                if(keys != null)
                {
                    LogHelper.Log("{0} keys are deleted.", keys.Count );
                }

                if (keys.Count > 0)
                {
                    foreach (SectionSetting setting in keys)
                    {
                        LogHelper.Log("DeletedKey : Section {0}, Setting {1}", setting.Section, setting.Setting);

                        if (exceptionList.Contains(setting))
                        {
                            LogHelper.Log("It is in the exception list.");
                        }
                        else
                        {
                            success &= false;
                        }
                    }
                }                
            }
            
            Assert.IsTrue(success, "Comparison completed. No keys are deleted.");
        }
    }
}