// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    using System;
    using System.Threading;

    public class ServiceFabricEnvironmentSyncHost
    {
        private readonly ServiceFabricEnvironmentSync serviceFabricEnvironmentSync;
        private readonly EventWaitHandle exitEvent;

        private int exitCode;

        public ServiceFabricEnvironmentSyncHost()
        {
            this.serviceFabricEnvironmentSync = new ServiceFabricEnvironmentSync(this.ExitProcess);
            this.exitEvent = new ManualResetEvent(false);
            this.exitCode = 0;
        }

        public static int Main(string[] args)
        {           
            /* 
             * As an AP application service, the service host should clean up and exit after receiving Ctrl + Break. 
             * ServiceFabricEnvironmentSync would subscribe to the event and signal service host for exit.
            */
            ServiceFabricEnvironmentSyncHost host = new ServiceFabricEnvironmentSyncHost();

            host.Start();

            host.WaitForExit();

            return host.exitCode;
        }

        private void Start()
        {
            this.serviceFabricEnvironmentSync.Start();
        }

        private void ExitProcess(bool succeeded)
        {
            this.exitCode = succeeded ? 0 : 1;

            this.exitEvent.Set();
        }

        private void WaitForExit()
        {
            this.exitEvent.WaitOne();
        }
    }
}