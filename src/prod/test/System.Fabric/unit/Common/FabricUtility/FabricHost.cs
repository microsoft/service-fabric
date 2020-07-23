// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.FabricUtility
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.Diagnostics;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Net;
    using System.Text;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Xml;
    using System.Xml.Serialization;
    /// <summary>
    /// Class encapsulating a fabric Host
    /// </summary>
    public class FabricHost
    {
        public static string FabricHostSettingsFile = "FabricHostSettings.xml";
        public static string FabricHostSettingsFileEnvVariable = "FabricSettingsFileName";
        public static string ExeName = "FabricHost.exe";
        public static string FabricSetupExeName = "FabricSetup.exe";

        private Process fabricHostProcess;
        private string deploymentRootPath;
        private string sourceBinPath;
        private string activatorAddress;
        private string fabrichostSettingsFilePath;

        public FabricHost(string sourceBinPath,
            string deploymentRootPath,
            string activatorAddress,
            string fabricBinPath)
        {
            Debug.Assert(!string.IsNullOrWhiteSpace(deploymentRootPath));
            Debug.Assert(!string.IsNullOrWhiteSpace(activatorAddress));
            Debug.Assert(!string.IsNullOrWhiteSpace(sourceBinPath));
            Debug.Assert(!string.IsNullOrWhiteSpace(fabricBinPath));
            this.deploymentRootPath = deploymentRootPath;
            this.activatorAddress = activatorAddress;
            this.sourceBinPath = sourceBinPath;
            this.fabrichostSettingsFilePath = Path.Combine(this.deploymentRootPath, FabricHostSettingsFile);
            using (this.GetLogContext("FabricHostConstructor"))
            {
                LogHelper.Log("Creating FabricHostSettings");
                this.CreateFabricHostSettingsFile(fabricBinPath);
            }
        }

        public void CreateFabricHostSettingsFile(string fabricBinPath)
        {
            StringBuilder path = new StringBuilder();
            string envPath = Environment.GetEnvironmentVariable("Path");
            if (!String.IsNullOrEmpty(envPath))
            {
                path.AppendFormat("{0};{1}", envPath, fabricBinPath);
            }
            else
            {
                path.Append(fabricBinPath);
            }
            Environment.SetEnvironmentVariable("Path", path.ToString());
            List<SettingsTypeSection> sections = new List<SettingsTypeSection>();
            SettingsTypeSection traceSection = new SettingsTypeSection();
            traceSection.Name = "Trace/Etw";
            SettingsTypeSectionParameter param = new SettingsTypeSectionParameter();
            param.Name = "Level";
            param.Value = "5";
            traceSection.Parameter = new SettingsTypeSectionParameter[1] { param };
            sections.Add(traceSection);
            SettingsTypeSection hostSettings = new SettingsTypeSection();
            hostSettings.Name = "FabricHost";
            SettingsTypeSectionParameter param1 = new SettingsTypeSectionParameter();
            param1.Name = "ActivatorServiceAddress";
            param1.Value = this.activatorAddress;
            hostSettings.Parameter = new SettingsTypeSectionParameter[1] { param1 };
            sections.Add(hostSettings);

            WriteFabricHostSettingsFile(new SettingsType() { Section = sections.ToArray() });
        }

        private void WriteFabricHostSettingsFile(SettingsType value)
        {
            XmlWriterSettings settings = new XmlWriterSettings();
            settings.Indent = true;
            using (XmlWriter writer = XmlWriter.Create(this.fabrichostSettingsFilePath, settings))
            {
                XmlSerializer serializer = new XmlSerializer(typeof(SettingsType));
                serializer.Serialize(writer, value);
            }
        }


        /// <summary>
        /// Deploys windows fabric host binaries
        /// </summary>
        /// <param name="binaries">all binaries, exe etc</param>
        public void DeployFabricHost()
        {
            using (this.GetLogContext("DeployFabricHost"))
            {
                FileUtility.CopyFilesToDirectory(this.GetFabricHostBinaries(), this.deploymentRootPath);
            }
        }

        /// <summary>
        /// Starts fabrichost.exe 
        /// </summary>
        public void Start()
        {
            Debug.Assert(!String.IsNullOrEmpty(this.deploymentRootPath), "FabricHostPath not present");
            
            Environment.SetEnvironmentVariable("FabricActivatorAddress", this.activatorAddress);

            using (this.GetLogContext("StartFabricHost"))
            {
                var psi = this.CreateProcessStartInfoForFabricHost();

                LogHelper.Log("Starting ...");
                this.fabricHostProcess = Process.Start(psi);
                LogHelper.Log("fabrichost.exe pid {0}", this.fabricHostProcess.Id);
            }
        }

        private ProcessStartInfo CreateProcessStartInfoForFabricHost()
        {
            ProcessStartInfo psi = new ProcessStartInfo()
            {
                Arguments = "-c -skipfabricsetup",
                FileName = Path.Combine(this.deploymentRootPath, "fabrichost.exe"),
                WorkingDirectory = this.deploymentRootPath,
                UseShellExecute = false,
            };

            psi.EnvironmentVariables[FabricHost.FabricHostSettingsFileEnvVariable] = this.fabrichostSettingsFilePath;

            return psi;
        }

        private IEnumerable<string> GetFabricHostBinaries()
        {
            List<string> binaries = new List<string>();
            binaries.Add(Path.Combine(this.sourceBinPath, FabricHost.ExeName));
            binaries.Add(Path.Combine(this.sourceBinPath, FabricHost.FabricSetupExeName));
            return binaries;
        }

        #region IDisposable Members

        public void Dispose()
        {
            if (this.fabricHostProcess != null)
            {
                LogHelper.Log("Cleaning up fabricExe: {0}", this.fabricHostProcess.Id);
                if (!this.fabricHostProcess.HasExited)
                {
                    LogHelper.Log("Cleaning up force close {0}", this.fabricHostProcess.Id);
                    this.fabricHostProcess.Kill();

                    LogHelper.Log("WaitForExit: {0}", this.fabricHostProcess.Id);
                    this.fabricHostProcess.WaitForExit();
                    LogHelper.Log("Completed kill of fabric.exe {0}", this.fabricHostProcess.Id);
                }

                this.fabricHostProcess.Close();
                this.fabricHostProcess = null;
            }
        }

        private IDisposable GetLogContext(string operationName)
        {
            return new LogHelper(string.Format("FabricHost: {0}", operationName));
        }

        #endregion
    }
}