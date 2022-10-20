// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Server
{
    using System.Threading;
    using Microsoft.Owin.Hosting;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System;
    using LogProvider;

    public class OwinHttpServer
    {
        private IDisposable serverHandle;

        private Startup owinStartup;

        internal OwinHttpServer(EventStoreRuntime currentRuntime, SecuritySetting fabricBrSecuritySetting, CancellationToken cancellationToken)
        {
            this.owinStartup = new Startup(currentRuntime, fabricBrSecuritySetting, cancellationToken);
        }

        public void StartOwinHttpServer(string listenAddress)
        {
            EventStoreLogger.Logger.LogMessage("Starting Owin Http Server @ {0}.", listenAddress);
            try
            {
                this.serverHandle = WebApp.Start(listenAddress, appBuilder => this.owinStartup.Configuration(appBuilder));
            }
            catch (Exception ex)
            {
                EventStoreLogger.Logger.LogError("Failed to start Owin Http Server with exception: {0}", ex.ToString());
                throw;
            }

            EventStoreLogger.Logger.LogMessage("Started Owin Http Server successfully.");
        }

        public void StopOwinHttpServer()
        {
            EventStoreLogger.Logger.LogMessage("Stop of Owin Http Server Requested.");
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
            EventStoreLogger.Logger.LogMessage("Owin Http Server Stopped.");
        }
    }
}