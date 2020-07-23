// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Query;

    internal class ServiceFabricRepairManagerFactory : IRepairManagerFactory
    {
        private static readonly Uri FabricSystemApplicationUri = new Uri("fabric:/System");
        private static readonly Uri FabricRepairManagerServiceUri = new Uri("fabric:/System/RepairManagerService");

        private readonly FabricClient fabricClient;
        private readonly IActivityLogger activityLogger;
        private readonly CoordinatorEnvironment environment;
        private readonly IRetryPolicyFactory retryPolicyFactory;

        public ServiceFabricRepairManagerFactory(CoordinatorEnvironment environment, IActivityLogger activityLogger)
        {
            this.activityLogger = activityLogger.Validate("activityLogger");
            this.fabricClient = new FabricClient();
            this.environment = environment.Validate("environment");

            this.retryPolicyFactory = new LinearRetryPolicyFactory(
                environment.DefaultTraceType,
                Constants.BackoffPeriodInMilliseconds,
                Constants.MaxRetryAttempts,
                ex => ex is TimeoutException);
        }

        public ServiceFabricRepairManagerFactory(CoordinatorEnvironment environment, IActivityLogger activityLogger, string[] hostEndpoints)
        {
            this.activityLogger = activityLogger.Validate("activityLogger");
            this.fabricClient = new FabricClient(new FabricClientSettings(), hostEndpoints);
            this.environment = environment.Validate("environment");
        }

        public IRepairManager Create()
        {
            bool exists = RepairManagerServiceExists();
            if (!exists)
            {
                this.environment.DefaultTraceType.WriteWarning("RepairManager service not created");
                return null;
            }

            return new ServiceFabricRepairManager(environment, activityLogger);
        }

        private bool RepairManagerServiceExists()
        {
            var serviceList = retryPolicyFactory.Create().Execute(
                () =>
                {
                    ServiceList list = this.fabricClient.QueryManager
                        .GetServiceListAsync(FabricSystemApplicationUri, FabricRepairManagerServiceUri)
                        .GetAwaiter()
                        .GetResult();
                    return list;
                },
                "GetServiceListAsync");

            this.environment.DefaultTraceType.WriteInfo(
                "GetServiceList('{0}','{1}'): result count = {2}",
                FabricSystemApplicationUri,
                FabricRepairManagerServiceUri,
                serviceList.Count);

            return serviceList.Count > 0;
        }
    }
}