// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using LocalClusterManager.Utilities;
using System;
using System.Diagnostics;
using System.Security.Principal;
using System.Threading;
using System.Windows.Forms;

namespace LocalClusterManager
{
    using System.Collections.Generic;
    using System.Text;

    internal class ApplicationManager
    {
        #region Private Fields

        private const string WaitForMutexArg = "-waitformutex";
        private const string MutexName = "{B6491173-AA2D-4895-9E8A-0A84809AA7FC}";

        private readonly ClusterOperationType startupOperationToExecute;
        private readonly bool clusterExistedAtStarup;
        private bool clusterResetInCurrentSession;

        private readonly bool waitForMutex;        
        private Mutex mutex;
        private bool acquiredMutex;

        #endregion

        #region C'tors

        public ApplicationManager(IReadOnlyCollection<string> appArgs)
        {
            this.mutex = null;
            this.waitForMutex = false;
            this.acquiredMutex = false;
            this.startupOperationToExecute = ClusterOperationType.None;
            this.clusterExistedAtStarup = ClusterManager.IsClusterSetup;
            this.clusterResetInCurrentSession = false;

            if (appArgs.Count <= 2)
            {
                foreach(var arg in appArgs)
                {
                    if (arg == WaitForMutexArg)
                    {
                        this.waitForMutex = true;
                    }
                    else
                    {
                        this.startupOperationToExecute = ProjectUtility.GetClusterOperationTypeFromString(arg.Substring(1));
                    }
                }
            }
        }

        #endregion

        #region Public Properties

        public ClusterOperationType StartupOperation
        {
            get { return this.startupOperationToExecute; }
        }
        
        public bool IsRestartPending
        {
            get
            {
                if((!this.clusterExistedAtStarup && ClusterManager.IsClusterSetup) ||
                    this.clusterResetInCurrentSession)
                {
                    return true;
                }

                return false;
            }
        }

        public bool HasAdminPrivileges
        {
            get
            {
                return new WindowsPrincipal(WindowsIdentity.GetCurrent()).IsInRole(WindowsBuiltInRole.Administrator);
            }
        }

        #endregion

        #region Public Functions

        public void EnsureSingleInstance()
        {
            this.mutex = new Mutex(true, MutexName, out this.acquiredMutex);

            if (!this.acquiredMutex)
            {
                if (this.waitForMutex)
                {
                    this.mutex.WaitOne();
                    this.acquiredMutex = true;
                }
                else
                {
                    Environment.Exit(0);
                }
            }
        }
        
        public void RestartApplicationAsAdmin(ClusterOperationType startupOperation)
        {
            this.LaunchInAdminMode(startupOperation);
        }

        public void RestartApplication(ClusterOperationType startupOperation)
        {
            this.Launch(startupOperation);
        }

        public void RecordClusterReset()
        {
            this.clusterResetInCurrentSession = true;
        }

        #endregion

        #region Private Helper Functions
        
        private static string GetLaunchArguments(ClusterOperationType startupOperation)
        {
            StringBuilder argsBuilder = new StringBuilder(WaitForMutexArg);

            if (startupOperation != ClusterOperationType.None)
            {
                argsBuilder.Append(" -");
                argsBuilder.Append(startupOperation);
            }

            return argsBuilder.ToString();
        }

        private void LaunchInAdminMode(ClusterOperationType startupOperation = ClusterOperationType.None)
        {
            this.Launch(true, GetLaunchArguments(startupOperation));
        }

        private void Launch(ClusterOperationType startupOperation = ClusterOperationType.None)
        {
            this.Launch(false, GetLaunchArguments(startupOperation));
        }

        private void Launch(bool isAdminMode, string args)
        {
            ProcessStartInfo procStartInfo = new ProcessStartInfo
            {
                UseShellExecute = true,
                WorkingDirectory = Environment.CurrentDirectory,
                FileName = Application.ExecutablePath,
                Arguments = args
            };

            if (isAdminMode)
            {
                procStartInfo.Verb = "runas";
            }

            try
            {
                Process.Start(procStartInfo);
            }
            catch
            {
                // Swallow exception if user refused to allow privileges elevation.                
            }
            finally
            {
                if (this.mutex != null && this.acquiredMutex)
                {
                    this.mutex.ReleaseMutex();
                }

                Environment.Exit(0);
            }
        }

        #endregion
    }
}