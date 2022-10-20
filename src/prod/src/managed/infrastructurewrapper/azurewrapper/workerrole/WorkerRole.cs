// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Threading;
using Microsoft.WindowsAzure.ServiceRuntime;

namespace Microsoft.Fabric.InfrastructureWrapper
{
    /// <summary>
    /// The default worker role implementation for deploying Windows Fabric on Windows Azure.
    /// Users could utilize role entry point methods like OnStart(), OnStop(), Run() to implement their own business logic. 
    /// Please note that role recycling (e.g. caused by exiting from Run()) would cause local instance of Windows Fabric to recycled accordingly.     
    /// Therefore, users should avoid exiting from Run() unnecessarily as suggested in http://msdn.microsoft.com/en-us/library/windowsazure/microsoft.windowsazure.serviceruntime.roleentrypoint.run.aspx.
    /// </summary>
    public class WorkerRole : RoleEntryPoint
    {
        private readonly ManualResetEvent eventToWait = new ManualResetEvent(false);

        public override void Run()
        {
            eventToWait.WaitOne();
        }

        public override bool OnStart()
        {
            // For information on handling configuration changes
            // see the MSDN topic at http://go.microsoft.com/fwlink/?LinkId=166357.
            return base.OnStart();
        }
    }
}