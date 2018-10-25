// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Diagnostics;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;

    internal class RollbackOperation : RemoveOperation
    {
        protected override void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType clusteManifest,
            Infrastructure infrastructure)
        {
            this.RevertTarget(parameters);
        }

        private void RevertTarget(DeploymentParameters parameters)
        {
            string targetInformationFile = parameters.DeploymentSpecification.GetTargetInformationFile();
            if (!File.Exists(targetInformationFile))
            {
                DeployerTrace.WriteInfo("TargetInformation.xml file not found. Return as no operation is needed.");
                return;
            }

            TargetInformationType targetInfo = XmlHelper.ReadXml<TargetInformationType>(targetInformationFile);
            if (targetInfo.CurrentInstallation == null)
            {
                DeployerTrace.WriteInfo("No current information. No operation to be done.");
                return;
            }

            TargetInformationType newTargetInfo = new TargetInformationType();
            newTargetInfo.TargetInstallation = targetInfo.CurrentInstallation;
            newTargetInfo.CurrentInstallation = targetInfo.TargetInstallation;
            XmlHelper.WriteXml<TargetInformationType>(targetInformationFile, newTargetInfo);

            // Do not need to reinstall the old MSI on rollback on linux. The updater service takes care of this
#if !DotNetCoreClrLinux
            if (!string.IsNullOrEmpty(targetInfo.CurrentInstallation.MSILocation))
            {
                string[] batFileContent = new string[4];
                batFileContent[0] = "sc stop FabricHostSvc";
                batFileContent[1] = "TIMEOUT /T 10";
                batFileContent[2] = string.Format(
                    CultureInfo.InvariantCulture,
                    "msiexec /i \"{0}\" /lv \"{1}\" /q IACCEPTEULA=Yes",
                    targetInfo.CurrentInstallation.MSILocation,
                    parameters.DeploymentSpecification.GetInstallerLogFile("Revert", Utility.GetCurrentCodeVersion(null)));
                batFileContent[3] = "sc start FabrichostSvc";
                string installerScriptFileLocation = parameters.DeploymentSpecification.GetInstallerScriptFile("Revert");
                File.WriteAllLines(installerScriptFileLocation, batFileContent);
                ExecuteScript(installerScriptFileLocation);
            }
#endif
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct STARTUPINFO
        {
            public Int32 cb;
            public string lpReserved;
            public string lpDesktop;
            public string lpTitle;
            public Int32 dwX;
            public Int32 dwY;
            public Int32 dwXSize;
            public Int32 dwYSize;
            public Int32 dwXCountChars;
            public Int32 dwYCountChars;
            public Int32 dwFillAttribute;
            public Int32 dwFlags;
            public Int16 wShowWindow;
            public Int16 cbReserved2;
            public IntPtr lpReserved2;
            public IntPtr hStdInput;
            public IntPtr hStdOutput;
            public IntPtr hStdError;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct PROCESS_INFORMATION
        {
            public IntPtr hProcess;
            public IntPtr hThread;
            public int dwProcessId;
            public int dwThreadId;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct SECURITY_ATTRIBUTES
        {
            public int nLength;
            public IntPtr lpSecurityDescriptor;
            public int bInheritHandle;
        }

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CreateProcess(string lpApplicationName,
            string lpCommandLine, ref SECURITY_ATTRIBUTES lpProcessAttributes,
            ref SECURITY_ATTRIBUTES lpThreadAttributes, bool bInheritHandles,
            uint dwCreationFlags, IntPtr lpEnvironment, string lpCurrentDirectory,
            [In] ref STARTUPINFO lpStartupInfo,
            out PROCESS_INFORMATION lpProcessInformation);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern UInt32 WaitForSingleObject(IntPtr hHandle, UInt32 dwMilliseconds);

        private const uint CREATE_UNICODE_ENVIRONMENT = 0x00000400;
        private const uint CREATE_BREAKAWAY_FROM_JOB = 0x01000000;
        private const uint CREATE_NO_WINDOW = 0x08000000;
        private const UInt32 INFINITE = 0xFFFFFFFF;

        private void ExecuteScript(string installerScriptFileLocation)
        {
            DeployerTrace.WriteInfo(String.Format("Rollback execute the installer script {0}", installerScriptFileLocation));
            bool retValue;
            PROCESS_INFORMATION pInfo = new PROCESS_INFORMATION();
            STARTUPINFO sInfo = new STARTUPINFO();
            SECURITY_ATTRIBUTES pSec = new SECURITY_ATTRIBUTES();
            SECURITY_ATTRIBUTES tSec = new SECURITY_ATTRIBUTES();
            pSec.nLength = Marshal.SizeOf(pSec);
            tSec.nLength = Marshal.SizeOf(tSec);
            // Setting this flag ensures that all the child processes are detached from the Job object that this process is associated with.
            retValue = CreateProcess(installerScriptFileLocation, null,
            ref pSec, ref tSec, false, CREATE_UNICODE_ENVIRONMENT | CREATE_BREAKAWAY_FROM_JOB | CREATE_NO_WINDOW,
            IntPtr.Zero, null, ref sInfo, out pInfo);
            
            if (!retValue)
            {
                DeployerTrace.WriteError("Rollback failed to execute the installer script");
                throw new InvalidDeploymentException(StringResources.Error_FabricDeployer_RollbackFailed_Formatted);
            }
            WaitForSingleObject(pInfo.hProcess, INFINITE);
        }
    }
}