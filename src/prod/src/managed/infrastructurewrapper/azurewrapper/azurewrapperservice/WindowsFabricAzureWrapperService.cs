// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.InfrastructureWrapper
{
    using System;
    using System.ComponentModel;    
    using System.Configuration.Install;
    using System.ServiceProcess;
    using System.Threading;

    using Microsoft.WindowsAzure.ServiceRuntime;

    public class WindowsFabricAzureWrapperService : ServiceBase
    {
        private readonly WindowsFabricAzureWrapper windowsFabricAzureWrapper = null;
        private readonly ManualResetEvent eventToWait = new ManualResetEvent(false);

        public WindowsFabricAzureWrapperService(bool serviceMode = true)
        {
            if (serviceMode)
            {
                this.CanPauseAndContinue = false;
                this.CanStop = true;
                this.CanShutdown = true;

                this.AutoLog = true;

                this.ServiceName = WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperServiceName;

                this.windowsFabricAzureWrapper = new WindowsFabricAzureWrapper(this.StopService);
            }
            else
            {
                this.windowsFabricAzureWrapper = new WindowsFabricAzureWrapper(this.StopProcess);
            }
        }

        public static void Main(string[] args)
        {
            if (RoleEnvironment.IsEmulated)
            {
                WindowsFabricAzureWrapperService windowsFabricAzureWrapperService = new WindowsFabricAzureWrapperService(false);

                windowsFabricAzureWrapperService.OnStart(new string[] { });

                windowsFabricAzureWrapperService.WaitForExit();
            }
            else
            {
                ServiceBase.Run(new WindowsFabricAzureWrapperService(true));
            }
        }

        protected override void OnStart(string[] args)
        {
            this.windowsFabricAzureWrapper.OnStart(args);
        }

        protected override void OnStop()
        {
            this.windowsFabricAzureWrapper.OnStop();
        }

        protected override void OnShutdown()
        {
            this.windowsFabricAzureWrapper.OnShutdown();
        }

        private void StopService()
        {
            this.Stop(); 
        }

        private void StopProcess()
        {
            this.eventToWait.Set();
        }

        private void WaitForExit()
        {
            this.eventToWait.WaitOne();
        }
    }
}