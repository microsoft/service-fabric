// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace LocalClusterManager
{
    using System;
    using System.ComponentModel;
    using System.Drawing;
    using System.IO;
    using System.IO.Compression;
    using System.Reflection;
    using System.Threading.Tasks;
    using System.Windows.Forms;
    using LocalClusterManager.Resources;
    using LocalClusterManager.Utilities;

    internal class TrayApplicationContext : ApplicationContext
    {
        #region Private Members

        private readonly NotifyIcon trayIcon;
        private readonly ApplicationManager appMgr;

        #endregion

        #region C'tors

        public TrayApplicationContext(ApplicationManager appMgr)
        {
            this.appMgr = appMgr;

            this.trayIcon = new NotifyIcon
            {
                ContextMenuStrip = new ContextMenuStrip(),
                Icon = this.GetTrayIcon(),
                Text = this.GetCurrentClusterModeText(ProjectUtility.GetCurrentClusterMode()),
                Visible = true
            };

            // Attach handler to dynamically build context menu each it opens.
            this.trayIcon.ContextMenuStrip.Opening += this.ContextMenuStripOnOpening;
            this.trayIcon.DoubleClick += this.OnTrayIconDoubleClick;

            // Register events to update tooltip text for the systray icon.
            // This helps in updating the tool tip if cluster is removed, recreated using the SDK PS scripts.
            this.trayIcon.Click += this.OnTrayIconClick;
            this.trayIcon.MouseClick += this.OnTrayIconClick;

            if (appMgr.StartupOperation == ClusterOperationType.None)
            {
                // Display Welcome message
                this.ShowBalloonTip(SR.WelcomeTrayIconText, SR.WelcomeBalloonText, ToolTipIcon.Info);
            }
            else
            {
                // Execute start up operation.
                this.ExecuteOperation(appMgr.StartupOperation);
            }
        }

        #endregion

        #region Private Methods

        private Icon GetTrayIcon()
        {
            Assembly currAssembly = Assembly.GetExecutingAssembly();

            using (Stream iconStream = currAssembly.GetManifestResourceStream(ProjectUtility.DefaultTrayIconName))
            {
                Icon trayIconFile = new Icon(iconStream);
                return trayIconFile;
            }
        }

        private void ContextMenuStripOnOpening(object sender, CancelEventArgs cancelEventArgs)
        {
            // Allow the context menu to continue opening.
            cancelEventArgs.Cancel = false;
            bool isClusterSetup = ClusterManager.IsClusterSetup;
            bool isMeshSDKInstalled = ClusterManager.IsMeshSDKInstalled;

            this.trayIcon.ContextMenuStrip.Items.Clear();

            this.trayIcon.ContextMenuStrip.Items.Add(
                ProjectUtility.CreateMenuItem(SR.ManageClusterMenuItemText, this.ManageClusterMenuItemClick, ClusterManager.IsClusterRunning));
            this.trayIcon.ContextMenuStrip.Items.Add(
                ProjectUtility.CreateMenuItem(SR.ResetClusterMenuItemText, this.ResetClusterMenuItemClick, true));

            this.trayIcon.ContextMenuStrip.Items.Add(new ToolStripSeparator());

            this.trayIcon.ContextMenuStrip.Items.Add(
                ProjectUtility.CreateMenuItem(SR.StartClusterMenuItemText, this.StartClusterMenuItemClick, ClusterManager.IsClusterStopppedOrStopPending));
            this.trayIcon.ContextMenuStrip.Items.Add(
                ProjectUtility.CreateMenuItem(SR.StopClusterMenuItemText, this.StopClusterMenuItemClick, ClusterManager.IsClusterRunningOrStartPending));

            this.trayIcon.ContextMenuStrip.Items.Add(new ToolStripSeparator());

            // change context menu text to say "Switch Cluster" if cluster is already setup and running.
            if (isClusterSetup)
            {
                var switchClusterMenu = ProjectUtility.CreateMenuItem(SR.SwitchClusterModeText);

                switchClusterMenu.DropDownItems.Add(
                    ProjectUtility.CreateMenuItem(
                        SR.ClusterMode_OneNode,
                        this.SwitchToOneNodeClusterMenuItemClick,
                        !ProjectUtility.GetCurrentClusterMode().Equals(ClusterMode.OneNode)));

                switchClusterMenu.DropDownItems.Add(
                    ProjectUtility.CreateMenuItem(
                        SR.ClusterMode_FiveNode,
                        this.SwitchToFiveNodeClusterMenuItemClick,
                        !ProjectUtility.GetCurrentClusterMode().Equals(ClusterMode.FiveNode)));

                if (isMeshSDKInstalled)
                {
                    switchClusterMenu.DropDownItems.Add(new ToolStripSeparator());

                    switchClusterMenu.DropDownItems.Add(
                        ProjectUtility.CreateMenuItem(
                            SR.ClusterMode_MeshOneNode,
                            this.SwitchToMeshOneNodeClusterMenuItemClick,
                            !ProjectUtility.GetCurrentClusterMode().Equals(ClusterMode.MeshOneNode)));

                    switchClusterMenu.DropDownItems.Add(
                        ProjectUtility.CreateMenuItem(
                            SR.ClusterMode_MeshFiveNode,
                            this.SwitchToMeshFiveNodeClusterMenuItemClick,
                            !ProjectUtility.GetCurrentClusterMode().Equals(ClusterMode.MeshFiveNode)));
                }

                this.trayIcon.ContextMenuStrip.Items.Add(switchClusterMenu);
            }
            else
            {
                // Add Set Up cluster menu and submenu for One Node and Five Node
                var setupClusterMenu = ProjectUtility.CreateMenuItem(SR.SetupClusterMenuItemText);
                setupClusterMenu.DropDownItems.Add(
                    ProjectUtility.CreateMenuItem(
                        SR.ClusterMode_OneNode,
                        this.SetupOneNodeClusterMenuItemClick));

                setupClusterMenu.DropDownItems.Add(
                    ProjectUtility.CreateMenuItem(
                        SR.ClusterMode_FiveNode,
                        this.SetupFiveNodeClusterMenuItemClick));

                if (isMeshSDKInstalled)
                {
                    setupClusterMenu.DropDownItems.Add(new ToolStripSeparator());

                    setupClusterMenu.DropDownItems.Add(
                        ProjectUtility.CreateMenuItem(
                            SR.ClusterMode_MeshOneNode,
                            this.SetupMeshOneNodeClusterMenuItemClick));

                    setupClusterMenu.DropDownItems.Add(
                        ProjectUtility.CreateMenuItem(
                            SR.ClusterMode_MeshFiveNode,
                            this.SetupMeshFiveNodeClusterMenuItemClick));
                }

                this.trayIcon.ContextMenuStrip.Items.Add(setupClusterMenu);
            }

            this.trayIcon.ContextMenuStrip.Items.Add(
                ProjectUtility.CreateMenuItem(SR.RemoveClusterMenuItemText, this.RemoveClusterMenuItemClick, ClusterManager.IsClusterSetup));

            this.trayIcon.ContextMenuStrip.Items.Add(new ToolStripSeparator());

            this.trayIcon.ContextMenuStrip.Items.Add(
                ProjectUtility.CreateMenuItem(SR.PackageLogsMenuItemText, this.PackageTracesMenuItemClick, true));

            this.trayIcon.ContextMenuStrip.Items.Add(new ToolStripSeparator());

            this.trayIcon.ContextMenuStrip.Items.Add(
                ProjectUtility.CreateMenuItem(SR.ExitMenuItemText, this.ExitMenuItemClick, true));

            this.UpdateTrayIconText();
        }

        private async void ManageClusterMenuItemClick(object sender, EventArgs e)
        {
            if (ClusterManager.IsClusterRunning)
            {
                this.trayIcon.ContextMenuStrip.Enabled = false;

                try
                {
                    this.CheckForPendingRestart(ClusterOperationType.Manage);

                    string sfxUrl = await ClusterManager.GetServiceFabricExplorerUrl();
                    ProjectUtility.LaunchServiceFabricExplorer(sfxUrl);
                }
                catch (Exception)
                {
                    this.ShowBalloonTip(SR.FailedToOpenLCE, ToolTipIcon.Error);
                }

                this.trayIcon.ContextMenuStrip.Enabled = true;
            }
            else
            {
                this.ShowBalloonTip(SR.ClusterNotSetupOrNotRunning, ToolTipIcon.Info);
            }
        }

        private async void SwitchToOneNodeClusterMenuItemClick(object sender, EventArgs e)
        {
            if (this.ShowYesNoMessageBox(SR.ClusterOneNodeSwitchConfirmation) == DialogResult.No)
            {
                return;
            }

            await this.SwitchToOneNodeCluster_ConfirmationObtained();
        }

        private async Task SwitchToOneNodeCluster_ConfirmationObtained()
        {
            this.CheckForAdminPrivileges(ClusterOperationType.SwitchToOneNodeCluster);
            this.trayIcon.ContextMenuStrip.Enabled = false;
            this.ShowBalloonTip(SR.SwitchToOneNodeClusterBalloonTip, ToolTipIcon.Info);
            this.SetTrayIconText(SR.SwitchToOneNodelusterToolTip);

            bool result = await this.PerformOperationWithRetry(
                () => ClusterManager.ResetClusterAsync(ClusterMode.OneNode),
                SR.SwitchClusterToOneNodeSuccessBalloonTip,
                WrapToolTipWithLogDetails(SR.ErroSwitchingToOneNodeClusterBalloonTip),
                WrapToolTipWithLogDetails(SR.SwitchClusterToOneNodeFailedBalloonTip));

            this.appMgr.RecordClusterReset();
            this.SetTrayIconText(this.GetCurrentClusterModeText(ClusterMode.OneNode));
            this.trayIcon.ContextMenuStrip.Enabled = true;
        }

        private async void SwitchToFiveNodeClusterMenuItemClick(object sender, EventArgs e)
        {
            if (this.ShowYesNoMessageBox(SR.ClusterFiveNodeSwitchConfirmation) == DialogResult.No)
            {
                return;
            }

            await this.SwitchToFiveNodeCluster_ConfirmationObtained();
        }

        private async Task SwitchToFiveNodeCluster_ConfirmationObtained()
        {
            this.CheckForAdminPrivileges(ClusterOperationType.SwitchToFiveNodeCluster);

            this.trayIcon.ContextMenuStrip.Enabled = false;
            this.ShowBalloonTip(SR.SwitchToFiveNodeClusterBalloonTip, ToolTipIcon.Info);
            this.SetTrayIconText(SR.SwitchToFiveNodelusterToolTip);

            await this.PerformOperationWithRetry(
                () => ClusterManager.ResetClusterAsync(ClusterMode.FiveNode),
                SR.SwitchClusterToFiveNodeSuccessBalloonTip,
                WrapToolTipWithLogDetails(SR.ErroSwitchingToFiveNodeClusterBalloonTip),
                WrapToolTipWithLogDetails(SR.SwitchClusterToFiveNodeFailedBalloonTip));

            this.appMgr.RecordClusterReset();
            this.SetTrayIconText(this.GetCurrentClusterModeText(ClusterMode.FiveNode));
            this.trayIcon.ContextMenuStrip.Enabled = true;
        }

        private async void SwitchToMeshOneNodeClusterMenuItemClick(object sender, EventArgs e)
        {
            if (this.ShowYesNoMessageBox(SR.ClusterOneNodeSwitchConfirmation) == DialogResult.No)
            {
                return;
            }

            await this.SwitchToMeshOneNodeCluster_ConfirmationObtained();
        }

        private async Task SwitchToMeshOneNodeCluster_ConfirmationObtained()
        {
            this.CheckForAdminPrivileges(ClusterOperationType.SwitchToMeshOneNodeCluster);
            this.trayIcon.ContextMenuStrip.Enabled = false;
            this.ShowBalloonTip(SR.SwitchToMeshOneNodeClusterBalloonTip, ToolTipIcon.Info);
            this.SetTrayIconText(SR.SwitchToMeshOneNodelusterToolTip);

            bool result = await this.PerformOperationWithRetry(
                () => ClusterManager.ResetClusterAsync(ClusterMode.MeshOneNode),
                SR.SwitchClusterToMeshOneNodeSuccessBalloonTip,
                WrapToolTipWithLogDetails(SR.ErrorSwitchingToMeshOneNodeClusterBalloonTip),
                WrapToolTipWithLogDetails(SR.SwitchClusterToOneNodeFailedBalloonTip));

            this.appMgr.RecordClusterReset();
            this.SetTrayIconText(this.GetCurrentClusterModeText(ClusterMode.MeshOneNode));
            this.trayIcon.ContextMenuStrip.Enabled = true;
        }

        private async void SwitchToMeshFiveNodeClusterMenuItemClick(object sender, EventArgs e)
        {
            if (this.ShowYesNoMessageBox(SR.ClusterMeshFiveNodeSwitchConfirmation) == DialogResult.No)
            {
                return;
            }

            await this.SwitchToMeshFiveNodeCluster_ConfirmationObtained();
        }

        private async Task SwitchToMeshFiveNodeCluster_ConfirmationObtained()
        {
            this.CheckForAdminPrivileges(ClusterOperationType.SwitchToMeshFiveNodeCluster);
            this.trayIcon.ContextMenuStrip.Enabled = false;
            this.ShowBalloonTip(SR.SwitchToMeshFiveNodeClusterBalloonTip1, ToolTipIcon.Info);
            this.SetTrayIconText(SR.SwitchToMeshFiveNodelusterToolTip1);

            bool result = await this.PerformOperationWithRetry(
                () => ClusterManager.ResetClusterAsync(ClusterMode.MeshFiveNode),
                SR.SwitchClusterToMeshFiveNodeSuccessBalloonTip1,
                WrapToolTipWithLogDetails(SR.ErrorSwitchingToMeshFiveNodeClusterBalloonTip1),
                WrapToolTipWithLogDetails(SR.SwitchClusterToMeshFiveNodeFailedBalloonTip1));

            this.appMgr.RecordClusterReset();
            this.SetTrayIconText(this.GetCurrentClusterModeText(ClusterMode.MeshFiveNode));
            this.trayIcon.ContextMenuStrip.Enabled = true;
        }

        private async void ResetClusterMenuItemClick(object sender, EventArgs e)
        {
            if (this.ShowYesNoMessageBox(SR.ClusterResetConfirmation) == DialogResult.No)
            {
                return;
            }

            await this.ResetCluster_ConfirmationObtained();
        }

        private async Task ResetCluster_ConfirmationObtained()
        {
            this.CheckForAdminPrivileges(ClusterOperationType.Reset);

            this.trayIcon.ContextMenuStrip.Enabled = false;
            this.ShowBalloonTip(SR.ResetClusterBalloonTip, ToolTipIcon.Info);
            this.SetTrayIconText(SR.ResetClusterToolTip);

            var currentClusterMode = ProjectUtility.GetCurrentClusterMode();

            await this.PerformOperationWithRetry(
                    () => ClusterManager.ResetClusterAsync(currentClusterMode),
                    SR.ClusterResetSuccessBalloonTip,
                    WrapToolTipWithLogDetails(SR.ErrorResettingClusterBalloonTip),
                    WrapToolTipWithLogDetails(SR.ClusterResetFailedBalloonTip));

            this.appMgr.RecordClusterReset();
            this.SetTrayIconText(this.GetCurrentClusterModeText(currentClusterMode));
            this.trayIcon.ContextMenuStrip.Enabled = true;
        }

        private async void StartClusterMenuItemClick(object sender, EventArgs e)
        {
            if (ClusterManager.IsClusterStopppedOrStopPending)
            {
                this.CheckForAdminPrivileges(ClusterOperationType.Start);
                this.CheckForPendingRestart(ClusterOperationType.Start);

                this.trayIcon.ContextMenuStrip.Enabled = false;
                this.ShowBalloonTip(SR.StartClusterBalloonTip, ToolTipIcon.Info);
                this.SetTrayIconText(SR.StartClusterToolTip);

                try
                {
                    await ClusterManager.StartClusterAsync();

                    this.ShowBalloonTip(SR.ClusterStartConnectBalloonTip, ToolTipIcon.Info);
                    this.SetTrayIconText(SR.ConnectClusterToolTip);

                    if (await ClusterManager.TryConnectClusterWithRetryAsync())
                    {
                        this.ShowBalloonTip(SR.ConnectClusterSuccessBalloonTip, ToolTipIcon.Info);
                    }
                    else
                    {
                        if (!ClusterManager.IsClusterRunning)
                        {
                            this.ShowBalloonTip(SR.ConnectClusterFailBalloonTip, ToolTipIcon.Error);
                        }
                        else
                        {
                            this.ShowBalloonTip(SR.ClusterRunningLongerConnectBalloonTip, ToolTipIcon.Warning);
                        }
                    }
                }
                catch (Exception)
                {
                    this.ShowBalloonTip(SR.StartClusterFailedBalloonTip, ToolTipIcon.Error);
                }

                this.SetTrayIconText(this.GetCurrentClusterModeText(ProjectUtility.GetCurrentClusterMode()));
                this.trayIcon.ContextMenuStrip.Enabled = true;
            }
            else
            {
                this.ShowBalloonTip(SR.ClusterAlreadyRunningBalloonTip, ToolTipIcon.Info);
            }
        }

        private async void StopClusterMenuItemClick(object sender, EventArgs e)
        {
            if (ClusterManager.IsClusterRunningOrStartPending)
            {
                if (this.ShowYesNoMessageBox(SR.StopClusterConfirmation) == DialogResult.No)
                {
                    return;
                }

                await this.StopCluster_ConfirmationObtained();
            }
            else
            {
                this.ShowBalloonTip(SR.ClusterAlreadyStoppedBalloonTip, ToolTipIcon.Info);
            }
        }

        private async Task StopCluster_ConfirmationObtained()
        {
            if (ClusterManager.IsClusterRunningOrStartPending)
            {
                this.CheckForAdminPrivileges(ClusterOperationType.Stop);

                this.trayIcon.ContextMenuStrip.Enabled = false;
                this.ShowBalloonTip(SR.StopClusterBalloonTip, ToolTipIcon.Info);
                this.SetTrayIconText(SR.StopClusterToolTip);

                try
                {
                    await ClusterManager.StopClusterAsync();
                    this.ShowBalloonTip(SR.StopClusterSuccessBalloonTip, ToolTipIcon.Info);
                }
                catch (Exception)
                {
                    this.ShowBalloonTip(SR.StopClusterFailedBalloonTip, ToolTipIcon.Error);
                }

                this.SetTrayIconText(this.GetCurrentClusterModeText(ProjectUtility.GetCurrentClusterMode()));
                this.trayIcon.ContextMenuStrip.Enabled = true;
            }
            else
            {
                this.ShowBalloonTip(SR.ClusterAlreadyStoppedBalloonTip, ToolTipIcon.Info);
            }
        }

        private async void SetupOneNodeClusterMenuItemClick(object sender, EventArgs e)
        {
            this.CheckForAdminPrivileges(ClusterOperationType.SetupOneNodeCluster);
            await SetupClusterAsync(ClusterMode.OneNode);
        }

        private async void SetupFiveNodeClusterMenuItemClick(object sender, EventArgs e)
        {
            this.CheckForAdminPrivileges(ClusterOperationType.SetupFiveNodeCluster);
            await SetupClusterAsync(ClusterMode.FiveNode);
        }

        private async void SetupMeshFiveNodeClusterMenuItemClick(object sender, EventArgs e)
        {
            this.CheckForAdminPrivileges(ClusterOperationType.SetupFiveNodeMeshCluster);
            await SetupClusterAsync(ClusterMode.MeshFiveNode);
        }

        private async void SetupMeshOneNodeClusterMenuItemClick(object sender, EventArgs e)
        {
            this.CheckForAdminPrivileges(ClusterOperationType.SetupOneNodeMeshCluster);
            await SetupClusterAsync(ClusterMode.MeshOneNode);
        }

        private static String WrapToolTipWithLogDetails(String toolTip)
        {
            return string.Format("{0} {1}:{2}", toolTip, SR.LogInfoBalloonTip, Path.Combine(ProjectUtility.DevClusterLogRoot, ProjectUtility.DefaultSetupLogFileName));
        }

        private async Task SetupClusterAsync(ClusterMode clusterMode)
        {
            if (!ClusterManager.IsClusterSetup)
            {
                this.trayIcon.ContextMenuStrip.Enabled = false;
                this.ShowBalloonTip(SR.SetupClusterBalloonTip, ToolTipIcon.Info);
                this.SetTrayIconText(SR.SetupClusterToolTip);


                await this.PerformOperationWithRetry(
                    () => ClusterManager.SetupClusterAsync(clusterMode),
                    SR.ClusterSetupSuccessBalloonTip,
                    WrapToolTipWithLogDetails(SR.ErrorSettingUpClusterBalloonTip),
                    WrapToolTipWithLogDetails(SR.ClusterSetupFailedBalloonTip));

                this.appMgr.RecordClusterReset();
                this.SetTrayIconText(this.GetCurrentClusterModeText(clusterMode));
                this.trayIcon.ContextMenuStrip.Enabled = true;
            }
            else
            {
                this.ShowBalloonTip(SR.ClusterAlreadySetupBalloonTip, ToolTipIcon.Info);
            }
        }

        private async void RemoveClusterMenuItemClick(object sender, EventArgs e)
        {
            if (ClusterManager.IsClusterSetup)
            {
                if (this.ShowYesNoMessageBox(SR.RemoveClusterConfirmation) == DialogResult.No)
                {
                    return;
                }

                await this.RemoveCluster_ConfirmationObtained();
            }
            else
            {
                this.ShowBalloonTip(SR.NoClusterSetupBalloonTip, ToolTipIcon.Info);
            }
        }

        private async Task RemoveCluster_ConfirmationObtained()
        {
            if (ClusterManager.IsClusterSetup)
            {
                this.CheckForAdminPrivileges(ClusterOperationType.Remove);
                this.trayIcon.ContextMenuStrip.Enabled = false;
                this.ShowBalloonTip(SR.RemoveClusterBalloonTip, ToolTipIcon.Info);
                this.SetTrayIconText(SR.RemoveClusterToolTip);

                await this.PerformOperationWithRetry(
                    () => ClusterManager.RemoveClusterAsync(),
                    SR.RemoveClusterSuccessBalloonTip,
                    WrapToolTipWithLogDetails(SR.ErrorRemovingClusterBalloonTip),
                    WrapToolTipWithLogDetails(SR.RemoveClusterFailedBalloonTip));

                this.SetTrayIconText(SR.DefaultTrayIconText);
                this.trayIcon.ContextMenuStrip.Enabled = true;
            }
            else
            {
                this.ShowBalloonTip(SR.NoClusterSetupBalloonTip, ToolTipIcon.Info);
            }
        }

        private async Task<bool> PerformOperationWithRetry(
            Func<Task<bool>> operation,
            string successMsg,
            string retryMsg,
            string failureMsg)
        {
            bool success = false;

            for (int i = 0; i < ProjectUtility.ClusterOperationRetryCount; i++)
            {
                if (await operation.Invoke())
                {
                    success = true;
                    break;
                }

                this.ShowBalloonTip(retryMsg, ToolTipIcon.Warning);
            }

            if (success)
            {
                this.ShowBalloonTip(successMsg, ToolTipIcon.Info);
            }
            else
            {
                this.ShowBalloonTip(failureMsg, ToolTipIcon.Error);
            }

            return success;
        }

        private async Task<bool> PerformOperationWithRetry(
            Func<Task<CommandExecutionResult>> operation,
            string successMsg,
            string retryMsg,
            string failureMsg)
        {
            bool success = false;
            CommandExecutionResult cmdExecutionResult;
            int retryCount = 0;

            do
            {
                cmdExecutionResult = await operation.Invoke();
                if (cmdExecutionResult.ExitCode == 0)
                {
                    success = true;
                    break;
                }
                if (!string.IsNullOrWhiteSpace(cmdExecutionResult.Output))
                {
                    this.ShowBalloonTip(retryMsg + " Error:" + cmdExecutionResult.Output, ToolTipIcon.Warning);
                }
                else
                {
                    this.ShowBalloonTip(retryMsg, ToolTipIcon.Warning);
                }
                retryCount++;
            }
            while (retryCount < ProjectUtility.ClusterOperationRetryCount);

            if (success)
            {
                this.ShowBalloonTip(successMsg, ToolTipIcon.Info);
            }
            else
            {
                if (!string.IsNullOrWhiteSpace(cmdExecutionResult.Output))
                {
                    this.ShowBalloonTip(failureMsg + " Error:" + cmdExecutionResult.Output, ToolTipIcon.Error);
                }
                else
                {
                    this.ShowBalloonTip(failureMsg, ToolTipIcon.Error);
                }
            }

            return success;
        }
        
        private void PackageTracesMenuItemClick(object sender, EventArgs e)
        {
            this.trayIcon.ContextMenuStrip.Enabled = false;

            var clusterLogFile = Path.Combine(ProjectUtility.DevClusterLogRoot, ProjectUtility.DefaultSetupLogFileName);
            if (File.Exists(clusterLogFile) || Directory.Exists(ProjectUtility.DevClusterTraceFolder))
            {
                this.SetTrayIconText(SR.PackagingLogsBalloonTip);
                var zipPath = Path.Combine(ProjectUtility.DevClusterLogRoot, String.Format("Logs_{0}.zip", DateTime.Now.ToString("yyyy-dd-MM-HH-mm-ss")));
                var tempFolder = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
                var clusterManifestFileName = String.Format("{0}{1}", Environment.MachineName, "-Server-ScaleMin.xml");

                try
                {
                    // Create a temp directory
                    Directory.CreateDirectory(tempFolder);

                    // Copy Dev cluster setup logs file
                    if (File.Exists(clusterLogFile))
                    {
                        var tempLogFile = Path.Combine(tempFolder, ProjectUtility.DefaultSetupLogFileName);
                        File.Copy(clusterLogFile, tempLogFile, true);
                    }

                    // Copy Cluster manifest
                    var clustermanifestFile = Path.Combine(Path.GetTempPath(), clusterManifestFileName);
                    if (File.Exists(clustermanifestFile))
                    {
                        var tempManifestFile = Path.Combine(tempFolder, clusterManifestFileName);
                        File.Copy(clustermanifestFile, tempManifestFile, true);
                    }

                    // Copy Trace files from the traces folder
                    if (Directory.Exists(ProjectUtility.DevClusterTraceFolder))
                        ProjectUtility.CopyAllFilesFromDir(ProjectUtility.DevClusterTraceFolder, tempFolder);

                    // Copy CrashDumps
                    if (Directory.Exists(ProjectUtility.DevClusterCrashDumpsFolder))
                        ProjectUtility.CopyAllFilesFromDir(ProjectUtility.DevClusterCrashDumpsFolder, tempFolder);

                    ZipFile.CreateFromDirectory(tempFolder, zipPath);
                    this.ShowBalloonTip(String.Format("{0}: {1}", SR.LogsPackagedBalloonTip, zipPath), ToolTipIcon.Info);
                }
                finally
                {
                    // Deleting temp directory
                    if(Directory.Exists(tempFolder))
                        Directory.Delete(tempFolder, true);
                }
            }
            else
            {
                this.ShowBalloonTip(SR.LogsNotPresentBalloonTip, ToolTipIcon.Warning);
            }

            this.SetTrayIconText(SR.DefaultTrayIconText);
            this.trayIcon.ContextMenuStrip.Enabled = true;
        }

        private void ExitMenuItemClick(object sender, EventArgs e)
        {
            if (this.ShowYesNoMessageBox(SR.ExitLCMConfirmation) == DialogResult.Yes)
            {
                this.trayIcon.Visible = false;
                Application.Exit();
            }
        }

        private DialogResult ShowYesNoMessageBox(string text)
        {
            return MessageBox.Show(
                text,
                SR.AppName,
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Question,
                MessageBoxDefaultButton.Button2);
        }

        private void OnTrayIconDoubleClick(object sender, EventArgs e)
        {
            this.ManageClusterMenuItemClick(sender, e);
            this.UpdateTrayIconText();
        }

        private void OnTrayIconClick(object sender, EventArgs e)
        {
            this.UpdateTrayIconText();
        }

        private void ShowBalloonTip(String tipText, ToolTipIcon tipIcon)
        {
            this.ShowBalloonTip(SR.DefaultTrayIconText, tipText, tipIcon);
        }

        private void ShowBalloonTip(String title, String tipText, ToolTipIcon tipIcon)
        {
            this.trayIcon.ShowBalloonTip(ProjectUtility.BalloonTipTimeout, title, tipText, tipIcon);
        }

        private void SetTrayIconText(String iconText)
        {
            // NOTE: All tooltip text must not be more than 63 chars long.
            this.trayIcon.Text = iconText;
        }

        private void ExecuteOperation(ClusterOperationType operationType)
        {
            switch (operationType)
            {
#pragma warning disable 4014
                case ClusterOperationType.Manage:
                    this.ManageClusterMenuItemClick(this, EventArgs.Empty); break;
                case ClusterOperationType.Reset:
                    this.ResetCluster_ConfirmationObtained(); break;
                case ClusterOperationType.Start:
                    this.StartClusterMenuItemClick(this, EventArgs.Empty); break;
                case ClusterOperationType.Stop:
                    this.StopCluster_ConfirmationObtained(); break;
                case ClusterOperationType.SetupOneNodeCluster:
                    this.SetupOneNodeClusterMenuItemClick(this, EventArgs.Empty); break;
                case ClusterOperationType.SetupFiveNodeCluster:
                    this.SetupFiveNodeClusterMenuItemClick(this, EventArgs.Empty); break;
                case ClusterOperationType.SwitchToOneNodeCluster:
                    this.SwitchToOneNodeCluster_ConfirmationObtained(); break;
                case ClusterOperationType.SwitchToFiveNodeCluster:
                    this.SwitchToFiveNodeCluster_ConfirmationObtained(); break;
                case ClusterOperationType.Remove:
                    this.RemoveCluster_ConfirmationObtained(); break;
                case ClusterOperationType.SetupOneNodeMeshCluster:
                    this.SetupMeshOneNodeClusterMenuItemClick(this, EventArgs.Empty); break;
                case ClusterOperationType.SetupFiveNodeMeshCluster:
                    this.SetupMeshFiveNodeClusterMenuItemClick(this, EventArgs.Empty); break;
                case ClusterOperationType.SwitchToMeshOneNodeCluster:
                    this.SwitchToMeshOneNodeCluster_ConfirmationObtained(); break;
                case ClusterOperationType.SwitchToMeshFiveNodeCluster:
                    this.SwitchToMeshFiveNodeCluster_ConfirmationObtained(); break;
#pragma warning restore 4014
            }
        }

        private void CheckForPendingRestart(ClusterOperationType startupOperation)
        {
            if (this.appMgr.IsRestartPending)
            {
                this.trayIcon.Visible = false;
                Application.Exit();
                this.appMgr.RestartApplication(startupOperation);
            }
        }

        private void CheckForAdminPrivileges(ClusterOperationType startupOperation)
        {
            if (!this.appMgr.HasAdminPrivileges)
            {
                this.trayIcon.Visible = false;
                Application.Exit();
                this.appMgr.RestartApplicationAsAdmin(startupOperation);
            }
        }

        private string GetCurrentClusterModeText(ClusterMode mode)
        {
            var msg = SR.AppName;

            switch (mode)
            {
                case ClusterMode.OneNode:
                    msg = SR.CurrentClusterOneNodeText;
                    break;
                case ClusterMode.FiveNode:
                    msg = SR.CurrentClusterFiveNodeText;
                    break;
                case ClusterMode.MeshOneNode:
                    msg = SR.CurrentClusterMeshOneNodeText;
                    break;
                case ClusterMode.MeshFiveNode:
                    msg = SR.CurrentClusterMeshFiveNodeText;
                    break;
                default:
                    break;
            }

            return msg;
        }

        private void UpdateTrayIconText()
        {
            // NOTE: All tooltip text must not be more than 63 chars long.
            // get cluster mode from registry and update tooltip text if an operation is not running.
            if (!this.IsOperationRunning())
            {
                this.SetTrayIconText(this.GetCurrentClusterModeText(ProjectUtility.GetCurrentClusterMode()));
            }
        }

        private bool IsOperationRunning()
        {
            // Context menu is disabled when an operation is in progress.
            return !this.trayIcon.ContextMenuStrip.Enabled;
        }

        #endregion

        #region Protected Methods

        protected override void ExitThreadCore()
        {
            this.trayIcon.Visible = false;
            base.ExitThreadCore();
        }

        #endregion
    }
}