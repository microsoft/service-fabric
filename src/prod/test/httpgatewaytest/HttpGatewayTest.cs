// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.HttpGatewayTest
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.Win32;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Repair;
    using System.Fabric.Query;
    using System.Fabric.Scenarios.Test.Hosting;
    using System.Fabric.Test.Common.FabricUtility;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Xml;
    using System.Xml.Serialization;
    using WEX.Logging.Interop;
    using WEX.TestExecution;
    using System.Fabric.dSTSClient;
    using System.Fabric.Health;

    [TestClass]
    public class HttpGatewayServiceTest
    {
        public static string TestDeploymentOutputPath = @"HttpGatewayService.Imagestore";
        public static Process fabricHostProcess = null;
        private static string currentClusterManifest;
        private static CredentialType clusterCredentialType;
        private static Random random;
        private static Tuple<RESTClient, FabricClient> fabricClients;

        [ClassInitialize]
        [Owner("bharatn")]
        public static void InitializeState()
        {
            HttpGatewayServiceTest.TestDeploymentOutputPath = Path.Combine(Directory.GetCurrentDirectory(), HttpGatewayServiceTest.TestDeploymentOutputPath);
            CleanupCounters();
            CleanupDirectories();
            Directory.CreateDirectory(HttpGatewayServiceTest.TestDeploymentOutputPath);
            MakeDrop();

            random = new Random((int)DateTime.Now.Ticks);
            int choice = random.Next(3) % 2;
            if (choice == 1)
            {
                SetupClusterNoSecurity();
            }
            else
            {
                SetupClusterForCertSecurity();
            }
        }

        [ClassCleanup]
        [Owner("bharatn")]
        public static void CleanUpState()
        {
            CleanupFabricClients();

            if (fabricHostProcess != null)
            {
                if (!fabricHostProcess.HasExited)
                {
                    fabricHostProcess.Kill();
                }
                fabricHostProcess.Dispose();
            }
            //
            // FabricCommon.dll might still be loaded in this process because we create fabricclients.
            // The filechange monitor inside fabriccommon might assert if the config files that are
            // monitored are deleted before unloading fabriccommon. So cleanup of directories shouldnt
            // happen here.
            //
            CleanupCounters();
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("bharatn")]
        public void RoleTest()
        {
            if (clusterCredentialType != CredentialType.X509)
            {
                Console.WriteLine("Not doing role test for credential type :{0}", clusterCredentialType);
                return;
            }

            LogHelper.Log("RoleTest - Begin");

            var clients = GetFabricClients();
            RESTClient adminRestClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;
            string manifestName = Environment.ExpandEnvironmentVariables(currentClusterManifest);
            string[] httpGatewayAddresses = GetHttpGatewayAddresses(manifestName);
            RESTClient userRestClient = new RESTClient(httpGatewayAddresses);
            userRestClient.thumbprint = Constants.UserCertificateThumbprint;
            userRestClient.hstsHeader = GetHSTSHeaderValue(manifestName);
            string appName = "appV1";

            ApplicationDeployer appDeployer = GenerateApplicationWithSingleStatelessService(Constants.TestApplicationTypeName, "1.0", 3, appName);

            //
            // Provision application
            //
            string appDir = Path.Combine(HttpGatewayServiceTest.TestDeploymentOutputPath, appName);
            UploadApplication(appDir, appName);
            Console.WriteLine("Attempting to Provision {0} as UserRole", appName);
            userRestClient.ProvisionApplicationType(Path.Combine(Constants.IncomingDirectory, appName), HttpStatusCode.Forbidden);

            Console.WriteLine("Verifying provision failure");
            List<ApplicationTypeResult> restAppTypeResult = GetApplicationTypes(userRestClient, fabricClient);
            Verify.IsTrue(restAppTypeResult.Count() == 0, "restAppTypeResult.Count() == 0");

            string applicationName = "fabric:/" + Constants.TestApplicationName;
            string serviceName1 = "fabric:/" + Constants.TestApplicationName + "/" + "service1";

            //
            // Create application
            //
            Console.WriteLine("Creating app - {0} as UserRole", applicationName);
            userRestClient.CreateApplication(applicationName, Constants.TestApplicationTypeName, ref appDeployer, HttpStatusCode.Forbidden); // Constants.TestApplicationName is the app Type.

            Console.WriteLine("Verifying CreateApplication failure");
            List<ApplicationsResult> appResult = GetApplications(userRestClient, fabricClient);
            Verify.IsTrue(appResult.Count() == 0);

            //
            // Create a service from the service template, delete and unprovision
            //
            Console.WriteLine("Creating service - {0} from template as UserRole", serviceName1);
            ServiceDeployer serviceDeployer = StatelessSanityTest.GetService(-1);
            userRestClient.CreateServiceFromTemplate(applicationName, serviceName1, ServicePackageActivationMode.SharedProcess, ref serviceDeployer, HttpStatusCode.Forbidden);

            Console.WriteLine("Deleting Application - {0} as UserRole", applicationName);
            userRestClient.DeleteApplication(applicationName, HttpStatusCode.Forbidden);

            Console.WriteLine("Unprovisioning appV1 as UserRole");
            userRestClient.UnprovisionApplicationType(Constants.TestApplicationTypeName, ref appDeployer, HttpStatusCode.Forbidden);

            //
            // Test Node and CodePackage Control fails with Forbidden when using user client cert
            //

            // Bug#2244833 - Figure out REST model for testability APIs
            // Currentlty the model for testability APIs and REST is not consistent hence removing them
            // after discussion with Vipul

            //Console.WriteLine("Testing NodeControl messages");
            //userRestClient.RestartNode("Node1", 0, HttpStatusCode.Forbidden);

            //userRestClient.StopNode("Node1", 0, HttpStatusCode.Forbidden);

            //Console.WriteLine("Testing CodePackageControl messages");
            //userRestClient.RestartCodePackage("Node1", "fabric:/App1", "ServiceManifest", "CodePackage", 0, HttpStatusCode.Forbidden);

            //
            // Do things as Admin role.
            //
            Console.WriteLine("Provisioning {0} as Admin", appName);
            adminRestClient.ProvisionApplicationType(Path.Combine(Constants.IncomingDirectory, appName));

            //
            // Verify
            //
            restAppTypeResult = GetApplicationTypes(adminRestClient, fabricClient);
            Verify.IsTrue(restAppTypeResult.Count() == 1);

            //
            // Verify
            //
            GetServiceTypes(adminRestClient, restAppTypeResult, fabricClient);

            Console.WriteLine("Creating app - {0} as AdminRole", applicationName);
            adminRestClient.CreateApplication(applicationName, Constants.TestApplicationTypeName, ref appDeployer); // Constants.TestApplicationName is the app Type.

            //
            // Verify
            //
            Console.WriteLine("Verifying CreateApplication failure");
            appResult = GetApplications(userRestClient, fabricClient);
            Verify.IsTrue(appResult.Count() == 1);

            //
            // Create a service from the service template
            //
            Console.WriteLine("Creating service - {0} from template as AdminRole", serviceName1);
            adminRestClient.CreateServiceFromTemplate(applicationName, serviceName1, ServicePackageActivationMode.SharedProcess, ref serviceDeployer);

            //
            // Wait for stabilization - mainly, health reports from FM to reach HM for the services created
            //
            Thread.Sleep(10000);

            //
            // Verify Services
            //
            List<ServiceResult> serviceResult = GetServices(applicationName, userRestClient, fabricClient);
            Verify.IsTrue(serviceResult.Count() == 2); // one default service, one service from template


            //
            // Delete the services
            //
            Console.WriteLine("Deleting Service - {0} as AdminRole", serviceName1);
            adminRestClient.DeleteService(applicationName, serviceName1);

            //
            // Delete application
            //
            Console.WriteLine("Deleting Application - {0} as AdminRole", applicationName);
            adminRestClient.DeleteApplication(applicationName);

            //
            // Unprovision application.
            //
            Console.WriteLine("Unprovisioning appV1 as AdminRole");
            adminRestClient.UnprovisionApplicationType(Constants.TestApplicationTypeName, ref appDeployer);

            LogHelper.Log("RoleTest - End");
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("bharatn")]
        public void CreateDeleteApplicationTest()
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;
            string appName = "appV1";

            LogHelper.Log("CreateDeleteApplicationTest - Begin");

            ApplicationDeployer appDeployer = GenerateApplicationWithSingleStatelessService(Constants.TestApplicationTypeName, "1.0", -1, appName);

            //
            // Provision application
            //
            string appDir = Path.Combine(HttpGatewayServiceTest.TestDeploymentOutputPath, appName);
            UploadApplication(appDir, appName);
            Console.WriteLine("Provisioning {0}", appName);
            restClient.ProvisionApplicationType(Path.Combine(Constants.IncomingDirectory, appName));

            //
            // Verify
            //
            List<ApplicationTypeResult> restAppTypeResult = GetApplicationTypes(restClient, fabricClient);
            Verify.IsTrue(restAppTypeResult.Count() == 1);

            //
            // Verify
            //
            GetServiceTypes(restClient, restAppTypeResult, fabricClient);

            string applicationName = "fabric:/" + Constants.TestApplicationName;
            string serviceName1 = "fabric:/" + Constants.TestApplicationName + "/" + "service1";
            string serviceName2 = "fabric:/" + Constants.TestApplicationName + "/" + "service2";
            string serviceExclusiveName1 = "fabric:/" + Constants.TestApplicationName + "/" + "serviceExclusive1";
            string serviceExclusiveName2 = "fabric:/" + Constants.TestApplicationName + "/" + "serviceExclusive2";

            //
            // Create application
            //
            Console.WriteLine("Creating app - " + applicationName);
            restClient.CreateApplication(applicationName, Constants.TestApplicationTypeName, ref appDeployer); // Constants.TestApplicationName is the app Type.

            //
            // Verify
            //
            List<ApplicationsResult> appResult = GetApplications(restClient, fabricClient);
            Verify.IsTrue(appResult.Count() == 1);

            //
            // Create a service from the service template
            //
            Console.WriteLine("Creating service - " + serviceName1 + " from template");
            ServiceDeployer serviceDeployer = StatelessSanityTest.GetService(-1);
            restClient.CreateServiceFromTemplate(applicationName, serviceName1, ServicePackageActivationMode.SharedProcess, ref serviceDeployer);

            Console.WriteLine("Creating service - " + serviceExclusiveName1 + " from template");
            restClient.CreateServiceFromTemplate(applicationName, serviceExclusiveName1, ServicePackageActivationMode.ExclusiveProcess, ref serviceDeployer);

            //
            // Create service
            //
            var createServiceData1 = this.GetCreateServiceInput(applicationName, serviceName2, ServicePackageActivationMode.SharedProcess, serviceDeployer);
            var createServiceData2 = this.GetCreateServiceInput(applicationName, serviceExclusiveName2, ServicePackageActivationMode.ExclusiveProcess, serviceDeployer);

            Console.WriteLine("Creating service - " + serviceName2);
            restClient.CreateService(ref createServiceData1);

            Console.WriteLine("Creating service - " + serviceExclusiveName2);
            restClient.CreateService(ref createServiceData2);

            restClient.UpdateService(applicationName, serviceName1, 5);

            //
            // Wait for things to be stabilized.
            //
            Thread.Sleep(40000);

            // Verify Cluster Load Information
            Console.WriteLine("VerifyingClusterLoad");
            GetClusterLoadInformation(restClient, fabricClient);

            // Verify Node Load Information
            List<NodesResult> nodesResult = GetNodes(restClient, fabricClient);
            foreach (var node in nodesResult)
            {
                Console.WriteLine("VerifyingNodeLoadQuery on Node - {0}", node.Name);
                GetNodeLoadInformation(node.Name, restClient, fabricClient);
            }

            //
            // Verify Services
            //
            List<ServiceResult> serviceResult = GetServices(applicationName, restClient, fabricClient);
            Verify.IsTrue(serviceResult.Count() == 5); // one default service, two service from template, and two adhoc service

            var serviceDesc1 = GetServiceDescription(applicationName, serviceName2, restClient, fabricClient);
            Verify.AreEqual(serviceDesc1.ServicePackageActivationMode, ServicePackageActivationMode.SharedProcess);

            var serviceDesc2 = GetServiceDescription(applicationName, serviceExclusiveName2, restClient, fabricClient);
            Verify.AreEqual(serviceDesc2.ServicePackageActivationMode, ServicePackageActivationMode.ExclusiveProcess);

            //
            // Resolve the service endpoint
            //
            var resolvedServicePartitionResult = ResolveServicePartition(applicationName, serviceName2, serviceDeployer.Partition, restClient, fabricClient);

            //
            // Verify Partitions
            //
            List<ServicePartitionResult> partitionResult = GetPartitions(applicationName, serviceName2, restClient, fabricClient);
            Verify.IsTrue(partitionResult.Count() == 1); // this service is using the default - which is singleton

            //
            // Verify Partitions again using partitionId
            //
            partitionResult = GetPartitions(partitionResult[0].PartitionInformation.Id, restClient, fabricClient);
            Verify.IsTrue(partitionResult.Count() == 1); // this service is using the default - which is singleton
            restClient.RecoverServicePartitions(applicationName, serviceName2);
            restClient.RecoverPartition(applicationName, serviceName2, partitionResult[0].PartitionInformation.Id);

            //
            // Verify Replicas
            //
            List<ServiceReplicaResult> replicaResult = GetReplicas(applicationName, serviceName2, partitionResult[0].PartitionInformation.Id.ToString(), restClient, fabricClient);

            //
            // Instance count of -1 should create replica instances in all nodes.
            //
            Verify.IsTrue(replicaResult.Count == 5);

            //
            // Verify Hosting Queries
            //
            VerifyHostingQueriesForAllAppsAndNodes(restClient, fabricClient);

            //
            // Verify Health Queries
            //
            VerifyHealthQueries(applicationName, serviceName2, restClient, fabricClient);

            //
            // Verify Partition Load Queries
            //
            VerifyPartitionLoadInformationQueries(applicationName, serviceName2, restClient, fabricClient);

            //
            // Verify Manifest Queries
            //
            VerifyManifestQueries(Constants.TestApplicationTypeName, "1.0", restClient, fabricClient);

            //
            // Delete the services
            //
            restClient.DeleteService(applicationName, serviceName1);
            restClient.DeleteService(applicationName, serviceName2);

            // Wait for the delete service to complete
            Thread.Sleep(TimeSpan.FromSeconds(30));

            // Service is deleted, resolve service with prev, should fail
            var temp = restClient.ResolveServicePartition(applicationName, serviceName2, resolvedServicePartitionResult.Version, HttpStatusCode.NotFound);
            Verify.IsTrue(temp == null, "ResolveServicePartition for a deleted service should fail.");

            //
            // Delete application
            //
            restClient.DeleteApplication(applicationName);

            //
            // Verify
            //
            appResult = GetApplications(restClient, fabricClient);
            Verify.IsTrue(appResult.Count() == 0);

            //
            // Unprovision application.
            //
            Console.WriteLine("Unprovisioning appV1");
            restClient.UnprovisionApplicationType(Constants.TestApplicationTypeName, ref appDeployer);

            //
            // Verify unprovision
            //
            restAppTypeResult = GetApplicationTypes(restClient, fabricClient);
            Verify.IsTrue(restAppTypeResult.Count() == 0);

            //
            // Delete the app directory
            //
            Directory.Delete(appDir, true);

            LogHelper.Log("CreateDeleteApplicationTest - End");
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("juhacket")]
        public void QueryApplicationIdServiceIdTest()
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;
            string appName = "appV1";
            string applicationName = "fabric:/myapp/app1";
            string applicationId = "myapp/app1";
            string applicationIdV6 = "myapp~app1";
            string serviceId = "myapp/app1/SanityTestStatelessService";
            string serviceIdV6 = "myapp~app1~SanityTestStatelessService";

            LogHelper.Log("QueryApplicationIdServiceIdTest - Begin");

            ApplicationDeployer appDeployer = GenerateApplicationWithSingleStatelessService(Constants.TestApplicationTypeName, "1.0", -1, appName);

            //
            // Provision application
            //
            string appDir = Path.Combine(HttpGatewayServiceTest.TestDeploymentOutputPath, appName);
            UploadApplication(appDir, appName);
            Console.WriteLine("Provisioning {0}", appName);
            restClient.ProvisionApplicationType(Path.Combine(Constants.IncomingDirectory, appName));

            //
            // Verify
            //
            List<ApplicationTypeResult> restAppTypeResult = GetApplicationTypes(restClient, fabricClient);
            Verify.IsTrue(restAppTypeResult.Count() == 1);

            //
            // Verify
            //
            GetServiceTypes(restClient, restAppTypeResult, fabricClient);

            //
            // Create application
            //
            Console.WriteLine("Creating app - " + applicationName);
            restClient.CreateApplication(applicationName, Constants.TestApplicationTypeName, ref appDeployer);

            //
            // Verify Application and ID in version 1.0
            //
            List<ApplicationsResult> appResult = GetApplications(restClient, fabricClient, UriSuffix.ApiVersion);
            Verify.IsTrue(appResult.Count() == 1);
            Verify.AreEqual(appResult[0].Id, applicationId);

            //
            // Verify Application ID in version 6.0
            //
            List<ApplicationsResult> appResult2 = GetApplications(restClient, fabricClient, UriSuffix.Api60Version);
            Verify.IsTrue(appResult2.Count() == 1);
            Verify.AreEqual(appResult2[0].Id, applicationIdV6);

            //
            // Verify Service ID in version 1.0
            //
            List<ServiceResult> serviceResult = GetServices(applicationName, restClient, fabricClient, UriSuffix.ApiVersion);
            Verify.IsTrue(serviceResult.Count() == 1);
            Verify.AreEqual(serviceResult[0].Id, serviceId);

            //
            // Verify Service ID in version 6.0
            //
            List<ServiceResult> serviceResult2 = GetServices(applicationName, restClient, fabricClient, UriSuffix.Api60Version);
            Verify.IsTrue(serviceResult2.Count() == 1);
            Verify.AreEqual(serviceResult2[0].Id, serviceIdV6);

            //
            // Delete application
            //
            restClient.DeleteApplication(applicationName);

            //
            // Unprovision application.
            //
            Console.WriteLine("Unprovisioning " + appName);
            restClient.UnprovisionApplicationType(Constants.TestApplicationTypeName, ref appDeployer);

            //
            // Delete the app directory
            //
            Directory.Delete(appDir, true);

            LogHelper.Log("QueryApplicationIdServiceIdTest - End");
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("jefchen")]
        public void TestCreateDeleteServiceGroupIsSuccess()
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;
            String appName = "application1";

            LogHelper.Log("TestCreateDeleteServiceGroupIsSuccess - Begin");

            string deployPath = Path.Combine(HttpGatewayServiceTest.TestDeploymentOutputPath, "application1");

            ApplicationDeployer appDeployer = GenerateApplicationWithSingleStatelessService(Constants.TestApplicationTypeName, "1.0", -1, appName);

            string applicationName = "fabric:/" + Constants.TestApplicationName;
            string serviceName1 = "fabric:/" + Constants.TestApplicationName + "/" + "service1";
            string serviceName2 = "fabric:/" + Constants.TestApplicationName + "/" + "service2";
            string serviceTypeName1 = "SanityTestStatelessService";

            ServiceDeployer serviceDeployer = StatelessSanityTest.GetService(-1);

            AddApplication(appDeployer, clients.Item1, clients.Item2, deployPath);

            //
            // Create service
            //
            var createServiceGroupData1 = this.GetCreateServiceGroupInput(
                applicationName,
                serviceTypeName1,
                serviceName1,
                ServicePackageActivationMode.SharedProcess,
                serviceDeployer);

            var createServiceGroupData2 = this.GetCreateServiceGroupInput(
                applicationName,
                serviceTypeName1,
                serviceName2,
                ServicePackageActivationMode.ExclusiveProcess,
                serviceDeployer);


            Console.WriteLine("Creating service group - " + serviceName1);
            restClient.CreateServiceGroup(ref createServiceGroupData1);

            Console.WriteLine("Creating service group - " + serviceName2);
            restClient.CreateServiceGroup(ref createServiceGroupData2);

            Thread.Sleep(20000);

            var serviceGroupDescription1 = this.GetServiceGroupDescription(applicationName, serviceName1, clients.Item1, clients.Item2);
            Verify.AreEqual(serviceGroupDescription1.ServicePackageActivationMode, ServicePackageActivationMode.SharedProcess);

            var serviceGroupDescription2 = this.GetServiceGroupDescription(applicationName, serviceName2, clients.Item1, clients.Item2);
            Verify.AreEqual(serviceGroupDescription2.ServicePackageActivationMode, ServicePackageActivationMode.ExclusiveProcess);

            var serviceGroupMemberResult = restClient.GetServiceGroupMembers(applicationName, serviceName1);
            Verify.IsTrue(serviceGroupMemberResult.ServiceGroupMemberDescription.Count == 1);
            Verify.AreEqual(createServiceGroupData1.ServiceGroupMemberDescription[0].ServiceName, serviceGroupMemberResult.ServiceGroupMemberDescription[0].ServiceName);

            restClient.UpdateServiceGroup(applicationName, serviceName1, 5);

            Thread.Sleep(10000);

            //
            // Verify Services
            //
            List<ServiceResult> serviceResult = GetServices(applicationName, restClient, fabricClient);
            Verify.IsTrue(serviceResult.Count() == 3); // one default service, and two adhoc service

            //
            // Delete the services
            //
            restClient.DeleteServiceGroup(applicationName, serviceName1);
            restClient.DeleteServiceGroup(applicationName, serviceName2);

            Thread.Sleep(10000);

            RemoveApplication(appDeployer, clients.Item1, clients.Item2, deployPath);

            LogHelper.Log("TestCreateDeleteServiceGroupIsSuccess - End");
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("hjzeng")]
        public void CreateDeleteApplicationWithAffinityTest()
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();

            LogHelper.Log("CreateDeleteApplicationWithAffinityTest - Begin");

            string deployPath = Path.Combine(HttpGatewayServiceTest.TestDeploymentOutputPath, "MyApplication");

            ApplicationDeployer appDeployer = new ApplicationDeployer()
            {
                Name = Constants.TestApplicationTypeName,
                Version = "1.0"
            };

            string applicationName = "fabric:/" + Constants.TestApplicationName;
            string serviceName2 = "fabric:/" + Constants.TestApplicationName + "/" + "service2";
            string serviceName3 = "fabric:/" + Constants.TestApplicationName + "/" + "service3";
            string serviceName4 = "fabric:/" + Constants.TestApplicationName + "/" + "service4";

            //creating one default service and one service template
            ServiceDeployer service1 = new ServiceDeployer()
            {
                CodePackageFiles = new List<string>(SanityTest.GetAllCodePackageBinaries()),
                CodePackageName = ServiceDeployer.DefaultCodePackageName,
                ConfigPackageName = SanityTest.ConfigurationPackageName,
                ServiceTypeImplementation = typeof(StatefulSanityTest.MyApplication),
                ServiceTypeName = "SanityTestStatefulService",
                ServiceName = "service1",
                MinReplicaSetSize = 2,
                TargetReplicaSetSize = 2
            };
            service1.ConfigurationSettings.Add(Tuple.Create(
                SanityTest.ConfigurationSectionName,
                SanityTest.ConfigurationParameterName,
                HttpGatewayServiceTest.TestDeploymentOutputPath));
            service1.ServiceCorrelations = new List<Fabric.Description.ServiceCorrelationDescription>();
            service1.ServiceCorrelations.Add(new System.Fabric.Description.ServiceCorrelationDescription()
            {
                // affinitized to service3
                ServiceName = new Uri(serviceName3),
                Scheme = ServiceCorrelationScheme.Affinity
            });

            appDeployer.Services.Add(service1);
            appDeployer.Deploy(deployPath);
            AddApplication(appDeployer, clients.Item1, clients.Item2, deployPath);

            //
            // Create a service from the service template
            //
            Console.WriteLine("Creating service - " + serviceName2 + " from template");
            clients.Item1.CreateServiceFromTemplate(applicationName, serviceName2, ServicePackageActivationMode.SharedProcess, ref service1);

            //
            // Create adhoc service
            //
            CreateServiceInput createServiceData = new CreateServiceInput();
            createServiceData.ApplicationName = applicationName;
            createServiceData.ServiceName = serviceName3;
            createServiceData.ServiceTypeName = "SanityTestStatefulService";
            createServiceData.ServiceKind = System.Fabric.Query.ServiceKind.Stateful;
            createServiceData.TargetReplicaSetSize = 3;
            createServiceData.MinReplicaSetSize = 3;
            createServiceData.PartitionDescription.PartitionScheme = PartitionScheme.Singleton;

            Console.WriteLine("Creating service - " + serviceName3);
            clients.Item1.CreateService(ref createServiceData);

            createServiceData = new CreateServiceInput();
            createServiceData.ApplicationName = applicationName;
            createServiceData.ServiceName = serviceName4;
            createServiceData.ServiceTypeName = "SanityTestStatefulService";
            createServiceData.ServiceKind = System.Fabric.Query.ServiceKind.Stateful;
            createServiceData.TargetReplicaSetSize = 3;
            createServiceData.MinReplicaSetSize = 3;
            createServiceData.PartitionDescription.PartitionScheme = PartitionScheme.Singleton;
            createServiceData.CorrelationScheme = new List<ServiceCorrelationDescription>();
            createServiceData.CorrelationScheme.Add(new ServiceCorrelationDescription()
            {
                ServiceName = serviceName3,
                Scheme = ServiceCorrelationScheme.NonAlignedAffinity
            });

            Console.WriteLine("Creating service - " + serviceName4);
            clients.Item1.CreateService(ref createServiceData);

            //
            // Wait for things to be stabilized.
            //
            Thread.Sleep(30000);

            //
            // Verify Services
            //
            List<ServiceResult> serviceResult = GetServices(applicationName, clients.Item1, clients.Item2);
            Verify.IsTrue(serviceResult.Count() == 4);

            RemoveApplication(appDeployer, clients.Item1, clients.Item2, deployPath);

            LogHelper.Log("CreateDeleteApplicationWithAffinityTest - End");
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("miradic")]
        public void CreateServiceAutoScaling()
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();

            LogHelper.Log("CreateServiceAutoScaling - Begin");

            var scalingPolicyTrigger1 = new System.Fabric.Description.AveragePartitionLoadScalingTrigger();
            var scalingPolicyMechanism1 = new System.Fabric.Description.PartitionInstanceCountScaleMechanism();
            scalingPolicyTrigger1.MetricName = "servicefabric:/_CpuCores";
            scalingPolicyMechanism1.MinInstanceCount = 1;
            scalingPolicyMechanism1.MaxInstanceCount = 3;
            scalingPolicyTrigger1.LowerLoadThreshold = 1.0;
            scalingPolicyTrigger1.UpperLoadThreshold = 2.0;
            scalingPolicyTrigger1.ScaleInterval = TimeSpan.FromSeconds(30);
            scalingPolicyMechanism1.ScaleIncrement = 1;

            var scalingPolicy1 = new ScalingPolicyDescription(scalingPolicyMechanism1, scalingPolicyTrigger1);

            var scalingPolicyTrigger2 = new AveragePartitionLoadTriggerTest();
            var scalingPolicyMechanism2 = new InstanceCountScaleMechanismTest();

            scalingPolicyTrigger2.MetricName = "servicefabric:/_CpuCores";
            scalingPolicyMechanism2.MinInstanceCount = 1;
            scalingPolicyMechanism2.MaxInstanceCount = 3;
            scalingPolicyTrigger2.LowerLoadThreshold = 1.0;
            scalingPolicyTrigger2.UpperLoadThreshold = 2.0;
            scalingPolicyTrigger2.ScaleIntervalInSeconds = 20;
            scalingPolicyMechanism2.ScaleIncrement = 2;

            var scalingPolicy2 = new ScalingPolicyDescriptionTest(scalingPolicyMechanism2, scalingPolicyTrigger2);

            var scalingPolicyTrigger3 = new AveragePartitionLoadTriggerTest();
            var scalingPolicyMechanism3= new InstanceCountScaleMechanismTest();

            scalingPolicyTrigger3.MetricName = "servicefabric:/_CpuCores";
            scalingPolicyMechanism3.MinInstanceCount = 1;
            scalingPolicyMechanism3.MaxInstanceCount = 3;
            scalingPolicyTrigger3.LowerLoadThreshold = 1.0;
            scalingPolicyTrigger3.UpperLoadThreshold = 4.0;
            scalingPolicyTrigger3.ScaleIntervalInSeconds = 20;
            scalingPolicyMechanism3.ScaleIncrement = 2;

            var scalingPolicy3 = new ScalingPolicyDescriptionTest(scalingPolicyMechanism3, scalingPolicyTrigger3);

            string deployPath = Path.Combine(HttpGatewayServiceTest.TestDeploymentOutputPath, "MyApplication");

            ApplicationDeployer appDeployer = new ApplicationDeployer()
            {
                Name = Constants.TestApplicationTypeName,
                Version = "1.0"
            };

            string applicationName = "fabric:/" + Constants.TestApplicationName;
            string serviceName1 = "fabric:/" + Constants.TestApplicationName + "/" + "service1";
            string serviceName2 = "fabric:/" + Constants.TestApplicationName + "/" + "service2";
            string serviceName3 = "fabric:/" + Constants.TestApplicationName + "/" + "service3";
            string serviceName4 = "fabric:/" + Constants.TestApplicationName + "/" + "service4";

            //creating one default service and one service template
            ServiceDeployer service1 = new ServiceDeployer()
            {
                CodePackageFiles = new List<string>(SanityTest.GetAllCodePackageBinaries()),
                CodePackageName = ServiceDeployer.DefaultCodePackageName,
                ConfigPackageName = SanityTest.ConfigurationPackageName,
                ServiceTypeImplementation = typeof(StatelessSanityTest.MyApplication),
                ServiceTypeName = "SanityTestStatelessService",
                ServiceName = serviceName1,
                InstanceCount = 2
            };

            service1.ScalingPolicies = new List<ScalingPolicyDescription>();
            service1.ScalingPolicies.Add(scalingPolicy1);

            service1.ConfigurationSettings.Add(Tuple.Create(
                SanityTest.ConfigurationSectionName,
                SanityTest.ConfigurationParameterName,
                HttpGatewayServiceTest.TestDeploymentOutputPath));

            appDeployer.Services.Add(service1);
            appDeployer.Deploy(deployPath);
            AddApplication(appDeployer, clients.Item1, clients.Item2, deployPath);

            //
            // Create a service from the service template
            //
            Console.WriteLine("Creating service - " + serviceName2 + " from template");
            clients.Item1.CreateServiceFromTemplate(applicationName, serviceName2, ServicePackageActivationMode.SharedProcess, ref service1);

            //
            // Create adhoc service
            //
            CreateServiceInput createServiceData = new CreateServiceInput();
            createServiceData.ApplicationName = applicationName;
            createServiceData.ServiceName = serviceName3;
            createServiceData.ServiceTypeName = "SanityTestStatelessService";
            createServiceData.ServiceKind = System.Fabric.Query.ServiceKind.Stateless;
            createServiceData.InstanceCount = 3;
            createServiceData.PartitionDescription.PartitionScheme = PartitionScheme.Singleton;
            createServiceData.ScalingPolicies.Add(scalingPolicy2);

            Console.WriteLine("Creating service - " + serviceName3);
            clients.Item1.CreateService(ref createServiceData);

            createServiceData = new CreateServiceInput();
            createServiceData.ApplicationName = applicationName;
            createServiceData.ServiceName = serviceName4;
            createServiceData.ServiceTypeName = "SanityTestStatelessService";
            createServiceData.ServiceKind = System.Fabric.Query.ServiceKind.Stateless;
            createServiceData.InstanceCount = 3;
            createServiceData.PartitionDescription.PartitionScheme = PartitionScheme.Singleton;
            createServiceData.ScalingPolicies.Add(scalingPolicy3);

            Console.WriteLine("Creating service - " + serviceName4);
            clients.Item1.CreateService(ref createServiceData);

            //
            // Wait for things to be stabilized.
            //
            Thread.Sleep(30000);

            //
            // Verify Services
            //
            List<ServiceResult> serviceResult = GetServices(applicationName, clients.Item1, clients.Item2);
            Verify.IsTrue(serviceResult.Count() == 4);

            var serviceActual2 = GetServiceDescription(applicationName, serviceName2, clients.Item1, clients.Item2);
            Verify.IsTrue(serviceActual2.ScalingPolicies.Count == 1);
            Verify.IsTrue(serviceActual2.ScalingPolicies[0].ScalingTrigger.MetricName == ((System.Fabric.Description.AveragePartitionLoadScalingTrigger)(scalingPolicy1.ScalingTrigger)).MetricName);
            Verify.IsTrue(serviceActual2.ScalingPolicies[0].ScalingMechanism.MinInstanceCount == ((System.Fabric.Description.PartitionInstanceCountScaleMechanism)(scalingPolicy1.ScalingMechanism)).MinInstanceCount);
            var serviceActual3 = GetServiceDescription(applicationName, serviceName3, clients.Item1, clients.Item2);
            Verify.IsTrue(serviceActual3.ScalingPolicies.Count == 1);
            Verify.IsTrue(serviceActual3.ScalingPolicies[0].ScalingTrigger.LowerLoadThreshold == scalingPolicy2.ScalingTrigger.LowerLoadThreshold);
            Verify.IsTrue(serviceActual3.ScalingPolicies[0].ScalingMechanism.MaxInstanceCount == scalingPolicy2.ScalingMechanism.MaxInstanceCount);
            var serviceActual4 = GetServiceDescription(applicationName, serviceName4, clients.Item1, clients.Item2);
            Verify.IsTrue(serviceActual4.ScalingPolicies.Count == 1);
            Verify.IsTrue(serviceActual4.ScalingPolicies[0].ScalingTrigger.MetricName == scalingPolicy3.ScalingTrigger.MetricName);
            Verify.IsTrue(serviceActual4.ScalingPolicies[0].ScalingMechanism.ScaleIncrement == scalingPolicy3.ScalingMechanism.ScaleIncrement);
            RemoveApplication(appDeployer, clients.Item1, clients.Item2, deployPath);

            LogHelper.Log("CreateServiceAutoScaling - End");
        }

        [TestMethod]
        [Owner("Bharatn")]
        public void ApplicationUpgradeTest()
        {
            LogHelper.Log("ApplicationUpgradeTest - Begin");

            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            string deployPath1 = Path.Combine(HttpGatewayServiceTest.TestDeploymentOutputPath, "appV1");
            string deployPath2 = Path.Combine(HttpGatewayServiceTest.TestDeploymentOutputPath, "appV2");
            ApplicationDeployer appV1Deployer = GenerateApplicationWithSingleStatelessService(Constants.TestApplicationTypeName, "1.0", 3, "appV1");
            ApplicationDeployer appV2Deployer = GenerateApplicationWithSingleStatelessService(Constants.TestApplicationTypeName, "2.0", 3, "appV2");

            //
            // Provision application
            //
            Console.WriteLine("Provisioning V1 and V2");
            string cwd = Directory.GetCurrentDirectory();
            UploadApplication(deployPath1, "appV1");
            UploadApplication(deployPath2, "appV2");
            restClient.ProvisionApplicationType(Path.Combine(Constants.IncomingDirectory, "appV1"));
            restClient.ProvisionApplicationType(Path.Combine(Constants.IncomingDirectory, "appV2"));

            //
            // Get Application types
            //
            List<ApplicationTypeResult> result = GetApplicationTypes(restClient, fabricClient);
            Verify.IsTrue((result.Count() == 2));

            //
            // Verify
            //
            GetServiceTypes(restClient, result, fabricClient);

            string applicationName = "fabric:/" + Constants.TestApplicationName;

            //
            // Create application
            //
            Console.WriteLine("Creating app with V1 type");
            restClient.CreateApplication(applicationName, Constants.TestApplicationTypeName, ref appV1Deployer); // Constants.TestApplicationName is the app Type.

            //
            // Verify
            //
            List<ApplicationsResult> appResult = GetApplications(restClient, fabricClient);
            Verify.IsTrue(appResult.Count() == 1);

            Console.WriteLine("Application name - {0}", appResult[0].Name);

            ApplicationUpgradeDescriptionInput upgradeDescription = new ApplicationUpgradeDescriptionInput();
            upgradeDescription.Name = applicationName;
            upgradeDescription.TargetApplicationTypeVersion = "2.0";
            upgradeDescription.UpgradeKind = UpgradeKind.Rolling;
            upgradeDescription.ForceRestart = false;
            upgradeDescription.RollingUpgradeMode = RollingUpgradeMode.UnmonitoredManual;
            Console.WriteLine("Upgrading to V2");
            restClient.UpgradeApplication(ref upgradeDescription);

            //
            // Start upgrade in manual mode and switch to auto after first UD completes
            //
            ApplicationUpgradeProgressResult progressResult = restClient.GetApplicationUpgradeProgress(applicationName);
            PrintAppUpgradeProgress(ref progressResult);

            while (progressResult.UpgradeState != ApplicationUpgradeState.RollingForwardPending)
            {
                Thread.Sleep(5000);
                progressResult = restClient.GetApplicationUpgradeProgress(applicationName);
                PrintAppUpgradeProgress(ref progressResult);
            }

            //
            // Cover that the update application upgrade REST API exists. Detailed testing
            // of JSON serialization/deserialization and functional testing of the feature
            // occurs at lower levels.
            //
            ApplicationUpgradeUpdateDescriptionInput updateDescription = new ApplicationUpgradeUpdateDescriptionInput()
            {
                Name = upgradeDescription.Name,
                UpgradeKind = upgradeDescription.UpgradeKind,
                UpdateDescription = new RollingUpgradeUpdateDescriptionInput()
                {
                    RollingUpgradeMode = RollingUpgradeMode.UnmonitoredAuto,
                }
            };

            restClient.UpdateApplicationUpgrade(ref updateDescription);

            while (progressResult.UpgradeState != ApplicationUpgradeState.RollingForwardCompleted)
            {
                Thread.Sleep(5000);
                progressResult = restClient.GetApplicationUpgradeProgress(applicationName);
                PrintAppUpgradeProgress(ref progressResult);
            }

            //
            // Delete application
            //
            restClient.DeleteApplication(applicationName);

            //
            // Unprovision application.
            //
            restClient.UnprovisionApplicationType(Constants.TestApplicationTypeName, ref appV1Deployer);
            restClient.UnprovisionApplicationType(Constants.TestApplicationTypeName, ref appV2Deployer);

            //
            // Verify
            //
            result = GetApplicationTypes(restClient, fabricClient);
            Verify.IsTrue(result.Count() == 0);

            //
            // Delete the app directory
            //
            Directory.Delete(deployPath1, true);
            Directory.Delete(deployPath2, true);

            LogHelper.Log("ApplicationUpgradeTest - End");
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("bharatn")]
        public void FabricManagementTest()
        {
            string upgradeManifestPath, upgradeManifestName;
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            LogHelper.Log("FabricManagementTest - Begin");

            GetClusterManifestForUpgrade("2.0", out upgradeManifestPath, out upgradeManifestName);
            File.Copy(upgradeManifestPath, Path.Combine(Constants.ImageStoreDirectory, Path.Combine(Constants.IncomingDirectory, upgradeManifestName)));
            string fabricMsiPath, fabricMsiName, fabricCodeVersion;
            GetFabricMsiPath(restClient, out fabricMsiName, out fabricMsiPath, out fabricCodeVersion);
            File.Copy(fabricMsiPath, Path.Combine(Constants.ImageStoreDirectory, Path.Combine(Constants.IncomingDirectory, fabricMsiName)));

            //
            // Provision the new cluster manifest and fabric code package
            //
            restClient.ProvisionFabric(Path.Combine(Constants.IncomingDirectory, fabricMsiName), Path.Combine(Constants.IncomingDirectory, upgradeManifestName));

            FabricUpgradeDescriptionInput upgradeDesc = new FabricUpgradeDescriptionInput();
            upgradeDesc.CodeVersion = fabricCodeVersion;
            upgradeDesc.ConfigVersion = "2.0";
            upgradeDesc.RollingUpgradeMode = RollingUpgradeMode.UnmonitoredAuto;
            upgradeDesc.UpgradeKind = UpgradeKind.Rolling;
            upgradeDesc.ForceRestart = false;
            restClient.UpgradeFabric(upgradeDesc);

            FabricUpgradeProgressResult progressResult = restClient.GetFabricUpgradeProgress();
            PrintFabricUpgradeProgress(ref progressResult);
            while (progressResult.UpgradeState != FabricUpgradeState.RollingForwardCompleted)
            {
                Thread.Sleep(20000);
                progressResult = restClient.GetFabricUpgradeProgress();
                PrintFabricUpgradeProgress(ref progressResult);
            }

            //
            // Verify System services queries
            //
            List<ServiceResult> systemServiceResult = GetSystemServices(restClient, fabricClient);
            List<ServicePartitionResult> systemServicePartitionResult = GetSystemServicePartitions(systemServiceResult[0].Name, restClient);
            List<ServiceReplicaResult> systemServiceReplicaResult = GetSystemServiceReplicas(
                systemServiceResult[0].Name,
                systemServicePartitionResult[0].PartitionInformation.Id.ToString(),
                restClient);

            var result = GetNodes(restClient, fabricClient);
            restClient.DeactivateNode(result[0].Name, NodeDeactivationIntent.Pause);
            restClient.ActivateNode(result[0].Name);

            restClient.RecoverSystemPartitions();
            restClient.RecoverAllPartitions();

            // Version 2.0 is being used, so unprovisionfabric should fail
            restClient.UnprovisionFabric(fabricCodeVersion, "2.0", HttpStatusCode.Conflict);

            LogHelper.Log("FabricManagementTest - End");
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("danmas")]
        public void RepairManagerTest()
        {
            const string TestNodeName = "Node1";
            //const string TestNodeName = "nodeid:10"; // for running against fabrictest

            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            LogHelper.Log("RepairManagerTest - Begin");

            Verify.AreEqual(0, restClient.GetRepairTaskList(null, null, null).Count);

            // Automatic repair

            long version = restClient.CreateRepairTask(new RepairTaskData()
            {
                TaskId = "TaskA",
                State = Repair.RepairTaskState.Created,
                Action = "MyAction",
                Description = "MyDescription",
                Target = new RepairTaskTargetData() { Kind = Repair.RepairTargetKind.Node, NodeNames = new[] { TestNodeName } },
            });
            Verify.AreNotEqual(0, version);

            var tasks = restClient.GetRepairTaskList(null, null, null);
            Verify.AreEqual(1, tasks.Count);

            var task = tasks[0];
            Verify.AreEqual(Repair.RepairScopeIdentifierKind.Cluster, task.Scope.Kind);
            Verify.AreEqual("TaskA", task.TaskId);
            Verify.AreEqual(version, long.Parse(task.Version));
            Verify.AreEqual("MyDescription", task.Description);
            Verify.AreEqual(Repair.RepairTaskState.Created, task.State);
            Verify.AreEqual(Repair.RepairTaskFlags.None, task.Flags);
            Verify.AreEqual("MyAction", task.Action);
            Verify.AreEqual(Repair.RepairTargetKind.Node, task.Target.Kind);
            Verify.AreEqual(1, task.Target.NodeNames.Count);
            Verify.AreEqual(TestNodeName, task.Target.NodeNames[0]);
            Verify.IsTrue(string.IsNullOrEmpty(task.Executor), "Executor");
            Verify.IsTrue(string.IsNullOrEmpty(task.ExecutorData), "ExecutorData");
            Verify.IsNull(task.Impact, "Impact");
            Verify.AreEqual(Repair.RepairTaskResult.Pending, task.ResultStatus);
            Verify.AreEqual(0, task.ResultCode);
            Verify.IsTrue(string.IsNullOrEmpty(task.ResultDetails), "ResultDetails");
            //Console.WriteLine(task.History.CreatedUtcTimestamp);
            //Console.WriteLine(task.History.CompletedUtcTimestamp);

            version = restClient.CancelRepairTask(task.TaskId, task.Version);
            Verify.IsLessThan(long.Parse(task.Version), version);

            tasks = restClient.GetRepairTaskList(null, null, null);
            Verify.AreEqual(1, tasks.Count);

            task = tasks[0];
            Verify.AreEqual(Repair.RepairScopeIdentifierKind.Cluster, task.Scope.Kind);
            Verify.AreEqual("TaskA", task.TaskId);
            Verify.AreEqual(version, long.Parse(task.Version));
            Verify.AreEqual("MyDescription", task.Description);
            Verify.AreEqual(Repair.RepairTaskState.Completed, task.State);
            Verify.AreEqual(Repair.RepairTaskFlags.CancelRequested, task.Flags);
            Verify.AreEqual("MyAction", task.Action);
            Verify.AreEqual(Repair.RepairTargetKind.Node, task.Target.Kind);
            Verify.AreEqual(1, task.Target.NodeNames.Count);
            Verify.AreEqual(TestNodeName, task.Target.NodeNames[0]);
            Verify.IsTrue(string.IsNullOrEmpty(task.Executor), "Executor");
            Verify.IsTrue(string.IsNullOrEmpty(task.ExecutorData), "ExecutorData");
            Verify.IsNull(task.Impact, "Impact");
            Verify.AreEqual(Repair.RepairTaskResult.Cancelled, task.ResultStatus);
            Verify.AreEqual(unchecked((int)0x80004004) /* E_ABORT */, task.ResultCode);
            Verify.IsTrue(!string.IsNullOrEmpty(task.ResultDetails), "ResultDetails");
            //Console.WriteLine(task.History.CreatedUtcTimestamp);
            //Console.WriteLine(task.History.CompletedUtcTimestamp);

            restClient.DeleteRepairTask(task.TaskId, task.Version);
            Verify.AreEqual(0, restClient.GetRepairTaskList(null, null, null).Count);

            // Manual repair

            version = restClient.CreateRepairTask(new RepairTaskData()
            {
                TaskId = "TaskB",
                State = Repair.RepairTaskState.Preparing,
                Action = "MyAction",
                Description = "MyDescription",
                Target = new RepairTaskTargetData() { Kind = Repair.RepairTargetKind.Node, NodeNames = new[] { TestNodeName } },
                Executor = "RE",
                ExecutorData = "REData",
                Impact = new RepairTaskImpactData()
                {
                    Kind = Repair.RepairImpactKind.Node,
                    NodeImpactList = new[]
                    {
                        new NodeImpactData() { NodeName = TestNodeName, ImpactLevel = Repair.NodeImpactLevel.Restart }
                    },
                },
            });
            Verify.AreNotEqual(0, version);

            task = WaitForRepairTaskStateChange(restClient, RepairTaskState.Approved, TimeSpan.FromMinutes(3));

            Verify.AreEqual(Repair.RepairScopeIdentifierKind.Cluster, task.Scope.Kind);
            Verify.AreEqual("TaskB", task.TaskId);
            Verify.AreEqual("MyDescription", task.Description);
            Verify.AreEqual(Repair.RepairTaskState.Approved, task.State);
            Verify.AreEqual(Repair.RepairTaskFlags.None, task.Flags);
            Verify.AreEqual("MyAction", task.Action);
            Verify.AreEqual(Repair.RepairTargetKind.Node, task.Target.Kind);
            Verify.AreEqual(1, task.Target.NodeNames.Count);
            Verify.AreEqual(TestNodeName, task.Target.NodeNames[0]);
            Verify.AreEqual("RE", task.Executor);
            Verify.AreEqual("REData", task.ExecutorData);
            Verify.AreEqual(Repair.RepairImpactKind.Node, task.Impact.Kind);
            Verify.AreEqual(1, task.Impact.NodeImpactList.Count);
            Verify.AreEqual(TestNodeName, task.Impact.NodeImpactList[0].NodeName);
            Verify.AreEqual(Repair.NodeImpactLevel.Restart, task.Impact.NodeImpactList[0].ImpactLevel);
            Verify.AreEqual(Repair.RepairTaskResult.Pending, task.ResultStatus);
            Verify.AreEqual(0, task.ResultCode);
            Verify.IsTrue(string.IsNullOrEmpty(task.ResultDetails), "ResultDetails");
            //Console.WriteLine(task.History.CreatedUtcTimestamp);
            //Console.WriteLine(task.History.CompletedUtcTimestamp);

            task.State = Repair.RepairTaskState.Restoring;
            task.ResultStatus = Repair.RepairTaskResult.Interrupted;
            task.ResultCode = 42;
            task.ResultDetails = "Details";

            version = restClient.UpdateRepairExecutionState(task);
            Verify.IsLessThan(long.Parse(task.Version), version);

            task = WaitForRepairTaskStateChange(restClient, RepairTaskState.Completed, TimeSpan.FromMinutes(3));

            Verify.AreEqual(Repair.RepairScopeIdentifierKind.Cluster, task.Scope.Kind);
            Verify.AreEqual("TaskB", task.TaskId);
            Verify.AreEqual("MyDescription", task.Description);
            Verify.AreEqual(Repair.RepairTaskState.Completed, task.State);
            Verify.AreEqual(Repair.RepairTaskFlags.None, task.Flags);
            Verify.AreEqual("MyAction", task.Action);
            Verify.AreEqual(Repair.RepairTargetKind.Node, task.Target.Kind);
            Verify.AreEqual(1, task.Target.NodeNames.Count);
            Verify.AreEqual(TestNodeName, task.Target.NodeNames[0]);
            Verify.AreEqual("RE", task.Executor);
            Verify.AreEqual("REData", task.ExecutorData);
            Verify.AreEqual(Repair.RepairImpactKind.Node, task.Impact.Kind);
            Verify.AreEqual(1, task.Impact.NodeImpactList.Count);
            Verify.AreEqual(TestNodeName, task.Impact.NodeImpactList[0].NodeName);
            Verify.AreEqual(Repair.NodeImpactLevel.Restart, task.Impact.NodeImpactList[0].ImpactLevel);
            Verify.AreEqual(Repair.RepairTaskResult.Interrupted, task.ResultStatus);
            Verify.AreEqual(42, task.ResultCode);
            Verify.AreEqual("Details", task.ResultDetails);
            //Console.WriteLine(task.History.CreatedUtcTimestamp);
            //Console.WriteLine(task.History.CompletedUtcTimestamp);

            restClient.DeleteRepairTask(task.TaskId, task.Version);
            Verify.AreEqual(0, restClient.GetRepairTaskList(null, null, null).Count);

            // GetRepairTaskList with filters

            restClient.CreateRepairTask(new RepairTaskData()
            {
                TaskId = "A/1",
                State = Repair.RepairTaskState.Created,
                Action = "MyAction",
                Description = "MyDescription",
                Target = new RepairTaskTargetData() { Kind = Repair.RepairTargetKind.Node, NodeNames = new[] { TestNodeName } },
            });

            restClient.CreateRepairTask(new RepairTaskData()
            {
                TaskId = "A/2",
                State = Repair.RepairTaskState.Created,
                Action = "MyAction",
                Description = "MyDescription",
                Target = new RepairTaskTargetData() { Kind = Repair.RepairTargetKind.Node, NodeNames = new[] { TestNodeName } },
            });

            restClient.CreateRepairTask(new RepairTaskData()
            {
                TaskId = "A/3",
                State = Repair.RepairTaskState.Claimed,
                Executor = "RE",
                Action = "MyAction",
                Description = "MyDescription",
                Target = new RepairTaskTargetData() { Kind = Repair.RepairTargetKind.Node, NodeNames = new[] { TestNodeName } },
            });

            restClient.CreateRepairTask(new RepairTaskData()
            {
                TaskId = "B/1",
                State = Repair.RepairTaskState.Claimed,
                Executor = "RE",
                Action = "MyAction",
                Description = "MyDescription",
                Target = new RepairTaskTargetData() { Kind = Repair.RepairTargetKind.Node, NodeNames = new[] { TestNodeName } },
            });

            restClient.CreateRepairTask(new RepairTaskData()
            {
                TaskId = "B/2",
                State = Repair.RepairTaskState.Claimed,
                Executor = "RE-2",
                Action = "MyAction",
                Description = "MyDescription",
                Target = new RepairTaskTargetData() { Kind = Repair.RepairTargetKind.Node, NodeNames = new[] { TestNodeName } },
            });

            Verify.AreEqual(5, restClient.GetRepairTaskList(null, null, null).Count);

            Verify.AreEqual(3, restClient.GetRepairTaskList("A/", null, null).Count);
            Verify.AreEqual(1, restClient.GetRepairTaskList("A/", Repair.RepairTaskStateFilter.Claimed, null).Count);
            Verify.AreEqual(1, restClient.GetRepairTaskList("A/3", null, null).Count);
            Verify.AreEqual(1, restClient.GetRepairTaskList("A/3", null, "RE").Count);
            Verify.AreEqual(0, restClient.GetRepairTaskList("A/3", null, "RE-2").Count);

            Verify.AreEqual(2, restClient.GetRepairTaskList("B/", null, null).Count);
            Verify.AreEqual(1, restClient.GetRepairTaskList("B/1", null, null).Count);

            Verify.AreEqual(2, restClient.GetRepairTaskList(null, null, "RE").Count);
            Verify.AreEqual(1, restClient.GetRepairTaskList(null, null, "RE-2").Count);

            Verify.AreEqual(0, restClient.GetRepairTaskList(null, Repair.RepairTaskStateFilter.Completed, null).Count);
            Verify.AreEqual(0, restClient.GetRepairTaskList(null, Repair.RepairTaskStateFilter.Created, "RE-2").Count);
            Verify.AreEqual(1, restClient.GetRepairTaskList(null, Repair.RepairTaskStateFilter.Claimed, "RE-2").Count);
            Verify.AreEqual(1, restClient.GetRepairTaskList("B", Repair.RepairTaskStateFilter.Claimed, "RE-2").Count);
            Verify.AreEqual(0, restClient.GetRepairTaskList("A", Repair.RepairTaskStateFilter.Claimed, "RE-2").Count);

            Verify.AreEqual(2, restClient.GetRepairTaskList(null, Repair.RepairTaskStateFilter.Created, null).Count);
            Verify.AreEqual(3, restClient.GetRepairTaskList(null, Repair.RepairTaskStateFilter.Claimed, null).Count);
            Verify.AreEqual(5, restClient.GetRepairTaskList(null, Repair.RepairTaskStateFilter.Created | Repair.RepairTaskStateFilter.Claimed, null).Count);
            Verify.AreEqual(0, restClient.GetRepairTaskList(null, Repair.RepairTaskStateFilter.Preparing, null).Count);

            restClient.CancelRepairTask("A/1", 0);
            restClient.CancelRepairTask("A/2", 0);
            restClient.CancelRepairTask("A/3", 0);
            restClient.CancelRepairTask("B/1", 0);
            restClient.CancelRepairTask("B/2", 0);

            Verify.AreEqual(5, restClient.GetRepairTaskList(null, Repair.RepairTaskStateFilter.Completed, null).Count);

            restClient.DeleteRepairTask("A/1", 0);
            restClient.DeleteRepairTask("A/2", 0);
            restClient.DeleteRepairTask("A/3", 0);
            restClient.DeleteRepairTask("B/1", 0);
            restClient.DeleteRepairTask("B/2", 0);

            Verify.AreEqual(0, restClient.GetRepairTaskList(null, null, null).Count);

            // verify basic REST dataflow for health policy API
            restClient.CreateRepairTask(
                new RepairTaskData
                {
                    TaskId = "C/1",
                    State = RepairTaskState.Claimed,
                    Executor = "RE-3",
                    Action = "MyAction",
                    Description = "MyDescription",
                    Target = new RepairTaskTargetData { Kind = RepairTargetKind.Node, NodeNames = new[] { TestNodeName } },
                });

            task = VerifyRepairTaskHealthPolicy(restClient, false, false);

            restClient.UpdateRepairTaskHealthPolicy(task.TaskId, long.Parse(task.Version), true, true);
            task = VerifyRepairTaskHealthPolicy(restClient, true, true);

            restClient.UpdateRepairTaskHealthPolicy(task.TaskId, long.Parse(task.Version), false, false);
            task = VerifyRepairTaskHealthPolicy(restClient, false, false);

            restClient.UpdateRepairTaskHealthPolicy(task.TaskId, long.Parse(task.Version), null, true);
            task = VerifyRepairTaskHealthPolicy(restClient, false, true);

            restClient.UpdateRepairTaskHealthPolicy(task.TaskId, long.Parse(task.Version), true, null);
            VerifyRepairTaskHealthPolicy(restClient, true, true);

            LogHelper.Log("RepairManagerTest - End");
        }

        // Bug#2244833 - Figure out REST model for testability APIs
        // Currentlty the model for testability APIs and REST is not consistent hence removing them
        // after discussion with Vipul

        //[TestMethod]
        //[TestProperty("ExecutionGroup", "TestGroup")]
        //[Owner("anmola")]
        public void NodeControlTest()
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            var nodes = GetNodes(restClient);

            // Try to fail Valid Node
            restClient.RestartNode(nodes[0].Name, nodes[0].InstanceId);

            Thread.Sleep(10000);

            // Try to fail node with '0' as instance id
            restClient.RestartNode(nodes[1].Name, 0);

            Thread.Sleep(10000);

            // Try to fail invalid nodes i.e. bad NodeName and bad InstanceId
            restClient.RestartNode(nodes[2].Name, 10, HttpStatusCode.Conflict);

            restClient.RestartNode("InvalidNode", 0, HttpStatusCode.BadRequest);

            string clusterManifestContent = GetClusterManifest(restClient, fabricClient);
            XmlSerializer serlializer = new XmlSerializer(typeof(ClusterManifestType));
            ClusterManifestType clusterManifestType;
            XmlReaderSettings settings = new XmlReaderSettings()
            {
                DtdProcessing = DtdProcessing.Prohibit,
                XmlResolver = null
            };

            using (XmlReader reader = XmlReader.Create(clusterManifestContent, settings))
            {
                clusterManifestType = (ClusterManifestType)serlializer.Deserialize(reader);
            }

            Console.WriteLine("Downing node {0} with instance id 0", nodes[3].Name);
            restClient.StopNode(nodes[3].Name, 0);
            WaitForNodeStatus(fabricClient, nodes[3].Name, NodeStatus.Down, TimeSpan.FromSeconds(60.0d));

            Console.WriteLine("Starting node {0} with instance id 0", nodes[3].Name);

            Verify.IsTrue(clusterManifestType.NodeTypes.Length != 0, "There should be at lease one node typein ClusterManifest");

            ClusterManifestTypeNodeType nodeType3 = clusterManifestType.NodeTypes.Where(n => n.Name == nodes[3].Type).FirstOrDefault();
            Verify.IsFalse(nodeType3 == null, "NodeType for node 3 not found");
            string clusterConnectionPort = nodeType3.Endpoints.ClusterConnectionEndpoint.Port;


            restClient.StartNode(0, nodes[3].Name, nodes[3].IpAddressOrFQDN, int.Parse(clusterConnectionPort), 0);
            WaitForNodeStatus(fabricClient, nodes[3].Name, NodeStatus.Up, TimeSpan.FromSeconds(60.0d));

            nodes = GetNodes(restClient);

            Console.WriteLine("Downing node {0} with instance id {1}", nodes[3].Name, nodes[3].InstanceId);
            restClient.StopNode(nodes[3].Name, nodes[3].InstanceId);
            WaitForNodeStatus(fabricClient, nodes[3].Name, NodeStatus.Down, TimeSpan.FromSeconds(60.0d));

            restClient.StartNode(0, nodes[3].Name, nodes[3].IpAddressOrFQDN, int.Parse(clusterConnectionPort), 12345, HttpStatusCode.Conflict);

            Console.WriteLine("Starting node {0} with instance id {1}", nodes[3].Name, nodes[3].InstanceId);

            restClient.StartNode(0, nodes[3].Name, nodes[3].IpAddressOrFQDN, int.Parse(clusterConnectionPort), nodes[3].InstanceId);
            WaitForNodeStatus(fabricClient, nodes[3].Name, NodeStatus.Up, TimeSpan.FromSeconds(60.0d));

            // Attempt to start an Up node with old instance id, should fail
            restClient.StartNode(0, nodes[3].Name, nodes[3].IpAddressOrFQDN, 0, nodes[3].InstanceId, HttpStatusCode.Conflict);

            // Attempt to start an Up node with a don't care instance id
            restClient.StartNode(0, nodes[3].Name, nodes[3].IpAddressOrFQDN, 0, 0, HttpStatusCode.Conflict);

            nodes = GetNodes(restClient);

            // Attempt to start an Up node with the current instance id, should fail
            restClient.StartNode(0, nodes[3].Name, nodes[3].IpAddressOrFQDN, 0, nodes[3].InstanceId, HttpStatusCode.Conflict);
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("rajeetn")]
        public void PredeploymentTest()
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;
            string appName = "appV1";

            LogHelper.Log("PredeploymentTest - Begin");

            ApplicationDeployer appDeployer = GenerateApplicationWithSingleStatelessService(Constants.TestApplicationTypeName, "1.0", -1, appName);

            //
            // Provision application
            //
            string appDir = Path.Combine(HttpGatewayServiceTest.TestDeploymentOutputPath, appName);
            UploadApplication(appDir, appName);
            Console.WriteLine("Provisioning {0}", appName);
            restClient.ProvisionApplicationType(Path.Combine(Constants.IncomingDirectory, appName));

            //
            // Verify
            //
            List<ApplicationTypeResult> restAppTypeResult = GetApplicationTypes(restClient, fabricClient);
            Verify.IsTrue(restAppTypeResult.Count() == 1);

            //
            // Verify
            //
            GetServiceTypes(restClient, restAppTypeResult, fabricClient);


            var nodes = GetNodes(restClient);

            //
            // predeploy packages
            //
            List<SharingPolicy> sharingPolicies = new List<SharingPolicy>();
            sharingPolicies.Add(new SharingPolicy(appDeployer.Services[0].CodePackageName, PackageSharingPolicyScope.Code));
            sharingPolicies.Add(new SharingPolicy(string.Empty, PackageSharingPolicyScope.Config));
            Console.WriteLine("Predeployment for ApplicationType {0}, serviceManifestName {1} and CodePackage {2} on NodeName {3}",
                Constants.TestApplicationTypeName, appDeployer.ServiceManifestName, appDeployer.Services[0].CodePackageName, nodes[0].Name);
            restClient.DeployServicePackageToNode(Constants.TestApplicationTypeName, "1.0", appDeployer.ServiceManifestName, nodes[0].Name, sharingPolicies);

            string storeAppPath = Path.Combine(
                Directory.GetCurrentDirectory(),
                Constants.DropDirectory,
                Constants.DataDirectory,
                nodes[0].Name,
                @"Fabric\work\ImageCache\Store",
                Constants.TestApplicationTypeName);
            //verify apptype directory is created in image cache.
            Verify.IsTrue(Directory.Exists(storeAppPath));

            string sharedAppPath = Path.Combine(
                Directory.GetCurrentDirectory(),
                Constants.DropDirectory,
                Constants.DataDirectory,
                nodes[0].Name,
                @"Fabric\work\Applications\__shared\Store",
                Constants.TestApplicationTypeName,
                String.Format("{0}.{1}.1.0", appDeployer.ServiceManifestName, appDeployer.Services[0].CodePackageName));

            //verify code folder directory is created in shared folder.
            Verify.IsTrue(Directory.Exists(sharedAppPath));
            //
            // Unprovision application.
            //
            Console.WriteLine("Unprovisioning appV1");
            restClient.UnprovisionApplicationType(Constants.TestApplicationTypeName, ref appDeployer);

            //
            // Verify unprovision
            //
            restAppTypeResult = GetApplicationTypes(restClient, fabricClient);
            Verify.IsTrue(restAppTypeResult.Count() == 0, "restAppTypeResult.Count() == 0");

            //
            // Delete the app directory
            //
            Directory.Delete(appDir, true);

            LogHelper.Log("PredeploymentTest - Begin");
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("aprameyr")]
        public void ReportFaultTransientTest()
        {
            LogHelper.Log("ReportFaultTransientTest - Begin");

            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            for (int i = 0; i < 5; i++)
            {
                if (this.ReportFaultTransientTestHelper(fabricClient, restClient))
                {
                    LogHelper.Log("ReportFaultTransientTest - End");

                    return;
                }
            }

            Assert.Fail("Failed after five attempts");
        }

        private bool ReportFaultTransientTestHelper(FabricClient fabricClient, RESTClient restClient)
        {
            var initialReplicas = this.GetAllFmReplicas(fabricClient);

            // Find the primary
            var initialPrimary = initialReplicas.First(e => e.ReplicaRole == ReplicaRole.Primary);

            // Report Fault Transient
            restClient.RestartReplica(initialPrimary.NodeName, Constants.FmPartitionId, initialPrimary.Id);

            // Sleep as that seems to be the simplest way to stabilize
            Thread.Sleep(10000);

            // Verify that the primary has moved and hope there is no LB movement
            var finalReplicas = this.GetAllFmReplicas(fabricClient);

            var finalReplica = finalReplicas.First(e => e.Id == initialPrimary.Id);

            // Assert that the role has changed
            return ReplicaRole.Primary != finalReplica.ReplicaRole;
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("aprameyr")]
        public void ReportFaultPermanentUsingRemoveWithForceFlagTest()
        {
            ReportFaultPermanentTest(false, true, "ReportFaultPermanentUsingRemoveWithForceFlagTest");
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("aprameyr")]
        public void ReportFaultPermanentUsingDeleteWithForceFlagTest()
        {
            ReportFaultPermanentTest(true, true, "ReportFaultPermanentUsingDeleteWithForceFlagTest");
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("aprameyr")]
        public void ReportFaultPermanentUsingRemoveTest()
        {
            ReportFaultPermanentTest(false, false, "ReportFaultPermanentUsingRemoveTest");
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("aprameyr")]
        public void ReportFaultPermanentUsingDeleteTest()
        {
            ReportFaultPermanentTest(true, false, "ReportFaultPermanentUsingDeleteTest");
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("bharatn")]
        public void ClaimsBasedAuthenticationTest()
        {
            if (clusterCredentialType != CredentialType.X509)
            {
                return;
            }

            LogHelper.Log("ClaimsBasedAuthenticationTest - Begin");

            Tuple<RESTClient, FabricClient> clients = CreateFabricClients(true);
            RESTClient restClient = clients.Item1;
            var nodes = GetNodes(restClient);
            Verify.IsTrue(nodes.Count != 0);

            //try with invalid claims
            restClient.authorizationHeader = "InvalidAuthHeaderValue";
            var dummyResult = restClient.GetNodes(HttpStatusCode.Forbidden);

            LogHelper.Log("ClaimsBasedAuthenticationTest - End");
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("bharatn")]
        public void NegativeTest()
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            LogHelper.Log("NegativeTest - Begin");

            restClient.PostRandomInvalidUri();
            string randomName = Path.GetRandomFileName();

            //
            // Create invalid body and send post requests to the valid URI's that accept body.
            //
            ServiceTypeDescription invalidBody = new ServiceTypeDescription();
            restClient.PostInvalidUri("", invalidBody);
            restClient.PostInvalidUri("Services", invalidBody);
            restClient.PostInvalidUri("Partitions", invalidBody);
            restClient.PostInvalidUri(UriSuffix.ProvisionApplicationTypeUriTemplate, invalidBody);
            restClient.PostInvalidUri(string.Format(UriSuffix.GetServiceTypesUriTemplate, randomName, randomName), invalidBody);
            restClient.PostInvalidUri(string.Format(UriSuffix.GetApplicationManifestUriTemplate, randomName, randomName), invalidBody);
            restClient.PostInvalidUri(string.Format(UriSuffix.GetServiceManifestUriTemplate, randomName, randomName, randomName), invalidBody);
            restClient.PostInvalidUri(string.Format(UriSuffix.GetServiceTypesByTypeUriTemplate, randomName, randomName, randomName), invalidBody);
            restClient.PostInvalidUri(UriSuffix.CreateApplicationUriTemplate, invalidBody);
            restClient.PostInvalidUri(string.Format(UriSuffix.UpgradeApplicationUriTemplate, randomName), invalidBody);
            restClient.PostInvalidUri(string.Format(UriSuffix.CreateServiceUriTemplate, randomName), invalidBody);
            restClient.PostInvalidUri(string.Format(UriSuffix.CreateServiceFromTemplateUriTemplate, randomName), invalidBody);

            restClient.PostInvalidUri(UriSuffix.ProvisionFabricUriTemplate, invalidBody);
            restClient.PostInvalidUri(UriSuffix.UnprovisionFabricUriTemplate, invalidBody);
            restClient.PostInvalidUri(UriSuffix.UpgradeFabricUriTemplate, invalidBody);
            restClient.PostInvalidUri(UriSuffix.UpdateFabricUpgradeUriTemplate, invalidBody);
            restClient.PostInvalidUri(string.Format(UriSuffix.DeactivateNodeUriTemplate, randomName), invalidBody);
            restClient.PostInvalidUri(UriSuffix.MoveNextFabricUpgradeDomainUriTemplate, invalidBody);

            LogHelper.Log("NegativeTest - End");
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("danmas")]
        public void InvokeInfrastructureTest()
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            LogHelper.Log("InvokeInfrastructureTest - Begin");

            Uri serviceUri = new Uri("fabric:/System/InfrastructureService");
            Uri serviceUriB = new Uri("fabric:/System/InfrastructureService/MultiStamp");
            Uri notFoundServiceUri = new Uri("fabric:/System/InfrastructureService/DoesNotExist");
            Uri invalidServiceUri = new Uri("fabric:/System/Invalid");

            string response = restClient.InvokeInfrastructureQuery(serviceUri, "echo");
            Verify.AreEqual(
                "[TestInfrastructureCoordinator reply for RunCommandAsync(False,'echo')]",
                response);

            response = restClient.InvokeInfrastructureQuery(serviceUriB, "echo");
            Verify.AreEqual(
                "[TestInfrastructureCoordinator reply for RunCommandAsync(False,'echo')]",
                response);

            restClient.InvokeInfrastructureQuery(notFoundServiceUri, "echo", HttpStatusCode.NotFound);
            restClient.InvokeInfrastructureQuery(invalidServiceUri, "echo", HttpStatusCode.BadRequest);

            response = restClient.InvokeInfrastructureCommand(serviceUri, "echo");
            Verify.AreEqual(
                "[TestInfrastructureCoordinator reply for RunCommandAsync(True,'echo')]",
                response);

            response = restClient.InvokeInfrastructureCommand(serviceUriB, "echo");
            Verify.AreEqual(
                "[TestInfrastructureCoordinator reply for RunCommandAsync(True,'echo')]",
                response);

            response = restClient.InvokeInfrastructureCommand(serviceUri, "emptyresponse");
            Verify.IsTrue(string.IsNullOrEmpty(response));

            restClient.InvokeInfrastructureCommand(notFoundServiceUri, "echo", HttpStatusCode.NotFound);
            restClient.InvokeInfrastructureCommand(invalidServiceUri, "echo", HttpStatusCode.BadRequest);
            restClient.InvokeInfrastructureCommand(serviceUri, "InvalidCommand", HttpStatusCode.BadRequest);

            LogHelper.Log("InvokeInfrastructureTest - End");
        }

        [TestMethod]
        [TestProperty("ExecutionGroup", "TestGroup")]
        [Owner("t-grzub")]
        public void NamesTest()
        {
            LogHelper.Log("NamesTest - Begin");

            TestNames();
            TestPropertyPut();
            TestPropertyDelete();
            TestPropertyBatch();

            LogHelper.Log("NamesTest - End");
        }

        #region Private Functions

        private void ReportFaultPermanentTest(bool useDelete, bool useForceFlag, string testName)
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            LogHelper.Log("{0} - Begin", testName);

            var initialReplicas = this.GetAllFmReplicas(fabricClient);

            // Find the primary
            var initialPrimary = initialReplicas.First(e => e.ReplicaRole == ReplicaRole.Primary);

            if (useForceFlag)
            {
                restClient.RemoveReplicaWithForce(initialPrimary.NodeName, Constants.FmPartitionId, initialPrimary.Id, useDelete);
            }
            else
            {
                restClient.RemoveReplica(initialPrimary.NodeName, Constants.FmPartitionId, initialPrimary.Id, useDelete);
            }

            // Sleep as that seems to be the simplest way to stabilize
            Thread.Sleep(20000);

            // Verify that the primary does not exist
            var finalReplicas = this.GetAllFmReplicas(fabricClient);

            var finalReplica = finalReplicas.FirstOrDefault(e => e.Id == initialPrimary.Id);

            Assert.IsTrue(finalReplica == null || finalReplica.ReplicaStatus == ServiceReplicaStatus.Dropped);

            LogHelper.Log("{0} - End", testName);
        }

        CreateServiceGroupInput GetCreateServiceGroupInput(
            string applicationName,
            string serviceTypeName,
            string serviceName,
            ServicePackageActivationMode servicePackageActivationMode,
            ServiceDeployer serviceDeployer)
        {
            var createServiceGroupData = new CreateServiceGroupInput
            {
                ApplicationName = applicationName,
                ServiceName = serviceName,
                ServicePackageActivationMode = servicePackageActivationMode,
                ServiceGroupMemberDescription = new List<ServiceGroupMemberDescription>()
                {
                    new ServiceGroupMemberDescription()
                    {
                        ServiceName = serviceName + "#member1",
                        ServiceTypeName = serviceTypeName
                    }
                },
                ServiceTypeName = serviceDeployer.ServiceTypeName,
                ServiceKind = System.Fabric.Query.ServiceKind.Stateless,
                TargetReplicaSetSize = serviceDeployer.TargetReplicaSetSize,
                MinReplicaSetSize = serviceDeployer.MinReplicaSetSize,
                PartitionDescription = { PartitionScheme = serviceDeployer.Partition.Scheme },
                InstanceCount = -1
            };

            return createServiceGroupData;
        }

        CreateServiceInput GetCreateServiceInput(
            string applicationName,
            string serviceName,
            ServicePackageActivationMode servicePackageActivationMode,
            ServiceDeployer serviceDeployer)
        {
            var createServiceData = new CreateServiceInput
            {
                ApplicationName = applicationName,
                ServiceName = serviceName,
                ServiceTypeName = serviceDeployer.ServiceTypeName,
                ServiceKind = System.Fabric.Query.ServiceKind.Stateless,
                TargetReplicaSetSize = serviceDeployer.TargetReplicaSetSize,
                MinReplicaSetSize = serviceDeployer.MinReplicaSetSize,
                PartitionDescription = { PartitionScheme = serviceDeployer.Partition.Scheme },
                InstanceCount = -1,
                ServicePackageActivationMode = servicePackageActivationMode
            };

            return createServiceData;
        }

        private void PrintFabricUpgradeProgress(ref FabricUpgradeProgressResult result)
        {
            string pendingDomains = "", completedDomains = "", inProgress = "";
            foreach (var item in result.UpgradeDomains)
            {
                if (item.State == UpgradeDomainState.Pending)
                {
                    pendingDomains += item.Name + ", ";
                }
                else if (item.State == UpgradeDomainState.Completed)
                {
                    completedDomains += item.Name + ", ";
                }
                else
                {
                    inProgress = item.Name;
                }
            }
            Console.WriteLine("Fabric Upgrade state - {0}", result.UpgradeState.ToString());
            Console.WriteLine("InProgress = {0}; Pending = {1}; Completed = {2}", inProgress, pendingDomains, completedDomains);
        }

        private void PrintAppUpgradeProgress(ref ApplicationUpgradeProgressResult result)
        {
            string pendingDomains = "", completedDomains = "", inProgress = "";
            if (result.UpgradeDomains != null)
            {
                foreach (var item in result.UpgradeDomains)
                {
                    if (item.State == UpgradeDomainState.Pending)
                    {
                        pendingDomains += item.Name + ", ";
                    }
                    else if (item.State == UpgradeDomainState.Completed)
                    {
                        completedDomains += item.Name + ", ";
                    }
                    else
                    {
                        inProgress = item.Name;
                    }
                }
            }
            Console.WriteLine("Upgrade state - {0}", result.UpgradeState.ToString());
            Console.WriteLine("InProgress = {0}; Pending = {1}; Completed = {2}", inProgress, pendingDomains, completedDomains);
        }

        private delegate IList<HealthEvent> GetEntityHealthEvent();
        private void VerifyHealthQueries(string appName, string serviceName, RESTClient restClient, FabricClient fabricClient = null)
        {
            Console.WriteLine("Verifying Health queries...");

            FabricClientSettings defaultSettings = new FabricClientSettings();
            Console.WriteLine("Default health report send interval is {0}", defaultSettings.HealthReportSendInterval);
            double sendDelayInSeconds = defaultSettings.HealthReportSendInterval.TotalSeconds;
            if (sendDelayInSeconds == 0)
            {
                sendDelayInSeconds = 10; // default to 10 seconds
            }

            TimeSpan reportSendDelay = TimeSpan.FromSeconds(sendDelayInSeconds * 2);

            string sourceIdREST = "RESTClient";
            string testProperty = "dummyProperty";
            string testDescription = "test_description";
            TimeSpan timeToLive20Min = TimeSpan.FromMinutes(20);
            long autoSequenceNumber = 0;
            long sequenceNumber = 32;
            bool removeWhenExpired1 = true;
            bool dontRemoveWhenExpired = false;

            Func<TimeSpan, long, bool, HealthInformation> lambdaGetHealthInfo =
                (ttl, sn, b) =>
                    new HealthInformation(sourceIdREST, testProperty, HealthState.Warning)
                    {
                        Description = testDescription,
                        TimeToLive = ttl,
                        SequenceNumber = sn,
                        RemoveWhenExpired = b
                    };

            // Report with specified SN
            var nodeHealthInfo = lambdaGetHealthInfo(timeToLive20Min, sequenceNumber, removeWhenExpired1);

            // Report with 0 SN, should be generated by the fabric client
            var appHealthInfo = lambdaGetHealthInfo(TimeSpan.FromHours(3), 0, removeWhenExpired1);

            // Report with auto SN, should be generated by the fabric client
            var serviceHealthInfo = lambdaGetHealthInfo(TimeSpan.FromMinutes(20), autoSequenceNumber, dontRemoveWhenExpired);

            long sequenceNumber2 = 23;
            var partitionHealthInfo = lambdaGetHealthInfo(TimeSpan.FromMinutes(40), sequenceNumber2, dontRemoveWhenExpired);

            var replicaHealthInfo = lambdaGetHealthInfo(TimeSpan.FromHours(1), autoSequenceNumber, dontRemoveWhenExpired);

            var clusterHealthInfo = new HealthInformation(sourceIdREST, testProperty, HealthState.Warning)
            {
                Description = testDescription,
                TimeToLive = TimeSpan.FromMinutes(90),
                SequenceNumber = sequenceNumber,
                RemoveWhenExpired = false
            };

            // Wait for all system reports to be received
            Console.WriteLine("Wait until health has all nodes");
            int totalNodeCount = 5;
            int maxRetryCount = 10;
            bool allNodesReportedOn = false;
            for (int i = 0; i < maxRetryCount; i++)
            {
                ClusterHealth clusterHealth = restClient.EvaluateClusterHealth();
                if (clusterHealth.NodeHealthStates.Count == totalNodeCount)
                {
                    allNodesReportedOn = true;
                    break;
                }
                else
                {
                    Thread.Sleep(reportSendDelay);
                }
            }

            Verify.IsTrue(allNodesReportedOn, "HM didn't receive all system node reports");

            var result = GetNodes(restClient);
            var nodeName = result[0].Name;
            var udName = result[0].UpgradeDomain;

            restClient.ReportNodeHealth(nodeName, nodeHealthInfo);
            restClient.ReportApplicationHealth(appName, appHealthInfo);
            restClient.ReportServicehealth(appName, serviceName, serviceHealthInfo);

            List<ServicePartitionResult> partitionResult = GetPartitions(appName, serviceName, restClient, fabricClient);
            Verify.IsTrue(partitionResult.Count != 0);
            restClient.ReportPartitionHealth(appName, serviceName, partitionResult[0].PartitionInformation.Id, partitionHealthInfo);

            List<ServiceReplicaResult> replicaResult = GetReplicas(appName, serviceName, partitionResult[0].PartitionInformation.Id.ToString(), restClient, fabricClient);
            Verify.IsTrue(replicaResult.Count != 0);

            Verify.IsTrue(replicaResult[0].ServiceKind == ServiceKind.Stateless, "Stateless service has Stateless replicas");
            string id = replicaResult[0].InstanceId;
            restClient.ReportReplicaHealth(appName, serviceName, partitionResult[0].PartitionInformation.Id, replicaResult[0], replicaHealthInfo);

            // Report on stateful replica
            string systemApp = "fabric:/System";
            string fmServiceName = "fabric:/System/FailoverManagerService";
            List<ServicePartitionResult> fmPartitions = GetPartitions(systemApp, fmServiceName, restClient, fabricClient);
            Verify.IsTrue(fmPartitions.Count != 0);
            List<ServiceReplicaResult> fmReplicas = GetReplicas(systemApp, fmServiceName, fmPartitions[0].PartitionInformation.Id.ToString(), restClient, fabricClient);
            Verify.IsTrue(fmReplicas.Count != 0);

            Verify.IsTrue(fmReplicas[0].ServiceKind == ServiceKind.Stateful, "FM service has stateful replicas");
            string id2 = fmReplicas[0].ReplicaId;
            restClient.ReportReplicaHealth(systemApp, fmServiceName, fmPartitions[0].PartitionInformation.Id, fmReplicas[0], replicaHealthInfo);

            restClient.ReportClusterHealth(clusterHealthInfo);

            maxRetryCount = 5;
            List<GetEntityHealthEvent> entitiesToCheck = new List<GetEntityHealthEvent>();
            entitiesToCheck.Add(() => { return fabricClient.HealthManager.GetNodeHealthAsync(nodeName).Result.HealthEvents; });
            entitiesToCheck.Add(() => { return fabricClient.HealthManager.GetApplicationHealthAsync(new Uri(appName)).Result.HealthEvents; });
            entitiesToCheck.Add(() => { return fabricClient.HealthManager.GetServiceHealthAsync(new Uri(serviceName)).Result.HealthEvents; });
            entitiesToCheck.Add(() => { return fabricClient.HealthManager.GetPartitionHealthAsync(partitionResult[0].PartitionInformation.Id).Result.HealthEvents; });
            entitiesToCheck.Add(() => { return fabricClient.HealthManager.GetReplicaHealthAsync(partitionResult[0].PartitionInformation.Id, Convert.ToInt64(id)).Result.HealthEvents; });
            entitiesToCheck.Add(() => { return fabricClient.HealthManager.GetReplicaHealthAsync(fmPartitions[0].PartitionInformation.Id, Convert.ToInt64(id2)).Result.HealthEvents; });
            entitiesToCheck.Add(() => { return fabricClient.HealthManager.GetClusterHealthAsync().Result.HealthEvents; });

            // Wait till the health reports that we sent is received at HM.
            for (int i = 0; i < maxRetryCount; ++i)
            {
                entitiesToCheck.RemoveAll(entityToCheck => CheckIfHealthEventExists(entityToCheck(), sourceIdREST, testProperty));

                if (entitiesToCheck.Count == 0)
                {
                    break;
                }
                else
                {
                    Thread.Sleep(reportSendDelay);
                }
            }

            var okOrErrorFilter = HealthStateFilter.Ok | HealthStateFilter.Error;
            var errorFilter = HealthStateFilter.Error;
            var allFilter = HealthStateFilter.All;
            var noneFilter = HealthStateFilter.None;

            var testEventsFilter = new HealthEventsFilter() { HealthStateFilterValue = okOrErrorFilter };
            var testNodesFilter = new NodeHealthStatesFilter() { HealthStateFilterValue = errorFilter };
            var testApplicationsFilter = new ApplicationHealthStatesFilter() { HealthStateFilterValue = allFilter };
            var testPartitionsFilter = new PartitionHealthStatesFilter() { HealthStateFilterValue = noneFilter };
            var testServicesFilter = new ServiceHealthStatesFilter() { HealthStateFilterValue = allFilter };
            var testDeployedApplicationsFilter = new DeployedApplicationHealthStatesFilter() { HealthStateFilterValue = errorFilter };
            var testReplicasFilter = new ReplicaHealthStatesFilter() { HealthStateFilterValue = okOrErrorFilter };
            var testDeployedServicePackagesFilter = new DeployedServicePackageHealthStatesFilter() { HealthStateFilterValue = allFilter };

            Console.WriteLine("Node Health...");
            GetNodeHealth(nodeName, restClient, fabricClient, nodeHealthInfo, testEventsFilter);
            GetNodeHealth(nodeName, restClient, fabricClient, nodeHealthInfo, null);

            Console.WriteLine("Application Health...");
            GetApplicationHealth(appName, restClient, fabricClient, appHealthInfo, null, testServicesFilter, null);
            GetApplicationHealth(appName, restClient, fabricClient, appHealthInfo, testEventsFilter, null, testDeployedApplicationsFilter);

            Console.WriteLine("Service Health...");
            GetServiceHealth(appName, serviceName, restClient, fabricClient, serviceHealthInfo, testEventsFilter, testPartitionsFilter);
            GetServiceHealth(appName, serviceName, restClient, fabricClient, serviceHealthInfo, null, null);

            Console.WriteLine("Partition Health...");
            GetPartitionHealth(appName, serviceName, partitionResult[0].PartitionInformation.Id, restClient, fabricClient, partitionHealthInfo, testEventsFilter, testReplicasFilter);
            GetPartitionHealth(appName, serviceName, partitionResult[0].PartitionInformation.Id, restClient, fabricClient, partitionHealthInfo, null, null);

            Console.WriteLine("Stateless Instance Health...");
            GetReplicaHealth(appName, serviceName, partitionResult[0].PartitionInformation.Id, id, restClient, fabricClient, replicaHealthInfo, testEventsFilter);
            GetReplicaHealth(appName, serviceName, partitionResult[0].PartitionInformation.Id, id, restClient, fabricClient, replicaHealthInfo, null);

            Console.WriteLine("FM Replica Health...");
            GetReplicaHealth(appName, fmServiceName, fmPartitions[0].PartitionInformation.Id, id2, restClient, fabricClient, replicaHealthInfo, testEventsFilter);
            GetReplicaHealth(appName, fmServiceName, fmPartitions[0].PartitionInformation.Id, id2, restClient, fabricClient, replicaHealthInfo, null);

            Console.WriteLine("Deployed application health...");
            GetDeployedApplicationHealth(nodeName, appName, restClient, fabricClient, testEventsFilter, testDeployedServicePackagesFilter);
            GetDeployedApplicationHealth(nodeName, appName, restClient, fabricClient, null, null);

            var servicePackagesOnNode = GetServiceManifestsOnNode(nodeName, appName, restClient, fabricClient);

            Console.WriteLine("Deployed service package health..");
            foreach (var servicePackage in servicePackagesOnNode)
            {
                GetDeployedServicePackageHealth(nodeName, appName, servicePackage.Name, servicePackage.ServicePackageActivationId, restClient, fabricClient, testEventsFilter);
                GetDeployedServicePackageHealth(nodeName, appName, servicePackage.Name, servicePackage.ServicePackageActivationId, restClient, fabricClient, null);
            }

            var testClusterHealthPolicy = new ClusterHealthPolicy();
            testClusterHealthPolicy.ConsiderWarningAsError = true;
            testClusterHealthPolicy.MaxPercentUnhealthyApplications = 10;
            testClusterHealthPolicy.MaxPercentUnhealthyNodes = 20;

            var appHealthPolicy = new ApplicationHealthPolicy();
            appHealthPolicy.DefaultServiceTypeHealthPolicy = new ServiceTypeHealthPolicy()
            {
                MaxPercentUnhealthyPartitionsPerService = 10,
                MaxPercentUnhealthyReplicasPerPartition = 20,
                MaxPercentUnhealthyServices = 15
            };

            var testClusterHealthPolicy2 = new ClusterHealthPolicy();
            testClusterHealthPolicy2.ConsiderWarningAsError = false;
            testClusterHealthPolicy2.MaxPercentUnhealthyNodes = 20;
            testClusterHealthPolicy2.ApplicationTypeHealthPolicyMap.Add(Constants.TestApplicationTypeName, 10);

            var applicationHealthPolicyMap = new ApplicationHealthPolicyMap();
            applicationHealthPolicyMap.Add(new Uri(appName), appHealthPolicy);

            GetClusterHealth(restClient, fabricClient, clusterHealthInfo, "2.0", null, null, testEventsFilter, testNodesFilter, testApplicationsFilter);
            GetClusterHealth(restClient, fabricClient, clusterHealthInfo, "1.0", null, null, testEventsFilter, null, null);
            GetClusterHealth(restClient, fabricClient, clusterHealthInfo, "2.0", testClusterHealthPolicy, applicationHealthPolicyMap, null, testNodesFilter, testApplicationsFilter);
            GetClusterHealth(restClient, fabricClient, clusterHealthInfo, "2.0", testClusterHealthPolicy2, applicationHealthPolicyMap, null, null, null);
            GetClusterHealth(restClient, fabricClient, clusterHealthInfo, "1.0", null, null, null, null, null);

            // Invalid reports
            string emptyString = string.Empty;
            var invalidHealthInfo1 = lambdaGetHealthInfo(TimeSpan.FromMilliseconds(random.Next()), random.Next(), true);
            invalidHealthInfo1.SourceId = emptyString;
            restClient.ReportNodeHealth(nodeName, invalidHealthInfo1, HttpStatusCode.BadRequest);

            // Empty property
            HealthInformation invalidHealthInfo2 = lambdaGetHealthInfo(TimeSpan.FromMilliseconds(random.Next()), random.Next(), true);
            invalidHealthInfo2.Property = emptyString;
            restClient.ReportApplicationHealth(appName, invalidHealthInfo2, HttpStatusCode.BadRequest);

            // Invalid source "System.REST"
            HealthInformation invalidHealthInfo3 = lambdaGetHealthInfo(TimeSpan.FromMilliseconds(random.Next()), random.Next(), true);
            invalidHealthInfo3.SourceId = "System.REST";
            restClient.ReportServicehealth(appName, serviceName, invalidHealthInfo3, HttpStatusCode.BadRequest);
        }

        private void VerifyPartitionLoadInformationQueries(string appName, string serviceName, RESTClient restClient, FabricClient fabricClient = null)
        {
            Console.WriteLine("Verifying Partition Load queries...");

            var result = GetNodes(restClient);

            List<ServicePartitionResult> partitionResult = GetPartitions(appName, serviceName, restClient, fabricClient);
            Verify.IsTrue(partitionResult.Count != 0);

            // The default HealthReportSendInterval is - 30 sec, wait till that time for the health report
            // to be propagated to HM.
            Thread.Sleep(30000);

            Console.WriteLine("Partition Load...");
            GetPartitionLoadInformation(appName, serviceName, partitionResult[0].PartitionInformation.Id, restClient, fabricClient);
        }

        private void VerifyHostingQueriesForAllAppsAndNodes(RESTClient restClient, FabricClient fabricClient = null)
        {
            Console.WriteLine("Verifying Hosting queries..");
            List<NodesResult> nodesResult = GetNodes(restClient, fabricClient);
            foreach (var node in nodesResult)
            {
                Console.WriteLine("VerifyingHostQueries on Node - {0}", node.Name);
                List<ApplicationsOnNodeResult> appsOnNode = GetApplicationsOnNode(node.Name, restClient, fabricClient);

                foreach (var app in appsOnNode)
                {
                    Console.WriteLine("VerifyingHostQueries on for application {0} on node - {1}", app.Name, node.Name);
                    VerifyHostingQueries(restClient, app.Name, node.Name, fabricClient);
                }
            }
        }
        private void VerifyHostingQueries(RESTClient restClient, string appName, string nodeName, FabricClient fabricClient = null)
        {
            Verify.IsTrue(appName != null);
            List<ServiceManifestOnNodeResult> serviceManOnNode = GetServiceManifestsOnNode(nodeName, appName, restClient, fabricClient);
            if (serviceManOnNode.Count() == 0)
            {
                //
                // If the service package hasnt been activated yet, or if the service package has been deactivated, then
                // the result would be empty.
                //
                return;
            }

            List<CodePackageOnNodeResult> codePackageOnNode = GetCodePackagesOnNode(nodeName, appName, serviceManOnNode[0].Name, restClient, fabricClient);
            if (codePackageOnNode.Count() == 0)
            {
                //
                // If the service package is still activating, then the code package result could be empty.
                //
                return;
            }

            List<ServiceReplicasOnNodeResult> serviceReplicasOnNode = GetServiceReplicasOnNode(nodeName, appName, serviceManOnNode[0].Name, restClient, fabricClient);
            Verify.IsTrue(serviceReplicasOnNode.Count() != 0);

            serviceReplicasOnNode = GetServiceReplicasOnNode(nodeName, appName, null, restClient, fabricClient);
            Verify.IsTrue(serviceReplicasOnNode.Count() != 0);

            foreach (var item in serviceReplicasOnNode)
            {
                VerifyDeployedReplicaDetail(nodeName, item, restClient, fabricClient);
            }
        }

        private void VerifyRestartCodePackage(RESTClient restClient)
        {
            Console.WriteLine("Verifying RestartCodePackage..");
            string nodeName;
            string appName;
            string serviceManifest;
            string codePackageName;
            long instanceId;

            GetCodePackageParamsForRestart(restClient, out nodeName, out appName, out serviceManifest, out codePackageName, out instanceId);

            restClient.RestartCodePackage(nodeName, appName, serviceManifest, codePackageName, instanceId);

            // Stabilize
            Thread.Sleep(25000);

            GetCodePackageParamsForRestart(restClient, out nodeName, out appName, out serviceManifest, out codePackageName, out instanceId);

            restClient.RestartCodePackage(nodeName, appName, serviceManifest, codePackageName, 0);

            // Stabilize
            Thread.Sleep(25000);

            GetCodePackageParamsForRestart(restClient, out nodeName, out appName, out serviceManifest, out codePackageName, out instanceId);

            restClient.RestartCodePackage(nodeName, appName, serviceManifest, codePackageName, 10, HttpStatusCode.Conflict);

            restClient.RestartCodePackage("Invalid", appName, serviceManifest, codePackageName, 0, HttpStatusCode.BadRequest);

            restClient.RestartCodePackage(nodeName, "fabric:/Invalid", serviceManifest, codePackageName, 0, HttpStatusCode.NotFound);

            restClient.RestartCodePackage(nodeName, appName, "Invalid", codePackageName, 0, HttpStatusCode.NotFound);

            restClient.RestartCodePackage(nodeName, appName, serviceManifest, "Invalid", 0, HttpStatusCode.NotFound);
        }

        private void GetCodePackageParamsForRestart(RESTClient restClient, out string nodeName, out string appName, out string serviceManifest, out string codePackageName, out long instanceId)
        {
            List<NodesResult> nodesResult = GetNodes(restClient);

            nodeName = null;
            appName = null;
            serviceManifest = null;
            codePackageName = null;
            instanceId = -1;

            for (int i = 0; i < nodesResult.Count(); i++)
            {
                nodeName = nodesResult[i].Name;
                List<ApplicationsOnNodeResult> appsOnNode = GetApplicationsOnNode(nodeName, restClient);
                if (appsOnNode.Count() != 0)
                {
                    appName = appsOnNode[0].Name;
                    List<ServiceManifestOnNodeResult> serviceManOnNode = GetServiceManifestsOnNode(nodeName, appName, restClient);
                    if (serviceManOnNode.Count() != 0)
                    {
                        serviceManifest = serviceManOnNode[0].Name;
                        List<CodePackageOnNodeResult> codePackageOnNode = GetCodePackagesOnNode(nodeName, appName, serviceManifest, restClient);
                        if (codePackageOnNode.Count() != 0)
                        {
                            codePackageName = codePackageOnNode[0].Name;
                            instanceId = codePackageOnNode[0].MainEntryPoint.InstanceId;
                            break;
                        }
                    }
                }
            }

            Verify.IsTrue(appName != null);
            Verify.IsTrue(nodeName != null);
            Verify.IsTrue(serviceManifest != null);
            Verify.IsTrue(codePackageName != null);
            Verify.IsTrue(instanceId != -1);
        }

        private void VerifyDeployedReplicaDetail(string nodeName, ServiceReplicasOnNodeResult item, RESTClient restClient, FabricClient fabricClient)
        {
            long id = long.Parse(item.ServiceKind == ServiceKind.Stateful ? item.ReplicaId : item.InstanceId);

            this.VerifyDeployedReplicaDetail(nodeName, item.PartitionId, id, restClient, fabricClient);
        }

        private void VerifyDeployedReplicaDetail(string nodeName, Guid partitionId, long id, RESTClient restClient, FabricClient fabricClient)
        {
            var restResult = restClient.GetReplicaDetail(nodeName, partitionId, id);
            var clientResult = fabricClient.QueryManager.GetDeployedReplicaDetailAsync(nodeName, partitionId, id).Result;

            Verify.AreEqual(clientResult.ServiceName, restResult.ServiceName, "ServiceName");
            Verify.AreEqual(clientResult.PartitionId, restResult.PartitionId, "PartitionId");
            Verify.AreEqual(clientResult.CurrentServiceOperation, restResult.CurrentServiceOperation, "CurrentServiceOp");

            // Verify.AreEqual(clientResult.CurrentServiceOperationStartTimeUtc.ToString(), restResult.CurrentServiceOperationStartTimeUtc, "StartTime");

            Verify.AreEqual(clientResult.ReportedLoad.Count, restResult.ReportedLoad.Count);
            for (int i = 0; i < clientResult.ReportedLoad.Count; i++)
            {
                VerifyLoadResult(clientResult.ReportedLoad[i], restResult.ReportedLoad[i]);
            }

            var sfClientResult = clientResult as DeployedStatefulServiceReplicaDetail;
            if (sfClientResult != null)
            {
                Verify.AreEqual(ServiceKind.Stateful, restResult.ServiceKind, "ServiceKind");
                Verify.AreEqual(sfClientResult.CurrentReplicatorOperation, restResult.CurrentReplicatorOperation, "ReplicatorOP");
                Verify.AreEqual(sfClientResult.ReadStatus, restResult.ReadStatus, "ReadStatus");
                Verify.AreEqual(sfClientResult.ReplicaId.ToString(), restResult.ReplicaId.ToString(), "ReplicaId");
                Verify.AreEqual(sfClientResult.WriteStatus, restResult.WriteStatus, "WriteStatus");
                VerifyReplicatorStatus(sfClientResult.ReplicatorStatus, restResult.ReplicatorStatus);
            }

            var slClientResult = clientResult as DeployedStatelessServiceInstanceDetail;
            if (slClientResult != null)
            {
                Verify.AreEqual(ServiceKind.Stateless, restResult.ServiceKind, "ServiceKind");
                Verify.AreEqual(slClientResult.InstanceId.ToString(), restResult.InstanceId.ToString(), "InstanceId");
            }
        }
        private void VerifyReplicatorStatus(System.Fabric.Query.ReplicatorStatus expected, ReplicatorStatusResult actual)
        {
            if (expected is PrimaryReplicatorStatus)
            {
                Verify.AreEqual(actual.Status.Kind, ReplicaRole.Primary);
                VerifyPrimaryReplicatorStatus(expected as PrimaryReplicatorStatus, actual.Status as PrimaryReplicatorStatusResult);
            }
            else if (expected is SecondaryReplicatorStatus)
            {
                Verify.IsTrue(actual.Status.Kind == ReplicaRole.IdleSecondary || actual.Status.Kind == ReplicaRole.ActiveSecondary, "ReplicatorRole");
                VerifySecondaryReplicatorStatus(expected as SecondaryReplicatorStatus, actual.Status as SecondaryReplicatorStatusResult);
            }
            else
            {
                Verify.Fail("Unknown type of replicatorstatus result returned");
            }
        }

        private void VerifyPrimaryReplicatorStatus(PrimaryReplicatorStatus expected, PrimaryReplicatorStatusResult actual)
        {
            VerifyReplicatorQueueStatus(expected.ReplicationQueueStatus, actual.ReplicationQueueStatus);

            for (int i = 0; i < expected.RemoteReplicators.Count; i++)
            {
                VerifyRemoteReplicatorStatus(expected.RemoteReplicators[i], actual.RemoteReplicators[i]);
            }
        }
        private void VerifySecondaryReplicatorStatus(SecondaryReplicatorStatus expected, SecondaryReplicatorStatusResult actual)
        {
            Verify.AreEqual(expected.IsInBuild, actual.IsInBuild);
            // Verify.AreEqual(expected.LastAcknowledgementSentTimeUtc.ToString(), actual.LastAcknowledgementSentTimeUtc);
            // Verify.AreEqual(expected.LastCopyOperationReceivedTimeUtc.ToString(), actual.LastCopyOperationReceivedTimeUtc);
            // Verify.AreEqual(expected.LastReplicationOperationReceivedTimeUtc.ToString(), actual.LastReplicationOperationReceivedTimeUtc);

            VerifyReplicatorQueueStatus(expected.ReplicationQueueStatus, actual.ReplicationQueueStatus);
            VerifyReplicatorQueueStatus(expected.CopyQueueStatus, actual.CopyQueueStatus);
        }

        private void VerifyRemoteReplicatorStatus(RemoteReplicatorStatus expected, RemoteReplicatorStatusResult actual)
        {
            Verify.AreEqual(expected.IsInBuild, actual.IsInBuild);
            // Verify.AreEqual(expected.LastAcknowledgementProcessedTimeUtc, actual.LastAcknowledgementProcessedTimeUtc);
            Verify.AreEqual(expected.LastAppliedCopySequenceNumber, actual.LastAppliedCopySequenceNumber);
            Verify.AreEqual(expected.LastAppliedReplicationSequenceNumber, actual.LastAppliedReplicationSequenceNumber);
            Verify.AreEqual(expected.LastReceivedCopySequenceNumber, actual.LastReceivedCopySequenceNumber);
            Verify.AreEqual(expected.LastReceivedReplicationSequenceNumber, actual.LastReceivedReplicationSequenceNumber);
            Verify.AreEqual(expected.ReplicaId, actual.ReplicaId);

            VerifyRemoteReplicatorAcknowledgementStatus(expected.RemoteReplicatorAcknowledgementStatus, (RemoteReplicatorAcknowledgementStatusResult)actual.RemoteReplicatorAcknowledgementStatus);
        }

        private void VerifyRemoteReplicatorAcknowledgementStatus(RemoteReplicatorAcknowledgementStatus expected, RemoteReplicatorAcknowledgementStatusResult actual)
        {
            VerifyRemoteReplicatorAcknowledgementDetail(expected.ReplicationStreamAcknowledgementDetail, actual.ReplicationStreamAcknowledgementDetail);
            VerifyRemoteReplicatorAcknowledgementDetail(expected.CopyStreamAcknowledgementDetail, actual.CopyStreamAcknowledgementDetail);
        }

        private void VerifyRemoteReplicatorAcknowledgementDetail(RemoteReplicatorAcknowledgementDetail expected, RemoteReplicatorAcknowledgementStatusDetail actual)
        {
            // Verify.AreEqual(expected.AverageReceiveAckDuration, actual.AverageReceiveAckDuration);
            // Verify.AreEqual(expected.AverageApplyAckDuration, actual.AverageApplyAckDuration);

            Verify.AreEqual(expected.NotReceivedCount, actual.NotReceivedCount);
            Verify.AreEqual(expected.ReceivedAndNotAppliedCount, actual.ReceivedAndNotAppliedCount);
        }

        private void VerifyReplicatorQueueStatus(ReplicatorQueueStatus expected, ReplicatorQueueStatusResult actual)
        {
            Verify.IsTrue(expected.QueueUtilizationPercentage <= 100 && expected.QueueUtilizationPercentage >= 0, "QueueUtilizationPercentage should be <= 100 && >=0");
            Verify.IsTrue(actual.QueueUtilizationPercentage <= 100 && actual.QueueUtilizationPercentage >= 0, "QueueUtilizationPercentage should be <= 100 && >=0");
            Verify.AreEqual(expected.QueueUtilizationPercentage, actual.QueueUtilizationPercentage);
            Verify.AreEqual(expected.CommittedSequenceNumber, actual.CommittedSequenceNumber);
            Verify.AreEqual(expected.CompletedSequenceNumber, actual.CompletedSequenceNumber);
            Verify.AreEqual(expected.FirstSequenceNumber, actual.FirstSequenceNumber);
            Verify.AreEqual(expected.LastSequenceNumber, actual.LastSequenceNumber);
            Verify.AreEqual(expected.QueueMemorySize, actual.QueueMemorySize);
        }

        private void VerifyLoadResult(LoadMetricReport expected, LoadMetricReportResult actual)
        {
            Verify.AreEqual(expected.Name, actual.Name);
            Verify.AreEqual(expected.Value, actual.Value);
            Verify.AreEqual(expected.LastReportedUtc, actual.LastReportedUtc);
        }

        private void VerifyLoadMetricInformation(LoadMetricInformation expected, LoadMetricInformationResult actual)
        {
            Verify.AreEqual(expected.IsBalancedBefore, actual.IsBalancedBefore);
            Verify.AreEqual(expected.IsBalancedAfter, actual.IsBalancedAfter);
            Verify.IsTrue(expected.DeviationBefore - actual.DeviationBefore < 0.1 || expected.DeviationBefore - actual.DeviationBefore > -0.1);
            Verify.IsTrue(expected.DeviationAfter - actual.DeviationAfter < 0.1 || expected.DeviationAfter - actual.DeviationAfter > -0.1);
            Verify.IsTrue(expected.BalancingThreshold - actual.BalancingThreshold < 0.1 || expected.BalancingThreshold - actual.BalancingThreshold > -0.1);
            //do not check action literaly because we can run Creation or CreationWithMove
            Verify.AreEqual(expected.ActivityThreshold, actual.ActivityThreshold);
            Verify.AreEqual(expected.ClusterCapacity, actual.ClusterCapacity);
            Verify.AreEqual(expected.ClusterLoad, actual.ClusterLoad);
            Verify.AreEqual(expected.NodeBufferPercentage, actual.NodeBufferPercentage);
            Verify.AreEqual(expected.ClusterBufferedCapacity, actual.BufferedCapacity);
            Verify.AreEqual(expected.IsClusterCapacityViolation, actual.IsClusterCapacityViolation);
            Verify.AreEqual(expected.MinNodeLoadValue, actual.MinNodeLoadValue);
            Verify.AreEqual(expected.MaxNodeLoadValue, actual.MaxNodeLoadValue);
            Verify.AreEqual(expected.ClusterRemainingBufferedCapacity, actual.RemainingBufferedCapacity);
            Verify.AreEqual(expected.CurrentClusterLoad, actual.CurrentClusterLoad);
            Verify.AreEqual(expected.ClusterCapacityRemaining, actual.ClusterCapacityRemaining);
            Verify.AreEqual(expected.BufferedClusterCapacityRemaining, actual.BufferedClusterCapacityRemaining);
            Verify.AreEqual(expected.MinimumNodeLoad, actual.MinimumNodeLoad);
            Verify.AreEqual(expected.MaximumNodeLoad, actual.MaximumNodeLoad);
        }

        private void VerifyNodeLoadMetricInformation(NodeLoadMetricInformation expected, NodeLoadMetricInformationResult actual)
        {
            Verify.AreEqual(expected.NodeCapacity, actual.NodeCapacity);
            Verify.AreEqual(expected.NodeLoad, actual.NodeLoad);
            Verify.AreEqual(expected.CurrentNodeLoad, actual.CurrentNodeLoad);
            Verify.AreEqual(expected.NodeRemainingCapacity, actual.NodeRemainingCapacity);
            Verify.AreEqual(expected.NodeCapacityRemaining, actual.NodeCapacityRemaining);
            Verify.AreEqual(expected.IsCapacityViolation, actual.IsCapacityViolation);
            Verify.AreEqual(expected.NodeBufferedCapacity, actual.NodeBufferedCapacity);
            Verify.AreEqual(expected.NodeRemainingBufferedCapacity, actual.NodeRemainingBufferedCapacity);
            Verify.AreEqual(expected.BufferedNodeCapacityRemaining, actual.BufferedNodeCapacityRemaining);
        }

        private CreateServiceInput GetServiceDescription(string appName, string serviceName, RESTClient restClient, FabricClient fabricClient = null)
        {
            var restResult = restClient.GetServiceDescription(appName, serviceName);
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.ServiceManager.GetServiceDescriptionAsync(new Uri(serviceName)).Result;
                Verify.IsTrue(restResult.ApplicationName == fabricClientResult.ApplicationName.ToString());
                Verify.IsTrue(restResult.CorrelationScheme.Count == fabricClientResult.Correlations.Count);
                Verify.IsTrue(restResult.ServiceName == fabricClientResult.ServiceName.ToString());
                Verify.IsTrue(restResult.ServiceTypeName == fabricClientResult.ServiceTypeName.ToString());
                Verify.IsTrue(restResult.PartitionDescription.PartitionScheme == fabricClientResult.PartitionSchemeDescription.Scheme);
                Verify.IsTrue(restResult.ServicePackageActivationMode == fabricClientResult.ServicePackageActivationMode);
                if (restResult.ScalingPolicies != null)
                {
                    Verify.IsTrue(restResult.ScalingPolicies.Count == fabricClientResult.ScalingPolicies.Count);

                    for (int i = 0; i < restResult.ScalingPolicies.Count; ++i)
                    {
                        Console.WriteLine("ScalingPolicy rest " + restResult.ScalingPolicies[i].ToString());
                        Console.WriteLine("ScalingPolicy fabric " + fabricClientResult.ScalingPolicies[i].ToString());
                        Verify.IsTrue(restResult.ScalingPolicies[i].ScalingMechanism.Kind == fabricClientResult.ScalingPolicies[i].ScalingMechanism.Kind);
                        Verify.IsTrue(restResult.ScalingPolicies[i].ScalingTrigger.Kind == fabricClientResult.ScalingPolicies[i].ScalingTrigger.Kind);
                    }
                }
            }
            return restResult;
        }

        private CreateServiceGroupInput GetServiceGroupDescription(string appName, string serviceGroupName, RESTClient restClient, FabricClient fabricClient = null)
        {
            var restResult = restClient.GetServiceGroupDescription(appName, serviceGroupName);

            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.ServiceGroupManager.GetServiceGroupDescriptionAsync(new Uri(serviceGroupName)).Result;

                Verify.IsTrue(restResult.ApplicationName == fabricClientResult.ServiceDescription.ApplicationName.ToString());
                Verify.IsTrue(restResult.CorrelationScheme.Count == fabricClientResult.ServiceDescription.Correlations.Count);
                Verify.IsTrue(restResult.ServiceName == fabricClientResult.ServiceDescription.ServiceName.ToString());
                Verify.IsTrue(restResult.ServiceTypeName == fabricClientResult.ServiceDescription.ServiceTypeName.ToString());
                Verify.IsTrue(restResult.PartitionDescription.PartitionScheme == fabricClientResult.ServiceDescription.PartitionSchemeDescription.Scheme);
                Verify.IsTrue(restResult.ServicePackageActivationMode == fabricClientResult.ServiceDescription.ServicePackageActivationMode);
            }

            return restResult;
        }

        private void VerifyManifestQueries(string appTypeName, string appTypeVersion, RESTClient restClient, FabricClient fabricClient = null)
        {
            Console.WriteLine("Verifying Manifest queries..");
            string manifest = GetClusterManifest(restClient, fabricClient);
            Verify.IsTrue(manifest.Length != 0);
            manifest = GetApplicationManifest(appTypeName, appTypeVersion, restClient, fabricClient);
            Verify.IsTrue(manifest.Length != 0);
        }

        private List<NodesResult> GetNodes(RESTClient restClient, FabricClient fabricClient = null)
        {
            List<NodesResult> restNodesResult = restClient.GetNodes();
            if (fabricClient != null)
            {
                var nodeResult = fabricClient.QueryManager.GetNodeListAsync().Result;
                Verify.IsTrue(nodeResult.Count == restNodesResult.Count);
                for (int i = 0; i < nodeResult.Count; i++)
                {
                    Verify.IsTrue(restNodesResult[i].IpAddressOrFQDN == nodeResult[i].IpAddressOrFQDN);
                    Verify.IsTrue(restNodesResult[i].Name == nodeResult[i].NodeName);
                    Verify.IsTrue(restNodesResult[i].Type == nodeResult[i].NodeType);
                    Verify.IsTrue(restNodesResult[i].CodeVersion == nodeResult[i].CodeVersion);
                    Verify.IsTrue(restNodesResult[i].ConfigVersion == nodeResult[i].ConfigVersion);
                    Verify.IsTrue(restNodesResult[i].NodeStatus == nodeResult[i].NodeStatus);
                    Verify.IsTrue(restNodesResult[i].UpgradeDomain == nodeResult[i].UpgradeDomain);
                    Verify.IsTrue(restNodesResult[i].FaultDomain == nodeResult[i].FaultDomain.ToString());
                    Verify.IsTrue(restNodesResult[i].HealthState == nodeResult[i].HealthState);
                    Verify.IsTrue(restNodesResult[i].Id.Id == nodeResult[i].NodeId.ToString());
                }
            }
            return restNodesResult;
        }

        private List<ApplicationTypeResult> GetApplicationTypes(RESTClient restClient, FabricClient fabricClient = null)
        {
            List<ApplicationTypeResult> restResult = restClient.GetApplicationTypes();
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.QueryManager.GetApplicationTypeListAsync().Result;
                Verify.IsTrue(restResult.Count == fabricClientResult.Count);
                for (int i = 0; i < fabricClientResult.Count; i++)
                {
                    bool found = false;
                    for (int j = 0; j < restResult.Count; j++)
                    {
                        if ((restResult[j].Name == fabricClientResult[i].ApplicationTypeName) &&
                            (restResult[j].Version == fabricClientResult[i].ApplicationTypeVersion))
                        {
                            found = true;
                            break;
                        }
                    }

                    Verify.IsTrue(found);
                }
            }
            return restResult;
        }

        private void GetServiceTypes(
            RESTClient restClient,
            List<ApplicationTypeResult> appTypeResults,
            FabricClient fabricClient = null)
        {
            foreach (var result in appTypeResults)
            {
                GetServiceTypes(
                    restClient,
                    result.Name,
                    result.Version,
                    fabricClient);
            }
        }

        private List<ServiceTypeResult> GetServiceTypes(
            RESTClient restClient,
            string appTypeName,
            string appTypeVersion,
            FabricClient fabricClient = null)
        {
            List<ServiceTypeResult> restResult = restClient.GetServiceTypes(appTypeName, appTypeVersion);
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.QueryManager.GetServiceTypeListAsync(appTypeName, appTypeVersion).Result;
                Verify.IsTrue(restResult.Count == fabricClientResult.Count);
                for (int i = 0; i < fabricClientResult.Count; i++)
                {
                    Verify.IsTrue(restResult[i].ServiceTypeDescription.ServiceTypeName == fabricClientResult[i].ServiceTypeDescription.ServiceTypeName);
                    Verify.IsTrue(restResult[i].ServiceManifestName == fabricClientResult[i].ServiceManifestName);
                    Verify.IsTrue(restResult[i].ServiceManifestVersion == fabricClientResult[i].ServiceManifestVersion);
                }
            }
            return restResult;
        }

        private List<ApplicationsResult> GetApplications(RESTClient restClient, FabricClient fabricClient = null, string apiVersion = UriSuffix.ApiVersion)
        {
            if (apiVersion != null && !apiVersion.Equals(UriSuffix.ApiVersion, StringComparison.OrdinalIgnoreCase))
            {
                return GetApplicationsWithPagedResults(restClient, fabricClient, apiVersion);
            }

            List<ApplicationsResult> restResult = restClient.GetApplications(apiVersion: apiVersion);
            if (fabricClient != null)
            {
                IList<Query.Application> fabricClientResult = (IList<Query.Application>)fabricClient.QueryManager.GetApplicationListAsync().Result;
                Verify.IsTrue(restResult.Count == fabricClientResult.Count);
                for (int i = 0; i < fabricClientResult.Count; i++)
                {
                    Verify.IsTrue(restResult[i].Name == fabricClientResult[i].ApplicationName.ToString());
                    Verify.IsTrue(restResult[i].TypeName == fabricClientResult[i].ApplicationTypeName);
                    Verify.IsTrue(restResult[i].TypeVersion == fabricClientResult[i].ApplicationTypeVersion);
                    Verify.IsTrue(restResult[i].Parameters.SequenceEqual(fabricClientResult[i].ApplicationParameters.AsDictionary()));
                    Verify.IsTrue(restResult[i].Status == fabricClientResult[i].ApplicationStatus);
                    Verify.IsTrue(restResult[i].HealthState == fabricClientResult[i].HealthState);
                }
            }

            return restResult;
        }

        private List<ApplicationsResult> GetApplicationsWithPagedResults(RESTClient restClient, FabricClient fabricClient = null, string apiVersion = UriSuffix.ApiVersion)
        {
            List<ApplicationsResult> restResult = restClient.GetApplicationsWithPagedResults(apiVersion: apiVersion);
            if (fabricClient != null)
            {
                string continuationToken = string.Empty;
                var fabricClientResult = new List<Query.Application>();
                do
                {
                    var results = fabricClient.QueryManager.GetApplicationListAsync(null, continuationToken).Result;
                    fabricClientResult.AddRange(results);
                    continuationToken = results.ContinuationToken;
                } while (!string.IsNullOrWhiteSpace(continuationToken));

                Verify.IsTrue(restResult.Count == fabricClientResult.Count);
                for (int i = 0; i < fabricClientResult.Count; i++)
                {
                    Verify.IsTrue(restResult[i].Name == fabricClientResult[i].ApplicationName.ToString());
                    Verify.IsTrue(restResult[i].TypeName == fabricClientResult[i].ApplicationTypeName);
                    Verify.IsTrue(restResult[i].TypeVersion == fabricClientResult[i].ApplicationTypeVersion);
                    Verify.IsTrue(restResult[i].Parameters.SequenceEqual(fabricClientResult[i].ApplicationParameters.AsDictionary()));
                    Verify.IsTrue(restResult[i].Status == fabricClientResult[i].ApplicationStatus);
                    Verify.IsTrue(restResult[i].HealthState == fabricClientResult[i].HealthState);
                }
            }
            return restResult;
        }

        private List<ServiceResult> GetServices(string appName, RESTClient restClient, FabricClient fabricClient = null, string apiVersion = UriSuffix.ApiVersion)
        {
            if (apiVersion != null && !apiVersion.Equals(UriSuffix.ApiVersion, StringComparison.OrdinalIgnoreCase))
            {
                return GetServicesWithPagedResults(appName, restClient, fabricClient, apiVersion);
            }

            List<ServiceResult> restResult = restClient.GetServices(appName, apiVersion: apiVersion);
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.QueryManager.GetServiceListAsync(new Uri(appName)).Result;
                Verify.IsTrue(restResult.Count == fabricClientResult.Count, "GetServices: restResult.Count == fabricClientResult.Count");
                for (int i = 0; i < fabricClientResult.Count; i++)
                {
                    Query.StatefulService statefulServiceResultItem = fabricClientResult[i] as Query.StatefulService;
                    if (statefulServiceResultItem != null)
                    {
                        Console.WriteLine("Stateful service");
                        Verify.IsTrue(restResult[i].ServiceKind == ServiceKind.Stateful);
                        Verify.IsTrue(restResult[i].Name == statefulServiceResultItem.ServiceName.ToString());
                        Verify.IsTrue(restResult[i].ManifestVersion == statefulServiceResultItem.ServiceManifestVersion);
                        Verify.IsTrue(restResult[i].HasPersistedState == statefulServiceResultItem.HasPersistedState);
                        Verify.IsTrue(restResult[i].HealthState == statefulServiceResultItem.HealthState);
                    }
                    else
                    {
                        Console.WriteLine("Stateless service");
                        Query.StatelessService statelessServiceResultItem = fabricClientResult[i] as Query.StatelessService;
                        Verify.IsTrue(restResult[i].ServiceKind == ServiceKind.Stateless);
                        Verify.IsTrue(restResult[i].Name == statelessServiceResultItem.ServiceName.ToString());
                        Verify.IsTrue(restResult[i].ManifestVersion == statelessServiceResultItem.ServiceManifestVersion);
                        Verify.IsTrue(restResult[i].HealthState == statelessServiceResultItem.HealthState);
                    }
                }
            }
            return restResult;
        }

        private List<ServiceResult> GetServicesWithPagedResults(string appName, RESTClient restClient, FabricClient fabricClient = null, string apiVersion = UriSuffix.ApiVersion)
        {
            List<ServiceResult> restResult = restClient.GetServicesWithPagedResults(appName, apiVersion: apiVersion);
            if (fabricClient != null)
            {
                string continuationToken = string.Empty;
                var fabricClientResult = new List<Query.Service>();
                do
                {
                    ServiceList results = fabricClient.QueryManager.GetServiceListAsync(new Uri(appName), null, continuationToken).Result;
                    fabricClientResult.AddRange(results);
                    continuationToken = results.ContinuationToken;
                } while (!string.IsNullOrWhiteSpace(continuationToken));

                Verify.IsTrue(restResult.Count == fabricClientResult.Count);
                for (int i = 0; i < fabricClientResult.Count; i++)
                {
                    Query.StatefulService statefulServiceResultItem = fabricClientResult[i] as Query.StatefulService;
                    if (statefulServiceResultItem != null)
                    {
                        Console.WriteLine("Stateful service");
                        Verify.IsTrue(restResult[i].ServiceKind == ServiceKind.Stateful);
                        Verify.IsTrue(restResult[i].Name == statefulServiceResultItem.ServiceName.ToString());
                        Verify.IsTrue(restResult[i].ManifestVersion == statefulServiceResultItem.ServiceManifestVersion);
                        Verify.IsTrue(restResult[i].HasPersistedState == statefulServiceResultItem.HasPersistedState);
                        Verify.IsTrue(restResult[i].HealthState == statefulServiceResultItem.HealthState);
                    }
                    else
                    {
                        Console.WriteLine("Stateless service");
                        Query.StatelessService statelessServiceResultItem = fabricClientResult[i] as Query.StatelessService;
                        Verify.IsTrue(restResult[i].ServiceKind == ServiceKind.Stateless);
                        Verify.IsTrue(restResult[i].Name == statelessServiceResultItem.ServiceName.ToString());
                        Verify.IsTrue(restResult[i].ManifestVersion == statelessServiceResultItem.ServiceManifestVersion);
                        Verify.IsTrue(restResult[i].HealthState == statelessServiceResultItem.HealthState);
                    }
                }
            }
            return restResult;
        }

        private ResolveServicePartitionResult ResolveServicePartition(string appName, string serviceName, PartitionSchemeDescription partitionScheme, RESTClient restClient, FabricClient fabricclient = null)
        {
            // Currently the partition scheme is singleton.
            Console.WriteLine("ResolveServicePartition for {0}, PartitionScheme : {1}", serviceName, partitionScheme.Scheme);
            ResolveServicePartitionResult restResult = restClient.ResolveServicePartition(appName, serviceName, "");
            if (fabricclient != null)
            {
                var fabricClientResult = fabricclient.ServiceManager.ResolveServicePartitionAsync(new Uri(serviceName)).Result;
                Verify.IsTrue(fabricClientResult.ServiceName.ToString() == restResult.Name, "ServiceName match");
                Verify.IsTrue(fabricClientResult.Info.Kind == restResult.PartitionInformation.ServicePartitionKind, "PartitionKind match");
                Verify.IsTrue(fabricClientResult.Endpoints.Count == restResult.Endpoints.Count, "EndpointCount match");
                foreach (ResolvedServiceEndpoint fcEndpoint in fabricClientResult.Endpoints)
                {
                    bool found = false;
                    for (int j = 0; j < restResult.Endpoints.Count; ++j)
                    {
                        if (restResult.Endpoints[j].Address == fcEndpoint.Address &&
                            restResult.Endpoints[j].Kind == fcEndpoint.Role)
                        {
                            found = true;
                            break;
                        }

                        if (!found)
                        {
                            Verify.IsTrue(false, String.Format("Endpoint {0} : {1} found in FabricClient result not in Rest client result", fcEndpoint.Role, fcEndpoint.Address));
                        }
                    }
                }
            }

            return restResult;
        }

        private List<ServicePartitionResult> GetPartitions(string appName, string serviceName, RESTClient restClient, FabricClient fabricClient = null)
        {
            List<ServicePartitionResult> restResult = restClient.GetPartitions(appName, serviceName);
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.QueryManager.GetPartitionListAsync(new Uri(serviceName)).Result;
                Verify.IsTrue(restResult.Count == fabricClientResult.Count, "GetPartitions: restResult.Count == fabricClientResult.Count");
                for (int i = 0; i < fabricClientResult.Count; i++)
                {
                    Query.StatefulServicePartition statefulResultItem = fabricClientResult[i] as Query.StatefulServicePartition;
                    if (statefulResultItem != null)
                    {
                        Console.WriteLine("Stateful service partition");
                        Verify.IsTrue(restResult[i].ServiceKind == ServiceKind.Stateful);
                        Verify.IsTrue(restResult[i].MinReplicaSetSize == statefulResultItem.MinReplicaSetSize);
                        Verify.IsTrue(restResult[i].TargetReplicaSetSize == statefulResultItem.TargetReplicaSetSize);
                        Verify.IsTrue(restResult[i].PartitionInformation.Id == statefulResultItem.PartitionInformation.Id);
                        Verify.IsTrue(restResult[i].HealthState == statefulResultItem.HealthState);
                        Verify.IsTrue(restResult[i].PartitionStatus == statefulResultItem.PartitionStatus);
                        Verify.IsTrue(restResult[i].LastQuorumLossDuration == statefulResultItem.LastQuorumLossDuration);
                    }
                    else
                    {
                        Console.WriteLine("Stateful service instance");
                        Query.StatelessServicePartition statelessResultItem = fabricClientResult[i] as Query.StatelessServicePartition;
                        Verify.IsTrue(restResult[i].ServiceKind == ServiceKind.Stateless);
                        Verify.IsTrue(restResult[i].PartitionInformation.Id == statelessResultItem.PartitionInformation.Id);
                        Verify.IsTrue(restResult[i].InstanceCount == statelessResultItem.InstanceCount);
                        Verify.IsTrue(restResult[i].HealthState == statelessResultItem.HealthState);
                        Verify.IsTrue(restResult[i].PartitionStatus == statelessResultItem.PartitionStatus);
                    }
                }
            }
            return restResult;
        }

        private List<ServicePartitionResult> GetPartitions(Guid partitionId, RESTClient restClient, FabricClient fabricClient = null)
        {
            ServicePartitionResult restResult = restClient.GetPartitions(partitionId);
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.QueryManager.GetPartitionAsync(partitionId).Result;
                Query.StatefulServicePartition statefulResultItem = fabricClientResult[0] as Query.StatefulServicePartition;
                if (statefulResultItem != null)
                {
                    Console.WriteLine("Stateful service partition");
                    Verify.IsTrue(restResult.ServiceKind == ServiceKind.Stateful);
                    Verify.IsTrue(restResult.MinReplicaSetSize == statefulResultItem.MinReplicaSetSize);
                    Verify.IsTrue(restResult.TargetReplicaSetSize == statefulResultItem.TargetReplicaSetSize);
                    Verify.IsTrue(restResult.PartitionInformation.Id == statefulResultItem.PartitionInformation.Id);
                    Verify.IsTrue(restResult.HealthState == statefulResultItem.HealthState);
                }
                else
                {
                    Console.WriteLine("Stateless service partition");
                    Query.StatelessServicePartition statelessResultItem = fabricClientResult[0] as Query.StatelessServicePartition;
                    Verify.IsTrue(restResult.ServiceKind == ServiceKind.Stateless);
                    Verify.IsTrue(restResult.PartitionInformation.Id == statelessResultItem.PartitionInformation.Id);
                    Verify.IsTrue(restResult.InstanceCount == statelessResultItem.InstanceCount);
                    Verify.IsTrue(restResult.HealthState == statelessResultItem.HealthState);
                    Verify.IsTrue(restResult.PartitionStatus == statelessResultItem.PartitionStatus);
                }
            }
            var result = new List<ServicePartitionResult>();
            result.Add(restResult);
            return result;
        }

        private List<ServiceReplicaResult> GetReplicas(string appName, string serviceName, string partId, RESTClient restClient, FabricClient fabricClient = null)
        {
            List<ServiceReplicaResult> restResult = restClient.GetServiceReplicas(appName, serviceName, partId);
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.QueryManager.GetReplicaListAsync(Guid.Parse(partId)).Result;
                Verify.IsTrue(restResult.Count == fabricClientResult.Count, "GetReplicas: restResult.Count == fabricClientResult.Count");
                for (int i = 0; i < restResult.Count; i++)
                {
                    Verify.IsTrue(restResult[i].Address == fabricClientResult[i].ReplicaAddress);
                    Verify.IsTrue(restResult[i].ReplicaStatus == fabricClientResult[i].ReplicaStatus);
                    Verify.IsTrue(restResult[i].HealthState == fabricClientResult[i].HealthState);
                    //Verify.IsTrue(restResult[i].LastInBuildDuration == fabricClientResult[i].LastInBuildDuration);
                }
            }
            return restResult;
        }

        private List<ServiceResult> GetSystemServices(RESTClient restClient, FabricClient fabricClient = null)
        {
            List<ServiceResult> restResult = restClient.GetSystemServices();
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.QueryManager.GetServiceListAsync(fabricClient.FabricSystemApplication).Result;
                Verify.IsTrue(restResult.Count == fabricClientResult.Count, "GetSystemServices: restResult.Count == fabricClientResult.Count");
                //
                // This is a parallel query, so the order of items could be different between 2 queries.
                //
                for (int i = 0; i < restResult.Count; i++)
                {
                    for (int j = 0; j < fabricClientResult.Count; j++)
                    {
                        Query.StatefulService statefulServiceResultItem = fabricClientResult[j] as Query.StatefulService;
                        if (statefulServiceResultItem != null)
                        {
                            if (restResult[i].Name == statefulServiceResultItem.ServiceName.ToString())
                            {
                                Verify.IsTrue(restResult[i].ServiceKind == ServiceKind.Stateful);
                                Verify.IsTrue(restResult[i].ManifestVersion == statefulServiceResultItem.ServiceManifestVersion);
                                Verify.IsTrue(restResult[i].HasPersistedState == statefulServiceResultItem.HasPersistedState);
                                Verify.IsTrue(restResult[i].HealthState == statefulServiceResultItem.HealthState, statefulServiceResultItem.HealthState + "!=" + statefulServiceResultItem.HealthState);
                                Verify.IsTrue(restResult[i].ServiceStatus == statefulServiceResultItem.ServiceStatus);
                                break;
                            }
                        }
                        else
                        {
                            Query.StatelessService statelessServiceResultItem = fabricClientResult[j] as Query.StatelessService;
                            if (restResult[i].Name == statelessServiceResultItem.ServiceName.ToString())
                            {
                                Verify.IsTrue(restResult[i].ServiceKind == ServiceKind.Stateless);
                                Verify.IsTrue(restResult[i].ManifestVersion == statelessServiceResultItem.ServiceManifestVersion);
                                Verify.IsTrue(restResult[i].HealthState == statelessServiceResultItem.HealthState, restResult[i].HealthState + "!=" + statelessServiceResultItem.HealthState);
                                Verify.IsTrue(restResult[i].ServiceStatus == statelessServiceResultItem.ServiceStatus);
                                break;
                            }
                        }
                    }
                }
            }
            return restResult;
        }

        private List<ServicePartitionResult> GetSystemServicePartitions(string serviceName, RESTClient restClient, FabricClient fabricClient = null)
        {
            List<ServicePartitionResult> restResult = restClient.GetSystemServicePartitions(serviceName);
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.QueryManager.GetPartitionListAsync(new Uri(serviceName)).Result;
                Verify.IsTrue(restResult.Count == fabricClientResult.Count, "GetSystemServicePartitions: restResult.Count == fabricClientResult.Count");
                for (int i = 0; i < fabricClientResult.Count; i++)
                {
                    Query.StatefulServicePartition statefulResultItem = fabricClientResult[i] as Query.StatefulServicePartition;
                    if (statefulResultItem != null)
                    {
                        Verify.IsTrue(restResult[i].ServiceKind == ServiceKind.Stateful);
                        Verify.IsTrue(restResult[i].MinReplicaSetSize == statefulResultItem.MinReplicaSetSize);
                        Verify.IsTrue(restResult[i].TargetReplicaSetSize == statefulResultItem.TargetReplicaSetSize);
                        Verify.IsTrue(restResult[i].PartitionInformation.Id == statefulResultItem.PartitionInformation.Id);
                        Verify.IsTrue(restResult[i].HealthState == statefulResultItem.HealthState);
                        Verify.IsTrue(restResult[i].PartitionStatus == statefulResultItem.PartitionStatus);
                        Verify.IsTrue(restResult[i].LastQuorumLossDuration == statefulResultItem.LastQuorumLossDuration);
                    }
                    else
                    {
                        Query.StatelessServicePartition statelessResultItem = fabricClientResult[i] as Query.StatelessServicePartition;
                        Verify.IsTrue(restResult[i].ServiceKind == ServiceKind.Stateless);
                        Verify.IsTrue(restResult[i].PartitionInformation.Id == statelessResultItem.PartitionInformation.Id);
                        Verify.IsTrue(restResult[i].InstanceCount == statelessResultItem.InstanceCount);
                        Verify.IsTrue(restResult[i].HealthState == statelessResultItem.HealthState);
                        Verify.IsTrue(restResult[i].PartitionStatus == statefulResultItem.PartitionStatus);
                    }
                }
            }
            return restResult;
        }

        private List<ServiceReplicaResult> GetSystemServiceReplicas(string serviceName, string partId, RESTClient restClient, FabricClient fabricClient = null)
        {
            List<ServiceReplicaResult> restResult = restClient.GetSystemServiceReplicas(serviceName, partId);
            if (fabricClient != null)
            {
                var fabricClientResult = (IList<Query.Replica>)fabricClient.QueryManager.GetReplicaListAsync(Guid.Parse(partId)).Result;
                Verify.IsTrue(restResult.Count == fabricClientResult.Count, "GetSystemServiceReplicas: restResult.Count == fabricClientResult.Count");
                for (int i = 0; i < restResult.Count; i++)
                {
                    Verify.IsTrue(restResult[i].Address == fabricClientResult[i].ReplicaAddress);
                    Verify.IsTrue(restResult[i].ReplicaStatus == fabricClientResult[i].ReplicaStatus);
                    Verify.IsTrue(restResult[i].LastInBuildDuration == fabricClientResult[i].LastInBuildDuration);
                }
            }
            return restResult;
        }

        private List<ApplicationsOnNodeResult> GetApplicationsOnNode(string nodeName, RESTClient restClient, FabricClient fabricClient = null)
        {
            List<ApplicationsOnNodeResult> restResult = restClient.GetApplicationsOnNode(nodeName);
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.QueryManager.GetDeployedApplicationListAsync(nodeName).Result;
                Verify.IsTrue(restResult.Count == fabricClientResult.Count, "GetApplicationsOnNode: restResult.Count == fabricClientResult.Count");
                for (int i = 0; i < restResult.Count; i++)
                {
                    Verify.IsTrue(restResult[i].Name == fabricClientResult[i].ApplicationName.ToString());
                    Verify.IsTrue(restResult[i].TypeName == fabricClientResult[i].ApplicationTypeName);
                }
            }

            return restResult;
        }

        private List<ServiceManifestOnNodeResult> GetServiceManifestsOnNode(string nodeName, string appName, RESTClient restClient, FabricClient fabricClient = null)
        {
            List<ServiceManifestOnNodeResult> restResult = restClient.GetServiceManifestsOnNode(nodeName, appName);
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.QueryManager.GetDeployedServicePackageListAsync(nodeName, new Uri(appName)).Result;
                Verify.IsTrue(restResult.Count == fabricClientResult.Count, "GetServiceManifestsOnNode: restResult.Count == fabricClientResult.Count");
                for (int i = 0; i < restResult.Count; i++)
                {
                    Verify.IsTrue(restResult[i].Name == fabricClientResult[i].ServiceManifestName);
                    Verify.IsTrue(restResult[i].Version == fabricClientResult[i].ServiceManifestVersion);
                    Verify.IsTrue(restResult[i].ServicePackageActivationId == fabricClientResult[i].ServicePackageActivationId);
                }
            }
            return restResult;
        }

        private List<CodePackageOnNodeResult> GetCodePackagesOnNode(string nodeName, string appName, string manifestName, RESTClient restClient, FabricClient fabricClient = null)
        {
            List<CodePackageOnNodeResult> restResult = restClient.GetCodePackagesOnNode(nodeName, appName, manifestName);
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.QueryManager.GetDeployedCodePackageListAsync(nodeName, new Uri(appName), manifestName, null).Result;
                Verify.IsTrue(restResult.Count == fabricClientResult.Count, "GetCodePackagesOnNode: restResult.Count == fabricClientResult.Count");
                for (int i = 0; i < restResult.Count; i++)
                {
                    Verify.IsTrue(restResult[i].Name == fabricClientResult[i].CodePackageName);
                    Verify.IsTrue(restResult[i].Version == fabricClientResult[i].CodePackageVersion);
                    Verify.IsTrue(restResult[i].ServicePackageActivationId == fabricClientResult[i].ServicePackageActivationId);
                }
            }
            return restResult;
        }

        private List<ServiceReplicasOnNodeResult> GetServiceReplicasOnNode(string nodeName, string appName, string manifestName, RESTClient restClient, FabricClient fabricClient = null)
        {
            List<ServiceReplicasOnNodeResult> restResult = restClient.GetServiceReplicasOnNode(nodeName, appName, manifestName);
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.QueryManager.GetDeployedReplicaListAsync(nodeName, new Uri(appName), manifestName, null).Result;
                Verify.IsTrue(restResult.Count == fabricClientResult.Count, "GetServiceReplicasOnNode: restResult.Count == fabricClientResult.Count");
                for (int i = 0; i < restResult.Count; i++)
                {
                    Verify.IsTrue(restResult[i].Address == fabricClientResult[i].Address);
                    Verify.IsTrue(restResult[i].CodePackageName == fabricClientResult[i].CodePackageName);
                    Verify.IsTrue(restResult[i].PartitionId == fabricClientResult[i].Partitionid);
                    Verify.IsTrue(restResult[i].ReplicaStatus == fabricClientResult[i].ReplicaStatus);
                    Verify.IsTrue(restResult[i].ServiceName == fabricClientResult[i].ServiceName.ToString());
                    Verify.IsTrue(restResult[i].ServiceTypeName == fabricClientResult[i].ServiceTypeName);

                    Verify.IsTrue(restResult[i].ServiceManifestVersion == fabricClientResult[i].ServiceManifestVersion_);
                    Verify.IsTrue(restResult[i].ServiceManifestName == fabricClientResult[i].ServiceManifestName);
                    Verify.IsTrue(restResult[i].ServicePackageActivationId == fabricClientResult[i].ServicePackageActivationId);

                    Query.DeployedStatefulServiceReplica statefulResultItem = fabricClientResult[i] as Query.DeployedStatefulServiceReplica;
                    if (statefulResultItem != null)
                    {
                        Verify.IsTrue(restResult[i].ReplicaId == statefulResultItem.ReplicaId.ToString());
                        Verify.IsTrue(restResult[i].ReplicaRole == statefulResultItem.ReplicaRole);
                    }
                    else
                    {
                        Query.DeployedStatelessServiceInstance statelessResultItem = fabricClientResult[i] as Query.DeployedStatelessServiceInstance;
                        Verify.IsTrue(restResult[i].InstanceId == statelessResultItem.InstanceId.ToString());
                    }
                }
            }
            return restResult;
        }

        private string GetClusterManifest(RESTClient restClient, FabricClient fabricClient = null)
        {
            string restResult = restClient.GetClusterManifest();
            if (fabricClient != null)
            {
                string fabricClientResult = (string)fabricClient.ClusterManager.GetClusterManifestAsync().Result;
                Verify.IsTrue(restResult == fabricClientResult);
            }
            return restResult;
        }

        private string GetApplicationManifest(string appTypeName, string appTypeVersion, RESTClient restClient, FabricClient fabricClient = null)
        {
            string restResult = restClient.GetApplicationManifest(appTypeName, appTypeVersion);
            if (fabricClient != null)
            {
                string fabricClientResult = fabricClient.ApplicationManager.GetApplicationManifestAsync(appTypeName, appTypeVersion).Result;
                Verify.IsTrue(restResult == fabricClientResult);
            }
            return restResult;
        }

        private NodeHealth GetNodeHealth(string nodeName, RESTClient restClient, FabricClient fabricClient, HealthInformation healthInfoToVerify, HealthEventsFilter eventsFilter)
        {
            var restHealthPolicy = new ClusterHealthPolicy();
            restHealthPolicy.ConsiderWarningAsError = true;
            restHealthPolicy.MaxPercentUnhealthyApplications = 99;
            restHealthPolicy.MaxPercentUnhealthyNodes = 99;

            var restResult = restClient.EvaluateNodeHealth(nodeName, HttpStatusCode.OK, restHealthPolicy, eventsFilter);
            if (fabricClient != null)
            {
                ClusterHealthPolicy fabricCientHealthPolicy = restHealthPolicy;
                var queryDescription = new NodeHealthQueryDescription(nodeName)
                {
                    EventsFilter = eventsFilter,
                    HealthPolicy = restHealthPolicy,
                };

                var fabricClientResult = fabricClient.HealthManager.GetNodeHealthAsync(queryDescription).Result;
                Verify.IsTrue(restResult.NodeName == fabricClientResult.NodeName, "NodeName");
                VerifyEntityHealthAreEqual(restResult, fabricClientResult);
            }

            return restResult;
        }

        private ApplicationHealth GetApplicationHealth(string appName, RESTClient restClient, FabricClient fabricClient, HealthInformation healthInfoToVerify, HealthEventsFilter eventsFilter, ServiceHealthStatesFilter servicesFilter, DeployedApplicationHealthStatesFilter deployedApplicationsFilter)
        {
            var restResult = restClient.EvaluateApplicationHealth(appName, HttpStatusCode.OK, eventsFilter, servicesFilter, deployedApplicationsFilter);
            if (fabricClient != null)
            {
                var queryDescription = new ApplicationHealthQueryDescription(new Uri(appName))
                {
                    EventsFilter = eventsFilter,
                    DeployedApplicationsFilter = deployedApplicationsFilter,
                    ServicesFilter = servicesFilter,
                };

                var fabricClientResult = fabricClient.HealthManager.GetApplicationHealthAsync(queryDescription).Result;
                Verify.IsTrue(restResult.ApplicationName.ToString() == fabricClientResult.ApplicationName.ToString(), "AppName");
                Verify.IsTrue(restResult.DeployedApplicationHealthStates.Count == fabricClientResult.DeployedApplicationHealthStates.Count, "DeployedApplicationHealthStates.Count");
                Verify.IsTrue(restResult.ServiceHealthStates.Count == fabricClientResult.ServiceHealthStates.Count, "ServiceHealthStates.Count");
                VerifyEntityHealthAreEqual(restResult, fabricClientResult);
            }
            return restResult;
        }

        private ServiceHealth GetServiceHealth(string appName, string serviceName, RESTClient restClient, FabricClient fabricClient, HealthInformation healthInfoToVerify, HealthEventsFilter eventsFilter, PartitionHealthStatesFilter partitionsFilter)
        {
            var restResult = restClient.EvaluateServicehealth(appName, serviceName, HttpStatusCode.OK, eventsFilter, partitionsFilter);
            if (fabricClient != null)
            {
                var queryDescription = new ServiceHealthQueryDescription(new Uri(serviceName))
                {
                    EventsFilter = eventsFilter,
                    PartitionsFilter = partitionsFilter,
                };

                var fabricClientResult = fabricClient.HealthManager.GetServiceHealthAsync(queryDescription).Result;
                Verify.IsTrue(restResult.ServiceName.ToString() == fabricClientResult.ServiceName.ToString());
                Verify.IsTrue(restResult.PartitionHealthStates.Count == fabricClientResult.PartitionHealthStates.Count);
                VerifyEntityHealthAreEqual(restResult, fabricClientResult);
            }

            return restResult;
        }

        private PartitionHealth GetPartitionHealth(string appName, string serviceName, Guid partId, RESTClient restClient, FabricClient fabricClient, HealthInformation healthInfoToVerify, HealthEventsFilter eventsFilter, ReplicaHealthStatesFilter replicasFilter)
        {
            var restResult = restClient.EvaluatePartitionHealth(appName, serviceName, partId, HttpStatusCode.OK, eventsFilter, replicasFilter);
            if (fabricClient != null)
            {
                var queryDescription = new PartitionHealthQueryDescription(partId)
                {
                    EventsFilter = eventsFilter,
                    ReplicasFilter = replicasFilter,
                };

                var fabricClientResult = fabricClient.HealthManager.GetPartitionHealthAsync(queryDescription).Result;
                Verify.IsTrue(restResult.PartitionId == fabricClientResult.PartitionId);
                Verify.IsTrue(restResult.ReplicaHealthStates.Count == fabricClientResult.ReplicaHealthStates.Count);
                VerifyEntityHealthAreEqual(restResult, fabricClientResult);
            }
            return restResult;
        }

        private ReplicaHealth GetReplicaHealth(string appName, string serviceName, Guid partId, string replicaId, RESTClient restClient, FabricClient fabricClient, HealthInformation healthInfoToVerify, HealthEventsFilter eventsFilter)
        {
            var restResult = restClient.EvaluateReplicaHealth(appName, serviceName, partId, replicaId, HttpStatusCode.OK, eventsFilter);

            if (fabricClient != null)
            {
                var queryDescription = new ReplicaHealthQueryDescription(partId, Convert.ToInt64(replicaId))
                {
                    EventsFilter = eventsFilter,
                };

                var fabricClientResult = fabricClient.HealthManager.GetReplicaHealthAsync(queryDescription).Result;
                Verify.IsTrue(restResult.Kind == fabricClientResult.Kind);
                Verify.IsTrue(restResult.Id == fabricClientResult.Id); // Replica or instance id
                Verify.IsTrue(restResult.PartitionId == fabricClientResult.PartitionId);

                VerifyEntityHealthAreEqual(restResult, fabricClientResult);
            }

            return restResult;
        }

        private DeployedApplicationHealth GetDeployedApplicationHealth(string nodeName, string appName, RESTClient restClient, FabricClient fabricClient, HealthEventsFilter eventsFilter, DeployedServicePackageHealthStatesFilter deployedServicePackagesFilter)
        {
            var restResult = restClient.EvaluateDeployedApplicationHealth(nodeName, appName, HttpStatusCode.OK, eventsFilter, deployedServicePackagesFilter);
            if (fabricClient != null)
            {
                var queryDescription = new DeployedApplicationHealthQueryDescription(new Uri(appName), nodeName)
                {
                    EventsFilter = eventsFilter,
                    DeployedServicePackagesFilter = deployedServicePackagesFilter,
                };

                var fabricClientResult = fabricClient.HealthManager.GetDeployedApplicationHealthAsync(queryDescription).Result;
                Verify.IsTrue(restResult.ApplicationName.ToString() == fabricClientResult.ApplicationName.ToString());
                Verify.IsTrue(restResult.NodeName == fabricClientResult.NodeName);
                Verify.IsTrue(restResult.DeployedServicePackageHealthStates.Count == fabricClientResult.DeployedServicePackageHealthStates.Count);

                VerifyEntityHealthAreEqual(restResult, fabricClientResult);
            }
            return restResult;
        }

        private DeployedServicePackageHealth GetDeployedServicePackageHealth(
            string nodeName,
            string appName,
            string serviceManifestName,
            string servicePackageActivationId,
            RESTClient restClient,
            FabricClient fabricClient,
            HealthEventsFilter eventsFilter)
        {
            var restResult = restClient.EvaluateDeployedServicePackageHealth(nodeName, appName, serviceManifestName, servicePackageActivationId, HttpStatusCode.OK, eventsFilter);
            if (fabricClient != null)
            {
                var queryDescription = new DeployedServicePackageHealthQueryDescription(new Uri(appName), nodeName, serviceManifestName, servicePackageActivationId)
                {
                    EventsFilter = eventsFilter,
                };

                var fabricClientResult = fabricClient.HealthManager.GetDeployedServicePackageHealthAsync(queryDescription).Result;
                Verify.IsTrue(restResult.ApplicationName.ToString() == fabricClientResult.ApplicationName.ToString());
                Verify.IsTrue(restResult.NodeName == fabricClientResult.NodeName);
                Verify.IsTrue(restResult.ServiceManifestName == fabricClientResult.ServiceManifestName);
                Verify.IsTrue(restResult.ServicePackageActivationId == fabricClientResult.ServicePackageActivationId);

                VerifyEntityHealthAreEqual(restResult, fabricClientResult);
            }
            return restResult;
        }

        private ClusterHealth GetClusterHealth(RESTClient restClient, FabricClient fabricClient, HealthInformation healthInfoToVerify, string apiVersion, ClusterHealthPolicy clusterHealthPolicy, ApplicationHealthPolicyMap appHealthPolicies, HealthEventsFilter eventsFilter, NodeHealthStatesFilter nodesFilter, ApplicationHealthStatesFilter applicationsFilter)
        {
            var restResult = restClient.EvaluateClusterHealth(HttpStatusCode.OK, apiVersion, clusterHealthPolicy, appHealthPolicies, eventsFilter, nodesFilter, applicationsFilter);

            if (fabricClient != null)
            {
                var queryDescription = new ClusterHealthQueryDescription()
                {
                    EventsFilter = eventsFilter,
                    NodesFilter = nodesFilter,
                    ApplicationsFilter = applicationsFilter,
                };

                queryDescription.HealthPolicy = clusterHealthPolicy;

                if (appHealthPolicies != null)
                {
                    foreach (var entry in appHealthPolicies)
                    {
                        queryDescription.ApplicationHealthPolicyMap.Add(entry.Key, entry.Value);
                    }
                }

                var fabricClientResult = fabricClient.HealthManager.GetClusterHealthAsync(queryDescription).Result;
                Verify.IsTrue(restResult.ApplicationHealthStates.Count == fabricClientResult.ApplicationHealthStates.Count, "ApplicationHealthStates.Count");
                Verify.IsTrue(restResult.NodeHealthStates.Count == fabricClientResult.NodeHealthStates.Count, "NodeHealthStates.Count");

                VerifyEntityHealthAreEqual(restResult, fabricClientResult);
            }
            return restResult;
        }

        private PartitionLoadInformationResult GetPartitionLoadInformation(string appName, string serviceName, Guid partId, RESTClient restClient, FabricClient fabricClient = null)
        {
            var restResult = restClient.GetPartitionLoadInformation(appName, serviceName, partId);
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.QueryManager.GetPartitionLoadInformationAsync(partId).Result;
                Verify.IsTrue(restResult.PartitionId == fabricClientResult.PartitionId);
                Verify.IsTrue(restResult.PrimaryLoadMetricReports.Count == fabricClientResult.PrimaryLoadMetricReports.Count);
                Verify.IsTrue(restResult.SecondaryLoadMetricReports.Count == fabricClientResult.SecondaryLoadMetricReports.Count);
            }
            return restResult;
        }

        private ClusterLoadInformationResult GetClusterLoadInformation(RESTClient restClient, FabricClient fabricClient = null)
        {
            var restResult = restClient.GetClusterLoadInformation();
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.QueryManager.GetClusterLoadInformationAsync().Result;
                Verify.IsTrue(restResult.LoadMetricInformation.Count == fabricClientResult.LoadMetricInformationList.Count);

                for (int j = 0; j < fabricClientResult.LoadMetricInformationList.Count; ++j)
                {
                    for (int i = 0; i < restResult.LoadMetricInformation.Count; i++)
                    {
                        //metrics could be out of order in different queries
                        if (fabricClientResult.LoadMetricInformationList[j].Name == restResult.LoadMetricInformation[i].Name)
                        {
                            VerifyLoadMetricInformation(fabricClientResult.LoadMetricInformationList[j], restResult.LoadMetricInformation[i]);
                        }
                    }
                }
            }
            return restResult;
        }

        private NodeLoadInformationResult GetNodeLoadInformation(string nodeName, RESTClient restClient, FabricClient fabricClient = null)
        {
            var restResult = restClient.GetNodeLoadInformation(nodeName);
            if (fabricClient != null)
            {
                var fabricClientResult = fabricClient.QueryManager.GetNodeLoadInformationAsync(nodeName).Result;
                Verify.IsTrue(restResult.NodeLoadMetricInformation.Count == fabricClientResult.NodeLoadMetricInformationList.Count);

                for (int j = 0; j < fabricClientResult.NodeLoadMetricInformationList.Count; ++j)
                {
                    for (int i = 0; i < restResult.NodeLoadMetricInformation.Count; i++)
                    {
                        if (fabricClientResult.NodeLoadMetricInformationList[j].Name == restResult.NodeLoadMetricInformation[i].Name)
                        {
                            VerifyNodeLoadMetricInformation(fabricClientResult.NodeLoadMetricInformationList[j], restResult.NodeLoadMetricInformation[i]);
                        }
                    }
                }
            }
            return restResult;
        }

        private bool CheckIfHealthEventExists(IList<HealthEvent> healthEvents, string sourceId, string property)
        {
            foreach (var healthEvent in healthEvents)
            {
                if ((healthEvent.HealthInformation.SourceId == sourceId) && (healthEvent.HealthInformation.Property == property))
                {
                    return true;
                }
            }

            return false;
        }

        private void VerifyEntityHealthAreEqual(EntityHealth a, EntityHealth b)
        {
            Verify.AreEqual(a.AggregatedHealthState, b.AggregatedHealthState);

            if (a.HealthEvents != b.HealthEvents) // if references are not equal then compare content
            {
                Verify.IsTrue(a.HealthEvents.Count == b.HealthEvents.Count);

                for (int i = 0; i < a.HealthEvents.Count; i++)
                {
                    Verify.IsTrue(this.AreHealthEventsEqual(a.HealthEvents[i], b.HealthEvents[i]), "HealthEvent comparision");
                }
            }

            VerifyHealthEvaluationReasons(a.UnhealthyEvaluations, b.UnhealthyEvaluations);
        }

        private bool AreHealthEventsEqual(HealthEvent a, HealthEvent b)
        {
            if (a == b)
            {
                // References are equal/ both are null
                return true;
            }

            if (a == null || b == null)
            {
                // One is null and one is non-null
                return false;
            }

            bool result =
                a.IsExpired == b.IsExpired
                && this.CompareDateTime(a.SourceUtcTimestamp, b.SourceUtcTimestamp)
                && this.CompareDateTime(a.LastModifiedUtcTimestamp, b.LastModifiedUtcTimestamp)
                && this.CompareDateTime(a.LastOkTransitionAt, b.LastOkTransitionAt)
                && this.CompareDateTime(a.LastWarningTransitionAt, b.LastWarningTransitionAt)
                && this.CompareDateTime(a.LastErrorTransitionAt, b.LastErrorTransitionAt);

            result &= HealthInformation.AreEqual(a.HealthInformation, b.HealthInformation);

            return result;
        }

        private bool CompareDateTime(DateTime a, DateTime b)
        {
            return (a.Date == b.Date) && // Date
                   (a.Day == b.Day) &&  // Time
                   (a.Hour == b.Hour) &&
                   (a.Minute == b.Minute) &&
                   (a.Second == b.Second) &&
                   (a.Millisecond == b.Millisecond);
        }

        private void VerifyHealthEvaluationReasonAreEqual(HealthEvaluation restEvaluation, HealthEvaluation fabricEvaluation)
        {
            if (restEvaluation == fabricEvaluation) // If references are equal or null return
            {
                return;
            }

            if (restEvaluation == null || fabricEvaluation == null)
            {
                Assert.Fail("One of the two evaluation reasons is null");
            }

            Verify.AreEqual(fabricEvaluation.Kind, restEvaluation.Kind);
            Verify.AreEqual(fabricEvaluation.AggregatedHealthState, restEvaluation.AggregatedHealthState);
            Verify.AreEqual(fabricEvaluation.Description, restEvaluation.Description);
            Verify.IsFalse(string.IsNullOrEmpty(restEvaluation.Description));

            switch (restEvaluation.Kind)
            {
                case HealthEvaluationKind.Event:
                    {
                        var fcReason = fabricEvaluation as EventHealthEvaluation;
                        var rcReason = restEvaluation as EventHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.IsTrue(fcReason.UnhealthyEvent != null && rcReason.UnhealthyEvent != null);
                        Verify.IsTrue(fcReason.ConsiderWarningAsError == rcReason.ConsiderWarningAsError);
                        Verify.AreEqual(fcReason.AggregatedHealthState, rcReason.AggregatedHealthState);
                        Verify.IsTrue(this.AreHealthEventsEqual(rcReason.UnhealthyEvent, fcReason.UnhealthyEvent));
                    }
                    break;
                case HealthEvaluationKind.Replicas:
                    {
                        var fcReason = fabricEvaluation as ReplicasHealthEvaluation;
                        var rcReason = restEvaluation as ReplicasHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.IsTrue(fcReason.TotalCount == rcReason.TotalCount);
                        Verify.IsTrue(fcReason.MaxPercentUnhealthyReplicasPerPartition == rcReason.MaxPercentUnhealthyReplicasPerPartition);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.Partitions:
                    {
                        var fcReason = fabricEvaluation as PartitionsHealthEvaluation;
                        var rcReason = restEvaluation as PartitionsHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.IsTrue(fcReason.TotalCount == rcReason.TotalCount);
                        Verify.IsTrue(fcReason.MaxPercentUnhealthyPartitionsPerService == rcReason.MaxPercentUnhealthyPartitionsPerService);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);

                    }
                    break;
                case HealthEvaluationKind.Services:
                    {
                        var fcReason = fabricEvaluation as ServicesHealthEvaluation;
                        var rcReason = restEvaluation as ServicesHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.IsTrue(fcReason.TotalCount == rcReason.TotalCount);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.DeployedServicePackages:
                    {
                        var fcReason = fabricEvaluation as DeployedServicePackagesHealthEvaluation;
                        var rcReason = restEvaluation as DeployedServicePackagesHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.IsTrue(fcReason.TotalCount == rcReason.TotalCount);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.DeployedApplications:
                    {
                        var fcReason = fabricEvaluation as DeployedApplicationsHealthEvaluation;
                        var rcReason = restEvaluation as DeployedApplicationsHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.IsTrue(fcReason.TotalCount == rcReason.TotalCount);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.Applications:
                    {
                        var fcReason = fabricEvaluation as ApplicationsHealthEvaluation;
                        var rcReason = restEvaluation as ApplicationsHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.IsTrue(fcReason.TotalCount == rcReason.TotalCount);
                        Verify.IsTrue(fcReason.MaxPercentUnhealthyApplications == rcReason.MaxPercentUnhealthyApplications);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.ApplicationTypeApplications:
                    {
                        var fcReason = fabricEvaluation as ApplicationTypeApplicationsHealthEvaluation;
                        var rcReason = restEvaluation as ApplicationTypeApplicationsHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.IsTrue(fcReason.ApplicationTypeName == rcReason.ApplicationTypeName);
                        Verify.IsTrue(fcReason.TotalCount == rcReason.TotalCount);
                        Verify.IsTrue(fcReason.MaxPercentUnhealthyApplications == rcReason.MaxPercentUnhealthyApplications);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.Nodes:
                    {
                        var fcReason = fabricEvaluation as NodesHealthEvaluation;
                        var rcReason = restEvaluation as NodesHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.IsTrue(fcReason.TotalCount == rcReason.TotalCount);
                        Verify.IsTrue(fcReason.MaxPercentUnhealthyNodes == rcReason.MaxPercentUnhealthyNodes);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.SystemApplication:
                    {
                        var fcReason = fabricEvaluation as SystemApplicationHealthEvaluation;
                        var rcReason = restEvaluation as SystemApplicationHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.UpgradeDomainNodes:
                    {
                        var fcReason = fabricEvaluation as UpgradeDomainNodesHealthEvaluation;
                        var rcReason = restEvaluation as UpgradeDomainNodesHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.IsTrue(fcReason.TotalCount == rcReason.TotalCount);
                        Verify.IsTrue(fcReason.UpgradeDomainName == rcReason.UpgradeDomainName);
                        Verify.IsTrue(fcReason.MaxPercentUnhealthyNodes == rcReason.MaxPercentUnhealthyNodes);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.UpgradeDomainDeployedApplications:
                    {
                        var fcReason = fabricEvaluation as UpgradeDomainDeployedApplicationsHealthEvaluation;
                        var rcReason = restEvaluation as UpgradeDomainDeployedApplicationsHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.IsTrue(fcReason.TotalCount == rcReason.TotalCount);
                        Verify.IsTrue(fcReason.UpgradeDomainName == rcReason.UpgradeDomainName);
                        Verify.IsTrue(fcReason.MaxPercentUnhealthyDeployedApplications == rcReason.MaxPercentUnhealthyDeployedApplications);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.Replica:
                    {
                        var fcReason = fabricEvaluation as ReplicaHealthEvaluation;
                        var rcReason = restEvaluation as ReplicaHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.IsTrue(fcReason.PartitionId == rcReason.PartitionId);
                        Verify.IsTrue(fcReason.ReplicaOrInstanceId == rcReason.ReplicaOrInstanceId);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.Partition:
                    {
                        var fcReason = fabricEvaluation as PartitionHealthEvaluation;
                        var rcReason = restEvaluation as PartitionHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.IsTrue(fcReason.PartitionId == rcReason.PartitionId);
                        Verify.IsTrue(fcReason.PartitionId == rcReason.PartitionId);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.Service:
                    {
                        var fcReason = fabricEvaluation as ServiceHealthEvaluation;
                        var rcReason = restEvaluation as ServiceHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.IsTrue(fcReason.ServiceName.ToString() == rcReason.ServiceName.ToString());
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.DeployedServicePackage:
                    {
                        var fcReason = fabricEvaluation as DeployedServicePackageHealthEvaluation;
                        var rcReason = restEvaluation as DeployedServicePackageHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.AreEqual(fcReason.ApplicationName.ToString(), rcReason.ApplicationName.ToString());
                        Verify.AreEqual(fcReason.NodeName, rcReason.NodeName);
                        Verify.AreEqual(fcReason.ServiceManifestName, rcReason.ServiceManifestName);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.DeployedApplication:
                    {
                        var fcReason = fabricEvaluation as DeployedApplicationHealthEvaluation;
                        var rcReason = restEvaluation as DeployedApplicationHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.AreEqual(fcReason.ApplicationName.ToString(), rcReason.ApplicationName.ToString());
                        Verify.AreEqual(fcReason.NodeName, rcReason.NodeName);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.Application:
                    {
                        var fcReason = fabricEvaluation as ApplicationHealthEvaluation;
                        var rcReason = restEvaluation as ApplicationHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.AreEqual(fcReason.ApplicationName.ToString(), rcReason.ApplicationName.ToString());
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                case HealthEvaluationKind.Node:
                    {
                        var fcReason = fabricEvaluation as NodeHealthEvaluation;
                        var rcReason = restEvaluation as NodeHealthEvaluation;
                        Verify.IsTrue(fcReason != null && rcReason != null);
                        Verify.AreEqual(fcReason.NodeName, rcReason.NodeName);
                        VerifyHealthEvaluationReasons(rcReason.UnhealthyEvaluations, fcReason.UnhealthyEvaluations);
                    }
                    break;
                default:
                    Assert.Fail("unknown evaluation reason kind");
                    break;
            }
        }

        private void VerifyHealthEvaluationReasons(IList<HealthEvaluation> restHealthEvaluationReasons, IList<HealthEvaluation> fcHealthEvaluationReasons)
        {
            if (restHealthEvaluationReasons == fcHealthEvaluationReasons) // If references are equal or null return
            {
                return;
            }

            Verify.IsTrue(restHealthEvaluationReasons.Count == fcHealthEvaluationReasons.Count);

            for (int i = 0; i < restHealthEvaluationReasons.Count; i++)
            {
                VerifyHealthEvaluationReasonAreEqual(restHealthEvaluationReasons[i], fcHealthEvaluationReasons[i]);
            }
        }

        private void GetFabricMsiPath(RESTClient client, out string msiFileName, out string msiFilePath, out string codeVersion)
        {
            // Get the fabric node version and generate a dummy msi.
            Console.WriteLine("GetFabricMsiPath");
            var nodesResult = client.GetNodes();
            Verify.IsTrue(nodesResult.Count > 0);

            codeVersion = nodesResult[0].CodeVersion;
            msiFileName = "WindowsFabric." + codeVersion + ".msi";
            msiFilePath = Path.Combine(Constants.DropDirectory, msiFileName);
            var fs = File.Create(msiFilePath);
            fs.Dispose();
        }

        private void GetClusterManifestForUpgrade(string version, out string clusterManifestPath, out string clusterManifestName)
        {
            // Get the currently deployed cluster manifest
            Console.WriteLine("GetClusterManifestForUpgrade");
            string[] clusterManifestFiles = Directory.GetFiles(Directory.GetCurrentDirectory(), "ClusterManifest.current.xml", SearchOption.AllDirectories);
            Verify.IsTrue(clusterManifestFiles.Length > 0);

            var newCM = ReadClusterManifest(clusterManifestFiles[0]);
            newCM.Version = version;
            clusterManifestName = Path.GetRandomFileName();
            clusterManifestPath = Path.Combine(Constants.DropDirectory, clusterManifestName);
            WriteClusterManifest(clusterManifestPath, newCM);
        }

        private Query.StatefulServiceReplica[] GetAllFmReplicas(FabricClient client)
        {
            return client.QueryManager.GetReplicaListAsync(Constants.FmPartitionId).Result.Select(e => e as StatefulServiceReplica).ToArray();
        }

        private static void SetupClusterNoSecurity()
        {
            clusterCredentialType = CredentialType.None;
            currentClusterManifest = Constants.ScaleMinNoSecurityClusterManifestName;
            string manifestName = Environment.ExpandEnvironmentVariables(currentClusterManifest);

            Console.WriteLine("Deploying cluster with no security");
            DeployCluster(manifestName);
            fabricHostProcess = StartProcess(Path.Combine(Constants.DropDirectory, Constants.BinDirectory, Constants.FabricHostFileName), Constants.FabricHostArgument);

            fabricClients = CreateFabricClients();
            Thread.Sleep(TimeSpan.FromMinutes(1));
            VerifyClusterIsUp(fabricClients.Item2, TimeSpan.FromMinutes(2));
        }

        private static void SetupClusterForCertSecurity()
        {
            clusterCredentialType = CredentialType.X509;
            currentClusterManifest = Constants.ScaleMinCertSecurityClusterManifestName;
            string manifestName = Environment.ExpandEnvironmentVariables(currentClusterManifest);

            Console.WriteLine("Deploying cluster with cert security");
            DeployCluster(manifestName);
            fabricHostProcess = StartProcess(Path.Combine(Constants.DropDirectory, Constants.BinDirectory, Constants.FabricHostFileName), Constants.FabricHostArgument);

            fabricClients = CreateFabricClients();
            Thread.Sleep(TimeSpan.FromMinutes(1));
            VerifyClusterIsUp(fabricClients.Item2, TimeSpan.FromMinutes(2));
        }

        private static void SetupClusterForKerberosSecurity()
        {
            clusterCredentialType = CredentialType.Windows;
            currentClusterManifest = Constants.ScaleMinKerberosSecurityClusterManifestName;

            string manifestName = Environment.ExpandEnvironmentVariables(currentClusterManifest);

            Console.WriteLine("Deploying cluster with Windows security");
            DeployCluster(manifestName, true);
            fabricHostProcess = StartProcess(Path.Combine(Constants.DropDirectory, Constants.BinDirectory, Constants.FabricHostFileName), Constants.FabricHostArgument, true);

            fabricClients = CreateFabricClients();
            Thread.Sleep(TimeSpan.FromMinutes(1));
            VerifyClusterIsUp(fabricClients.Item2, TimeSpan.FromMinutes(2));
        }

        private static void VerifyClusterIsUp(FabricClient fabricClient, TimeSpan timeout)
        {
            Console.WriteLine("Verifying cluster is up: timeout {0}", timeout);
            int maxRetryCount = 10;

            for (int i = 0; i < maxRetryCount; i++)
            {
                try
                {
                    fabricClient.PropertyManager
                        .NameExistsAsync(new Uri(@"fabric:/WinFabric"), timeout, CancellationToken.None)
                        .GetAwaiter().GetResult();
                    break;
                }
                catch (FabricTransientException e) when (i < maxRetryCount)
                {
                    Console.WriteLine("Retryable exception: {0} thrown during NameExistsAsync call. RetryCount: {1}", e, i);
                    Thread.Sleep(TimeSpan.FromSeconds(5));
                }
            }
        }

        private static void CleanupCounters()
        {
            StopFabricCounters();
            DeleteFabricCounters();
        }

        private static void CleanupDirectories()
        {
            if (Directory.Exists(Constants.DropDirectory))
            {
                HttpGatewayServiceTest.DeleteDrop();
            }
            if (Directory.Exists(HttpGatewayServiceTest.TestDeploymentOutputPath))
            {
                Directory.Delete(HttpGatewayServiceTest.TestDeploymentOutputPath, true);
            }
        }

        private static void StopFabricCounters()
        {
            Process p = StartProcess(Constants.LogmanFileName, Constants.LogmanStopFabricCounters);
            p.WaitForExit();
            Verify.IsTrue((p.ExitCode == Constants.LogmanSuccessExitCode) ||
                          (p.ExitCode == Constants.LogmanDataCollectorNotRunningExitCode) ||
                          (p.ExitCode == Constants.LogmanDataCollectorNotFoundExitCode));
        }

        private static void DeleteFabricCounters()
        {
            Process p = StartProcess(Constants.LogmanFileName, Constants.LogmanDeleteFabricCounters);
            p.WaitForExit();
            Verify.IsTrue((p.ExitCode == Constants.LogmanSuccessExitCode) ||
                          (p.ExitCode == Constants.LogmanDataCollectorNotFoundExitCode));
        }

        private static void DeleteDrop()
        {
            foreach (string fileName in Directory.EnumerateFiles(Constants.DropDirectory, "*", SearchOption.AllDirectories))
            {
                try
                {
                    FileAttributes attribute = File.GetAttributes(fileName);
                    attribute = attribute & ~FileAttributes.ReadOnly;
                    File.SetAttributes(fileName, attribute);
                }
                catch (System.IO.FileNotFoundException)
                { }
            }

            string[] files = Directory.GetFiles(Constants.DropDirectory);
            foreach (string filename in files)
            {
                File.Delete(filename);
            }
            Directory.Delete(Constants.DropDirectory, true);
        }

        private static void DeployCluster(string manifestName, bool runAsSystem = false)
        {
            string path = Environment.GetEnvironmentVariable("path");
            string binPath = Path.Combine(Directory.GetCurrentDirectory(), Constants.DropDirectory, Constants.BinDirectory);
            string dataPath = Path.Combine(Directory.GetCurrentDirectory(), Constants.DropDirectory, Constants.DataDirectory);
            string logRoot = Path.Combine(Directory.GetCurrentDirectory(), Constants.DropDirectory, Constants.DataDirectory, "log");
            string fabricCodePackage = null;
            string fabricAppDirectory = Path.Combine(Directory.GetCurrentDirectory(), Constants.DropDirectory, Constants.FabricAppDirectory);
            foreach (string dir in Directory.EnumerateDirectories(fabricAppDirectory, Constants.FabricCodePattern))
            {
                fabricCodePackage = dir;
            }
            path = string.Format(CultureInfo.InvariantCulture, "{0};{1}", path, fabricCodePackage);
            Environment.SetEnvironmentVariable("path", path);
            Environment.SetEnvironmentVariable("FabricConfigFileName", "");
            Environment.SetEnvironmentVariable("FabricPackageFileName", "");
            Environment.SetEnvironmentVariable(Constants.FabricBinRoot, binPath);
            Environment.SetEnvironmentVariable(Constants.FabricDataRoot, dataPath);
            Environment.SetEnvironmentVariable(Constants.FabricLogRoot, logRoot);
            Environment.SetEnvironmentVariable(Constants.FabricCodePath, fabricCodePackage);

            string fabricDeployerFileName = Path.Combine(Constants.DropDirectory, Constants.FabricAppDirectory, Path.GetFileName(fabricCodePackage), Constants.FabricDeployerFileName);
            string fabricDeployerArguments = string.Format(
                Constants.FabricDeployerArgumentPattern,
                binPath,
                dataPath,
                manifestName);
            ExecuteProcess(fabricDeployerFileName, fabricDeployerArguments, runAsSystem);
        }

        private static void MakeDrop()
        {
            string fabricDropPath = Environment.ExpandEnvironmentVariables(Constants.FabricDropPath);
            DirectoryCopy(fabricDropPath, Constants.DropDirectory, true);

            OverwriteInfrastructureServiceTestManifests();
        }

        /// <summary>
        /// <see cref="Constants.ISAppManifestFileName"/>
        /// </summary>
        private static void OverwriteInfrastructureServiceTestManifests()
        {
            string sourceDir = Environment.ExpandEnvironmentVariables(Constants.FabricUnitTestsSystemAppsPath);

            string sourceAppManifest = Path.Combine(sourceDir, Constants.ISAppManifestFileName);
            string sourcePackageManifest = Path.Combine(sourceDir, Constants.ISPackageManifestFileName);

            string destDir = Path.Combine(Constants.DropDirectory, Constants.DestinationPathForISManifests);
            string destAppManifest = Path.Combine(destDir, Constants.ISAppManifestFileName);
            string destPackageManifest = Path.Combine(destDir, Constants.ISPackageManifestFileName);

            File.Copy(sourceAppManifest, destAppManifest, true);
            File.Copy(sourcePackageManifest, destPackageManifest, true);

            Console.WriteLine("Successfully copied test InfrastructureService manifests from {0} to {1}", sourceDir, destDir);
        }

        private static void DirectoryCopy(string sourceDirName, string destDirName, bool copySubDirs)
        {
            DirectoryInfo dir = new DirectoryInfo(sourceDirName);
            DirectoryInfo[] dirs = dir.GetDirectories();

            if (!dir.Exists)
            {
                throw new DirectoryNotFoundException(string.Format(
                    CultureInfo.InvariantCulture,
                    "Source directory {0} does not exist or could not be found",
                    sourceDirName));
            }

            if (!Directory.Exists(destDirName))
            {
                Directory.CreateDirectory(destDirName);
            }

            FileInfo[] files = dir.GetFiles();
            foreach (FileInfo file in files)
            {
                string temppath = Path.Combine(destDirName, file.Name);
                file.CopyTo(temppath, true);
            }

            if (copySubDirs)
            {
                foreach (DirectoryInfo subdir in dirs)
                {
                    string temppath = Path.Combine(destDirName, subdir.Name);
                    DirectoryCopy(subdir.FullName, temppath, copySubDirs);
                }
            }
        }

        //
        // Creates a System.Fabric FabricClient and a REST fabric client. If the cluster is setup with security,
        // then both these clients are configured with Admin security credentials.
        //
        private static Tuple<RESTClient, FabricClient> CreateFabricClients(bool tryClaimsAuth = false)
        {
            string manifestName = Environment.ExpandEnvironmentVariables(currentClusterManifest);
            string[] httpGatewayAddresses = GetHttpGatewayAddresses(manifestName);
            string[] clientConnectionAddresses = GetClientConnectionAddresses(manifestName);
            FabricClient fabricClient = null;
            string claimsToken = null;

            if (clusterCredentialType == CredentialType.X509)
            {
                X509Credentials creds = new X509Credentials();
                creds.RemoteCommonNames.Add(Constants.ServerAllowedCommonName);
                creds.FindType = System.Security.Cryptography.X509Certificates.X509FindType.FindByThumbprint;
                creds.FindValue = Constants.AdminCertificateThumbprint;
                creds.ProtectionLevel = ProtectionLevel.EncryptAndSign;
                creds.StoreLocation = Constants.clientCertificateLocation;
                creds.StoreName = Constants.clientCertificateStoreName.ToString();
                fabricClient = new FabricClient(creds, clientConnectionAddresses);
                if (tryClaimsAuth && IsClaimsAuthEnabled(manifestName))
                {
                    claimsToken = GetClaimsToken(manifestName, creds.RemoteCommonNames);
                }

            }
            else if (clusterCredentialType == CredentialType.Windows)
            {
                WindowsCredentials creds = new WindowsCredentials();
                creds.ProtectionLevel = ProtectionLevel.EncryptAndSign;
                fabricClient = new FabricClient(creds, clientConnectionAddresses);
            }
            else
            {
                fabricClient = new FabricClient(clientConnectionAddresses);
            }

            RESTClient restClient = new RESTClient(httpGatewayAddresses, null, claimsToken, GetHSTSHeaderValue(manifestName));
            return new Tuple<RESTClient, FabricClient>(restClient, fabricClient);
        }

        private static Tuple<RESTClient, FabricClient> GetFabricClients()
        {
            return fabricClients;
        }

        private static void CleanupFabricClients()
        {
            //
            // Dispose the System.Fabric fabricclient so that resources associated
            // with it are cleaned up correctly.
            //
            fabricClients.Item2.Dispose();
        }

        private static string GetClaimsToken(string manifestName, IList<string> serverAllowedCommonNames)
        {
            var dstsclient = dSTSClient.CreatedSTSClient(GetMetadataEndpoint(manifestName), serverAllowedCommonNames.ToArray(), null);
            string token = null;
            for (int i = 0; i < 5; i++)
            {
                try
                {
                    token = dstsclient.GetSecurityToken();
                    break;
                }
                catch (FabricTransientException)
                {
                    Thread.Sleep(1000 * 30);
                    continue;
                }
            }

            if (token == null)
            {
                Console.WriteLine("GetSecurityToken failed...");
                Assert.Fail("GetSecurityToken failed..");
            }
            else
            {
                Console.WriteLine("GetSecurityToken successful..");
            }
            return token;
        }

        private void AddApplication(ApplicationDeployer appDeployer, RESTClient restClient, FabricClient fabricClient, string deployPath)
        {
            //
            // Provision application
            //
            int index = deployPath.LastIndexOf("\\");
            string name = deployPath.Substring(index + 1);
            Console.WriteLine("Provisioning - {0}", name);

            UploadApplication(deployPath, name);
            restClient.ProvisionApplicationType(Path.Combine(Constants.IncomingDirectory, name));

            //
            // Verify
            //
            List<ApplicationTypeResult> restAppTypeResult = GetApplicationTypes(restClient, fabricClient);
            Verify.IsTrue(restAppTypeResult.Count() == 1);

            //
            // Verify
            //
            GetServiceTypes(restClient, restAppTypeResult, fabricClient);

            string applicationName = "fabric:/" + Constants.TestApplicationName;

            //
            // Create application
            //
            Console.WriteLine("Creating app - " + applicationName);
            restClient.CreateApplication(applicationName, Constants.TestApplicationTypeName, ref appDeployer); // Constants.TestApplicationName is the app Type.

            //
            // Verify
            //
            List<ApplicationsResult> appResult = GetApplications(restClient, fabricClient);
            Verify.IsTrue(appResult.Count() == 1);
        }

        private void RemoveApplication(ApplicationDeployer appDeployer, RESTClient restClient, FabricClient fabricClient, string deployPath)
        {
            string applicationName = "fabric:/" + Constants.TestApplicationName;

            //
            // Delete application
            //
            restClient.DeleteApplication(applicationName);

            //
            // Verify
            //
            List<ApplicationsResult> appResult = GetApplications(restClient, fabricClient);
            Verify.IsTrue(appResult.Count() == 0);

            //
            // Unprovision application.
            //
            Console.WriteLine("Unprovisioning app");
            restClient.UnprovisionApplicationType(Constants.TestApplicationTypeName, ref appDeployer);

            //
            // Verify unprovision
            //
            List<ApplicationTypeResult> restAppTypeResult = GetApplicationTypes(restClient, fabricClient);
            Verify.IsTrue(restAppTypeResult.Count() == 0);

            Directory.Delete(deployPath, true);
        }

        private ApplicationDeployer GenerateApplicationWithSingleStatelessService(string appTypeName, string version, int instanceCount, string appName)
        {
            ApplicationDeployer deployer = new ApplicationDeployer()
            {
                Name = appTypeName,
                Version = version
            };

            ServiceDeployer service = StatelessSanityTest.GetService(instanceCount);
            service.ConfigPackageName = SanityTest.ConfigurationPackageName;
            deployer.Services.Add(service);
            deployer.Services[0].ConfigurationSettings.Add(Tuple.Create(
                SanityTest.ConfigurationSectionName,
                SanityTest.ConfigurationParameterName,
                HttpGatewayServiceTest.TestDeploymentOutputPath));

            deployer.Deploy(Path.Combine(HttpGatewayServiceTest.TestDeploymentOutputPath, appName));
            return deployer;
        }

        private void UploadApplication(string appPath, string appName)
        {
            string destPath = Path.Combine(Constants.ImageStoreDirectory, Path.Combine(Constants.IncomingDirectory, appName));

            DirectoryCopy(appPath, destPath, true);
        }

        private static Process StartProcess(string fileName, string arguments = null, bool runAsSystem = false)
        {
            ProcessStartInfo processStartInfo = new ProcessStartInfo()
            {
                FileName = fileName,
                WindowStyle = ProcessWindowStyle.Hidden,
                Arguments = arguments,
            };

            if (runAsSystem)
            {
                // try to start this process under system account
            }

            return Process.Start(processStartInfo);
        }

        private static void ExecuteProcess(string fileName, string arguments = null, bool runAsSystem = false)
        {
            Process p = StartProcess(fileName, arguments, runAsSystem);
            p.WaitForExit();
            Verify.AreEqual<int>(p.ExitCode, 0);
        }

        private static string[] GetHttpGatewayAddresses(string manifestName)
        {
            List<string> addresses = new List<string>();
            XmlReaderSettings settings = new XmlReaderSettings()
            {
                XmlResolver = null,
                DtdProcessing = DtdProcessing.Prohibit
            };

            using (XmlReader reader = XmlReader.Create(manifestName, settings))
            {
                XmlSerializer serlializer = new XmlSerializer(typeof(ClusterManifestType));
                ClusterManifestType cm = (ClusterManifestType)serlializer.Deserialize(reader);
                FabricNodeType[] nodes = ((ClusterManifestTypeInfrastructureWindowsServer)cm.Infrastructure.Item).NodeList;

                Array.Sort(nodes, delegate (FabricNodeType nodeType1, FabricNodeType nodeType2)
                {
                    return nodeType1.NodeName.CompareTo(nodeType2.NodeName);
                });

                foreach (FabricNodeType node in nodes)
                {
                    var nodesType = cm.NodeTypes.FirstOrDefault(nodeType => node.NodeTypeRef.Equals(nodeType.Name, StringComparison.OrdinalIgnoreCase));
                    string httpGatewayAddress = string.Format(
                        "{0}://{1}:{2}/",
                        nodesType.Endpoints.HttpGatewayEndpoint.Protocol,
                        node.IPAddressOrFQDN,
                        nodesType.Endpoints.HttpGatewayEndpoint.Port);
                    addresses.Add(httpGatewayAddress);
                }
            }
            return addresses.ToArray();
        }

        private static string GetMetadataEndpoint(string manifestName)
        {
            XmlReaderSettings settings = new XmlReaderSettings()
            {
                XmlResolver = null,
                DtdProcessing = DtdProcessing.Prohibit
            };

            using (XmlReader reader = XmlReader.Create(manifestName, settings))
            {
                XmlSerializer serlializer = new XmlSerializer(typeof(ClusterManifestType));
                ClusterManifestType cm = (ClusterManifestType)serlializer.Deserialize(reader);
                FabricNodeType[] nodes = ((ClusterManifestTypeInfrastructureWindowsServer)cm.Infrastructure.Item).NodeList;

                var nodesType = cm.NodeTypes.FirstOrDefault(nodeType => nodes[0].NodeTypeRef.Equals(nodeType.Name, StringComparison.OrdinalIgnoreCase));
                string metadataEndpoint = string.Format(
                    "{0}:{1}",
                    nodes[0].IPAddressOrFQDN,
                    nodesType.Endpoints.HttpGatewayEndpoint.Port);

                return metadataEndpoint;
            }
        }

        private static string[] GetClientConnectionAddresses(string manifestName)
        {
            XmlReaderSettings settings = new XmlReaderSettings()
            {
                XmlResolver = null,
                DtdProcessing = DtdProcessing.Prohibit
            };

            List<string> clientConnectionAddresses = new List<string>();
            using (XmlReader reader = XmlReader.Create(manifestName, settings))
            {
                XmlSerializer serlializer = new XmlSerializer(typeof(ClusterManifestType));
                ClusterManifestType cm = (ClusterManifestType)serlializer.Deserialize(reader);
                FabricNodeType[] nodes = ((ClusterManifestTypeInfrastructureWindowsServer)cm.Infrastructure.Item).NodeList;
                foreach (FabricNodeType node in nodes)
                {
                    var nodesType = cm.NodeTypes.FirstOrDefault(nodeType => node.NodeTypeRef.Equals(nodeType.Name, StringComparison.OrdinalIgnoreCase));
                    string clientConnectionAddress = string.Format(
                        "{0}:{1}",
                        node.IPAddressOrFQDN,
                        nodesType.Endpoints.ClientConnectionEndpoint.Port);
                    clientConnectionAddresses.Add(clientConnectionAddress);
                }
            }
            return clientConnectionAddresses.ToArray();
        }

        private static bool IsClaimsAuthEnabled(string manifestName)
        {
            XmlReaderSettings settings = new XmlReaderSettings()
            {
                XmlResolver = null,
                DtdProcessing = DtdProcessing.Prohibit
            };

            using (XmlReader reader = XmlReader.Create(manifestName, settings))
            {
                XmlSerializer serlializer = new XmlSerializer(typeof(ClusterManifestType));
                ClusterManifestType cm = (ClusterManifestType)serlializer.Deserialize(reader);
                foreach (var section in cm.FabricSettings)
                {
                    if (section.Name == Constants.SecuritySectionName && section.Parameter != null)
                    {
                        foreach (var param in section.Parameter)
                        {
                            if (param.Name == Constants.ClaimsAuthEnabledParamName)
                            {
                                return Convert.ToBoolean(param.Value);
                            }
                        }
                        return false;
                    }
                }
            }

            return false;
        }

        private static string GetHSTSHeaderValue(string manifestName)
        {
            XmlReaderSettings settings = new XmlReaderSettings()
            {
                XmlResolver = null,
                DtdProcessing = DtdProcessing.Prohibit
            };

            using (XmlReader reader = XmlReader.Create(manifestName, settings))
            {
                XmlSerializer serlializer = new XmlSerializer(typeof(ClusterManifestType));
                ClusterManifestType cm = (ClusterManifestType)serlializer.Deserialize(reader);
                foreach (var section in cm.FabricSettings)
                {
                    if (section.Name == Constants.HttpGatewaySectionName && section.Parameter != null)
                    {
                        foreach (var param in section.Parameter)
                        {
                            if (param.Name == Constants.HSTSHeaderParamName)
                            {
                                return param.Value;
                            }
                        }
                        return String.Empty;
                    }
                }
            }

            return String.Empty;
        }

        public ClusterManifestType ReadClusterManifest(string path)
        {
            using (FileStream fileStream = new FileStream(path, FileMode.Open, FileAccess.Read))
            {

                XmlReaderSettings readerSettings = new XmlReaderSettings()
                {
                    DtdProcessing = DtdProcessing.Prohibit,
                    XmlResolver = null
                };

                readerSettings.ValidationType = ValidationType.Schema;
                readerSettings.Schemas.Add(Constants.FabricNamespace, Path.Combine(Constants.DropDirectory, Constants.FabricAppDirectory, Constants.FabricCodePattern, "ServiceFabricServiceModel.xsd"));

                XmlSerializer xmlSerializer = new XmlSerializer(typeof(ClusterManifestType));
                XmlReader reader = XmlReader.Create(fileStream, readerSettings);

                return (ClusterManifestType)xmlSerializer.Deserialize(reader);
            }
        }

        public static void WriteClusterManifest(string path, ClusterManifestType value)
        {
            using (FileStream fileStream = new FileStream(path, FileMode.Create))
            {
                XmlSerializer xmlSerializer = new XmlSerializer(typeof(ClusterManifestType));
                xmlSerializer.Serialize(fileStream, value);
            }
        }

        private void WaitForNodeStatus(FabricClient fabricClient, string nodeName, NodeStatus expectedStatus, TimeSpan timeout)
        {
            bool nodeHasExpectedStatus = false;
            Stopwatch stopWatch = Stopwatch.StartNew();

            do
            {
                Console.WriteLine("WaitForNodeState: {0} of {1} elapsed", stopWatch.Elapsed, timeout);
                NodeList nodes;

                try
                {
                    nodes = fabricClient.QueryManager.GetNodeListAsync(null, TimeSpan.FromSeconds(10.0d), CancellationToken.None).Result;
                }
                catch (AggregateException e)
                {
                    Console.WriteLine("Caught '{0}' retrying", e);
                    continue;
                }

                Node node = nodes.Where(n => n.NodeName == nodeName).FirstOrDefault();
                if (node != null)
                {
                    Console.WriteLine("WaitForNodeState: Node {0} has NodeStatus {1}", node.NodeName, node.NodeStatus);
                    if (node.NodeStatus == expectedStatus)
                    {
                        nodeHasExpectedStatus = true;
                        break;
                    }
                }

                Thread.Sleep(TimeSpan.FromSeconds(1.0d));
            }
            while (stopWatch.Elapsed <= timeout);

            Console.WriteLine("WaitForNodeState: {0} of {1} elapsed", stopWatch.Elapsed, timeout);

            Verify.IsTrue(nodeHasExpectedStatus == true);
        }

        /// <summary>
        /// Gets all tasks from Repair Manager, verifies if there is only 1 task and that it is in the expected state
        /// and returns the task. Keeps looping till this is done or fails the test on timeout.
        /// </summary>
        private static RepairTaskData WaitForRepairTaskStateChange(
            RESTClient restClient,
            RepairTaskState expectedNewState,
            TimeSpan timeout)
        {
            var stopWatch = Stopwatch.StartNew();

            do
            {
                var tasks = restClient.GetRepairTaskList(null, null, null);
                Verify.AreEqual(1, tasks.Count);

                var task = tasks[0];

                Console.WriteLine("WaitForRepairTaskStateChange: Repair task '{0}' has State: {1}", task.TaskId, task.State);

                if (task.State == expectedNewState)
                {
                    return task;
                }

                Thread.Sleep(TimeSpan.FromSeconds(3));

            } while (stopWatch.Elapsed <= timeout);

            Verify.Fail(
                string.Format(
                    CultureInfo.InvariantCulture,
                    "Timed out waiting for repair task to move to new state: {0}", expectedNewState));

            return null;
        }

        private static RepairTaskData VerifyRepairTaskHealthPolicy(
            RESTClient restClient,
            bool expectedPerformPreparingHealthCheck,
            bool expectedPerformRestoringHealthCheck)
        {
            var tasks = restClient.GetRepairTaskList(null, null, null);
            Verify.AreEqual(1, tasks.Count);
            var task = tasks[0];
            Verify.AreEqual(task.PerformPreparingHealthCheck, expectedPerformPreparingHealthCheck);
            Verify.AreEqual(task.PerformRestoringHealthCheck, expectedPerformRestoringHealthCheck);

            return task;
        }

        private void TestNames()
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;

            Uri name = new Uri("fabric:/names/test");

            // CreateName
            restClient.CreateName(name);
            CheckNameExists(name);

            restClient.CreateName(name, HttpStatusCode.Conflict);
            CheckNameExists(name);

            // DeleteName
            restClient.DeleteName(name);
            CheckNameExists(name, false);

            restClient.DeleteName(name, HttpStatusCode.NotFound);
            CheckNameExists(name, false);

            restClient.CreateName(name);

            string nameBase = name.ToString();
            for (int i = 0; i < 10; ++i)
            {
                restClient.CreateName(new Uri(nameBase + i.ToString()));
            }

            // GetSubNameInfoList
            CheckSubNameInfoList(name);
            CheckSubNameInfoList(new Uri("fabric:/names"), true);
        }

        private void TestPropertyPut()
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;

            // GetPropertyInfo on non-existent name will return not found
            restClient.GetPropertyInfo(new Uri("fabric:/DOESNOTEXIST"), "DOESNOTEXIST", HttpStatusCode.NotFound);

            Uri name = new Uri("fabric:/names/test/properties");
            restClient.CreateName(name);
            CheckNameExists(name);

            // GetPropertyInfo on non-existent property will return not found
            restClient.GetPropertyInfo(name, "DOESNOTEXIST", HttpStatusCode.NotFound);

            // PutProperty for data that serializes as string
            PropertyDescription int64Property = new PropertyDescription
            {
                PropertyName = "int64_test_property",
                Value = new PropertyValue
                {
                    Kind = "Int64",
                    Int64Data = 1234
                }
            };
            restClient.PutProperty(name, int64Property);
            // check that value has properly changed
            CheckPropertyValue(name, int64Property.PropertyName, int64Property.Value);
            // check that change is properly reflected in fabricClient
            CheckProperty(name, int64Property.PropertyName, int64Property.Value.Kind);

            PropertyDescription stringProperty = new PropertyDescription
            {
                PropertyName = "string_test_property",
                Value = new PropertyValue
                {
                    Kind = "String",
                    StringData = "testvalue"
                }
            };
            restClient.PutProperty(name, stringProperty);
            CheckPropertyValue(name, stringProperty.PropertyName, stringProperty.Value);
            CheckProperty(name, stringProperty.PropertyName, stringProperty.Value.Kind);

            PropertyDescription guidProperty = new PropertyDescription
            {
                PropertyName = "guid_test_property",
                Value = new PropertyValue
                {
                    Kind = "Guid",
                    GuidData = Guid.NewGuid()
                }
            };
            restClient.PutProperty(name, guidProperty);
            CheckPropertyValue(name, guidProperty.PropertyName, guidProperty.Value);
            CheckProperty(name, guidProperty.PropertyName, guidProperty.Value.Kind);

            // GetPropertyInfoList for data that serializes as string
            CheckPropertyInfoList(name, true);

            // Put for Binary
            Uri binaryName = new Uri(name.ToString() + "/binary");
            restClient.CreateName(binaryName);

            BinaryPropertyDescription binaryProperty = new BinaryPropertyDescription
            {
                PropertyName = "binary_test_property",
                Value = new BinaryPropertyValue
                {
                    Data = new byte[] { 1, 2, 3, 4, 5 }
                }
            };
            restClient.PutProperty(binaryName, binaryProperty);
            CheckPropertyValue(binaryName, binaryProperty.PropertyName, binaryProperty.Value);
            CheckProperty(binaryName, binaryProperty.PropertyName, binaryProperty.Value.Kind);

            // GetPropertyInfoList for Binary
            string binaryPropertyName = binaryProperty.PropertyName;
            for (int i = 0; i < 15; ++i)
            {
                binaryProperty.PropertyName = binaryPropertyName + i.ToString();
                binaryProperty.Value.Data = new byte[] { (byte)(i % 255) };
                restClient.PutProperty(binaryName, binaryProperty);
                CheckPropertyValue(binaryName, binaryProperty.PropertyName, binaryProperty.Value);
            }
            CheckBinaryPropertyInfoList(binaryName, true);

            // Put for Double
            Uri doubleName = new Uri(name.ToString() + "/double");
            restClient.CreateName(doubleName);

            DoublePropertyDescription doubleProperty = new DoublePropertyDescription
            {
                PropertyName = "double_test_property",
                Value = new DoublePropertyValue
                {
                    Data = 12.34
                }
            };
            restClient.PutProperty(doubleName, doubleProperty);
            CheckPropertyValue(doubleName, doubleProperty.PropertyName, doubleProperty.Value);
            CheckProperty(doubleName, doubleProperty.PropertyName, doubleProperty.Value.Kind);

            // GetPropertyInfoList for Double
            string doublePropertyName = doubleProperty.PropertyName;
            for (int i = 0; i < 15; ++i)
            {
                doubleProperty.PropertyName = doublePropertyName + i.ToString();
                doubleProperty.Value.Data = (double)i;
                restClient.PutProperty(doubleName, doubleProperty);
                CheckPropertyValue(doubleName, doubleProperty.PropertyName, doubleProperty.Value);
            }
            CheckDoublePropertyInfoList(doubleName, true);
        }

        private void TestPropertyDelete()
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;

            Uri name = new Uri("fabric:/names/test/properties/delete");
            restClient.CreateName(name);

            // deleting a non-existant property should return not found
            restClient.DeleteProperty(name, "DOESNOTEXIST", HttpStatusCode.NotFound);

            string propertyName = "delete_test_property";
            for (int i = 0; i < 10; ++i)
            {
                PropertyDescription deleteProperty = new PropertyDescription
                {
                    PropertyName = propertyName + i.ToString(),
                    Value = new PropertyValue
                    {
                        Kind = "String",
                        StringData = "testvalue" + i.ToString()
                    }
                };
                restClient.PutProperty(name, deleteProperty);
                restClient.GetPropertyInfo(name, deleteProperty.PropertyName);
                restClient.DeleteProperty(name, deleteProperty.PropertyName);
                restClient.GetPropertyInfo(name, deleteProperty.PropertyName, HttpStatusCode.NotFound);
            }
            CheckPropertyInfoList(name);
        }

        private void TestPropertyBatch()
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;

            Uri name = new Uri("fabric:/names/test/properties/batch");
            restClient.CreateName(name);

            PropertyDescription property = new PropertyDescription
            {
                PropertyName = "batch_test_property",
                Value = new PropertyValue
                {
                    Kind = "Int64",
                    Int64Data = 9876
                }
            };
            restClient.PutProperty(name, property);
            PropertyInfo propertyInfo = restClient.GetPropertyInfo(name, property.PropertyName);

            List<PropertyBatchOperation> operations = new List<PropertyBatchOperation>();
            operations.Add(new PropertyBatchOperation
            {
                Kind = "CheckExists",
                PropertyName = "DOESNOTEXIST",
                Exists = false
            });
            operations.Add(new PropertyBatchOperation
            {
                Kind = "CheckExists",
                PropertyName = property.PropertyName,
                Exists = true
            });
            operations.Add(new PropertyBatchOperation
            {
                Kind = "CheckSequence",
                PropertyName = property.PropertyName,
                SequenceNumber = propertyInfo.Metadata.SequenceNumber
            });
            operations.Add(new PropertyBatchOperation
            {
                Kind = "CheckValue",
                PropertyName = property.PropertyName,
                Value = property.Value
            });
            operations.Add(new PropertyBatchOperation
            {
                Kind = "Put",
                PropertyName = property.PropertyName,
                CustomTypeId = "testtype",
                Value = new PropertyValue
                {
                    Kind = "Int64",
                    Int64Data = 5678
                }
            });
            operations.Add(new PropertyBatchOperation
            {
                Kind = "Get",
                PropertyName = property.PropertyName,
                IncludeValue = true
            });
            PropertyBatchDescription batch = new PropertyBatchDescription { Operations = operations };
            PropertyBatchInfo batchResult = restClient.SubmitPropertyBatch(name, batch);

            Verify.AreEqual(batchResult.Kind, "Successful");
            Verify.AreEqual(batchResult.Properties.Count, 1);
            Verify.IsTrue(batchResult.ErrorMessage == null);
            // this check relies on the "Get" being last
            string key = (operations.Count - 1).ToString();
            Verify.IsTrue(batchResult.Properties.ContainsKey(key));
            CheckPropertyValue(name, property.PropertyName, batchResult.Properties[key].Value);
            CheckProperty(name, property.PropertyName, property.Value.Kind);

            // test remaining operation types
            operations = new List<PropertyBatchOperation>();
            operations.Add(new PropertyBatchOperation
            {
                Kind = "Delete",
                PropertyName = property.PropertyName
            });
            batch = new PropertyBatchDescription { Operations = operations };
            batchResult = restClient.SubmitPropertyBatch(name, batch);
            restClient.GetPropertyInfo(name, property.PropertyName, HttpStatusCode.NotFound);

            Verify.AreEqual(batchResult.Kind, "Successful");
            Verify.AreEqual(batchResult.Properties.Count, 0);

            // test failed operation (property was already deleted)
            // add operation to the front so failed index isn't default value
            batch.Operations.Insert(0, new PropertyBatchOperation
            {
                Kind = "CheckExists",
                PropertyName = "DOESNOTEXIST",
                Exists = false
            });
            batchResult = restClient.SubmitPropertyBatch(name, batch, HttpStatusCode.Conflict);

            Verify.AreEqual(batchResult.Kind, "Failed");
            Verify.IsTrue(batchResult.Properties == null);
            Verify.IsTrue(batchResult.OperationIndex == 1);
            Verify.AreEqual(batchResult.ErrorMessage, "FABRIC_E_PROPERTY_DOES_NOT_EXIST");

        }

        private void CheckNameExists(Uri name, bool shouldExist = true)
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            Console.WriteLine(string.Format("Checking if name \"{0}\" exists...", name.ToString()));
            if (shouldExist)
            {
                restClient.GetNameExists(name);
            }
            else
            {
                restClient.GetNameExists(name, HttpStatusCode.NotFound);
            }
            Verify.AreEqual(shouldExist, fabricClient.PropertyManager.NameExistsAsync(name).Result);
        }

        private void CheckProperty(Uri name, string propertyName, string propertyType)
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            Console.WriteLine(string.Format("Checking property \"{0}\" under name \"{1}\"...", propertyName, name.ToString()));

            NamedProperty fabricProperty = null;
            fabricProperty = fabricClient.PropertyManager.GetPropertyAsync(name, propertyName).Result;
            Verify.IsTrue(fabricProperty != null);

            if (propertyType == "Binary")
            {
                BinaryPropertyInfo restProperty = restClient.GetBinaryPropertyInfo(name, propertyName);
                VerifyProperty(restProperty, fabricProperty);
            }
            else if (propertyType == "Double")
            {
                DoublePropertyInfo restProperty = restClient.GetDoublePropertyInfo(name, propertyName);
                VerifyProperty(restProperty, fabricProperty);
            }
            else
            {
                PropertyInfo restProperty = restClient.GetPropertyInfo(name, propertyName);
                VerifyProperty(restProperty, fabricProperty);
            }
        }

        private void CheckPropertyValue(Uri name, string propertyName, PropertyValue expected)
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            Console.WriteLine(string.Format("Checking property value for \"{0}\" under name \"{1}\"...", propertyName, name.ToString()));

            PropertyInfo actual = restClient.GetPropertyInfo(name, propertyName);

            Verify.AreEqual(actual.Value.Kind, expected.Kind);

            switch (actual.Value.Kind)
            {
                case "Int64":
                    Verify.AreEqual(actual.Value.Int64Data, expected.Int64Data);
                    break;
                case "String":
                    Verify.AreEqual(actual.Value.StringData, expected.StringData);
                    break;
                case "Guid":
                    Verify.AreEqual(actual.Value.GuidData, expected.GuidData);
                    break;
                default:
                    Verify.IsTrue(false, string.Format("Property value \"{0}\" under name \"{1}\" has invalid data type.", actual.Value.Kind, name.ToString()));
                    break;
            }
        }

        private void CheckPropertyValue(Uri name, string propertyName, BinaryPropertyValue expected)
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            Console.WriteLine(string.Format("Checking property value for \"{0}\" under name \"{1}\"...", propertyName, name.ToString()));

            BinaryPropertyInfo actual = restClient.GetBinaryPropertyInfo(name, propertyName);

            Verify.AreEqual(actual.Value.Kind, expected.Kind);
            Verify.IsTrue(actual.Value.Data.SequenceEqual(expected.Data));
        }

        private void CheckPropertyValue(Uri name, string propertyName, DoublePropertyValue expected)
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            Console.WriteLine(string.Format("Checking property value for \"{0}\" under name \"{1}\"...", propertyName, name.ToString()));

            DoublePropertyInfo actual = restClient.GetDoublePropertyInfo(name, propertyName);

            Verify.AreEqual(actual.Value.Kind, expected.Kind);
            Verify.AreEqual(actual.Value.Data, expected.Data);
        }

        private void CheckSubNameInfoList(Uri name, bool recursive = false)
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            Console.WriteLine(string.Format("Checking subnames under \"{0}\"...", name.ToString()));

            HashSet<Uri> fabricSubNames = new HashSet<Uri>();
            NameEnumerationResult fabricResult = null;
            do
            {
                fabricResult = fabricClient.PropertyManager.EnumerateSubNamesAsync(name, fabricResult, recursive).Result;
                fabricSubNames.UnionWith(fabricResult);
            }
            while (fabricResult.HasMoreData);

            PagedSubNameInfoList restResult = null;
            string continuationToken = "";
            do
            {
                restResult = restClient.GetSubNameInfoList(name, continuationToken, recursive);
                continuationToken = restResult.ContinuationToken;
                foreach (Uri subName in restResult.SubNames)
                {
                    Verify.IsTrue(fabricSubNames.Contains(subName));
                    fabricSubNames.Remove(subName);
                }
            }
            while (continuationToken != "");

            Verify.AreEqual(fabricSubNames.Count, 0);
        }

        private void CheckPropertyInfoList(Uri name, bool includeValues = false)
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            Console.WriteLine(string.Format("Checking properties under \"{0}\"...", name.ToString()));

            Dictionary<string, NamedProperty> fabricProperties = new Dictionary<string, NamedProperty>();
            PropertyEnumerationResult fabricResult = null;
            do
            {
                fabricResult = fabricClient.PropertyManager.EnumeratePropertiesAsync(name, includeValues, fabricResult).Result;
                foreach (NamedProperty property in fabricResult)
                {
                    fabricProperties.Add(property.Metadata.PropertyName, property);
                }
            }
            while (fabricResult.HasMoreData);

            PagedPropertyInfoList restResult = null;
            string continuationToken = "";
            do
            {
                restResult = restClient.GetPropertyInfoList(name, continuationToken, includeValues);
                continuationToken = restResult.ContinuationToken;
                foreach (PropertyInfo property in restResult.Properties)
                {
                    NamedProperty fabricProperty;
                    Verify.IsTrue(fabricProperties.TryGetValue(property.Name, out fabricProperty));
                    VerifyProperty(property, fabricProperty);
                    fabricProperties.Remove(property.Name);
                }
            }
            while (continuationToken != "");

            Verify.AreEqual(fabricProperties.Count, 0);
        }

        private void CheckBinaryPropertyInfoList(Uri name, bool includeValues = false)
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            Console.WriteLine(string.Format("Checking properties under \"{0}\"...", name.ToString()));

            Dictionary<string, NamedProperty> fabricProperties = new Dictionary<string, NamedProperty>();
            PropertyEnumerationResult fabricResult = null;
            do
            {
                fabricResult = fabricClient.PropertyManager.EnumeratePropertiesAsync(name, includeValues, fabricResult).Result;
                foreach (NamedProperty property in fabricResult)
                {
                    fabricProperties.Add(property.Metadata.PropertyName, property);
                }
            }
            while (fabricResult.HasMoreData);

            PagedBinaryPropertyInfoList restResult = null;
            string continuationToken = "";
            do
            {
                restResult = restClient.GetBinaryPropertyInfoList(name, continuationToken, includeValues);
                continuationToken = restResult.ContinuationToken;
                foreach (BinaryPropertyInfo property in restResult.Properties)
                {
                    NamedProperty fabricProperty;
                    Verify.IsTrue(fabricProperties.TryGetValue(property.Name, out fabricProperty));
                    VerifyProperty(property, fabricProperty);
                    fabricProperties.Remove(property.Name);
                }
            }
            while (continuationToken != "");

            Verify.AreEqual(fabricProperties.Count, 0);
        }

        private void CheckDoublePropertyInfoList(Uri name, bool includeValues = false)
        {
            Tuple<RESTClient, FabricClient> clients = GetFabricClients();
            RESTClient restClient = clients.Item1;
            FabricClient fabricClient = clients.Item2;

            Console.WriteLine(string.Format("Checking properties under \"{0}\"...", name.ToString()));

            Dictionary<string, NamedProperty> fabricProperties = new Dictionary<string, NamedProperty>();
            PropertyEnumerationResult fabricResult = null;
            do
            {
                fabricResult = fabricClient.PropertyManager.EnumeratePropertiesAsync(name, includeValues, fabricResult).Result;
                foreach (NamedProperty property in fabricResult)
                {
                    fabricProperties.Add(property.Metadata.PropertyName, property);
                }
            }
            while (fabricResult.HasMoreData);

            PagedDoublePropertyInfoList restResult = null;
            string continuationToken = "";
            do
            {
                restResult = restClient.GetDoublePropertyInfoList(name, continuationToken, includeValues);
                continuationToken = restResult.ContinuationToken;
                foreach (DoublePropertyInfo property in restResult.Properties)
                {
                    NamedProperty fabricProperty;
                    Verify.IsTrue(fabricProperties.TryGetValue(property.Name, out fabricProperty));
                    VerifyProperty(property, fabricProperty);
                    fabricProperties.Remove(property.Name);
                }
            }
            while (continuationToken != "");

            Verify.AreEqual(fabricProperties.Count, 0);
        }

        private void VerifyProperty(PropertyInfo restProperty, NamedProperty fabricProperty)
        {
            Verify.AreEqual(restProperty.Name, fabricProperty.Metadata.PropertyName);
            VerifyPropertyMetadata(restProperty.Metadata, fabricProperty.Metadata);
            switch (restProperty.Value.Kind)
            {
                case "Int64":
                    Verify.AreEqual(restProperty.Value.Int64Data, fabricProperty.GetValue<long>());
                    break;
                case "String":
                    Verify.AreEqual(restProperty.Value.StringData, fabricProperty.GetValue<string>());
                    break;
                case "Guid":
                    Verify.AreEqual(restProperty.Value.GuidData, fabricProperty.GetValue<Guid>());
                    break;
                default:
                    Verify.IsTrue(false, string.Format("Property \"{0}\" under name \"{1}\" has invalid data type.", restProperty.Name, restProperty.Metadata.Parent));
                    break;
            }
        }

        private void VerifyProperty(BinaryPropertyInfo restProperty, NamedProperty fabricProperty)
        {
            Verify.AreEqual(restProperty.Name, fabricProperty.Metadata.PropertyName);
            VerifyPropertyMetadata(restProperty.Metadata, fabricProperty.Metadata);
            Verify.IsTrue(restProperty.Value.Data.SequenceEqual(fabricProperty.GetValue<byte[]>()));
        }

        private void VerifyProperty(DoublePropertyInfo restProperty, NamedProperty fabricProperty)
        {
            Verify.AreEqual(restProperty.Name, fabricProperty.Metadata.PropertyName);
            VerifyPropertyMetadata(restProperty.Metadata, fabricProperty.Metadata);
            Verify.AreEqual(restProperty.Value.Data, fabricProperty.GetValue<double>());
        }

        private void VerifyPropertyMetadata(PropertyMetadata restMetadata, NamedPropertyMetadata fabricMetadata)
        {
            Verify.AreEqual(restMetadata.TypeId, fabricMetadata.TypeId.ToString());
            if (restMetadata.CustomTypeId.Length == 0)
            {
                Verify.IsTrue(fabricMetadata.CustomTypeId == null);
            }
            else
            {
                Verify.AreEqual(restMetadata.CustomTypeId, fabricMetadata.CustomTypeId);
            }
            Verify.AreEqual(new Uri(restMetadata.Parent), fabricMetadata.Parent);
            Verify.AreEqual(Int64.Parse(restMetadata.SequenceNumber), fabricMetadata.SequenceNumber);
            Verify.AreEqual(restMetadata.SizeInBytes, fabricMetadata.ValueSize);

            DateTime restDateTime;
            Verify.IsTrue(DateTime.TryParse(restMetadata.LastModifiedUtcTimestamp, out restDateTime));
            Verify.AreEqual(restDateTime.ToUniversalTime().Hour, fabricMetadata.LastModifiedUtc.Hour);
            Verify.AreEqual(restDateTime.ToUniversalTime().Minute, fabricMetadata.LastModifiedUtc.Minute);
            Verify.AreEqual(restDateTime.ToUniversalTime().Second, fabricMetadata.LastModifiedUtc.Second);
        }

        #endregion
    }
}
