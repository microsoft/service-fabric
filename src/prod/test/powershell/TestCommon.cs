// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell.Test
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.Fabric.Test;
    using System.Globalization;
    using System.IO;
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using System.Reflection;
    using System.Text;
    using System.Threading;

    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;

    public class TestCommon
    {
        public static Runspace CreateRunSpace(string baseDropPath)
        {
            var sessionState = InitialSessionState.CreateDefault();
            var clientPackageLocation = Path.Combine(baseDropPath, Constants.FabricClientPackageRelativePathToDrop);
            Assert.IsTrue(Directory.Exists(clientPackageLocation));
            var powershellAssemblyLocation = Path.Combine(clientPackageLocation, Constants.PowershellAssemblyName);
            sessionState.ImportPSModule(new[] { powershellAssemblyLocation });
            var runspace = RunspaceFactory.CreateRunspace(sessionState);
            return runspace;
        }

        public static void Log(Command command)
        {
            var paramsBuilder = new StringBuilder();
            if (command.Parameters != null)
            {
                foreach (var param in command.Parameters)
                {
                    paramsBuilder.Append('-');
                    paramsBuilder.Append(param.Name);
                    paramsBuilder.Append(' ');

                    if (param.Value != null)
                    {
                        paramsBuilder.Append(param.Value);
                        paramsBuilder.Append(' ');
                    }
                }
            }

            LogHelper.Log("Execute " + command.CommandText + " " + paramsBuilder.ToString().TrimEnd(' '));
        }

        public static void Log(IEnumerable<PSObject> result)
        {
            if (result == null)
            {
                LogHelper.Log("Output null result");
                return;
            }

            var outputBuilder = new StringBuilder();
            foreach (var obj in result)
            {
                if (obj == null)
                {
                    LogHelper.Log("null");
                    continue;
                }

                outputBuilder.Append(obj);
                outputBuilder.Append('\n');
                foreach (var property in obj.Properties)
                {
                    outputBuilder.Append(property.Name);
                    outputBuilder.Append(':');
                    outputBuilder.Append((property.Value == null) ? "null" : property.Value.ToString());
                    outputBuilder.Append('\n');
                }
            }

            LogHelper.Log("output {0}", outputBuilder.ToString());
        }

        public static void MakeDrop(string dropFolderName)
        {
            var makeDropScriptPath = Path.Combine(Environment.ExpandEnvironmentVariables(Constants.BaseScriptPath), Constants.MakeDropScriptFileName);
            if (File.Exists(makeDropScriptPath))
            {
                var makeDropArguments = string.Format(CultureInfo.InvariantCulture, Constants.MakeDropArgumentPattern, dropFolderName);
                LogHelper.Log("MakeDrop: " + makeDropScriptPath + " " + makeDropArguments);
                ExecuteProcess(makeDropScriptPath, makeDropArguments);
            }
            else
            {
                string fabricDropPath = Environment.ExpandEnvironmentVariables(Constants.FabricDropPath);
                LogHelper.Log("CopyDrop: " + fabricDropPath + " " + dropFolderName);
                DirectoryCopy(fabricDropPath, dropFolderName, true);
            }
        }

        private static void DirectoryCopy(string sourceDirName, string destDirName, bool copySubDirs)
        {
            DirectoryInfo dir = new DirectoryInfo(sourceDirName);
            DirectoryInfo[] dirs = dir.GetDirectories();

            if (!dir.Exists)
            {
                throw new DirectoryNotFoundException(string.Format(
                    CultureInfo.InvariantCulture,
                    "Source directory {0} does not exist or could not be found",
                    sourceDirName));
            }

            if (!Directory.Exists(destDirName))
            {
                Directory.CreateDirectory(destDirName);
            }

            FileInfo[] files = dir.GetFiles();
            foreach (FileInfo file in files)
            {
                string temppath = Path.Combine(destDirName, file.Name);
                file.CopyTo(temppath, false);
            }

            if (copySubDirs)
            {
                foreach (DirectoryInfo subdir in dirs)
                {
                    string temppath = Path.Combine(destDirName, subdir.Name);
                    DirectoryCopy(subdir.FullName, temppath, copySubDirs);
                }
            }
        }

        private static void ExecuteProcess(string command, string arguments = null, int expectExitCode = 0, int timeout = 600000)
        {
            var process = StartProcess(command, arguments);
            if (!process.WaitForExit(timeout))
            {
                process.Kill();
                Assert.Fail("process " + command + " " + arguments + " timeout");
            }

            TestUtility.AssertAreEqual(expectExitCode, process.ExitCode, "process exit code " + process.ExitCode + " should match expect " + expectExitCode);
        }

        private static Process StartProcess(string fileName, string arguments = null)
        {
            var processStartInfo = new ProcessStartInfo()
            {
                FileName = fileName,
                Arguments = arguments,
                WindowStyle = ProcessWindowStyle.Hidden
            };

            return Process.Start(processStartInfo);
        }
    }
}