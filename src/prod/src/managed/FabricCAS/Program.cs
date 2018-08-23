// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting.ContainerActivatorService
{
    using System;
    using System.Threading;

    internal class Program
    {
        public static int Main(string[] args)
        {
            Utility.InitializeTracing();
            HostingTrace.Initialize();

            try
            {
                HostingConfig.Initialize();

                var containerActivatorService = new ContainerActivatorService();
                containerActivatorService.Start();

                Thread.Sleep(Timeout.Infinite);

                GC.KeepAlive(containerActivatorService);
            }
            catch (Exception ex)
            {
                HostingTrace.Source.WriteError("FabricCAS", "Failed with Exception={0}", ex.ToString());

                throw ex;
            }

            return 0;
        }
    }
}