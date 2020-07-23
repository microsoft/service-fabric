// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using Microsoft.AspNetCore;
using Microsoft.AspNetCore.Hosting;
using Microsoft.Extensions.Logging;

namespace webapi
{
    public class Program
    {
        public static void Main(string[] args)
        {
            var port = System.Environment.GetEnvironmentVariable("Fabric_Endpoint_GatewayProxyResolverEndpoint");
            var dynamicPort = System.Environment.GetEnvironmentVariable("Gateway_Resolver_Uses_Dynamic_Port");
            if (dynamicPort == "false")
            {
                port = "19079";
            }
            string listenUrl = string.Format("http://*:{0}", port);
            BuildWebHost(args, listenUrl).Run();
        }

        public static IWebHost BuildWebHost(string[] args, string listenUrl) =>
            WebHost.CreateDefaultBuilder(args)
                .ConfigureLogging((hostingContext, logging) =>
                {
                    logging.AddConfiguration(hostingContext.Configuration.GetSection("Logging"));
                    logging.SetMinimumLevel(LogLevel.Warning);
                })
                .UseUrls(listenUrl)
                .UseStartup<Startup>()
                .Build();
    }
}
