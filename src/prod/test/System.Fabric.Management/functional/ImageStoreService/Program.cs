﻿// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace MS.Test.ImageStoreService
{
    using System;
    using System.Diagnostics;
    using System.Diagnostics.Tracing;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;

    internal static class Program
    {
        /// <summary>
        /// This is the entry point of the service host process.
        /// </summary>
        private static void Main()
        {
            try
            {
                ServiceEventListener listener = new ServiceEventListener("TestImageStoreService");
                listener.EnableEvents(ServiceEventSource.Current, EventLevel.LogAlways, EventKeywords.All);

                // The ServiceManifest.XML file defines one or more service type names.
                // Registering a service maps a service type name to a .NET type.
                // When Service Fabric creates an instance of this service type,
                // an instance of the class is created in this host process.

                ServiceRuntime.RegisterServiceAsync("ImageStoreServiceType",
                    context => new TestImageStoreService(context)).GetAwaiter().GetResult();

                ServiceEventSource.Current.ServiceTypeRegistered(Process.GetCurrentProcess().Id, typeof(TestImageStoreService).Name);

                // Prevents this host process from terminating so services keeps running.
                Thread.Sleep(Timeout.Infinite);
            }
            catch (Exception e)
            {
                ServiceEventSource.Current.ServiceHostInitializationFailed(e.ToString());
                throw;
            }
        }
    }
}
