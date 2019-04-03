// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;
    using Globalization;
    using InfrastructureService.Test;
    using RD.Fabric.PolicyAgent;
    using Repair;
    using Threading;

    /// <summary>
    /// A class for testing directly on the VM that has MR enabled
    /// </summary>
    internal class ManualTester
    {
        private readonly IDictionary<string, Func<string[], int>> actions;

        public ManualTester()
        {
            actions = new Dictionary<string, Func<string[], int>>(StringComparer.OrdinalIgnoreCase)
            {
                { "GetDoc", GetDoc },
                { "CreateRepairTask", CreateRepairTask },
                { "GetNamingStoreContents", GetNamingStoreContents },
                { "AcknowledgeJob", AcknowledgeJob },
            };
        }

        public int Execute(string[] args)
        {
            if (args.Length == 0)
            {
                Constants.TraceType.ConsoleWriteLine("Invalid command. Please type /? for help");
                return -1;
            }

            string command = args[0];

            if (!actions.ContainsKey(command))
            {
                Constants.TraceType.ConsoleWriteLine("Invalid command. Please type /? for help");
                return -1;
            }

            var func = actions[command];

            try
            {
                int status = func(args);
                return status;
            }
            catch (Exception ex)
            {
                Constants.TraceType.ConsoleWriteLine("Error while executing '{0}'.{1}Exception: {2}'", command, Environment.NewLine, ex);
                return -2;
            }
        }

        public static int GetDoc(string[] args)
        {
            CoordinatorEnvironment env = new CoordinatorEnvironment(
                "fabric:/System/InfrastructureService",
                new ConfigSection(Constants.TraceType, new MockConfigStore(), "ConfigSection"),
                string.Empty,
                new MockInfrastructureAgentWrapper());

            Uri zeroSdkUri = null;

            if (args.Length >= 2)
            {
                zeroSdkUri = new Uri(args[1], UriKind.Absolute);
            }

            var activityLogger = new ActivityLoggerFactory().Create(Constants.TraceType);
            var pac = new PolicyAgentClient(env, new PolicyAgentServiceWrapper(env, activityLogger, zeroSdkUri), activityLogger);

            IPolicyAgentDocumentForTenant doc = pac.GetDocumentAsync(Guid.NewGuid(), CancellationToken.None).GetAwaiter().GetResult();

            Constants.TraceType.ConsoleWriteLine("Doc: {0}", doc);

            foreach (ITenantJob job in doc.Jobs)
            {
                Constants.TraceType.ConsoleWriteLine(job.ToJson());
            }

            foreach (RoleInstanceHealthInfo health in doc.RoleInstanceHealthInfos)
            {
                Constants.TraceType.ConsoleWriteLine(health.ToJson());
            }

            return 0;
        }

        public static int AcknowledgeJob(string[] args)
        {
            CoordinatorEnvironment env = new CoordinatorEnvironment(
                "fabric:/System/InfrastructureService",
                new ConfigSection(Constants.TraceType, new MockConfigStore(), "ConfigSection"),
                string.Empty,
                new MockInfrastructureAgentWrapper());

            Uri zeroSdkUri = null;

            if (args.Length >= 2)
            {
                zeroSdkUri = new Uri(args[1], UriKind.Absolute);
            }

            int incarnationNumber = int.Parse(args[2]);
            Guid jobId = Guid.Parse(args[3]);
            var activityLogger = new ActivityLoggerFactory().Create(Constants.TraceType);
            var pac = new PolicyAgentClient(env, new PolicyAgentServiceWrapper(env, activityLogger, zeroSdkUri), activityLogger);
            var jobResponseMap = new Dictionary<Guid, JobStepResponseEnum> { { jobId, JobStepResponseEnum.Acknowledged } };

            try
            {
                pac.SendJobResponseAsync(
                    Guid.NewGuid(),
                    incarnationNumber,
                    jobResponseMap,
                    "{0} by operator command".ToString(JobStepResponseEnum.Acknowledged),
                    CancellationToken.None).ConfigureAwait(false).GetAwaiter().GetResult();
                Console.WriteLine("{0} job response sent successfully", JobStepResponseEnum.Acknowledged);
            }
            catch (ManagementException ex)
            {
                Console.WriteLine("Error in management protocol while sending job response. Exception: {0}", ex);
                return -1;
            }

            return 0;
        }

        public static int CreateRepairTask(string[] args)
        {
            const string DefaultRepairTaskIdFormat = "InfrastructureService/{0}";

            string repairTaskId = string.Format(CultureInfo.InvariantCulture, DefaultRepairTaskIdFormat, Guid.NewGuid());

            var nodeNames = args[1].Split(new[] {',', ';'}, StringSplitOptions.RemoveEmptyEntries);
            var action = args[2];

            var repairTask = new ClusterRepairTask(repairTaskId, action) // TODO
            {
                Description = "Manual repair task for testing",
                Target = new NodeRepairTargetDescription(nodeNames)
            };

            var activityLogger = new ActivityLoggerFactory().Create(Constants.TraceType);
            var rm = new ServiceFabricRepairManagerFactory(
                new CoordinatorEnvironment(
                    "fabric:/System/InfrastructureService",
                    new ConfigSection(Constants.TraceType, new MockConfigStore(), "ConfigSection"),
                    Constants.TraceTypeName,
                    new MockInfrastructureAgentWrapper()),
                activityLogger,
                new [] { "paralleljobs1-bn-cs.cloudapp.net:19000" }).Create();

            long status = rm.CreateRepairTaskAsync(Guid.NewGuid(), new ServiceFabricRepairTask(repairTask)).GetAwaiter().GetResult();

            return (int)status;
        }


        public static int GetNamingStoreContents(string[] args)
        {
            var hostEndPoint = args[1]; // e.g. "paralleljobs2-bn-cs.cloudapp.net:19000"
            
            // e.g. "Fabric:/__FabricSystem_App4294967295" or "Fabric:/__FabricSystem_App4294967295/InfrastructureService" (for multi-tenant store)
            var storeName = args[2]; 

            var pm = new FabricClient(new FabricClientSettings(), hostEndPoint).PropertyManager;
            var storeUri = new Uri(storeName);
            PropertyEnumerationResult properties = null; // TODO use this in a loop

            properties = pm.EnumeratePropertiesAsync(storeUri, true, properties).GetAwaiter().GetResult();
            foreach (var property in properties)
            {
                Console.WriteLine("{0}, {1}, {2:o}, {3}", property.Metadata.PropertyName, GetValue(property),
                    property.Metadata.LastModifiedUtc, property.Metadata.TypeId);
            }

            return 0;
        }

        private static string GetValue(NamedProperty property)
        {
            switch (property.Metadata.TypeId)
            {
                case PropertyTypeId.String:
                    return property.GetValue<string>();

                case PropertyTypeId.Double:
                    return property.GetValue<double>().ToString();

                case PropertyTypeId.Guid:
                    return property.GetValue<Guid>().ToString();

                case PropertyTypeId.Int64:
                    return property.GetValue<Int64>().ToString();

                case PropertyTypeId.Binary:
                    return "binary " + property.Metadata.ValueSize + " bytes";

                case PropertyTypeId.Invalid:
                default:
                    return "invalid";
            }
        }
    }
}