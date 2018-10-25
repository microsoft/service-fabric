// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.TokenValidationService
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Text;

    public sealed class TokenValidationServiceFactory : IStatelessServiceFactory
    {
        private const string TraceType = "TokenValidationServiceFactory";

        internal static FabricEvents.ExtensionsEvents TraceSource = FabricEvents.ExtensionsEvents.GetEventSource(FabricEvents.Tasks.ManagedTokenValidationService);

        private readonly FabricTokenValidationServiceAgent serviceAgent;

        private TokenValidationServiceFactory()
        {
            this.serviceAgent = new FabricTokenValidationServiceAgent();
        }

        public static void Run()
        {
            TraceConfig.InitializeFromConfigStore();

            using (var runtime = FabricRuntime.Create())
            {
                runtime.RegisterStatelessServiceFactory("TokenValidationServiceType", new TokenValidationServiceFactory());

                Console.ReadKey();
            }
        }

        public IStatelessServiceInstance CreateInstance(string serviceTypeName, Uri serviceName, byte[] initializationData, Guid partitionId, long instanceId)
        {
            string configSection = Encoding.Unicode.GetString(initializationData);

            TraceSource.WriteInfo(TraceType,
                "Creating TokenValidationService instanceId {0}, partition id {1} and config section {2}",
                instanceId,
                partitionId,
                configSection);

            return new TokenValidationService(
                this.serviceAgent,
                TokenValidationProviderFactory.Create(configSection));
        }
    }
}