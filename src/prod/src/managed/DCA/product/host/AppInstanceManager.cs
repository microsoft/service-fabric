// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Dca;

    // This class manages the data collector objects for all the application instances
    internal class AppInstanceManager : IDisposable
    {
        // Constants
        private const string TraceType = "AppInstanceManager";

        private readonly Dictionary<string, AppInstance> applicationInstances;
        private readonly DiskSpaceManager diskSpaceManager;

        // Whether or not the object has been disposed
        private bool disposed;

        internal AppInstanceManager()
        {
            this.applicationInstances = new Dictionary<string, AppInstance>();
            this.diskSpaceManager = new DiskSpaceManager();
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            foreach (AppInstance appInstance in this.applicationInstances.Values)
            {
                appInstance.Dispose();
            }

            GC.SuppressFinalize(this);
        }

        public virtual bool Contains(string appInstanceId)
        {
            return this.applicationInstances.ContainsKey(appInstanceId);
        }

        internal virtual void CreateApplicationInstance(string applicationInstanceId, AppConfig appConfig)
        {
            this.CreateApplicationInstance(applicationInstanceId, DateTime.MaxValue, appConfig, null, null);
        }

        internal void CreateApplicationInstance(
                        string applicationInstanceId,
                        DateTime activationTime,
                        AppConfig appConfig, 
                        string servicePackageName, 
                        ServiceConfig serviceConfig)
        {
            if (false == activationTime.Equals(DateTime.MaxValue))
            {
                AppActivationTable.AddRecord(applicationInstanceId, appConfig.ApplicationType, activationTime);
            }

            AppInstance appInstance = new AppInstance(applicationInstanceId, appConfig, servicePackageName, serviceConfig, this.diskSpaceManager);
            lock (this.applicationInstances)
            {
                this.applicationInstances[applicationInstanceId] = appInstance;
            }

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Created data collector for application instance {0}.",
                applicationInstanceId);
        }

        internal void UpdateApplicationInstance(string applicationInstanceId, AppConfig appConfig, HashSet<string> changedSections)
        {
            AppInstance appInstance;
            lock (this.applicationInstances)
            {
                appInstance = this.applicationInstances[applicationInstanceId];
            }

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Updating data collector for application instance {0} ...",
                applicationInstanceId);

            appInstance.Update(appConfig, changedSections);
        }

        internal void AddServiceToApplicationInstance(string applicationInstanceId, AppConfig appConfig, string servicePackageName, ServiceConfig serviceConfig)
        {
            AppInstance appInstance;
            lock (this.applicationInstances)
            {
                appInstance = this.applicationInstances[applicationInstanceId];
            }

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Updating data collector for application instance {0} ...",
                applicationInstanceId);

            appInstance.AddService(appConfig, servicePackageName, serviceConfig);
        }

        internal void RemoveServiceFromApplicationInstance(string applicationInstanceId, string servicePackageName)
        {
            AppInstance appInstance;
            lock (this.applicationInstances)
            {
                appInstance = this.applicationInstances[applicationInstanceId];
            }

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Updating data collector for application instance {0} ...",
                applicationInstanceId);

            appInstance.RemoveService(servicePackageName);
        }

        internal virtual void DeleteApplicationInstance(string applicationInstanceId)
        {
            AppInstance appInstance;
            lock (this.applicationInstances)
            {
                appInstance = this.applicationInstances[applicationInstanceId];
                this.applicationInstances.Remove(applicationInstanceId);
            }

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Deleting data collector for application instance {0} ...",
                applicationInstanceId);

            appInstance.FlushData();
            appInstance.Dispose();

            AppActivationTable.RemoveRecord(applicationInstanceId);
        }
    }
}