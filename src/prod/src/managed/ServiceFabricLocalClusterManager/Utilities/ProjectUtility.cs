// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace LocalClusterManager.Utilities
{
    using System;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Common;
    using System.IO;
    using System.Security;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Windows.Forms;
    using Microsoft.Win32;

    internal class ProjectUtility
    {
        #region Public Static Properties

        internal static readonly String ServiceFabricHostServiceName = "FabricHostSvc";
        
        internal static readonly int BalloonTipTimeout = 30000;
        internal static readonly int ClusterOperationRetryCount = 2;
        internal static readonly int ClusterConnectionRetryCount = 6;
        internal static readonly int ClusterConnectionRetryDelay = 30000;
        internal static readonly int ClusterSetupTimeout = 240000;
        internal static readonly int MeshClusterSetupTimeout = ClusterSetupTimeout + 180000;
        internal static readonly int ClusterCleanupTimeout = 180000;
        
        internal static readonly String DefaultTrayIconName = "LocalClusterManager.Resources.ServiceFabricIcon16.ico";
        internal static readonly String DefaultSetupLogFileName = "DevClusterSetup.log";

        internal static readonly String ServiceFabricSdkRegKey = @"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Service Fabric SDK";
        internal static readonly String FabricMeshSDKVersionRegValue = "FabricMeshSDKVersion";
        internal static readonly String FabricSdkInstallPathRegValueName = "FabricSDKInstallPath";
        internal static readonly String LocalClusterNodeCountRegValueName = "LocalClusterNodeCount";
        internal static readonly String IsMeshClusterRegValueName = "IsMeshCluster";
        internal static readonly String FabricSdkInstallPath = (String)Registry.GetValue(ServiceFabricSdkRegKey, FabricSdkInstallPathRegValueName, null);
        internal static readonly String DevClusterRoot = Environment.GetEnvironmentVariable("SystemDrive") + @"\SFDevCluster";
        internal static readonly String DevClusterLogRoot = Path.Combine(DevClusterRoot, "Log");
        internal static readonly String DevClusterTraceFolder = Path.Combine(DevClusterLogRoot, "Traces");
        internal static readonly String DevClusterCrashDumpsFolder = Path.Combine(DevClusterLogRoot, "CrashDumps");

        internal static readonly String TicketFileDirectoryRelativePath = @"Fabric\Work";

        #endregion

        #region Public Static Methods

        public static ToolStripMenuItem CreateMenuItem(String menuItemText, EventHandler onClickHandler = null, bool enableMenuItem = true)
        {
            ToolStripMenuItem menuItem = new ToolStripMenuItem
            {
                Text = menuItemText,
                Enabled = enableMenuItem
            };

            if (onClickHandler != null)
            {
                menuItem.Click += onClickHandler;
            }

            return menuItem;
        }

        public static String GetSetupClusterScriptPath()
        {
            return Path.Combine(FabricSdkInstallPath, @"ClusterSetup\DevClusterSetup.ps1");
        }

        public static ClusterMode GetCurrentClusterMode()
        {
            var clusterMode = ClusterMode.None;

            try
            {
                string localClusterNodeCount = (string)Registry.GetValue(ServiceFabricSdkRegKey, LocalClusterNodeCountRegValueName, null);
                string IsMeshCluster = (string)Registry.GetValue(ServiceFabricSdkRegKey, IsMeshClusterRegValueName, null);

                if (localClusterNodeCount != null)
                {
                    if (String.IsNullOrEmpty(IsMeshCluster) || IsMeshCluster.ToLower().Equals("false"))
                    {
                        if (localClusterNodeCount.Equals("1", StringComparison.OrdinalIgnoreCase))
                        {
                            clusterMode = ClusterMode.OneNode;
                        }
                        else if (localClusterNodeCount.Equals("5", StringComparison.OrdinalIgnoreCase))
                        {
                            clusterMode = ClusterMode.FiveNode;
                        }
                    }
                    else // Mesh Cluster
                    {
                        if (localClusterNodeCount.Equals("1", StringComparison.OrdinalIgnoreCase))
                        {
                            clusterMode = ClusterMode.MeshOneNode;
                        }
                        else if (localClusterNodeCount.Equals("5", StringComparison.OrdinalIgnoreCase))
                        {
                            clusterMode = ClusterMode.MeshFiveNode;
                        }
                    }
                }
            }
            catch (SecurityException)
            {
            }
            catch (IOException)
            {
            }

            return clusterMode;
        }

        public static String GetRemoveClusterScriptPath()
        {
            return Path.Combine(FabricSdkInstallPath, @"ClusterSetup\CleanCluster.ps1");
        }

        public static String GetServiceFabricExplorerPath()
        {
            return Path.Combine(FabricSdkInstallPath, @"Tools\ServiceFabricExplorer\ServiceFabricExplorer.exe");
        }

        public static CommandExecutionResult ExecutePowerShellScript(String pathToScriptFile, string arguments, int timeout)
        {
            String psCommand = "PowerShell.exe";
            
            StringBuilder argsBuilder = new StringBuilder();
            argsBuilder.Append(" -WindowStyle Hidden");
            argsBuilder.Append(" -NonInteractive");
            argsBuilder.Append(" -ExecutionPolicy RemoteSigned");
            argsBuilder.Append(" -Command \"& '");
            argsBuilder.Append(pathToScriptFile);
            argsBuilder.Append("' " + arguments +"\"");

            var cmdExecutionResult = ExecuteCommand(psCommand, argsBuilder.ToString(), timeout);
            return cmdExecutionResult;
        }

        public static CommandExecutionResult ExecuteCommand(String commandName, String arguments, int timeout)
        {
            using (Process proc = new Process())
            {
                var commandExecutionResult = new CommandExecutionResult();

                proc.StartInfo.FileName = commandName;
                proc.StartInfo.Arguments = arguments;
                proc.StartInfo.UseShellExecute = false;
                proc.StartInfo.RedirectStandardError = true;

                proc.Start();
                if (timeout < 0)
                {
                    timeout = -1;
                }

                if (!proc.WaitForExit(timeout))
                {
                    try
                    {
                        proc.Kill();

                        // Make sure the standard output and error buffers are flushed.
                        proc.WaitForExit();
                    }
                    catch (Exception)
                    {
                        // Ignore
                    }

                    commandExecutionResult.ExitCode = 1;
                    return commandExecutionResult;
                }

                var standardErrorOutput = proc.StandardError.ReadToEnd();                
                commandExecutionResult.ExitCode = proc.ExitCode;
                commandExecutionResult.AppendOutput(standardErrorOutput);
                return commandExecutionResult;
            }
        }

        public static void LaunchServiceFabricExplorer(string url)
        {
            Process.Start(url);
        }

        public static void ExecuteCommandAsync(String commandName, String arguments)
        {
            using (Process proc = new Process())
            {
                proc.StartInfo.FileName = commandName;
                proc.StartInfo.Arguments = arguments;
                proc.StartInfo.UseShellExecute = false;
                
                proc.Start();
            }
        }

        public static bool IsNodeConfigurationDeployed()
        {
            try
            {
                return (RunInMta(NodeConfiguration.GetNodeConfiguration) != null);
            }
            catch (Exception)
            {
                return false;
            }
        }

        public static String GetLocalClusterDataRoot()
        {
            try
            {
                return RunInMta(FabricEnvironment.GetDataRoot);
            }
            catch (Exception)
            {
                return null;
            }
        }

        public static TResult RunInMta<TResult>(Func<TResult> func)
        {
            if (Thread.CurrentThread.GetApartmentState() == ApartmentState.MTA)
            {
                return func();
            }

            try
            {
                return Task.Factory.StartNew(func).Result;
            }
            catch (AggregateException e)
            {
                throw e.InnerException;
            }
        }

        public static void DeleteFileWithRetry(String filePath)
        {
            for (int i = 0; i < 3; i++)
            {
                try
                {
                    File.Delete(filePath);
                    return;
                }
                catch (Exception)
                {
                    // Sleep for few seconds before retrying
                    Thread.Sleep(3);
                }
            }
        }

        internal static ClusterOperationType GetClusterOperationTypeFromString(string operationName)
        {
            operationName = operationName.ToLowerInvariant();

            switch (operationName)
            {
                case "manage":
                    return ClusterOperationType.Manage;
                case "reset":
                    return ClusterOperationType.Reset;
                case "start":
                    return ClusterOperationType.Start;
                case "stop":
                    return ClusterOperationType.Stop;
                case "setuponenodecluster":
                    return ClusterOperationType.SetupOneNodeCluster;
                case "setupfivenodecluster":
                    return ClusterOperationType.SetupFiveNodeCluster;
                case "switchtoonenodecluster":
                    return ClusterOperationType.SwitchToOneNodeCluster;
                case "switchtofivenodecluster":
                    return ClusterOperationType.SwitchToFiveNodeCluster;
                case "remove":
                    return ClusterOperationType.Remove;
                case "setuponenodemeshcluster":
                    return ClusterOperationType.SetupOneNodeMeshCluster;
                case "setupfivenodemeshcluster":
                    return ClusterOperationType.SetupFiveNodeMeshCluster;
                case "switchtoonenodemeshcluster":
                    return ClusterOperationType.SwitchToMeshOneNodeCluster;
                case "switchtofivenodemeshcluster":
                    return ClusterOperationType.SwitchToMeshFiveNodeCluster;
                default:
                    return ClusterOperationType.None;
            }
        }

        internal static void CopyAllFilesFromDir(string srcDir, string dstDir, string pattern = "*")
        {
            var fileList = Directory.GetFiles(srcDir, pattern);
            foreach (var fileIter in fileList)
            {
                var tempFile = Path.Combine(dstDir, fileIter.Substring(srcDir.Length + 1));
                File.Copy(fileIter, tempFile, true);
            }
        }

        #endregion
    }
}