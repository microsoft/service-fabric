//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace BSDockerVolumePlugin
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.AspNetCore.Hosting;
    using Microsoft.Extensions.DependencyInjection;
    using Microsoft.ServiceFabric.Services.Communication.AspNetCore;
    using Microsoft.ServiceFabric.Services.Communication.Runtime;
    using Microsoft.ServiceFabric.Services.Runtime;

#if DotNetCoreClrLinux
    internal sealed class SynchronizedKestrelCommunicationListener : KestrelCommunicationListener
    {
        internal static int TotalMountedVolumesOnNode = 0;
        public SynchronizedKestrelCommunicationListener(ServiceContext serviceContext, string endpointName, Func<string, AspNetCoreCommunicationListener, IWebHost> build): base(serviceContext, endpointName, build)
        {
        }

        public override Task CloseAsync(CancellationToken cancellationToken)
        {
            int totalSleep = 0;
            while(TotalMountedVolumesOnNode > 0)
            {
                // Wait for 5 minutes for umounts to complete on this node.
                if(totalSleep == Constants.MaxKestralCloseTimeoutSeconds) break;

                TraceWriter.WriteInfo(Constants.TraceSource, String.Format("SynchronizedKestrelCommunicationListener::CloseAsync Sleeping for 10 seconds. Total slept {0}", totalSleep));
                
                Thread.Sleep(Constants.KestralCloseSleepIntervalSeconds * 1000);
                totalSleep += Constants.KestralCloseSleepIntervalSeconds;
            }

            return base.CloseAsync(cancellationToken);
        }
    }
#endif

    /// <summary>
    /// The FabricRuntime creates an instance of this class for each service type instance. 
    /// </summary>
    internal sealed class BSDockerVolumePluginService : StatelessService
    {
        public BSDockerVolumePluginService(StatelessServiceContext context)
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
#if DotNetCoreClrLinux
                    new SynchronizedKestrelCommunicationListener(serviceContext, "ServiceEndpoint", (url, listener) =>
#else
                    new KestrelCommunicationListener(serviceContext, "ServiceEndpoint", (url, listener) =>
#endif
                    {

                        return new WebHostBuilder()
                                    .UseKestrel()
                                    .ConfigureServices(
                                        services => {
                                        services
                                            .AddSingleton<StatelessServiceContext>(serviceContext);
                                        services
                                            .AddSingleton<VolumeMap>(new VolumeMap(serviceContext));
                                            })
                                    .UseContentRoot(Directory.GetCurrentDirectory())
                                    .UseStartup<Startup>()
                                    .UseServiceFabricIntegration(listener, ServiceFabricIntegrationOptions.None)
                                    .UseUrls(url)
                                    .Build();
                    }))
            };
        }
    }
}
