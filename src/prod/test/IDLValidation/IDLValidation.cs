// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Fabric.Test;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace System.Fabric.Setup
{
    [TestClass]
    public class IDLValidation
    {
        public static string UnitTestDirectory = Environment.ExpandEnvironmentVariables(Constants.UnitTestDirectory);
        public static string DropDirectory = Environment.ExpandEnvironmentVariables(Constants.FabricDropPath);

        /// <summary>
        /// Make a private drop to get current .h files from IDL
        /// </summary>
        [ClassInitialize]
        public static void Initialization()
        {
        }

        /// <summary>
        /// Read IDL (.h) files 
        /// Compare interface definitions from current build and 
        /// public interfaces that have been released in a previous version of winfab.
        /// No new Methods added to existing interface
        /// No existing methods removed
        /// Existing method signature should not be changed.
        /// Guid’s used to identify interfaces should be unique(catch copy/paste errors)
        /// </summary>
        [TestMethod]
        public void ValidateInterfaces()
        {
            List<string> files = GetFileList();
            bool success = true;

            foreach (string file in files)
            {
                LogHelper.Log("File {0} :", file);

                string curPath = ExtractFilePath(file, string.Empty, false);
                Interfaces curInterfaces = ValidationHelper.GetInterfaceObjects(curPath);

                List<string> versions = GetVersionList();
                foreach (string version in versions)
                {
                    LogHelper.Log("Version {0} :", version);
                    string prePath = ExtractFilePath(file, version, true);
                    Interfaces preInterfaces = ValidationHelper.GetInterfaceObjects(prePath);

                    LogHelper.Log("**************************************************************************");
                    LogHelper.Log("Comparing Interfaces:");
                    List<string> differences = preInterfaces.Compare(curInterfaces);
                    success &= (differences == null || differences.Count == 0);
                    LogHelper.Log("{0} interfaces are different.", (differences == null) ? 0 : differences.Count);

                    LogHelper.Log("Comparing Guid:");
                    if (curInterfaces != null && curInterfaces.SameGuidInterfaces.Count > 0)
                    {
                        success &= false;
                        foreach (KeyValuePair<Guid, List<string>> id in curInterfaces.SameGuidInterfaces)
                        {
                            LogHelper.Log("Guid id {0} is used in interfaces :", id.Key);
                            foreach (string s in id.Value)
                            {
                                LogHelper.Log(s);
                            }
                        }
                    }

                    LogHelper.Log("{0} Guids are duplicated.", curInterfaces.SameGuidInterfaces.Count);
                    LogHelper.Log("**************************************************************************");
                }
            }

            Assert.IsTrue(success, "Comparison completed. No interface is changed. No Guid is duplicated.");
        }

        /// <summary>
        /// Read IDL (.h) files 
        /// Compare struct definitions from current build and 
        /// public structs that have been released in a previous version of winfab.
        /// No new members added to existing interface
        /// No existing members removed
        /// Existing members should not be changed.
        /// </summary>
        [TestMethod]
        public void ValidateStructs()
        {
            List<string> files = GetFileList();
            bool success = true;

            foreach (string file in files)
            {
                LogHelper.Log("File {0} :", file);

                string curPath = ExtractFilePath(file, string.Empty, false);

                List<string> content = ValidationHelper.ReadAllLines(curPath);
                Structs curStructs = ValidationHelper.ParseStructs(content);

                List<string> versions = GetVersionList();
                foreach (string version in versions)
                {
                    LogHelper.Log("Version {0} :", version);
                    string prePath = ExtractFilePath(file, version, true);
                    List<string> preContent = ValidationHelper.ReadAllLines(prePath);
                    Structs preStructs = ValidationHelper.ParseStructs(preContent);

                    LogHelper.Log("**************************************************************************");
                    LogHelper.Log("Comparing structs:");
                    List<string> differences = preStructs.Compare(curStructs);
                    success &= (differences == null || differences.Count == 0);
                    LogHelper.Log("{0} structs are different.", (differences == null) ? 0 : differences.Count);                  
                    LogHelper.Log("**************************************************************************");
                }
            }

            Assert.IsTrue(success, "Comparison completed. No structs is changed.");
        }

        /// <summary>
        /// IDL files to compare
        /// </summary>
        /// <returns>file list</returns>
        public static List<string> GetFileList()
        {
            List<string> files = new List<string>();

            // Public IDL
            files.Add("FabricCommon");
            files.Add("FabricRuntime");
            files.Add("FabricClient");
            files.Add("FabricTypes");

            // Internal IDL
            // Required for NetStandard SDK.
            files.Add("fabricbackuprestoreservice_");
            files.Add("FabricClient_");
            files.Add("FabricCommon_");
            files.Add("FabricContainerActivatorService_");
            files.Add("FabricFaultAnalysisService_");
            files.Add("FabricImageStore_");
            files.Add("FabricInfrastructureService_");
            files.Add("FabricReliableMessaging_");
            files.Add("FabricRuntime_");
            files.Add("FabricServiceCommunication_");
            files.Add("FabricTokenValidationService_");
            files.Add("FabricTransport_");
            files.Add("FabricTypes_");
            files.Add("FabricUpgradeOrchestrationService_");
            files.Add("KPhysicalLog_");

            return files;
        }

        /// <summary>
        /// Official release versions
        /// Match directory structure under folder Release
        /// </summary>
        /// <returns>version list</returns>
        public static List<string> GetVersionList()
        {
            List<string> versions = new List<string>();
            versions.Add("v6.2");

            return versions;
        }

        /// <summary>
        /// Get aboslute path for current version or release version
        /// </summary>
        /// <param name="name">File name</param>
        /// <param name="version">Release version (v3.0 /v3.0CU1)</param>
        /// <param name="isRelease">True if it is release version</param>
        /// <returns>Absolute path of file. (The file name of release version will contains the version as suffix.)</returns>
        public static string ExtractFilePath(string name, string version, bool isRelease)
        {
            // If the file name ends with '_', it is an internal idl file.
            string intermediatePath = (name.Substring(name.Length - 1, 1).Equals("_"))
                ? Constants.InternalIncDirectory
                : Constants.IncDirectory;

            string path = isRelease
                ? Path.Combine(UnitTestDirectory, Constants.ReleaseFileDirectory, version, string.Format(@"{0}.h", name))
                : Path.Combine(DropDirectory, intermediatePath, string.Format("{0}.h", name));

            return path;
        }        
    }
}