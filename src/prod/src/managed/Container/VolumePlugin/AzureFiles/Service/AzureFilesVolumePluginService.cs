//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePlugin
{
    using System.Collections.Generic;
    using System.Fabric;
    using System.IO;
    using Microsoft.AspNetCore.Hosting;
    using Microsoft.Extensions.DependencyInjection;
    using Microsoft.ServiceFabric.Diagnostics.Tracing;
    using Microsoft.ServiceFabric.Services.Communication.AspNetCore;
    using Microsoft.ServiceFabric.Services.Communication.Runtime;
    using Microsoft.ServiceFabric.Services.Runtime;

    /// <summary>
    /// The FabricRuntime creates an instance of this class for each service type instance. 
    /// </summary>
    internal sealed class AzureFilesVolumePluginService : StatelessService
    {
        private const string TraceType = "AzureFilesVolService";

        public AzureFilesVolumePluginService(
            StatelessServiceContext context)
            : base(context)
        {
        }

        /// <summary>
        /// Optional override to create listeners (like tcp, http) for this service instance.
        /// </summary>
        /// <returns>The collection of listeners.</returns>
        protected override IEnumerable<ServiceInstanceListener> CreateServiceInstanceListeners()
        {
            return new ServiceInstanceListener[]
            {
                new ServiceInstanceListener(
                    serviceContext =>
                    {
                        var createWebHostHelper = new CreateWebHostHelper(serviceContext);
                        return new KestrelCommunicationListener(serviceContext, Constants.HttpEndpointName, createWebHostHelper.Create);
                    }
                    )
            };
        }

        class CreateWebHostHelper
        {
            private StatelessServiceContext serviceContext;

            internal CreateWebHostHelper(StatelessServiceContext serviceContext)
            {
                this.serviceContext = serviceContext;
            }

            internal IWebHost Create(string url, AspNetCoreCommunicationListener listener)
            {
                TraceWriter.WriteInfoWithId(
                    TraceType,
                    this.serviceContext.TraceId,
                    $"Starting Kestrel on {url}");

                return new WebHostBuilder()
                            .UseKestrel()
                            .ConfigureServices(
                                services => {
                                    services.AddSingleton<StatelessServiceContext>(this.serviceContext);
                                    services.AddSingleton<VolumeMap>(new VolumeMap(this.serviceContext));
                                })
                            .UseContentRoot(Directory.GetCurrentDirectory())
                            .UseStartup<Startup>()
                            .UseServiceFabricIntegration(listener, ServiceFabricIntegrationOptions.None)
                            .UseUrls(url)
                            .Build();
            }
        }
    }
}
