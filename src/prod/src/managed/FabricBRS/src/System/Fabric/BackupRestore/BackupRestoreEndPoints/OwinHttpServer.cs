// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints
{
    using System.Fabric.Common;
    using System.Threading;
    using Microsoft.Owin.Hosting;
    using Microsoft.ServiceFabric.Services.Runtime;

    public class OwinHttpServer
    {
        private IDisposable serverHandle;
        private Startup owinStartup;
        private const string TraceType = "OwinHttpServer";

        internal OwinHttpServer(StatefulService statefulService, SecuritySetting fabricBrSecuritySetting, CancellationToken cancellationToken)
        {
            this.owinStartup = new Startup(statefulService, fabricBrSecuritySetting, cancellationToken);
        }

        public void StartOwinHttpServer(string listenAddress)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Starting Owin Http Server @ {0}.", listenAddress);
            try
            {
                this.serverHandle = WebApp.Start(listenAddress, appBuilder => this.owinStartup.Configuration(appBuilder));
            }
            catch (Exception ex)
            {
                BackupRestoreTrace.TraceSource.WriteExceptionAsError(TraceType, ex, "Failed to start Owin Http Server with exception");
                throw;
            }

            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Started Owin Http Server successfully.");
        }

        public void StopOwinHttpServer()
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Stop of Owin Http Server Requested.");
            try
            {
                if (this.serverHandle != null)
                {
                    this.serverHandle.Dispose();
                }
            }
            catch (ObjectDisposedException)
            {
                //Do Nothing
            }
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Owin Http Server Stopped.");
        }
    }
}