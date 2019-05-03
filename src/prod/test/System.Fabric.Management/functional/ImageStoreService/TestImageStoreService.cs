// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Fabric;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Hosting;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.ServiceFabric.Services.Communication.AspNetCore;
using Microsoft.ServiceFabric.Services.Communication.Runtime;
using Microsoft.ServiceFabric.Services.Runtime;

namespace MS.Test.ImageStoreService
{
    /// <summary>
    /// The FabricRuntime creates an instance of this class for each service type instance.
    /// </summary>
    internal sealed class TestImageStoreService : StatelessService
    {
        public TestImageStoreService(StatelessServiceContext context)
            : base(context)
        { }

        /// <summary>
        /// Optional override to create listeners (like tcp, http) for this service instance.
        /// </summary>
        /// <returns>The collection of listeners.</returns>
        protected override IEnumerable<ServiceInstanceListener> CreateServiceInstanceListeners()
        {
            return new ServiceInstanceListener[]
            {
                new ServiceInstanceListener(serviceContext =>
                    new KestrelCommunicationListener(serviceContext, "ServiceEndpoint", (url, listener) =>
                    {
                        ServiceEventSource.Current.ServiceMessage(serviceContext, $"Starting Kestrel on {url}");

                        try
                        {
                            var webHostBuilder = new WebHostBuilder()
                                        .UseKestrel()
                                        .ConfigureServices(
                                            services => services
                                                .AddSingleton<StatelessServiceContext>(serviceContext))
                                        .UseContentRoot(Directory.GetCurrentDirectory())
                                        .UseStartup<Startup>()
                                        .UseServiceFabricIntegration(listener, ServiceFabricIntegrationOptions.None)
                                        .UseUrls(url)  // url is expected to be http(s)://+:8737
                                        .Build();

                            ServiceEventSource.Current.Message("Successfully created webhostbuilder");

                            return webHostBuilder;
                        }
                        catch (Exception ex)
                        {
                            ServiceEventSource.Current.Message("Got exception in creating WebHostBuilder: " + ex);
                            throw;
                        }
                    }))
            };
        }
    }
}
