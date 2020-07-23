// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.HttpGatewayTest
{
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Fabric.Test.Common.FabricUtility;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Runtime.Serialization.Json;
    using WEX.TestExecution;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using System.Net.Security;
    using System.Diagnostics;
    using System.Threading;
    using System.Fabric.Health;
    using System.Text;
    using System.Fabric;
    using System.Fabric.JsonSerializerWrapper;
    using System.Fabric.Query;
    using System.Fabric.Repair;

    //
    // DataContractJsonSerializer serializes and deserializes enums numbers. So this test
    // will test if our serializer can deserialize enums as numbers(enums as strings will be
    // tested by our REST client in scripttest).
    //

    /// <summary>
    /// Class that contains the uri templates for the REST requests.
    /// </summary>
    public class UriSuffix
    {
        public const string InvalidUriTemplate = "{0}/{1}/";

        #region Root Related Templates
        // GETs
        public const string GetClusterManifestUriTemplate = "$/GetClusterManifest?api-version=3.0";
        public const string GetFabricUpgradeProgressUriTemplate = "$/GetUpgradeProgress?api-version=1.0";
        
        public const string GetClusterHealthUriTemplate = "$/GetClusterHealth?api-version={0}";
        public const string GetClusterHealthWithAllFiltersUriTemplate = "$/GetClusterHealth?api-version={0}&EventsHealthStateFilter={1}&NodesHealthStateFilter={2}&ApplicationsHealthStateFilter={3}";
        public const string GetClusterHealthWithEventsAndAppsFiltersUriTemplate = "$/GetClusterHealth?api-version={0}&EventsHealthStateFilter={1}&ApplicationsHealthStateFilter={2}";
        public const string GetClusterHealthWithNodesAndAppsFiltersUriTemplate = "$/GetClusterHealth?api-version={0}&NodesHealthStateFilter={1}&ApplicationsHealthStateFilter={2}";

        public const string GetClusterLoadInformationUriTemplate = "$/GetLoadInformation?api-version=3.0";

        public const string InvokeInfrastructureQueryTemplate = "$/InvokeInfrastructureQuery?api-version=3.0&Command={0}&ServiceId={1}";

        public const string ReportClusterHealthUriTemplate = "$/ReportClusterHealth?api-version=3.0";

        public const string GetRepairTaskListUriTemplate = "$/GetRepairTaskList?api-version=1.0";

        // Actions
        public const string ProvisionFabricUriTemplate = "$/Provision?api-version=1.0";
        public const string UnprovisionFabricUriTemplate = "$/Unprovision?api-version=1.0";
        public const string UpgradeFabricUriTemplate = "$/Upgrade?api-version=1.0";
        public const string UpdateFabricUpgradeUriTemplate = "$/UpdateUpgrade?api-version=3.0";
        public const string MoveNextFabricUpgradeDomainUriTemplate = "$/MoveToNextUpgradeDomain?api-version=3.0";
        public const string RecoverAllPartitionsUriTemplate = "$/RecoverAllPartitions?api-version=3.0";
        public const string RecoverSystemPartitionsUriTemplate = "$/RecoverSystemPartitions?api-version=3.0";
        public const string InvokeInfrastructureCommandTemplate = "$/InvokeInfrastructureCommand?api-version=3.0&Command={0}&ServiceId={1}";
        public const string CreateRepairTaskUriTemplate = "$/CreateRepairTask?api-version=3.0";
        public const string CancelRepairTaskUriTemplate = "$/CancelRepairTask?api-version=3.0";
        public const string ForceApproveRepairTaskUriTemplate = "$/ForceApproveRepairTask?api-version=3.0";
        public const string DeleteRepairTaskUriTemplate = "$/DeleteRepairTask?api-version=3.0";
        public const string UpdateRepairExecutionStateUriTemplate = "$/UpdateRepairExecutionState?api-version=3.0";
        public const string UpdateRepairTaskHealthPolicyUriTemplate = "$/UpdateRepairTaskHealthPolicy?api-version=3.0";

        public const string ServiceKindParameter = "ServiceKind";
        public const string ContinuationTokenParameter = "ContinuationToken";
        public const string Applications = "Applications";
        public const string GetServices = "$/GetServices";
        public const string Services = "Services";
        public const string Partitions = "Partitions";
        public const string GetPartitions = "$/GetPartitions";
        public const string GetReplicas = "$/GetReplicas";
        public const string ReportHealth = "$/ReportHealth";
        public const string ApiVersionUri = "api-version";
        public const string ApiVersion = "1.0";
        public const string Api20Version = "2.0";
        public const string Api30Version = "3.0";
        public const string Api40Version = "4.0";
        public const string Api60Version = "6.0";
        public const string FirstQueryStringFormat = "?{0}={1}";
        public const string QueryStringFormat = "&{0}={1}";
        #endregion

        #region Node Related Templates
        public const string GetNodesUriTemplate = "Nodes?api-version=1.0";
        public const string GetNodeHealthUriTemplate = "Nodes/{0}/$/GetHealth?api-version=3.0";
        public const string GetNodeHealthWithEventsFilterUriTemplate = "Nodes/{0}/$/GetHealth?api-version=3.0&EventsHealthStateFilter={1}";
        public const string GetNodeLoadInformationUriTemplate = "/Nodes/{0}/$/GetLoadInformation?api-version=3.0";

        // Actions
        public const string ActivateNodeUriTemplate = "Nodes/{0}/$/Activate?api-version=1.0";
        public const string DeactivateNodeUriTemplate = "Nodes/{0}/$/Deactivate?api-version=1.0";
        public const string NodeStateRemovedUriTemplate = "Nodes/{0}/$/RemoveNodeState?api-version=1.0";
        public const string ReportNodeHealthUriTemplate = "Nodes/{0}/$/ReportHealth?api-version=3.0";

        public const string RestartNodeUriTemplate = "Nodes/{0}/$/Restart?api-version=1.0";
        public const string StopNodeUriTemplate = "Nodes/{0}/$/Stop?api-version=3.0";

        public const string StartNodeUriTemplate = "Nodes/{0}/$/Start?api-version=3.0";
        
        #endregion

        #region Node/Application Related Templates
        public const string GetApplicationsOnNodeUriTemplate = "Nodes/{0}/$/GetApplications?api-version=1.0";
        public const string GetDeployedApplicationHealthUriTemplate = "Nodes/{0}/$/GetApplications/{1}/$/GetHealth?api-version=1.0";
        public const string GetDeployedApplicationHealthWithFiltersUriTemplate = "Nodes/{0}/$/GetApplications/{1}/$/GetHealth?api-version=1.0&EventsHealthStateFilter={2}&DeployedServicePackagesHealthStateFilter={3}";
        
        public const string ReportDeployedApplicationHealthUriTemplate = "Nodes/{0}/$/GetApplications/{1}/$/ReportHealth?api-version=3.0";
        #endregion

        #region Node/Application/ServicePackages Related Templates
        public const string GetServiceManifestOnNodeUriTemplate = "Nodes/{0}/$/GetApplications/{1}/$/GetServicePackages?api-version=1.0";

        public const string GetDeployedServicePackageHealthUriTemplate = "Nodes/{0}/$/GetApplications/{1}/$/GetServicePackages/{2}/$/GetHealth?api-version=1.0";
        public const string GetDeployedServicePackageHealthWithFiltersUriTemplate = "Nodes/{0}/$/GetApplications/{1}/$/GetServicePackages/{2}/$/GetHealth?api-version=1.0&EventsHealthStateFilter={3}";
        
        public const string GetDeployedServicePackageHealthWithActivationIdUriTemplate = "Nodes/{0}/$/GetApplications/{1}/$/GetServicePackages/{2}/$/GetHealth?api-version=1.0&ServicePackageActivationId={3}";
        public const string GetDeployedServicePackageHealthWithActivationIdAndFiltersUriTemplate = "Nodes/{0}/$/GetApplications/{1}/$/GetServicePackages/{2}/$/GetHealth?api-version=1.0&ServicePackageActivationId={3}&EventsHealthStateFilter={4}";

        public const string ReportDeployedServicePackageHealthUriTemplate = "Nodes/{0}/$/GetApplications/{1}/$/GetServicePackages/{2}/$/ReportHealth?api-version=3.0";

        public const string DeployServicePackageToNodeUriTemplate = "Nodes/{0}/$/DeployServicePackage?api-version=1.0";
        #endregion

        #region Node/Application/CodePackages Related Templates
        public const string GetCodePackagesOnNodeUriTemplate = "Nodes/{0}/$/GetApplications/{1}/$/GetCodePackages?api-version=1.0&ServicePackageName={2}";
        public const string RestartCodePackagesOnNodeUriTemplate = "Nodes/{0}/$/GetApplications/{1}/$/GetCodePackages/$/Restart?api-version=1.0";
        #endregion

        #region Node/Application/Replicas Related Templates
        public const string GetServiceReplicasOnNodeUriTemplate = "Nodes/{0}/$/GetApplications/{1}/$/GetReplicas?api-version=1.0&ServicePackageName={2}";
        public const string GetServiceReplicasOnNodeNoServicePackageUriTemplate = "Nodes/{0}/$/GetApplications/{1}/$/GetReplicas?api-version=1.0";
        public const string ReportServiceReplicasOnNodeNoServicePackageUriTemplate = "Nodes/{0}/$/GetApplications/{1}/$/GetReplicas{2}/$/ReportHealth?api-version=4.0";
        #endregion

        #region Node/Partitions/Replicas Related Templates
        public const string RemoveReplicaOnNodeUriTemplate = "Nodes/{0}/$/GetPartitions/{1}/$/GetReplicas/{2}/$/Remove?api-version=1.0";
        public const string RemoveReplicaOnNodeUriWithForceTemplate = "Nodes/{0}/$/GetPartitions/{1}/$/GetReplicas/{2}/$/Remove?api-version=1.0&ForceRemove=true";
        public const string DeleteReplicaOnNodeUriTemplate = "Nodes/{0}/$/GetPartitions/{1}/$/GetReplicas/{2}/$/Delete?api-version=1.0";
        public const string DeleteReplicaOnNodeUriWithForceTemplate = "Nodes/{0}/$/GetPartitions/{1}/$/GetReplicas/{2}/$/Delete?api-version=1.0&ForceRemove=true";
        public const string RestartReplicaOnNodeUriTemplate = "Nodes/{0}/$/GetPartitions/{1}/$/GetReplicas/{2}/$/Restart?api-version=1.0";
        public const string GetReplicaDetailOnNodeUriTemplate = "Nodes/{0}/$/GetPartitions/{1}/$/GetReplicas/{2}/$/GetDetail?api-version=1.0";
        #endregion

        #region ApplicationType Templates
        public const string GetApplicationTypesUriTemplate = "ApplicationTypes?api-version=1.0";
        public const string GetApplicationManifestUriTemplate = "ApplicationTypes/{0}/$/GetApplicationManifest?ApplicationTypeVersion={1}&api-version=1.0";
        public const string GetServiceManifestUriTemplate = "ApplicationTypes/{0}/$/GetServiceManifest?ApplicationTypeVersion={1}&ServiceManifestName={2}&api-version=1.0";

        public const string ProvisionApplicationTypeUriTemplate = "ApplicationTypes/$/Provision?api-version=1.0";
        public const string UnprovisionApplicationTypeUriTemplate = "ApplicationTypes/{0}/$/Unprovision?api-version=1.0";
        #endregion

        #region ApplicationType/ServiceType Related Templates
        public const string GetServiceTypesUriTemplate = "ApplicationTypes/{0}/$/GetServiceTypes?ApplicationTypeVersion={1}&api-version=1.0";
        
        // For getting a specific service type.
        public const string GetServiceTypesByTypeUriTemplate = "ApplicationTypes/{0}/$/GetServiceTypes/{1}?ApplicationTypeVersion={2}&api-version=1.0";
        #endregion

        #region Application Related Tempaltes
        public const string GetApplicationsUriTemplate = "Applications?api-version=1.0";
        public const string GetAppUpgradeProgressUriTemplate = "Applications/{0}/$/GetUpgradeProgress?api-version=1.0";

        public const string CreateApplicationUriTemplate = "Applications/$/Create?api-version=1.0";
        public const string DeleteApplicationUriTemplate = "Applications/{0}/$/Delete?api-version=1.0";
        public const string UpgradeApplicationUriTemplate = "Applications/{0}/$/Upgrade?api-version=1.0";
        public const string UpdateApplicationUpgradeUriTemplate = "Applications/{0}/$/UpdateUpgrade?api-version=1.0";

        public const string GetApplicationHealthUriTemplate = "Applications/{0}/$/GetHealth?api-version=1.0";
        public const string GetApplicationHealthWithAllFiltersUriTemplate = "Applications/{0}/$/GetHealth?api-version=1.0&EventsHealthStateFilter={1}&ServicesHealthStatesFilter={2}&DeployedApplicationsHealthStateFilter={3}";
        public const string GetApplicationHealthWithEventsAndServicesFiltersUriTemplate = "Applications/{0}/$/GetHealth?api-version=1.0&EventsHealthStateFilter={1}&ServicesHealthStatesFilter={2}";
        public const string GetApplicationHealthWithEventsAndDeployedAppsFiltersUriTemplate = "Applications/{0}/$/GetHealth?api-version=1.0&EventsHealthStateFilter={1}&DeployedApplicationsHealthStateFilter={2}";
        public const string GetApplicationHealthWithServicesAndDeployedAppsFilterUriTemplate = "Applications/{0}/$/GetHealth?api-version=1.0";

        public const string ReportApplicationHealthUriTemplate = "Applications/{0}/$/ReportHealth?api-version=3.0";

        #endregion

        #region Applications/Service/ServiceGroup Related Templates
        public const string GetServicesUriTemplate = "Applications/{0}/$/GetServices?api-version=1.0";
        public const string GetServiceGroupMembersUriTemplate = "Applications/{0}/$/GetServices/{1}/$/GetServiceGroupMembers?api-version=1.0";
        
        public const string CreateServiceUriTemplate = "Applications/{0}/$/GetServices/$/Create?api-version=1.0";
        public const string CreateServiceGroupUriTemplate = "Applications/{0}/$/GetServiceGroups/$/Create?api-version=1.0";
        public const string CreateServiceFromTemplateUriTemplate = "Applications/{0}/$/GetServices/$/CreateFromTemplate?api-version=1.0";
        
        public const string DeleteServiceViaApplicationUriTemplate = "Applications/{0}/$/GetServices/{1}/$/Delete?api-version=1.0";
        public const string DeleteServiceGroupViaApplicationUriTemplate = "Applications/{0}/$/GetServiceGroups/{1}/$/Delete?api-version=1.0";
        public const string DeleteServiceViaServiceUriTemplate = "Services/{0}/$/Delete?api-version=1.0";
        
        public const string UpdateServiceViaApplicationUriTemplate = "Applications/{0}/$/GetServices/{1}/$/Update?api-version=1.0";
        public const string UpdateServiceGroupViaApplicationUriTemplate = "Applications/{0}/$/GetServiceGroups/{1}/$/Update?api-version=1.0";
        public const string UpdateServiceViaServiceUriTemplate = "Services/{0}/$/Update?api-version=1.0";

        public const string GetServiceDescriptionViaApplicationUriTemplate = "Applications/{0}/$/GetServices/{1}/$/GetDescription?api-version=1.0";
        public const string GetServiceDescriptionViaServiceUriTemplate = "Services/{0}/$/GetDescription?api-version=1.0";

        public const string GetServiceGroupDescriptionViaApplicationUriTemplate = "Applications/{0}/$/GetServices/{1}/$/GetServiceGroupDescription?api-version=1.0";
        public const string GetServiceGroupDescriptionViaServiceUriTemplate = "Services/{0}/$/GetServiceGroupDescription?api-version=1.0";

        public const string GetServiceHealthUriTemplate = "Applications/{0}/$/GetServices/{1}/$/GetHealth?api-version=1.0";
        public const string GetServiceHealthWithFiltersUriTemplate = "Applications/{0}/$/GetServices/{1}/$/GetHealth?api-version=1.0&EventsHealthStateFilter={2}&PartitionsHealthStateFilter={3}";
        public const string GetServiceHealthViaServiceUriTemplate = "Services/{0}/$/GetHealth?api-version=1.0";
        public const string GetServiceHealthViaServiceWithFiltersUriTemplate = "Services/{0}/$/GetHealth?api-version=1.0&EventsHealthStateFilter={1}&PartitionsHealthStateFilter={2}";
                
        public const string ReportServiceHealthUriTemplate = "Applications/{0}/$/GetServices/{1}/$/ReportHealth?api-version=3.0";
        public const string ReportServiceHealthViaServiceUriTemplate = "Services/{0}/$/ReportHealth?api-version=3.0";

        public const string ResolveServiceViaApplicationUriTemplate = "Applications/{0}/$/GetServices/{1}/$/ResolvePartition?api-version=1.0&PartitionKeyType={2}&PartitionKeyValue={3}&PreviousRspVersion={4}";
        public const string ResolveServiceViaServiceUriTemplate = "Services/{0}/$/ResolvePartition?api-version=1.0&PartitionKeyType={1}&PartitionKeyValue={2}&PreviousRspVersion={3}";
        public const string SingletonResolveServiceViaServiceUriTemplate = "Services/{0}/$/ResolvePartition?api-version=1.0&PreviousRspVersion={1}";

        #endregion

        #region Applications/Services/Partition Related Templates
        public const string GetServicePartitionViaApplicationUriTemplate = "Applications/{0}/$/GetServices/{1}/$/GetPartitions?api-version=1.0";
        public const string GetServicePartitionViaServiceUriTemplate = "Services/{0}/$/GetPartitions?api-version=1.0";

        public const string GetPartitionHealthUriTemplate = "Applications/{0}/$/GetServices/{1}/$/GetPartitions/{2}/$/GetHealth?api-version=1.0";
        public const string GetPartitionHealthViaServiceUriTemplate = "Services/{0}/$/GetPartitions/{1}/$/GetHealth?api-version=1.0";
        public const string GetPartitionHealthViaPartitionUriTemplate = "Partitions/{0}/$/GetHealth?api-version=1.0";

        public const string GetPartitionHealthWithFiltersUriTemplate = "Applications/{0}/$/GetServices/{1}/$/GetPartitions/{2}/$/GetHealth?api-version=1.0&EventsHealthStateFilter={3}&ReplicasHealthStateFilter={4}";
        public const string GetPartitionHealthViaServiceWithFiltersUriTemplate = "Services/{0}/$/GetPartitions/{1}/$/GetHealth?api-version=1.0&EventsHealthStateFilter={2}&ReplicasHealthStateFilter={3}";
        public const string GetPartitionHealthViaPartitionWithFiltersUriTemplate = "Partitions/{0}/$/GetHealth?api-version=1.0&EventsHealthStateFilter={1}&ReplicasHealthStateFilter={2}";

        public const string GetPartitionLoadUriTemplate = "Applications/{0}/$/GetServices/{1}/$/GetPartitions/{2}/$/GetLoadInformation?api-version=1.0";
        public const string GetPartitionLoadViaServiceUriTemplate = "Services/{0}/$/GetPartitions/{1}/$/GetLoadInformation?api-version=1.0";
        public const string GetPartitionLoadViaPartitionUriTemplate = "Partitions/{0}/$/GetLoadInformation?api-version=1.0";

        public const string ReportPartitionHealthUriTemplate = "Applications/{0}/$/GetServices/{1}/$/GetPartitions/{2}/$/ReportHealth?api-version=3.0";
        public const string ReportPartitionHealthViaServiceUriTemplate = "Services/{0}/$/GetPartitions/{1}/$/ReportHealth?api-version=3.0";
        public const string ReportPartitionHealthViaPartitionUriTemplate = "Partitions/{0}/$/ReportHealth?api-version=3.0";

        public const string RecoverAllServicePartitionsUriTemplate = "Applications/{0}/$/GetServices/{1}/$/GetPartitions/$/Recover?api-version=1.0";
        public const string RecoverAllServicePartitionsViaServiceUriTemplate = "Services/{0}/$/GetPartitions/$/Recover?api-version=1.0";
        
        public const string RecoverPartitionUriTemplate = "Applications/{0}/$/GetServices/{1}/$/GetPartitions/{2}/$/Recover?api-version=1.0";
        public const string RecoverPartitionViaServiceUriTemplate = "Services/{0}/$/GetPartitions/{1}/$/Recover?api-version=1.0";
        public const string RecoverPartitionViaPartitionUriTemplate = "Partitions/{0}/$/Recover?api-version=1.0";

        public const string GetServicePartitionByIdNoServiceName = "Partitions/{0}?api-version=1.0";

        #endregion

        #region Applications/Services/Partitions/Replica Related Templates
        public const string GetServiceReplicasViaApplicationUriTemplate = "Applications/{0}/$/GetServices/{1}/$/GetPartitions/{2}/$/GetReplicas?api-version=1.0";
        public const string GetServiceReplicasViaServiceUriTemplate = "Services/{0}/$/GetPartitions/{1}/$/GetReplicas?api-version=1.0";
        public const string GetServiceReplicasViaPartititonUriTemplate = "Partitions/{0}/$/GetReplicas?api-version=1.0";

        public const string GetReplicaHealthUriTemplate = "Applications/{0}/$/GetServices/{1}/$/GetPartitions/{2}/$/GetReplicas/{3}/$/GetHealth?api-version=1.0";
        public const string GetReplicaHealthViaServiceUriTemplate = "Services/{0}/$/GetPartitions/{1}/$/GetReplicas/{2}/$/GetHealth?api-version=1.0";
        public const string GetReplicaHealthViaPartitionUriTemplate = "Partitions/{0}/$/GetReplicas/{1}/$/GetHealth?api-version=1.0";

        public const string GetReplicaHealthWithFiltersUriTemplate = "Applications/{0}/$/GetServices/{1}/$/GetPartitions/{2}/$/GetReplicas/{3}/$/GetHealth?api-version=1.0&EventsHealthStateFilter={4}";
        public const string GetReplicaHealthViaServiceWithFiltersUriTemplate = "Services/{0}/$/GetPartitions/{1}/$/GetReplicas/{2}/$/GetHealth?api-version=1.0&EventsHealthStateFilter={3}";
        public const string GetReplicaHealthViaPartitionWithFiltersUriTemplate = "Partitions/{0}/$/GetReplicas/{1}/$/GetHealth?api-version=1.0&EventsHealthStateFilter={2}";

        #endregion

        #region Names Related Templates
        public const string CreateNameUriTemplate = "Names/$/Create?api-version=4.0";
        public const string NameUriTemplate = "Names/{0}?api-version=4.0";
        public const string GetSubNamesUriTemplate = "Names/{0}/$/GetSubNames";

        public const string GetPropertyUriTemplate = "Names/{0}/$/GetProperty?api-version=4.0";
        public const string GetPropertyWithNameUriTemplate = "Names/{0}/$/GetProperty?api-version=4.0&PropertyName={1}";
        public const string GetPropertiesUriTemplate = "Names/{0}/$/GetProperties";
        public const string SubmitPropertyBatchUriTemplate = "Names/{0}/$/GetProperties/$/SubmitBatch?api-version=4.0";
        #endregion
    }

    /// <summary>
    /// Test REST client for making REST requests to the HttpGateway
    /// </summary>
    internal class RESTClient
    {
        public const string HttpPOST = "POST";
        public const string HttpGET = "GET";
        public const string HttpDELETE = "DELETE";
        public const string HttpPUT = "PUT";
        public const string SystemApplicationName = "fabric:/System";
        public string thumbprint;
        public string authorizationHeader;
        public string hstsHeader;
        public RemoteCertificateValidationCallback certificateValidationCallback;
        private IJsonSerializer serializer;
        private static readonly Encoding encoding = Encoding.UTF8;
        
        /// <summary>
        /// Initializes a new instance of the RESTClient class.
        /// </summary>
        /// <param name="httpGatewayAddresses">Address of the httpGateway that the client will connect.</param>
        public RESTClient(string[] httpGatewayAddresses, string thumbprint = null, string authorizationHeader = null, string hstsHeaderValue = null)
        {
            this.HttpGatewayAddresses = httpGatewayAddresses;

            // The ssl server certificate used in httpgateway doesnot have a subject name that
            // matches the uri. So we need to add custom handler to validate the server subject name
            certificateValidationCallback = new RemoteCertificateValidationCallback(ValidateServerCertificate);
            ServicePointManager.ServerCertificateValidationCallback += certificateValidationCallback;
            if (thumbprint == null)
            {
                this.thumbprint = Constants.AdminCertificateThumbprint;
            }

            this.authorizationHeader = authorizationHeader;
            this.serializer = JsonSerializerImplDllLoader.TryGetJsonSerializerImpl();

            if (this.serializer == null)
            {
                // Fail if serializer was not loaded successfully.
                throw new DllNotFoundException("RESTClient..ctor(): Call to JsonSerializerImplDllLoader.TryGetJsonSerializerImpl() failed.");
            }

            this.hstsHeader = hstsHeaderValue;
        }

        ~RESTClient()
        {
            ServicePointManager.ServerCertificateValidationCallback -= certificateValidationCallback;
        }

        /// <summary>
        /// Gets or sets the array of Http Gateway addresses.
        /// This is the list of addresses that the client can connect to.
        /// </summary>
        public string[] HttpGatewayAddresses { get; set; }

        #region POST REQUESTS

        public void ProvisionApplicationType(string path, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            ProvisionApplicationTypeInput body = new ProvisionApplicationTypeInput(path);
            string uri = this.GetUri(UriSuffix.ProvisionApplicationTypeUriTemplate);
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(ProvisionApplicationTypeInput));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, body);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus, "FABRIC_E_APPLICATION_TYPE_ALREADY_EXISTS");
        }

        public void CreateApplication(string appName, string appTypeName, ref ApplicationDeployer appDeployer, HttpStatusCode exceptionStatus = HttpStatusCode.Created)
        {
            Dictionary<string, string> paramsList = new Dictionary<string, string>();

            CreateApplicationInput body = new CreateApplicationInput(appName, appTypeName, appDeployer.Version, paramsList);
            string uri = this.GetUri(UriSuffix.CreateApplicationUriTemplate);
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(CreateApplicationInput));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, body);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus, "FABRIC_E_APPLICATION_ALREADY_EXISTS");
            Console.WriteLine("Matching responseCode with the expected status {0}=={1}", response.StatusCode, exceptionStatus);
            Verify.IsTrue(response.StatusCode == exceptionStatus);
        }

        public void UpgradeApplication(ref ApplicationUpgradeDescriptionInput upgradeDescription, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(upgradeDescription.Name);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.UpgradeApplicationUriTemplate, appId));
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(ApplicationUpgradeDescriptionInput));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, upgradeDescription);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus, "FABRIC_E_APPLICATION_ALREADY_IN_TARGET_VERSION");
        }

        public void UpdateApplicationUpgrade(ref ApplicationUpgradeUpdateDescriptionInput updateDescription, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(updateDescription.Name);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.UpdateApplicationUpgradeUriTemplate, appId));
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(ApplicationUpgradeUpdateDescriptionInput));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, updateDescription);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus, "FABRIC_E_APPLICATION_UPGRADE_IN_PROGRESS");
        }

        public void DeleteApplication(string appName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string id = this.FabricNameToId(appName);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.DeleteApplicationUriTemplate, id));

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, null, exceptionStatus, "FABRIC_E_APPLICATION_NOT_FOUND");
        }

        public void UnprovisionApplicationType(string appTypeName, ref ApplicationDeployer appDeployer, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            UnprovisionApplicationTypeInput body = new UnprovisionApplicationTypeInput(appDeployer.Version);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.UnprovisionApplicationTypeUriTemplate, appTypeName));

            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(UnprovisionApplicationTypeInput));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, body);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus, "FABRIC_E_APPLICATION_TYPE_NOT_FOUND");
        }

        public void CreateService(ref CreateServiceInput input, HttpStatusCode exceptionStatus = HttpStatusCode.Accepted)
        {
            string appId = this.FabricNameToId(input.ApplicationName);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.CreateServiceUriTemplate, appId));

            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(CreateServiceInput));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, input);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus, "FABRIC_E_SERVICE_ALREADY_EXISTS");
            Console.WriteLine("Matching responseCode with the expected status {0}=={1}", response.StatusCode, exceptionStatus);
            Verify.IsTrue(response.StatusCode == exceptionStatus);
        }

        public void CreateServiceGroup(ref CreateServiceGroupInput input, HttpStatusCode exceptionStatus = HttpStatusCode.Accepted)
        {
            string appId = this.FabricNameToId(input.ApplicationName);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.CreateServiceGroupUriTemplate, appId));

            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(CreateServiceGroupInput));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, input);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus, "FABRIC_E_SERVICE_ALREADY_EXISTS");
            Console.WriteLine("Matching responseCode with the expected status {0}=={1}", response.StatusCode, exceptionStatus);
            Verify.IsTrue(response.StatusCode == exceptionStatus);
        }

        public void CreateServiceFromTemplate(
            string appName, 
            string svcName, 
            ServicePackageActivationMode servicePackageActivationMode, 
            ref ServiceDeployer serviceDeployer, 
            HttpStatusCode exceptionStatus = HttpStatusCode.Accepted)
        {
            // Dummy initialization data
            byte[] initData = { 0xa5, 0xa5, 0xa5, 0xa5 };
            CreateServiceFromTemplateInput input = new CreateServiceFromTemplateInput();
            input.ApplicationName = appName;
            input.ServiceName = svcName;
            input.ServiceTypeName = serviceDeployer.ServiceTypeName;
            input.ServicePackageActivationMode = servicePackageActivationMode;
            input.InitializationData = initData;

            string appId = this.FabricNameToId(appName);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.CreateServiceFromTemplateUriTemplate, appId));

            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(CreateServiceFromTemplateInput));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, input);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus, "FABRIC_E_SERVICE_ALREADY_EXISTS");
            Console.WriteLine("Matching responseCode with the expected status {0}=={1}", response.StatusCode, exceptionStatus);
            Verify.IsTrue(response.StatusCode == exceptionStatus);
        }

        public void DeleteService(string appName, string serviceName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(appName);
            string serviceId = this.FabricNameToId(serviceName);
            string uri;
            Random random = new Random();
            if (random.Next(3) % 2 == 0)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.DeleteServiceViaApplicationUriTemplate, appId, serviceId));
            }
            else
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.DeleteServiceViaServiceUriTemplate, serviceId));
            }

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, null, exceptionStatus, "FABRIC_E_SERVICE_DOES_NOT_EXIST");
        }

        public void DeployServicePackageToNode(string applicationTypeName, string applicationTypeVersion, string serviceManifestName, string nodeName, List<SharingPolicy> sharingPolicies)
        {
            PredeploymentData data = new PredeploymentData(applicationTypeName, applicationTypeVersion, nodeName, serviceManifestName, sharingPolicies);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.DeployServicePackageToNodeUriTemplate, nodeName));
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(PredeploymentData));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, data);
            HttpStatusCode exceptionStatus = HttpStatusCode.OK;
            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus);
            Verify.IsTrue(response.StatusCode == exceptionStatus);
        }

        public void RecoverServicePartitions(string appName, string serviceName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(appName);
            string serviceId = this.FabricNameToId(serviceName);
            string uri;
            Random random = new Random();
            if (random.Next(3) % 2 == 0)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.RecoverAllServicePartitionsUriTemplate, appId, serviceId));
            }
            else
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.RecoverAllServicePartitionsViaServiceUriTemplate, serviceId));
            }

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, null, exceptionStatus);
        }

        public void RecoverPartition(string appName, string serviceName, Guid partitionId, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(appName);
            string serviceId = this.FabricNameToId(serviceName);
            string uri;
            Random random = new Random();
            var choice = random.Next(3) % 3;
            if (choice == 0)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.RecoverPartitionUriTemplate, appId, serviceId, partitionId.ToString()));
            }
            else if (choice == 1)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.RecoverPartitionViaServiceUriTemplate, serviceId, partitionId.ToString()));
            }
            else
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.RecoverPartitionViaPartitionUriTemplate, partitionId.ToString()));
            }

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, null, exceptionStatus);
        }

        public void DeleteServiceGroup(string appName, string serviceName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(appName);
            string serviceId = this.FabricNameToId(serviceName);
            string uri;
            Random random = new Random();
            if (random.Next(3) % 2 == 0)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.DeleteServiceGroupViaApplicationUriTemplate, appId, serviceId));
            }
            else
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.DeleteServiceViaServiceUriTemplate, serviceId));
            }

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, null, exceptionStatus, "FABRIC_E_SERVICE_GROUP_DOES_NOT_EXIST");
        }

        public void UpdateService(string appName, string serviceName, int targetReplicaSetSize, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            UpdateServiceDescriptionInput body = new UpdateServiceDescriptionInput();
            body.ServiceKind = System.Fabric.Query.ServiceKind.Stateless;
            body.Flags = 1; // TargetReplicaSetSize
            body.InstanceCount = targetReplicaSetSize;
            string uri;

            string appId = this.FabricNameToId(appName);
            string serviceId = this.FabricNameToId(serviceName);
            Random random = new Random();
            if (random.Next(3) % 2 == 0)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.UpdateServiceViaApplicationUriTemplate, appId, serviceId));
            }
            else
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.UpdateServiceViaServiceUriTemplate, serviceId));
            }

            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(UpdateServiceDescriptionInput));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, body);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus);
        }

        public void UpdateServiceGroup(string appName, string serviceName, int targetReplicaSetSize, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            UpdateServiceDescriptionInput body = new UpdateServiceDescriptionInput();
            body.ServiceKind = System.Fabric.Query.ServiceKind.Stateless;
            body.Flags = 1; // TargetReplicaSetSize
            body.InstanceCount = targetReplicaSetSize;
            string uri;

            string appId = this.FabricNameToId(appName);
            string serviceId = this.FabricNameToId(serviceName);
            Random random = new Random();
            uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.UpdateServiceGroupViaApplicationUriTemplate, appId, serviceId));
            
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(UpdateServiceDescriptionInput));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, body);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus);
        }

        public void ProvisionFabric(string codeFilePath, string clusterManFilePath, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            ProvisionFabricInput body = new ProvisionFabricInput(codeFilePath, clusterManFilePath);
            string uri = this.GetUri(UriSuffix.ProvisionFabricUriTemplate);
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(ProvisionFabricInput));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, body);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus);
        }

        public void UpgradeFabric(FabricUpgradeDescriptionInput upgradeDescription, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(UriSuffix.UpgradeFabricUriTemplate);

            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(FabricUpgradeDescriptionInput));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, upgradeDescription);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus);
        }

        public void UnprovisionFabric(string codeVersion, string configVersion, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            UnprovisionFabricInput body = new UnprovisionFabricInput(codeVersion, configVersion);
            string uri = this.GetUri(UriSuffix.UnprovisionFabricUriTemplate);

            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(UnprovisionFabricInput));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, body);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus);
        }

        public void ActivateNode(string nodeId, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.ActivateNodeUriTemplate, nodeId));

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, null, exceptionStatus);
        }

        public void DeactivateNode(string nodeId, System.Fabric.NodeDeactivationIntent deactivationIntent, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            DeactivateNodeInput body = new DeactivateNodeInput(deactivationIntent);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.DeactivateNodeUriTemplate, nodeId));

            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(DeactivateNodeInput));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, body);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus);
        }
        
        public Health.ClusterHealth EvaluateClusterHealth(
            HttpStatusCode exceptionStatus = HttpStatusCode.OK,
            string apiVersion = "1.0",
            Health.ClusterHealthPolicy clusterHealthPolicy = null,
            Health.ApplicationHealthPolicyMap appHealthPolicies = null,
            HealthEventsFilter eventsFilter = null,
            NodeHealthStatesFilter nodesFilter = null,
            ApplicationHealthStatesFilter applicationsFilter = null)
        {
            string uri;

            if (eventsFilter == null && nodesFilter == null && applicationsFilter == null)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetClusterHealthUriTemplate, apiVersion));
            }
            else
            {
                var filter1 = (eventsFilter != null) ? (long)eventsFilter.HealthStateFilterValue : (long)HealthStateFilter.Default;
                var filter2 = (nodesFilter != null) ? (long)nodesFilter.HealthStateFilterValue : (long)HealthStateFilter.Default;
                var filter3 = (applicationsFilter != null) ? (long)applicationsFilter.HealthStateFilterValue : (long)HealthStateFilter.Default;

                if (eventsFilter == null)
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetClusterHealthWithNodesAndAppsFiltersUriTemplate, apiVersion, filter2, filter3));
                }
                else if (nodesFilter == null)
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetClusterHealthWithEventsAndAppsFiltersUriTemplate, apiVersion, filter1, filter3));
                }
                else
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetClusterHealthWithAllFiltersUriTemplate, apiVersion, filter1, filter2, filter3));
                }
            }

            string verb = HttpPOST;

            MemoryStream stream = null;
            if (clusterHealthPolicy != null)
            {
                if (appHealthPolicies != null)
                {
                    Dictionary<string, dynamic> jsonObject = new Dictionary<string, dynamic>();
                    jsonObject.Add("ApplicationHealthPolicyMap", new List<KeyValuePair<Uri,  Health.ApplicationHealthPolicy>>(appHealthPolicies));
                    jsonObject.Add("ClusterHealthPolicy", clusterHealthPolicy);
                    stream = this.SerializeObject(jsonObject);
                }
                else
                {
                    stream = this.SerializeObject(clusterHealthPolicy);
                }
            }
            else if (apiVersion == "1.0")
            {
                Random random = new Random();
                if ((random.Next(2) % 2) == 0) { verb = HttpGET; }
            }

            Health.ClusterHealth result = this.GetHttpResponseResult<Health.ClusterHealth>(uri, verb, stream, exceptionStatus);
            return result;
        }

        public Health.NodeHealth EvaluateNodeHealth(string nodeId, HttpStatusCode exceptionStatus, Health.ClusterHealthPolicy healthPolicy = null, HealthEventsFilter eventsFilter = null)
        {
            string uri;
            if (eventsFilter != null)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetNodeHealthWithEventsFilterUriTemplate, nodeId, (long)eventsFilter.HealthStateFilterValue));
            }
            else
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetNodeHealthUriTemplate, nodeId));
            }

            MemoryStream stream = null;
            if (healthPolicy != null)
            {
                stream = SerializeObject(healthPolicy);
            }

            var result = this.GetHttpResponseResult<Health.NodeHealth>(uri, HttpPOST, stream, exceptionStatus);
            return result;
        }

        public Health.ApplicationHealth EvaluateApplicationHealth(string applicationName, HttpStatusCode exceptionStatus = HttpStatusCode.OK, HealthEventsFilter eventsFilter = null, ServiceHealthStatesFilter servicesFilter = null, DeployedApplicationHealthStatesFilter deployedApplicationsFilter = null)
        {
            string appId = this.FabricNameToId(applicationName);
            string uri;
            if (eventsFilter != null && servicesFilter != null && deployedApplicationsFilter != null)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetApplicationHealthWithAllFiltersUriTemplate, appId, (long)eventsFilter.HealthStateFilterValue, (long)servicesFilter.HealthStateFilterValue, (long)deployedApplicationsFilter.HealthStateFilterValue));
            }
            else if (eventsFilter != null && servicesFilter != null)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetApplicationHealthWithEventsAndServicesFiltersUriTemplate, appId, (long)eventsFilter.HealthStateFilterValue, (long)servicesFilter.HealthStateFilterValue));
            }
            else if (eventsFilter != null && deployedApplicationsFilter != null)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetApplicationHealthWithEventsAndDeployedAppsFiltersUriTemplate, appId, (long)eventsFilter.HealthStateFilterValue, (long)deployedApplicationsFilter.HealthStateFilterValue));
            }
            else if (servicesFilter != null && deployedApplicationsFilter != null)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetApplicationHealthWithServicesAndDeployedAppsFilterUriTemplate, appId, (long)servicesFilter.HealthStateFilterValue, (long)deployedApplicationsFilter.HealthStateFilterValue));
            }
            else
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetApplicationHealthUriTemplate, appId));
            }

            var result = this.GetHttpResponseResult<Health.ApplicationHealth>(uri, HttpPOST, null, exceptionStatus);
            return result;
        }

        public Health.ServiceHealth EvaluateServicehealth(string applicationName, string serviceName, HttpStatusCode exceptionStatus = HttpStatusCode.OK, HealthEventsFilter eventsFilter = null, PartitionHealthStatesFilter partitionsFilter = null) // add healthpolicy parameter
        {
            string appId = this.FabricNameToId(applicationName);
            string serviceId = this.FabricNameToId(serviceName);
            string uri;
            Random random = new Random();

            long filter1 = (eventsFilter != null) ? (long)eventsFilter.HealthStateFilterValue : (long)HealthStateFilter.Default;
            long filter2 = (partitionsFilter != null) ? (long)partitionsFilter.HealthStateFilterValue : (long)HealthStateFilter.Default;
            bool useFilters = (eventsFilter != null || partitionsFilter != null);

            if (random.Next(3) % 2 == 0)
            {
                if (useFilters)
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceHealthWithFiltersUriTemplate, appId, serviceId, filter1, filter2));
                }
                else
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceHealthUriTemplate, appId, serviceId));
                }
            }
            else
            {
                if (useFilters)
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceHealthViaServiceWithFiltersUriTemplate, serviceId, filter1, filter2));
                }
                else
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceHealthViaServiceUriTemplate, serviceId));
                }
            }

            var result = this.GetHttpResponseResult<Health.ServiceHealth>(uri, HttpPOST, null, exceptionStatus);
            return result;
        }

        public Health.PartitionHealth EvaluatePartitionHealth(string applicationName, string serviceName, Guid partitionId, HttpStatusCode exceptionStatus = HttpStatusCode.OK, HealthEventsFilter eventsFilter = null, ReplicaHealthStatesFilter replicasFilter = null)
        {
            string appId = this.FabricNameToId(applicationName);
            string serviceId = this.FabricNameToId(serviceName);
            string uri;
            Random random = new Random();
            int choice = random.Next(3);
            long filter1 = (eventsFilter != null) ? (long)eventsFilter.HealthStateFilterValue : (long)HealthStateFilter.Default;
            long filter2 = (replicasFilter != null) ? (long)replicasFilter.HealthStateFilterValue : (long)HealthStateFilter.Default;
            bool useFilters = (eventsFilter != null || replicasFilter != null);

            if (choice == 0)
            {
                if (useFilters)
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPartitionHealthWithFiltersUriTemplate, appId, serviceId, partitionId.ToString(), filter1, filter2));
                }
                else
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPartitionHealthUriTemplate, appId, serviceId, partitionId.ToString()));
                }
            }
            else if (choice == 1)
            {
                if (useFilters)
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPartitionHealthViaServiceWithFiltersUriTemplate, serviceId, partitionId.ToString(), filter1, filter2));
                }
                else
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPartitionHealthViaServiceUriTemplate, serviceId, partitionId.ToString()));
                }
            }
            else
            {
                if (useFilters)
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPartitionHealthViaPartitionWithFiltersUriTemplate, partitionId.ToString(), filter1, filter2));
                }
                else
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPartitionHealthViaPartitionUriTemplate, partitionId.ToString()));
                }
            }

            var result = this.GetHttpResponseResult<Health.PartitionHealth>(uri, HttpPOST, null, exceptionStatus);
            return result;
        }

        public Health.ReplicaHealth EvaluateReplicaHealth(string applicationName, string serviceName, Guid partitionId, string replicaId, HttpStatusCode exceptionStatus = HttpStatusCode.OK, HealthEventsFilter eventsFilter = null)
        {
            string appId = this.FabricNameToId(applicationName);
            string serviceId = this.FabricNameToId(serviceName);
            string uri;
            Random random = new Random();
            int choice = random.Next(3);
            if (choice == 0)
            {
                if (eventsFilter != null)
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetReplicaHealthWithFiltersUriTemplate, appId, serviceId, partitionId.ToString(), replicaId, (long)eventsFilter.HealthStateFilterValue));
                }
                else
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetReplicaHealthUriTemplate, appId, serviceId, partitionId.ToString(), replicaId));
                }
            }
            else if (choice == 1)
            {
                if (eventsFilter != null)
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetReplicaHealthViaServiceWithFiltersUriTemplate, serviceId, partitionId.ToString(), replicaId, (long)eventsFilter.HealthStateFilterValue));
                }
                else
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetReplicaHealthViaServiceUriTemplate, serviceId, partitionId.ToString(), replicaId));
                }
            }
            else
            {
                if (eventsFilter != null)
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetReplicaHealthViaPartitionWithFiltersUriTemplate, partitionId.ToString(), replicaId, (long)eventsFilter.HealthStateFilterValue));
                }
                else
                {
                    uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetReplicaHealthViaPartitionUriTemplate, partitionId.ToString(), replicaId));
                }
            }

            var result = this.GetHttpResponseResult<Health.ReplicaHealth>(uri, HttpPOST, null, exceptionStatus);
            return result;
        }

        public Health.DeployedApplicationHealth EvaluateDeployedApplicationHealth(string nodeName, string applicationName, HttpStatusCode exceptionStatus = HttpStatusCode.OK, HealthEventsFilter eventsFilter = null, DeployedServicePackageHealthStatesFilter deployedServicePackagesFilter = null)
        {
            string appId = this.FabricNameToId(applicationName);
            string uri;

            long filter1 = (eventsFilter != null) ? (long)eventsFilter.HealthStateFilterValue : (long)HealthStateFilter.Default;
            long filter2 = (deployedServicePackagesFilter != null) ? (long)deployedServicePackagesFilter.HealthStateFilterValue : (long)HealthStateFilter.Default;
            bool useFilters = (eventsFilter != null || deployedServicePackagesFilter != null);

            if (useFilters)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetDeployedApplicationHealthWithFiltersUriTemplate, nodeName, appId, filter1, filter2));
            }
            else
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetDeployedApplicationHealthUriTemplate, nodeName, appId));
            }

            var result = this.GetHttpResponseResult<Health.DeployedApplicationHealth>(uri, HttpPOST, null, exceptionStatus);
            return result;
        }

        public Health.DeployedServicePackageHealth EvaluateDeployedServicePackageHealth(
            string nodeName, 
            string applicationName, 
            string serviceManifestName, 
            string servicePackageActivationId,
            HttpStatusCode exceptionStatus = HttpStatusCode.OK, 
            HealthEventsFilter eventsFilter = null)
        {
            string appId = this.FabricNameToId(applicationName);
            string uri;

            if (eventsFilter != null)
            {
                if (string.IsNullOrWhiteSpace(servicePackageActivationId))
                {
                    uri = this.GetUri(
                        string.Format(
                            CultureInfo.InvariantCulture,
                            UriSuffix.GetDeployedServicePackageHealthWithFiltersUriTemplate,
                            nodeName,
                            appId,
                            serviceManifestName,
                            (long)eventsFilter.HealthStateFilterValue));
                }
                else
                {
                    uri = this.GetUri(
                        string.Format(
                            CultureInfo.InvariantCulture,
                            UriSuffix.GetDeployedServicePackageHealthWithActivationIdAndFiltersUriTemplate,
                            nodeName,
                            appId,
                            serviceManifestName,
                            servicePackageActivationId,
                            (long)eventsFilter.HealthStateFilterValue));
                }
            }
            else
            {
                if (string.IsNullOrWhiteSpace(servicePackageActivationId))
                {
                    uri = this.GetUri(
                        string.Format(
                            CultureInfo.InvariantCulture,
                            UriSuffix.GetDeployedServicePackageHealthUriTemplate,
                            nodeName,
                            appId,
                            serviceManifestName));
                }
                else
                {
                    uri = this.GetUri(
                        string.Format(
                            CultureInfo.InvariantCulture,
                            UriSuffix.GetDeployedServicePackageHealthWithActivationIdUriTemplate,
                            nodeName,
                            appId,
                            serviceManifestName,
                            servicePackageActivationId));
                }
            }

            var result = this.GetHttpResponseResult<Health.DeployedServicePackageHealth>(uri, HttpPOST, null, exceptionStatus);
            return result;
        }

        public void ReportNodeHealth(string nodeId, Health.HealthInformation healthInfo, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.ReportNodeHealthUriTemplate, nodeId));
            this.PostRequest(uri, healthInfo, exceptionStatus);
        }

        public void ReportApplicationHealth(string applicationName, Health.HealthInformation healthInfo, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(applicationName);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.ReportApplicationHealthUriTemplate, appId));

            this.PostRequest(uri, healthInfo, exceptionStatus);
        }

        public void ReportServicehealth(string applicationName, string serviceName, Health.HealthInformation healthInfo, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(applicationName);
            string serviceId = this.FabricNameToId(serviceName);
            string uri;
            Random random = new Random();
            if (random.Next(3) % 2 == 0)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.ReportServiceHealthUriTemplate, appId, serviceId));
            }
            else
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.ReportServiceHealthViaServiceUriTemplate, serviceId));
            }

            this.PostRequest(uri, healthInfo, exceptionStatus);
        }

        public void ReportPartitionHealth(string applicationName, string serviceName, Guid partitionId, Health.HealthInformation healthInfo, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(applicationName);
            string serviceId = this.FabricNameToId(serviceName);
            string uri;
            Random random = new Random();
            int choice = random.Next(3);
            if (choice == 0)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.ReportPartitionHealthUriTemplate, appId, serviceId, partitionId.ToString()));
            }
            else if (choice == 1)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.ReportPartitionHealthViaServiceUriTemplate, serviceId, partitionId.ToString()));
            }
            else
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.ReportPartitionHealthViaPartitionUriTemplate, partitionId.ToString()));
            }

            this.PostRequest(uri, healthInfo, exceptionStatus);
        }

        public void ReportReplicaHealth(string applicationName, string serviceName, Guid partitionId, ServiceReplicaResult replica, Health.HealthInformation healthInfo, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string replicaId;
            Dictionary<string, string> queryParameters = new Dictionary<string, string>();
            Random random = new Random();

            queryParameters.Add(UriSuffix.ServiceKindParameter, string.Format("{0}", (int)replica.ServiceKind));
            if (replica.ServiceKind == ServiceKind.Stateful)
            {
                replicaId = replica.ReplicaId;
            }
            else
            {
                replicaId = replica.InstanceId;
            }

            string appId = this.FabricNameToId(applicationName);
            string serviceId = this.FabricNameToId(serviceName);
            string uri;
            
            int choice = random.Next(3);
            if (choice == 0)
            {
                string[] resourceIds = new string[] { appId, serviceId, partitionId.ToString(), replicaId }; 
                string[] resourceTypes = new string[] { UriSuffix.Applications, UriSuffix.GetServices, UriSuffix.GetPartitions, UriSuffix.GetReplicas, UriSuffix.ReportHealth };
                uri = this.GetUriByIdWithParameters(resourceTypes, resourceIds, queryParameters);
            }
            else if (choice == 1)
            {
                string[] resourceIds = new string[] { serviceId, partitionId.ToString(), replicaId }; 
                string[] resourceTypes = new string[] { UriSuffix.Services, UriSuffix.GetPartitions, UriSuffix.GetReplicas, UriSuffix.ReportHealth };
                uri = this.GetUriByIdWithParameters(resourceTypes, resourceIds, queryParameters);
            }
            else
            {
                string[] resourceIds = new string[] { partitionId.ToString(), replicaId }; 
                string[] resourceTypes = new string[] { UriSuffix.Partitions, UriSuffix.GetReplicas, UriSuffix.ReportHealth };
                uri = this.GetUriByIdWithParameters(resourceTypes, resourceIds, queryParameters);
            }

            this.PostRequest(uri, healthInfo, exceptionStatus);
        }

        public void ReportClusterHealth(Health.HealthInformation healthInfo, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.ReportClusterHealthUriTemplate));
            this.PostRequest(uri, healthInfo, exceptionStatus);
        }

        public void ReportDeployedApplicationHealth(string nodeName, string applicationName, Health.HealthInformation healthInfo, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(applicationName);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.ReportDeployedApplicationHealthUriTemplate, nodeName, appId));

            this.PostRequest(uri, healthInfo, exceptionStatus);
        }

        public void ReportDeployedServicePackageHealth(string nodeName, string applicationName, string serviceManifestName, Health.HealthInformation healthInfo, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(applicationName);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.ReportDeployedServicePackageHealthUriTemplate, nodeName, appId, serviceManifestName));

            this.PostRequest(uri, healthInfo, exceptionStatus);
        }

        public PartitionLoadInformationResult GetPartitionLoadInformation(string applicationName, string serviceName, Guid partitionId, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(applicationName);
            string serviceId = this.FabricNameToId(serviceName);
            string uri;
            Random random = new Random();
            int choice = random.Next(3);
            if (choice == 0)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPartitionLoadUriTemplate, appId, serviceId, partitionId.ToString()));
            }
            else if (choice == 1)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPartitionLoadViaServiceUriTemplate, serviceId, partitionId.ToString()));
            }
            else
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPartitionLoadViaPartitionUriTemplate, partitionId.ToString()));
            }

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(PartitionLoadInformationResult));
            PartitionLoadInformationResult result = (PartitionLoadInformationResult)deserializer.ReadObject(response.GetResponseStream());
            return result;
        }

        public ClusterLoadInformationResult GetClusterLoadInformation(HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri;
            Random random = new Random();
            uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetClusterLoadInformationUriTemplate));

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(ClusterLoadInformationResult));
            ClusterLoadInformationResult result = (ClusterLoadInformationResult)deserializer.ReadObject(response.GetResponseStream());
            return result;
        }

        public NodeLoadInformationResult GetNodeLoadInformation(string nodeName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri;
            Random random = new Random();
            uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetNodeLoadInformationUriTemplate, nodeName));

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(NodeLoadInformationResult));
            NodeLoadInformationResult result = (NodeLoadInformationResult)deserializer.ReadObject(response.GetResponseStream());
            return result;
        }

        public void RecoverAllPartitions(HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(UriSuffix.RecoverAllPartitionsUriTemplate);
            HttpWebResponse response = MakeRequest(uri, HttpPOST, null, exceptionStatus);
        }

        public void RecoverSystemPartitions(HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(UriSuffix.RecoverSystemPartitionsUriTemplate);
            HttpWebResponse response = MakeRequest(uri, HttpPOST, null, exceptionStatus);
        }

        public void RemoveReplica(string nodeName, Guid partitionId, long replicaId, bool useDelete, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, useDelete ? UriSuffix.DeleteReplicaOnNodeUriTemplate : UriSuffix.RemoveReplicaOnNodeUriTemplate, nodeName, partitionId, replicaId));
            MakeRequest(uri, HttpPOST, null, exceptionStatus);
        }

        public void RemoveReplicaWithForce(string nodeName, Guid partitionId, long replicaId, bool useDelete, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, useDelete ? UriSuffix.DeleteReplicaOnNodeUriWithForceTemplate : UriSuffix.RemoveReplicaOnNodeUriWithForceTemplate, nodeName, partitionId, replicaId));
            MakeRequest(uri, HttpPOST, null, exceptionStatus);
        }

        public void RestartReplica(string nodeName, Guid partitionId, long replicaId, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.RestartReplicaOnNodeUriTemplate, nodeName, partitionId, replicaId));
            MakeRequest(uri, HttpPOST, null, exceptionStatus);
        }
        
        public void StartNode(int gatewayNodeIndex, string nodeName, string ipAddressOrFQDN, int clusterConnectionPort, long nodeInstanceId, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(StartNodeData));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, new StartNodeData() 
            {                 
                IpAddressOrFQDN = ipAddressOrFQDN,
                ClusterConnectionPort = clusterConnectionPort,
                NodeInstanceId = Convert.ToString(nodeInstanceId)
            });

            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.StartNodeUriTemplate, nodeName), gatewayNodeIndex);
            MakeRequest(uri, HttpPOST, stream, exceptionStatus);
        }

        public void RestartNode(string nodeName, long nodeInstanceId, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(NodeControlData));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, new NodeControlData() { NodeInstanceId = nodeInstanceId.ToString() });

            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.RestartNodeUriTemplate, nodeName));
            MakeRequest(uri, HttpPOST, stream, exceptionStatus);
        }

        public void RestartCodePackage(string nodeName, string applicationName, string serviceManifestName, string codePackageName, long codePackageInstanceId, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(applicationName);
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(RestartCodePackageData));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, new RestartCodePackageData() { CodePackageInstanceId = codePackageInstanceId.ToString(), ServiceManifestName = serviceManifestName, CodePackageName = codePackageName });

            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.RestartCodePackagesOnNodeUriTemplate, nodeName, appId));
            MakeRequest(uri, HttpPOST, stream, exceptionStatus);
        }

        public void StopNode(string nodeName, long nodeInstanceId, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(NodeControlData));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, new NodeControlData() { NodeInstanceId = nodeInstanceId.ToString() });

            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.StopNodeUriTemplate, nodeName));
            MakeRequest(uri, HttpPOST, stream, exceptionStatus);
        }

        public DeployedReplicaDetail GetReplicaDetail(string nodeName, Guid partitionId, long replicaId, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetReplicaDetailOnNodeUriTemplate, nodeName, partitionId, replicaId));
            var resp = MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(DeployedReplicaDetail));
            return (DeployedReplicaDetail)deserializer.ReadObject(resp.GetResponseStream());
        }

        public string InvokeInfrastructureCommand(Uri serviceName, string command, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string serviceId = FabricNameToId(serviceName.ToString());

            string uri = this.GetUri(string.Format(
                CultureInfo.InvariantCulture,
                UriSuffix.InvokeInfrastructureCommandTemplate,
                Uri.EscapeDataString(command),
                Uri.EscapeDataString(serviceId)));

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, null, exceptionStatus);

            using (var reader = new StreamReader(response.GetResponseStream()))
            {
                return reader.ReadToEnd();
            }
        }

        public long CreateRepairTask(RepairTaskData repairTask, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            return this.CreateOrUpdateRepairTaskInternal(
                UriSuffix.CreateRepairTaskUriTemplate,
                repairTask,
                exceptionStatus);
        }

        public long CancelRepairTask(string taskId, string version, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            return this.CancelRepairTask(taskId, long.Parse(version), exceptionStatus);
        }

        public long CancelRepairTask(string taskId, long version, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            return this.SimpleUpdateRepairTaskInternal(
                UriSuffix.CancelRepairTaskUriTemplate,
                taskId,
                version,
                exceptionStatus);
        }

        public long ForceApproveRepairTask(string taskId, long version, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            return this.SimpleUpdateRepairTaskInternal(
                UriSuffix.ForceApproveRepairTaskUriTemplate,
                taskId,
                version,
                exceptionStatus);
        }

        public void DeleteRepairTask(string taskId, string version, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            this.DeleteRepairTask(taskId, long.Parse(version), exceptionStatus);
        }

        public void DeleteRepairTask(string taskId, long version, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            this.SimpleUpdateRepairTaskInternal(
                UriSuffix.DeleteRepairTaskUriTemplate,
                taskId,
                version,
                exceptionStatus);
        }

        public long UpdateRepairExecutionState(RepairTaskData repairTask, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            return this.CreateOrUpdateRepairTaskInternal(
                UriSuffix.UpdateRepairExecutionStateUriTemplate,
                repairTask,
                exceptionStatus);
        }

        public long UpdateRepairTaskHealthPolicy(
            string taskId, 
            long version, 
            bool? performPreparingHealthCheck = null,
            bool? performRestoringHealthCheck = null, 
            HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            return this.UpdateRepairTaskHealthPolicyInternal(
                UriSuffix.UpdateRepairTaskHealthPolicyUriTemplate,
                taskId,
                version,
                performPreparingHealthCheck,
                performRestoringHealthCheck,
                exceptionStatus);
        }

        private long CreateOrUpdateRepairTaskInternal(string uriTemplate, RepairTaskData repairTask, HttpStatusCode exceptionStatus)
        {
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(RepairTaskData));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, repairTask);

            string s = System.Text.Encoding.UTF8.GetString(stream.GetBuffer(), 0, (int)stream.Length);
            Console.WriteLine("Encoded: [{0}]", s);

            string uri = this.GetUri(uriTemplate);
            var resp = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus);

            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(RepairIdentifierData));
            var respData = (RepairIdentifierData)deserializer.ReadObject(resp.GetResponseStream());
            return long.Parse(respData.Version);
        }

        private long SimpleUpdateRepairTaskInternal(string uriTemplate, string taskId, long version, HttpStatusCode exceptionStatus)
        {
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(RepairIdentifierData));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, new RepairIdentifierData()
            {
                TaskId = taskId,
                Version = version.ToString(),
            });

            string uri = this.GetUri(uriTemplate);
            var resp = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus);

            if (resp.ContentLength > 0)
            {
                DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(RepairIdentifierData));
                var respData = (RepairIdentifierData)deserializer.ReadObject(resp.GetResponseStream());
                return long.Parse(respData.Version);
            }
            else
            {
                return -1;
            }
        }

        private long UpdateRepairTaskHealthPolicyInternal(
            string uriTemplate, 
            string taskId, 
            long version, 
            bool? performPreparingHealthCheck, 
            bool? performRestoringHealthCheck, 
            HttpStatusCode exceptionStatus)
        {
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(RepairTaskHealthPolicyData));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(
                stream,
                new RepairTaskHealthPolicyData
                {
                    TaskId = taskId,
                    Version = version.ToString(),
                    PerformPreparingHealthCheck = performPreparingHealthCheck,
                    PerformRestoringHealthCheck = performRestoringHealthCheck,
                });

            string uri = this.GetUri(uriTemplate);
            var resp = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus);

            if (resp.ContentLength > 0)
            {
                DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(RepairIdentifierData));
                var respData = (RepairIdentifierData)deserializer.ReadObject(resp.GetResponseStream());
                return long.Parse(respData.Version);
            }
            else
            {
                return -1;
            }
        }

        public void CreateName(Uri name, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            NameDescription body = new NameDescription(name);
            string uri = this.GetUri(UriSuffix.CreateNameUriTemplate);
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(NameDescription));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, body);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus, "FABRIC_E_NAME_ALREADY_EXISTS");
        }

        public PropertyBatchInfo SubmitPropertyBatch(Uri name, PropertyBatchDescription property, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            foreach (PropertyBatchOperation operation in property.Operations)
            {
                if (operation.Value != null)
                {
                    operation.Value.OnSerialize();
                }
            }
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.SubmitPropertyBatchUriTemplate, this.FabricNameToId(name.ToString())));

            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(PropertyBatchDescription));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, property);

            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(PropertyBatchInfo), new DataContractJsonSerializerSettings { UseSimpleDictionaryFormat = true });
            PropertyBatchInfo result = (PropertyBatchInfo)deserializer.ReadObject(response.GetResponseStream());

            if (result.Properties != null)
            {
                foreach (KeyValuePair<string, PropertyInfo> pair in result.Properties)
                {
                    pair.Value.OnDeserialized();
                }
            }
            return result;
        }

        #endregion POST REQUESTS

        #region GET REQUESTS

        public List<RepairTaskData> GetRepairTaskList(string taskIdFilter, Repair.RepairTaskStateFilter? stateFilter, string executorFilter, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = UriSuffix.GetRepairTaskListUriTemplate;

            if (taskIdFilter != null)
            {
                uri += "&TaskIdFilter=" + Uri.EscapeDataString(taskIdFilter);
            }

            if (stateFilter != null)
            {
                uri += "&StateFilter=" + (int)stateFilter.Value;
            }

            if (executorFilter != null)
            {
                uri += "&ExecutorFilter=" + Uri.EscapeDataString(executorFilter);
            }

            uri = this.GetUri(uri);
            var resp = this.MakeRequest(uri, HttpGET, null, exceptionStatus);

            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(
                typeof(IEnumerable<RepairTaskData>));

            return new List<RepairTaskData>(
                (IEnumerable<RepairTaskData>)deserializer.ReadObject(resp.GetResponseStream()));
        }

        public FabricUpgradeProgressResult GetFabricUpgradeProgress(HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(UriSuffix.GetFabricUpgradeProgressUriTemplate);

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(FabricUpgradeProgressResult));
            FabricUpgradeProgressResult result = (FabricUpgradeProgressResult)deserializer.ReadObject(response.GetResponseStream());
            return result;
        }

        public ApplicationUpgradeProgressResult GetApplicationUpgradeProgress(string appName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(appName);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetAppUpgradeProgressUriTemplate, appId));

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(ApplicationUpgradeProgressResult));
            ApplicationUpgradeProgressResult result = (ApplicationUpgradeProgressResult)deserializer.ReadObject(response.GetResponseStream());
            return result;
        }

        public List<NodesResult> GetNodes(HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(UriSuffix.GetNodesUriTemplate);
            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            List<NodesResult> nodesResult = null;
            if (response != null && response.StatusCode == HttpStatusCode.OK)
            {
                DataContractJsonSerializer nodesDeserializer = new DataContractJsonSerializer(typeof(IEnumerable<NodesResult>));
                nodesResult = new List<NodesResult>((IEnumerable<NodesResult>)nodesDeserializer.ReadObject(response.GetResponseStream()));
            }
            return nodesResult;
        }

        public List<ApplicationTypeResult> GetApplicationTypes(HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(UriSuffix.GetApplicationTypesUriTemplate);

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(IEnumerable<ApplicationTypeResult>));
            List<ApplicationTypeResult> result = new List<ApplicationTypeResult>((IEnumerable<ApplicationTypeResult>)deserializer.ReadObject(response.GetResponseStream()));
            return result;
        }

        public List<ApplicationsResult> GetApplications(HttpStatusCode exceptionStatus = HttpStatusCode.OK, string apiVersion = UriSuffix.ApiVersion)
        {
            string uri = this.GetUriWithParameters(UriSuffix.Applications, null, apiVersion);

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(IEnumerable<ApplicationsResult>));
            List<ApplicationsResult> result = new List<ApplicationsResult>((IEnumerable<ApplicationsResult>)deserializer.ReadObject(response.GetResponseStream()));
            return result;
        }

        public List<ApplicationsResult> GetApplicationsWithPagedResults(HttpStatusCode exceptionStatus = HttpStatusCode.OK, string apiVersion = UriSuffix.Api20Version)
        {
            Verify.AreNotEqual(apiVersion, UriSuffix.ApiVersion, "Paging is not supported in API version 1.0");

            string continuationToken = string.Empty;
            List<ApplicationsResult> result = new List<ApplicationsResult>();
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(ApplicationsPagedResult));
            do
            {
                Dictionary<string, string> parameters = new Dictionary<string, string>();
                if (!string.IsNullOrWhiteSpace(continuationToken))
                {
                    parameters.Add(UriSuffix.ContinuationTokenParameter, continuationToken);
                }

                string uri = this.GetUriWithParameters(UriSuffix.Applications, parameters, apiVersion);
                HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
                var pagedResult = (ApplicationsPagedResult)deserializer.ReadObject(response.GetResponseStream());
                result.AddRange(pagedResult.Items);
                continuationToken = pagedResult.ContinuationToken;
            } while (!string.IsNullOrWhiteSpace(continuationToken));

            return result;
        }

        public List<ServiceReplicaResult> GetServiceReplicas(string appName, string serviceName, string partitionId, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(appName);
            string serviceId = this.FabricNameToId(serviceName);
            string uri;
            Random random = new Random();
            int choice = random.Next(3);
            if (choice == 0)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceReplicasViaApplicationUriTemplate, appId, serviceId, partitionId));
            } 
            else if (choice == 1)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceReplicasViaServiceUriTemplate, serviceId, partitionId));
            }
            else
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceReplicasViaPartititonUriTemplate, partitionId));
            }

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(IEnumerable<ServiceReplicaResult>));
            List<ServiceReplicaResult> result = new List<ServiceReplicaResult>((IEnumerable<ServiceReplicaResult>)deserializer.ReadObject(response.GetResponseStream()));
            return result;
        }

        public List<ServiceResult> GetServices(string appName, HttpStatusCode exceptionStatus = HttpStatusCode.OK, string apiVersion = UriSuffix.ApiVersion)
        {
            string appId = this.FabricNameToId(appName);
            string[] resourceIds = new string[] { appId };
            string[] resourceTypes = new string[] { UriSuffix.Applications, UriSuffix.GetServices };
            string uri = this.GetUriByIdWithParameters(resourceTypes, resourceIds, null, apiVersion);

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(IEnumerable<ServiceResult>));
            List<ServiceResult> result = new List<ServiceResult>((IEnumerable<ServiceResult>)deserializer.ReadObject(response.GetResponseStream()));
            return result;
        }

        public List<ServiceResult> GetServicesWithPagedResults(string appName, HttpStatusCode exceptionStatus = HttpStatusCode.OK, string apiVersion = UriSuffix.ApiVersion)
        {
            Verify.AreNotEqual(apiVersion, UriSuffix.ApiVersion, "Paging is not supported in API version 1.0");

            string continuationToken = string.Empty;
            var result = new List<ServiceResult>();
            var deserializer = new DataContractJsonSerializer(typeof(ServicePagedResult));

            string appId = this.FabricNameToId(appName);
            string[] resourceIds = new string[] { appId };
            string[] resourceTypes = new string[] { UriSuffix.Applications, UriSuffix.GetServices };

            do
            {
                Dictionary<string, string> parameters = new Dictionary<string, string>();
                if (!string.IsNullOrWhiteSpace(continuationToken))
                {
                    parameters.Add(UriSuffix.ContinuationTokenParameter, continuationToken);
                }

                string uri = this.GetUriByIdWithParameters(resourceTypes, resourceIds, null, apiVersion);

                HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
                var pagedResult = (ServicePagedResult)deserializer.ReadObject(response.GetResponseStream());
                result.AddRange(pagedResult.Items);
                continuationToken = pagedResult.ContinuationToken;
            } while (!string.IsNullOrWhiteSpace(continuationToken));

            return result;
        }

        public ServiceGroupMembersResult GetServiceGroupMembers(string appName, string serviceName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(appName);
            string serviceId = this.FabricNameToId(serviceName);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceGroupMembersUriTemplate, appId, serviceId));

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(ServiceGroupMembersResult));
            ServiceGroupMembersResult result = (ServiceGroupMembersResult)deserializer.ReadObject(response.GetResponseStream());
            return result;
        }

        public List<ServicePartitionResult> GetPartitions(string appName, string serviceName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(appName);
            string serviceId = this.FabricNameToId(serviceName);
            string uri;
            Random random = new Random();
            if (random.Next(3) % 2 == 0)
            {
                uri = uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServicePartitionViaApplicationUriTemplate, appId, serviceId));
            }
            else
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServicePartitionViaServiceUriTemplate, serviceId));
            }

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(IEnumerable<ServicePartitionResult>));
            List<ServicePartitionResult> result = new List<ServicePartitionResult>((IEnumerable<ServicePartitionResult>)deserializer.ReadObject(response.GetResponseStream()));
            return result;
        }

        public ServicePartitionResult GetPartitions(Guid partitionId, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServicePartitionByIdNoServiceName, partitionId));
            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(ServicePartitionResult));
            ServicePartitionResult result = ((ServicePartitionResult)deserializer.ReadObject(response.GetResponseStream()));
            return result;
        }

        public string GetServiceManifest(string applicationTypeName, string applicationTypeVersion, string serviceManifestName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceManifestUriTemplate, applicationTypeName, applicationTypeVersion, serviceManifestName));

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(ManifestResult));
            ManifestResult result = (ManifestResult)deserializer.ReadObject(response.GetResponseStream());
            return result.Manifest;
        }

        public string GetApplicationManifest(string applicationTypeName, string applicationTypeVersion, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetApplicationManifestUriTemplate, applicationTypeName, applicationTypeVersion));

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(ManifestResult));
            ManifestResult result = (ManifestResult)deserializer.ReadObject(response.GetResponseStream());
            return result.Manifest;
        }

        public string GetClusterManifest(HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(UriSuffix.GetClusterManifestUriTemplate);

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(ManifestResult));
            ManifestResult result = (ManifestResult)deserializer.ReadObject(response.GetResponseStream());
            return result.Manifest;
        }

        public List<ServiceResult> GetSystemServices(HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            return this.GetServices(SystemApplicationName, exceptionStatus);
        }

        public List<ServiceTypeResult> GetServiceTypes(string applicationTypeName, string applicationTypeVersion, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceTypesUriTemplate, applicationTypeName, applicationTypeVersion));

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(IEnumerable<ServiceTypeResult>));
            List<ServiceTypeResult> result = new List<ServiceTypeResult>((IEnumerable<ServiceTypeResult>)deserializer.ReadObject(response.GetResponseStream()));
            return result;
        }

        public List<ServicePartitionResult> GetSystemServicePartitions(string serviceName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            return this.GetPartitions(SystemApplicationName, serviceName, exceptionStatus);
        }

        public List<ServiceReplicaResult> GetSystemServiceReplicas(string serviceName, string partitionId, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            return this.GetServiceReplicas(SystemApplicationName, serviceName, partitionId, exceptionStatus);
        }

        public List<ApplicationsOnNodeResult> GetApplicationsOnNode(string nodeName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetApplicationsOnNodeUriTemplate, nodeName));

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(IEnumerable<ApplicationsOnNodeResult>));
            List<ApplicationsOnNodeResult> result = new List<ApplicationsOnNodeResult>((IEnumerable<ApplicationsOnNodeResult>)deserializer.ReadObject(response.GetResponseStream()));
            return result;
        }

        public List<ServiceManifestOnNodeResult> GetServiceManifestsOnNode(string nodeName, string appName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(appName);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceManifestOnNodeUriTemplate, nodeName, appId));

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(IEnumerable<ServiceManifestOnNodeResult>));
            List<ServiceManifestOnNodeResult> result = new List<ServiceManifestOnNodeResult>((IEnumerable<ServiceManifestOnNodeResult>)deserializer.ReadObject(response.GetResponseStream()));
            return result;
        }

        public List<CodePackageOnNodeResult> GetCodePackagesOnNode(string nodeName, string appName, string manifestName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(appName);
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetCodePackagesOnNodeUriTemplate, nodeName, appId, manifestName));

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(IEnumerable<CodePackageOnNodeResult>));
            List<CodePackageOnNodeResult> result = new List<CodePackageOnNodeResult>((IEnumerable<CodePackageOnNodeResult>)deserializer.ReadObject(response.GetResponseStream()));
            return result;
        }

        public List<ServiceReplicasOnNodeResult> GetServiceReplicasOnNode(string nodeName, string appName, string manifestName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(appName);
            if (manifestName != null)
            {
                string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceReplicasOnNodeUriTemplate, nodeName, appId, manifestName));

                HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
                DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(IEnumerable<ServiceReplicasOnNodeResult>));
                List<ServiceReplicasOnNodeResult> result = new List<ServiceReplicasOnNodeResult>((IEnumerable<ServiceReplicasOnNodeResult>)deserializer.ReadObject(response.GetResponseStream()));
                return result;
            }
            else
            {
                string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceReplicasOnNodeNoServicePackageUriTemplate, nodeName, appId));

                HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
                DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(IEnumerable<ServiceReplicasOnNodeResult>));
                List<ServiceReplicasOnNodeResult> result = new List<ServiceReplicasOnNodeResult>((IEnumerable<ServiceReplicasOnNodeResult>)deserializer.ReadObject(response.GetResponseStream()));
                return result;
            }
        }

        public CreateServiceInput GetServiceDescription(string appName, string serviceName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(appName);
            string serviceId = this.FabricNameToId(serviceName);
            string uri;
            Random random = new Random();
            if (random.Next(3) % 2 == 0)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceDescriptionViaApplicationUriTemplate, appId, serviceId));
            }
            else
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceDescriptionViaServiceUriTemplate, serviceId));
            }

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(CreateServiceInput));
            CreateServiceInput result = (CreateServiceInput)deserializer.ReadObject(response.GetResponseStream());
            return result;
        }

        public CreateServiceGroupInput GetServiceGroupDescription(string appName, string serviceName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            var appId = this.FabricNameToId(appName);
            var serviceId = this.FabricNameToId(serviceName);
            string uri;

            var random = new Random();

            if (random.Next(3) % 2 == 0)
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceGroupDescriptionViaApplicationUriTemplate, appId, serviceId));
            }
            else
            {
                uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetServiceGroupDescriptionViaServiceUriTemplate, serviceId));
            }

            var response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            var deserializer = new DataContractJsonSerializer(typeof(CreateServiceGroupInput));

            return (CreateServiceGroupInput)deserializer.ReadObject(response.GetResponseStream());
        }

        public string InvokeInfrastructureQuery(Uri serviceName, string command, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string serviceId = FabricNameToId(serviceName.ToString());

            string uri = this.GetUri(string.Format(
                CultureInfo.InvariantCulture,
                UriSuffix.InvokeInfrastructureQueryTemplate,
                Uri.EscapeDataString(command),
                Uri.EscapeDataString(serviceId)));

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);

            using (var reader = new StreamReader(response.GetResponseStream()))
            {
                return reader.ReadToEnd();
            }
        }

        public ResolveServicePartitionResult ResolveServicePartition(string appName, string serviceName, string previousVersion, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string appId = this.FabricNameToId(appName);
            string serviceId = this.FabricNameToId(serviceName);

            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.SingletonResolveServiceViaServiceUriTemplate, serviceId, previousVersion));

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            ResolveServicePartitionResult result = null;
            if (response != null && response.StatusCode == HttpStatusCode.OK)
            {
                DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(ResolveServicePartitionResult));
                result = (ResolveServicePartitionResult)deserializer.ReadObject(response.GetResponseStream());
            }

            return result;
        }

        public void GetNameExists(Uri name, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.NameUriTemplate, this.FabricNameToId(name.ToString())));
            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
        }

        public PagedSubNameInfoList GetSubNameInfoList(Uri name, string continuationToken = "", bool recursive = false, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            Dictionary<string, string> parameters = new Dictionary<string, string>();
            if (continuationToken != "")
            {
                parameters.Add(UriSuffix.ContinuationTokenParameter, continuationToken);
            }
            if (recursive)
            {
                parameters.Add("Recursive", "true");
            }
            string uri = this.GetUriWithParameters(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetSubNamesUriTemplate, this.FabricNameToId(name.ToString())), parameters, "4.0");

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(PagedSubNameInfoList));
            PagedSubNameInfoList result = (PagedSubNameInfoList)deserializer.ReadObject(response.GetResponseStream());
            return result;
        }

        public PropertyInfo GetPropertyInfo(Uri name, string propertyName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPropertyWithNameUriTemplate, this.FabricNameToId(name.ToString()), propertyName));

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(PropertyInfo));
            PropertyInfo result = (PropertyInfo)deserializer.ReadObject(response.GetResponseStream());
            if (response.StatusCode == HttpStatusCode.OK)
            {
                Verify.IsTrue(result.OnDeserialized());
            }
            return result;
        }

        public BinaryPropertyInfo GetBinaryPropertyInfo(Uri name, string propertyName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPropertyWithNameUriTemplate, this.FabricNameToId(name.ToString()), propertyName));

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(BinaryPropertyInfo));
            BinaryPropertyInfo result = (BinaryPropertyInfo)deserializer.ReadObject(response.GetResponseStream());
            return result;
        }

        public DoublePropertyInfo GetDoublePropertyInfo(Uri name, string propertyName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPropertyWithNameUriTemplate, this.FabricNameToId(name.ToString()), propertyName));

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(DoublePropertyInfo));
            DoublePropertyInfo result = (DoublePropertyInfo)deserializer.ReadObject(response.GetResponseStream());
            return result;
        }

        public PagedPropertyInfoList GetPropertyInfoList(Uri name, string continuationToken = "", bool includeValues = false, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            Dictionary<string, string> parameters = new Dictionary<string, string>();
            if (continuationToken != "")
            {
                parameters.Add(UriSuffix.ContinuationTokenParameter, continuationToken);
            }
            if (includeValues)
            {
                parameters.Add("IncludeValues", "true");
            }
            string uri = this.GetUriWithParameters(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPropertiesUriTemplate, this.FabricNameToId(name.ToString())), parameters, "4.0");

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(PagedPropertyInfoList));
            PagedPropertyInfoList result = (PagedPropertyInfoList)deserializer.ReadObject(response.GetResponseStream());
            if (response.StatusCode == HttpStatusCode.OK)
            {
                Verify.IsTrue(result.OnDeserialized());
            }
            return result;
        }

        public PagedBinaryPropertyInfoList GetBinaryPropertyInfoList(Uri name, string continuationToken = "", bool includeValues = false, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            Dictionary<string, string> parameters = new Dictionary<string, string>();
            if (continuationToken != "")
            {
                parameters.Add(UriSuffix.ContinuationTokenParameter, continuationToken);
            }
            if (includeValues)
            {
                parameters.Add("IncludeValues", "true");
            }
            string uri = this.GetUriWithParameters(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPropertiesUriTemplate, this.FabricNameToId(name.ToString())), parameters, "4.0");

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(PagedBinaryPropertyInfoList));
            PagedBinaryPropertyInfoList result = (PagedBinaryPropertyInfoList)deserializer.ReadObject(response.GetResponseStream());
            return result;
        }

        public PagedDoublePropertyInfoList GetDoublePropertyInfoList(Uri name, string continuationToken = "", bool includeValues = false, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            Dictionary<string, string> parameters = new Dictionary<string, string>();
            if (continuationToken != "")
            {
                parameters.Add(UriSuffix.ContinuationTokenParameter, continuationToken);
            }
            if (includeValues)
            {
                parameters.Add("IncludeValues", "true");
            }
            string uri = this.GetUriWithParameters(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPropertiesUriTemplate, this.FabricNameToId(name.ToString())), parameters, "4.0");

            HttpWebResponse response = this.MakeRequest(uri, HttpGET, null, exceptionStatus);
            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(PagedDoublePropertyInfoList));
            PagedDoublePropertyInfoList result = (PagedDoublePropertyInfoList)deserializer.ReadObject(response.GetResponseStream());
            return result;
        }

        #endregion GET REQUESTS

        #region DELETE REQUESTS

        public void DeleteName(Uri name, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.NameUriTemplate, this.FabricNameToId(name.ToString())));
            HttpWebResponse response = this.MakeRequest(uri, HttpDELETE, null, exceptionStatus, "FABRIC_E_NAME_DOES_NOT_EXIST");
        }

        public void DeleteProperty(Uri name, string propertyName, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPropertyWithNameUriTemplate, this.FabricNameToId(name.ToString()), propertyName));
            HttpWebResponse response = this.MakeRequest(uri, HttpDELETE, null, exceptionStatus, "FABRIC_E_PROPERTY_DOES_NOT_EXIST");
        }

        #endregion

        #region PUT REQUESTS

        public void PutProperty(Uri name, PropertyDescription property, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            property.OnSerialize();
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPropertyUriTemplate, this.FabricNameToId(name.ToString())));
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(PropertyDescription));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, property);

            HttpWebResponse response = this.MakeRequest(uri, HttpPUT, stream, exceptionStatus);
        }

        public void PutProperty(Uri name, BinaryPropertyDescription property, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPropertyUriTemplate, this.FabricNameToId(name.ToString())));
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(BinaryPropertyDescription));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, property);

            HttpWebResponse response = this.MakeRequest(uri, HttpPUT, stream, exceptionStatus);
        }

        public void PutProperty(Uri name, DoublePropertyDescription property, HttpStatusCode exceptionStatus = HttpStatusCode.OK)
        {
            string uri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.GetPropertyUriTemplate, this.FabricNameToId(name.ToString())));
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(DoublePropertyDescription));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, property);

            HttpWebResponse response = this.MakeRequest(uri, HttpPUT, stream, exceptionStatus);
        }

        #endregion

        #region INVALID REQUEST HELPERS

        public void PostRandomInvalidUri()
        {
            string randomString1 = Path.GetRandomFileName();
            string randomString2 = Path.GetRandomFileName();

            string invalidUri = this.GetUri(string.Format(CultureInfo.InvariantCulture, UriSuffix.InvalidUriTemplate, randomString1, randomString2));
            Random random = new Random();

            for (int i = 0; i < random.Next(20); i++)
            {
                randomString1 = Path.GetRandomFileName();
                randomString2 = Path.GetRandomFileName();

                string segments = string.Format(CultureInfo.InvariantCulture, UriSuffix.InvalidUriTemplate, randomString1, randomString2);
                invalidUri += segments;
            }

            Console.WriteLine("Issueing a POST without data to invalid URI - " + invalidUri);

            HttpWebResponse response = this.MakeRequest(invalidUri, HttpPOST, null, HttpStatusCode.BadRequest);
        }

        public void PostInvalidUri<T>(string suffixString, T body)
        {
            string uri = this.GetUri(suffixString);
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(T));
            MemoryStream stream = new MemoryStream();
            serializer.WriteObject(stream, body);

            Console.WriteLine("Sending random data to uri - " + uri);
            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, HttpStatusCode.BadRequest);
        }

        #endregion
        
        // This routine makes the actual http request to the fabric http gateway, with retry. In addition, 
        // * takes in an expected http exception for that request, to help in handling negative test cases.
        // * takes in a Fabric error, that is actually an expected response if the request succeeds during a 
        //   retry - eg: createapprequest->e_abort response->createapprequest_retry->FABRIC_E_APPLICATION_ALREADY_EXISTS
        private HttpWebResponse MakeRequest(string uri, string method, MemoryStream body, HttpStatusCode exceptionStatus, string expectedFabricErrorOnRetry = null)
        {
            // Retry for a max of 5min
            bool inRetry = false;
            string message;
            HttpWebResponse response = null;
            X509Certificate2 clientCertificate = null;
            bool useDefaultCredentials = false;
            string actualHstsHeader;
            for (int i = 0; i < 10; i++)
            {
                response = null;
                HttpWebRequest req = (HttpWebRequest)WebRequest.Create(uri);
                req.Method = method;

                if (method == "POST" || method == "PUT")
                {
                    req.ContentType = "application/json; charset=utf-8";
                    if (body != null)
                    {
                        req.GetRequestStream().Write(body.GetBuffer(), 0, (int)body.Length);
                    }
                    else
                    {
                        req.ContentLength = 0;
                    }
                }

                if (authorizationHeader != null)
                {
                    req.Headers[HttpRequestHeader.Authorization] = authorizationHeader;
                }
                else if (req.ClientCertificates.Count == 0 && clientCertificate != null)
                {
                    req.ClientCertificates.Add(clientCertificate);
                }
                else if (useDefaultCredentials == true)
                {
                    req.UseDefaultCredentials = true;
                }

                try
                {
                    response = (HttpWebResponse)req.GetResponse();
                }
                catch (WebException e)
                {
                    HttpWebResponse errorResponse = (HttpWebResponse)e.Response;
                    if (errorResponse != null)
                    {
                        Console.WriteLine("Error Response for uri: {0} is not a webresponse - {1}::{2}", uri, e.Message, e.Status);

                        if (!string.IsNullOrEmpty(errorResponse.GetResponseHeader("Content-Type")) 
                            && errorResponse.GetResponseHeader("X-Content-Type-Options") != "nosniff")
                        {
                            message = string.Format(CultureInfo.InvariantCulture, "{0} Response does not contain X-Content-Type-Options: nosniff header", uri);
                            Verify.Fail(message);
                        }

                        actualHstsHeader = errorResponse.GetResponseHeader("Strict-Transport-Security");
                        if (actualHstsHeader != hstsHeader)
                        {
                            message = string.Format(CultureInfo.InvariantCulture, "{0} Response does not contain Strict-Transport-Security: {1} header, actual header value = {2}", uri, hstsHeader, actualHstsHeader);
                            Verify.Fail(message);
                        }
                    }

                    if (this.IsRetriableStatus(e))
                    {
                        // Sleep for 30 seconds.
                        if (e.Status == WebExceptionStatus.ProtocolError)
                        {
                            message = string.Format(CultureInfo.InvariantCulture, "Operation {0} failed with {1} - {2}, retrying...", uri, errorResponse.StatusCode.ToString(), errorResponse.StatusDescription);
                        }
                        else
                        {
                            message = string.Format(CultureInfo.InvariantCulture, "Operation {0} failed with {1}, retrying...", uri, e.Status);
                        }
                        Console.WriteLine(message);
                        inRetry = true;
                        Threading.Thread.Sleep(30000);
                        continue;
                    }
                    else if (errorResponse.StatusCode == HttpStatusCode.Forbidden && clientCertificate == null) // certs invalid/access denied if Forbidden after sending certs
                    {
                        message = string.Format(CultureInfo.InvariantCulture, "Operation {0} failed with {1} - {2}, retrying with certs..", uri, errorResponse.StatusCode.ToString(), errorResponse.StatusDescription);
                        Console.WriteLine(message);
                        inRetry = true;
                        clientCertificate = GetClientCertificate();
                        continue;
                    }
                    else if (errorResponse.StatusCode == HttpStatusCode.Unauthorized)
                    {
                        string challenge = errorResponse.GetResponseHeader("WWW-Authenticate");
                        Verify.IsTrue(challenge != null, "No WWW-Authenticate header when response is 401-UnAuthorized");
                        message = string.Format(CultureInfo.InvariantCulture, "Unknown authentication mechanism - Header value - {0}", challenge);
                        Verify.IsTrue(challenge == "Negotiate", message);
                        message = string.Format(CultureInfo.InvariantCulture, "Operation {0} failed with {1} - {2}, retrying with credentials..", uri, errorResponse.StatusCode.ToString(), errorResponse.StatusDescription);
                        Console.WriteLine(message);
                        useDefaultCredentials = true;
                    }
                    else if (errorResponse.StatusCode == exceptionStatus)
                    {
                        message = string.Format(CultureInfo.InvariantCulture, "Status {0} for Operation {2} is an expected failure", errorResponse.StatusCode.ToString(), errorResponse.StatusDescription, uri);
                        Console.WriteLine(message);
                        return errorResponse;
                    }

                    if (inRetry && (errorResponse.StatusDescription != null) && (expectedFabricErrorOnRetry != null))
                    {
                        if (errorResponse.StatusDescription == expectedFabricErrorOnRetry)
                        {
                            message = string.Format(CultureInfo.InvariantCulture, "Operation {0} completed with {1} on retry", uri, expectedFabricErrorOnRetry);
                            Console.WriteLine(message);
                            response = errorResponse;
                            break;
                        }
                    }

                    message = string.Format(CultureInfo.InvariantCulture, "{0} failed with {1}-{2}", uri, errorResponse.StatusCode.ToString(), errorResponse.StatusDescription);
                    Verify.Fail(message);
                    return errorResponse;
                }

                break;
            }

            if (response == null)
            {
                message = string.Format(CultureInfo.InvariantCulture, "{0} falied with {1}", uri, "timeout");
                Verify.Fail(message);
            }

            if (!string.IsNullOrEmpty(response.GetResponseHeader("Content-Type"))
                && response.GetResponseHeader("X-Content-Type-Options") != "nosniff")
            {
                message = string.Format(CultureInfo.InvariantCulture, "{0} Response does not contain X-Content-Type-Options: nosniff header", uri);
                Verify.Fail(message);
            }

            actualHstsHeader = response.GetResponseHeader("Strict-Transport-Security");
            if (actualHstsHeader != hstsHeader)
            {
                message = string.Format(CultureInfo.InvariantCulture, "{0} Response does not contain Strict-Transport-Security: {1} header, actual header value = {2}", uri, hstsHeader, actualHstsHeader);
                Verify.Fail(message);
            }

            return response;
        }

        public static bool ValidateServerCertificate(object sender, X509Certificate certificate, X509Chain chain, SslPolicyErrors sslPolicyErrors)
        {
            if (sslPolicyErrors == SslPolicyErrors.None)
            {
                return true;
            }
            else if(sslPolicyErrors == SslPolicyErrors.RemoteCertificateNameMismatch)
            {
                X509Certificate2 cert2 = certificate as X509Certificate2;
                if (cert2 != null)
                {
                    if (cert2.Subject == Constants.ServerAllowedCommonName ||
                        cert2.GetNameInfo(X509NameType.DnsFromAlternativeName, false) == Constants.ServerAllowedCommonName ||
                        cert2.GetNameInfo(X509NameType.DnsName, false) == Constants.ServerAllowedCommonName)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    Console.WriteLine("Server Certificate subjectname  mismatch - {0}", certificate.Subject);
                    return false;
                }
            }
            else
            {
                Console.WriteLine("Server Certificate error");
                Console.WriteLine("SslPolicyError: {0}, SubjectName: {2}", sslPolicyErrors.ToString(), certificate.Subject);
                return false;
            }
        }

        public static void WaitForDebugger(TimeSpan waitDuration)
        {
            DateTime start = DateTime.UtcNow;
            while (!Debugger.IsAttached)
            {
                Console.WriteLine("Waiting for debugger");
                Thread.Sleep(1000);

                if ((DateTime.UtcNow - start) > waitDuration)
                {
                    Console.WriteLine("Debugger did not attach. Continuing");
                    break;
                }
            }
        }

        private static string RemoveExtraCharacter(string endpoint)
        {
            if (endpoint.LastIndexOf("/", StringComparison.Ordinal) == (endpoint.Length - 1))
            {
                endpoint = endpoint.Substring(0, endpoint.Length - 1);
            }

            return endpoint;
        }

        private X509Certificate2 GetClientCertificate()
        {
            X509Store store = new X509Store(Constants.clientCertificateStoreName, Constants.clientCertificateLocation);
            store.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly);

            X509Certificate2Collection collection = (X509Certificate2Collection)store.Certificates;
            X509Certificate2Collection ret = (X509Certificate2Collection)collection.Find(X509FindType.FindByThumbprint, thumbprint, true);
            return ret[0];
        }

        private bool IsRetriableStatus(WebException e)
        {
            if (e.Status == WebExceptionStatus.ProtocolError)
            {
                var status = ((HttpWebResponse)(e.Response)).StatusCode;
                if ((status == HttpStatusCode.ServiceUnavailable) ||
                (status == HttpStatusCode.GatewayTimeout))
                {
                    return true;
                }
            }
            else if (e.Status == WebExceptionStatus.Timeout)
            {
                return true;
            }

            return false;
        }

        private string GetUriWithParameters(string resource, Dictionary<string, string> queryParameters, string apiVersion, int gatewayNodeIndex = -1)
        {
            if (gatewayNodeIndex < 0)
            {
                Random random = new Random();
                gatewayNodeIndex = random.Next(this.HttpGatewayAddresses.Count());
            }

            string endpoint = this.HttpGatewayAddresses[gatewayNodeIndex];
            endpoint = RESTClient.RemoveExtraCharacter(endpoint);

            StringBuilder sb = new StringBuilder();
            sb.Append(endpoint);
            sb.Append("/");
            sb.Append(resource);

            sb.Append(string.Format(CultureInfo.InvariantCulture, UriSuffix.FirstQueryStringFormat, UriSuffix.ApiVersionUri, apiVersion));

            if (queryParameters != null)
            {
                foreach (var key in queryParameters.Keys)
                {
                    sb.Append(string.Format(CultureInfo.InvariantCulture, UriSuffix.QueryStringFormat, key, queryParameters[key]));
                }
            }

            return new Uri(sb.ToString()).ToString();
        }

        private string GetUriByIdWithParameters(string[] resourceTypes, string[] resourceIds, Dictionary<string, string> queryParameters, string apiVersion = UriSuffix.Api40Version, int gatewayNodeIndex = -1)
        {
            Verify.IsNotNull(resourceTypes, "resourceTypes");
            Verify.IsNotNull(resourceIds, "resourceIds");

            if (gatewayNodeIndex < 0)
            {
                Random random = new Random();
                gatewayNodeIndex = random.Next(this.HttpGatewayAddresses.Count());
            }

            string endpoint = this.HttpGatewayAddresses[gatewayNodeIndex];
            endpoint = RESTClient.RemoveExtraCharacter(endpoint);

            StringBuilder sb = new StringBuilder();
            sb.Append(endpoint);

            for (int i = 0; i < resourceTypes.Length; i++)
            {
                sb.Append("/");
                sb.Append(resourceTypes[i]);

                if (resourceIds.Length > i)
                {
                    sb.Append("/");
                    sb.Append(resourceIds[i]);
                }
            }

            sb.Append(string.Format(CultureInfo.InvariantCulture, UriSuffix.FirstQueryStringFormat, UriSuffix.ApiVersionUri, apiVersion));

            if (queryParameters != null)
            {
                foreach (var key in queryParameters.Keys)
                {
                    sb.Append(string.Format(CultureInfo.InvariantCulture, UriSuffix.QueryStringFormat, key, queryParameters[key]));
                }
            }

            return new Uri(sb.ToString()).ToString();
        }
       
        private string GetUri(string suffix, int gatewayNodeIndex = -1)
        {
            if (gatewayNodeIndex < 0)
            {
                Random random = new Random();
                gatewayNodeIndex = random.Next(this.HttpGatewayAddresses.Count());
            }

            return new Uri(this.HttpGatewayAddresses[gatewayNodeIndex] + suffix).ToString();
        }

        private string FabricNameToId(string name)
        {
            Uri nameUri = new Uri(name);
            char[] temp = { '/' };
            string id = nameUri.AbsolutePath.TrimStart(temp);
            return id;
        }

        private T GetHttpResponseResult<T>(string uri, string method, MemoryStream body, HttpStatusCode exceptionStatus, string expectedFabricErrorOnRetry = null)
        {
            var httpResponse = MakeRequest(uri, method, body, exceptionStatus, expectedFabricErrorOnRetry);
            return this.DeserializeFromHttpResponse<T>(httpResponse);
        }

        private HttpWebResponse PostRequest(string uri, object objectForPostBody, HttpStatusCode exceptionStatus)
        {
            MemoryStream stream = this.SerializeObject(objectForPostBody);
            HttpWebResponse response = this.MakeRequest(uri, HttpPOST, stream, exceptionStatus);
            return response;
        }

        private MemoryStream SerializeObject(object item)
        {
            string jsonString = this.serializer.Serialize(item);
            return GetMemoryStream(jsonString);
        }

        private T DeserializeFromHttpResponse<T>(HttpWebResponse httpResponse)
        {
            string jsonString = new StreamReader(httpResponse.GetResponseStream()).ReadToEnd();
            Type targetType = typeof(T);

            return (T)this.serializer.Deserialize(jsonString, typeof(T));
        }

        // returns MemoryStream which supports GetBuffer() method call.
        private static MemoryStream GetMemoryStream(string str)
        {
            byte[] bytes = encoding.GetBytes(str);
            var buffer = new MemoryStream(bytes, 0, bytes.Length, true, true);
            return buffer;
        }
    }
}