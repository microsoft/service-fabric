// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.ClientSwitch
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Fabric.JsonSerializerWrapper;
    using System.Fabric.ImageStore;
    using System.Fabric.Query;
    using System.Fabric.Testability.Client;
    using System.Fabric.Testability.Client.Structures;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Net.Security;
    using System.Numerics;
    using System.Runtime.Serialization.Json;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Newtonsoft.Json.Linq;
    using Runtime.Serialization;

#if !DotNetCoreClr

    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Fabric.BackupRestore.Enums;

#endif

    public sealed class RestFabricClient : FabricClientObject
    {
        private const string TraceSource = "RestFabricClient";

        private const string HttpGet = "GET";
        private const string HttpPost = "POST";
        private const string HttpDelete = "DELETE";
        private const string HttpPut = "PUT";
        private const string TimeoutError = "Gateway Timeout";
        private const string BadRequestError = "Bad Request";
        private const string FabricError1 = "FABRIC_E";
        private const string FabricError2 = "E_";
        private const string ApplicationUriPrefix = "fabric:/";
        private const string DefaultContentType = "application/json; charset=utf-8";

        private static readonly DataContractJsonSerializer JsonManifestResultSerializer = new DataContractJsonSerializer(typeof(WinFabricManifestResult));
        private static readonly DataContractJsonSerializer JsonUnprovisionApplicationSerializer = new DataContractJsonSerializer(typeof(WinFabricUnprovisionApplication));
        private static readonly DataContractJsonSerializer JsonServiceFromTemplateSerializer = new DataContractJsonSerializer(typeof(WinFabricCreateServiceFromTemplate));
        private static readonly DataContractJsonSerializer JsonProvisionFabricSerializer = new DataContractJsonSerializer(typeof(WinFabricProvisionFabric));
        private static readonly DataContractJsonSerializer JsonUnprovisionFabricSerializer = new DataContractJsonSerializer(typeof(WinFabricUnprovisionFabric));
        private static readonly DataContractJsonSerializer JsonNodeControlDataSerializer = new DataContractJsonSerializer(typeof(WinFabricNodeControlData));
        private static readonly DataContractJsonSerializer JsonRestartNodeControlDataSerializer = new DataContractJsonSerializer(typeof(WinFabricRestartNodeControlData));
        private static readonly DataContractJsonSerializer JsonStartNodeDataSerializer = new DataContractJsonSerializer(typeof(WinFabricStartNodeData));
        private static readonly DataContractJsonSerializer JsonCodePackageControlDataSerializer = new DataContractJsonSerializer(typeof(WinFabricCodePackageControlData));
        private static readonly DataContractJsonSerializer JsonCodeDeactivateDataSerializer = new DataContractJsonSerializer(typeof(DeactivateWrapper));

        private static readonly IJsonSerializer NewtonsoftJsonSerializer = JsonSerializerImplDllLoader.TryGetJsonSerializerImpl();

        private readonly IList<string> httpGatewayAddresses;
        private readonly X509Certificate2 clientCertificate;
        private readonly IList<string> ServerCommonNames;
        private readonly IList<string> ServerCertThumbprints;
        private readonly CredentialType clusterCredentialType;
        private string bearer;
        private IJsonSerializer jsonSerializer;

        public RestFabricClient(
            IEnumerable<string> httpGatewayAddresses,
            SecurityCredentials securityCredentials)
        {
            this.httpGatewayAddresses = new List<string>();
            string scheme = Uri.UriSchemeHttp;

            if (securityCredentials != null)
            {
                this.clusterCredentialType = securityCredentials.CredentialType;
                if (clusterCredentialType == CredentialType.X509)
                {
                    scheme = Uri.UriSchemeHttps;
                    X509Credentials x509Credentials = securityCredentials as X509Credentials;
                    this.clientCertificate = LookUpClientCertificate(x509Credentials);
                    if (x509Credentials.RemoteCommonNames != null && x509Credentials.RemoteCommonNames.Any())
                    {
                        this.ServerCommonNames = x509Credentials.RemoteCommonNames;
                    }
                    else if (x509Credentials.RemoteCertThumbprints != null && x509Credentials.RemoteCertThumbprints.Any())
                    {
                        this.ServerCertThumbprints = x509Credentials.RemoteCertThumbprints;
                    }

                    ServicePointManager.ServerCertificateValidationCallback = new RemoteCertificateValidationCallback(this.ValidateCertificate);
                }
                //ClaimsCredentials can technically be used for both DSTS and AAD, but ScriptTest is only supporting AAD at this point
                else if (clusterCredentialType == CredentialType.Claims)
                {
                    scheme = Uri.UriSchemeHttps;
                    ClaimsCredentials AADCredentials = securityCredentials as ClaimsCredentials;
                    this.ServerCommonNames = AADCredentials.ServerCommonNames;
                    ServicePointManager.ServerCertificateValidationCallback = new RemoteCertificateValidationCallback(this.ValidateCertificate);
                    this.bearer = AADCredentials.LocalClaims;
                }
            }
            else
            {
                clusterCredentialType = CredentialType.None;
            }

            if (httpGatewayAddresses != null)
            {
                UriBuilder builder = null;
                foreach (string address in httpGatewayAddresses)
                {
                    string t = address;
                    int indexOfSeparator = t.LastIndexOf(':');
                    ThrowIf.IsTrue(indexOfSeparator == -1, "Address '{0}' is not properly formatted", address);

                    int port = int.Parse(t.Substring(indexOfSeparator + 1), CultureInfo.InvariantCulture);
                    builder = new UriBuilder(scheme, t.Substring(0, indexOfSeparator), port);

                    this.httpGatewayAddresses.Add(builder.Uri.OriginalString);
                }
            }

            ThrowIf.IsTrue(this.httpGatewayAddresses.Count == 0, "Http Gateway address is invalid");
            this.jsonSerializer = JsonSerializerImplDllLoader.TryGetJsonSerializerImpl();
            ServicePointManager.DefaultConnectionLimit = 100;

            if (this.jsonSerializer == null)
            {
                // Fail if serializer was not loaded successfully.
                throw new DllNotFoundException("RestFabricClient..ctor(): Call to JsonSerializerImplDllLoader.TryGetJsonSerializerImpl() failed.");
            }
        }

        public override async Task<OperationResult> CreateApplicationAsync(ApplicationDescription applicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.Applications, RestPathBuilder.Create };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriByTypes(this.httpGatewayAddresses, resourceTypes);
                    byte[] buffer = this.GetSerializedObject(applicationDescription);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CreateServiceAsync(ServiceDescription winFabricServiceDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // workaround for 9065869
            if (winFabricServiceDescription.ServiceDnsName == null)
            {
                winFabricServiceDescription.ServiceDnsName = string.Empty;
            }

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.Applications, RestPathBuilder.CreateService };
                    string[] resourceIds = new string[] { winFabricServiceDescription.ApplicationName.OriginalString.Substring(8) };

                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);

                    byte[] buffer = this.GetSerializedObject(winFabricServiceDescription);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CreateServiceFromTemplateAsync(
            Uri applicationName,
            Uri serviceName,
            string serviceDnsName,
            string serviceTypeName,
            ServicePackageActivationMode servicePackageActivationMode,
            byte[] initializationData,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    var resourceTypes = new string[] { RestPathBuilder.Applications, RestPathBuilder.CreateServiceFromTemplate };
                    var resourceIds = new string[] { applicationName.OriginalString.Substring(8) };
                    var httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);

                    var createServiceFromTemplateObject = new WinFabricCreateServiceFromTemplate()
                    {
                        ApplicationName = applicationName.OriginalString,
                        ServiceName = serviceName.OriginalString,
                        ServiceDnsName = serviceDnsName == null ? string.Empty : serviceDnsName, // workaround for 9065869
                        ServiceTypeName = serviceTypeName,
                        ServicePackageActivationMode = servicePackageActivationMode,
                        InitializationData = initializationData ?? new byte[0]
                    };

                    byte[] buffer = RestFabricClient.GetSerializedObject(RestFabricClient.JsonServiceFromTemplateSerializer, createServiceFromTemplateObject);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CopyApplicationPackageAsync(string applicationPackagePath, string imageStoreConnectionString, string applicationPackagePathInImageStore, bool compress, TimeSpan timeout, CancellationToken cancellationToken)
        {
            //if the "applicationPackagePathInImageStore" is empty, the store relative path in the URI will be started from the folder name of the "applicationPackagePath".
            //otherwise, in the store relative path, the folder name of the "applicationPackagePath" will be replaced by the "applicationPackagePathInImageStore".
            DirectoryInfo packageDirectory = new DirectoryInfo(applicationPackagePath);
            int prefixIndex = string.IsNullOrEmpty(applicationPackagePathInImageStore)
                ? packageDirectory.FullName.Substring(0, packageDirectory.FullName.LastIndexOf(packageDirectory.Name)).Length
                : packageDirectory.FullName.Length;
            TestabilityTrace.TraceSource.WriteInfo(TraceSource, "Process directory {0} with prefix index {1}", packageDirectory.FullName, prefixIndex);

#if !DotNetCoreClr
            // Todo: RDBug 10584947:Application package compress currently disabled for CoreClr
            if (compress)
            {
                ImageStoreUtility.ArchiveApplicationPackage(applicationPackagePath, null);
            }
#endif

            Dictionary<FileInfo, string> filesToUpload = new Dictionary<FileInfo, string>();
            ProcessDirectory(packageDirectory, prefixIndex, applicationPackagePathInImageStore, filesToUpload);
            string[] resourceTypes = new string[] { RestPathBuilder.ImageStore };
            foreach (KeyValuePair<FileInfo, string> file in filesToUpload)
            {
                NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceIds = new string[] { file.Value };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);
                    TestabilityTrace.TraceSource.WriteInfo(TraceSource, "Upload file {0} to {1}", file.Key.FullName, httpGatewayEndpoint.ToString());
                    byte[] data = new byte[file.Key.Length];
                    using (FileStream sourceStream = new FileStream(file.Key.FullName, FileMode.Open, FileAccess.Read, FileShare.None))
                    {
                        sourceStream.Read(data, 0, data.Length);
                    }

                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPut, data, "application/octet-stream");
                });

                if (errorCode != NativeTypes.FABRIC_ERROR_CODE.S_OK)
                {
                    return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
                }
            }

            return FabricClientState.CreateOperationResultFromNativeErrorCode(NativeTypes.FABRIC_ERROR_CODE.S_OK);
        }

        public override async Task<OperationResult> DeleteApplicationAsync(DeleteApplicationDescription deleteApplicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.Applications, RestPathBuilder.Delete };
                    string[] resourceIds = new string[] { deleteApplicationDescription.ApplicationName.AbsolutePath };
                    Dictionary<string, string> queryParameters = new Dictionary<string, string>();
                    queryParameters.Add(RestPathBuilder.ForceRemove, deleteApplicationDescription.ForceDelete.ToString());

                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, null);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> DeleteServiceAsync(DeleteServiceDescription deleteServiceDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.Services, RestPathBuilder.Delete };
                    string[] resourceIds = new string[] { deleteServiceDescription.ServiceName.OriginalString.Substring(8) };
                    Dictionary<string, string> queryParameters = new Dictionary<string, string>();
                    queryParameters.Add(RestPathBuilder.ForceRemove, deleteServiceDescription.ForceDelete.ToString());

                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, null);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> ActivateNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.Activate };
                    string[] resourceIds = new string[] { nodeName };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, null);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> DeactivateNodeAsync(string nodeName, NodeDeactivationIntent deactivationIntent, TimeSpan timeout, CancellationToken cancellationToken)
        {
            DeactivateWrapper body = new DeactivateWrapper();
            body.DeactivationIntent = deactivationIntent;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.Deactivate };
                    string[] resourceIds = new string[] { nodeName };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);
                    byte[] buffer = RestFabricClient.GetSerializedObject(RestFabricClient.JsonCodeDeactivateDataSerializer, body);

                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RemoveNodeStateAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.NodeStateRemoved };
                    string[] resourceIds = new string[] { nodeName };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, null);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> ProvisionApplicationAsync(string buildPath, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await ProvisionApplicationAsync(new ProvisionApplicationTypeDescription(buildPath), timeout, cancellationToken);
        }

        public override async Task<OperationResult> ProvisionApplicationAsync(ProvisionApplicationTypeDescriptionBase description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.ApplicationTypes, RestPathBuilder.Provision };
                    Dictionary<string, string> queryParameters = new Dictionary<string, string>();
                    queryParameters.Add(RestPathBuilder.TimeoutQueryParameter, string.Format("{0}", timeout.TotalSeconds));
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriByTypes(this.httpGatewayAddresses, resourceTypes, queryParameters);
                    byte[] buffer = this.GetSerializedObject(description);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> ProvisionFabricAsync(string codeFilePath, string clusterManifestFilePath, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    if (string.IsNullOrEmpty(codeFilePath) && string.IsNullOrEmpty(clusterManifestFilePath))
                    {
                        throw new InvalidOperationException("CodePackagePathParameter and ClusterManifestPathParameter both cannot be empty at the same time");
                    }

                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, RestPathBuilder.Provision);

                    string codeFilePathLocal = codeFilePath == null ? string.Empty : codeFilePath;
                    string clusterManifestFilePathLocal = clusterManifestFilePath == null ? string.Empty : clusterManifestFilePath;

                    WinFabricProvisionFabric argument = new WinFabricProvisionFabric(codeFilePathLocal, clusterManifestFilePathLocal);

                    byte[] buffer = RestFabricClient.GetSerializedObject(RestFabricClient.JsonProvisionFabricSerializer, argument);
                    await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UnprovisionApplicationAsync(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.UnprovisionApplicationAsync(new UnprovisionApplicationTypeDescription(applicationTypeName, applicationTypeVersion), timeout, cancellationToken);
        }

        public override async Task<OperationResult> UnprovisionApplicationAsync(UnprovisionApplicationTypeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.ApplicationTypes, RestPathBuilder.Unprovision };
                    string[] resourceIds = new string[] { description.ApplicationTypeName };
                    Dictionary<string, string> queryParameters = new Dictionary<string, string>();
                    queryParameters.Add(RestPathBuilder.TimeoutQueryParameter, string.Format("{0}", timeout.TotalSeconds));
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters);
                    WinFabricUnprovisionApplication argument = new WinFabricUnprovisionApplication()
                    {
                        ApplicationTypeVersion = description.ApplicationTypeVersion,
                        Async = description.Async
                    };

                    byte[] buffer = RestFabricClient.GetSerializedObject(RestFabricClient.JsonUnprovisionApplicationSerializer, argument);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UnprovisionFabricAsync(string codeVersion, string configVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
               async delegate
               {
                   Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, RestPathBuilder.Unprovision);
                   string codeVersionLocal = codeVersion == null ? string.Empty : codeVersion;
                   string configVersionLocal = configVersion == null ? string.Empty : configVersion;

                   WinFabricUnprovisionFabric argument = new WinFabricUnprovisionFabric(codeVersionLocal, configVersionLocal);

                   byte[] buffer = RestFabricClient.GetSerializedObject(RestFabricClient.JsonUnprovisionFabricSerializer, argument);
                   await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
               });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UpgradeApplicationAsync(System.Fabric.Description.ApplicationUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.Applications, RestPathBuilder.Upgrade };
                    string[] resourceIds = new string[] { description.ApplicationName.OriginalString.Substring(8) };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);

                    byte[] buffer = this.GetSerializedObject(description);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UpdateApplicationUpgradeAsync(ApplicationUpgradeUpdateDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.Applications, RestPathBuilder.UpdateUpgrade };
                    string[] resourceIds = new string[] { description.ApplicationName.OriginalString.Substring(8) };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);
                    byte[] buffer = this.GetSerializedObject(description);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UpdateServiceAsync(Uri name, ServiceUpdateDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.Services, RestPathBuilder.Update };
                    string[] resourceIds = new string[] { name.OriginalString.Substring(8) };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);

                    byte[] buffer = this.GetSerializedObject(description);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> PutApplicationResourceAsync(string applicationName, string descriptionJson, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.ApplicationResource, applicationName };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriByTypes(this.httpGatewayAddresses, resourceTypes);
                    byte[] buffer = Encoding.ASCII.GetBytes(descriptionJson);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPut, buffer);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> DeleteApplicationResourceAsync(string applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.ApplicationResource};
                    string[] resourceIds = new string[] { applicationName };

                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpDelete, null);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<ApplicationResourceList>> GetApplicationResourceListAsync(string applicationName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Dictionary<string, string> queryParameters = new Dictionary<string, string>();
            string[] resourceTypes = new string[] { RestPathBuilder.ApplicationResource };

            if (!string.IsNullOrEmpty(applicationName))
            {
                string[] resourceIds = new string[] { applicationName };
                Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters);

                var singleItemResult = await this.GetOperationResultFromRequest<ApplicationResource>(httpGatewayEndpoint, null);
                return ConvertOperationResultPagedList<ApplicationResourceList, ApplicationResource>(singleItemResult);
            }
            else
            {
                if (!string.IsNullOrEmpty(continuationToken))
                {
                    queryParameters.Add(RestPathBuilder.ContinuationToken, continuationToken);
                }

                string[] resourceIds = new string[0];
                Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters);
                return await this.GetOperationPagedResultListFromRequest<ApplicationResourceList>(httpGatewayEndpoint, null);
            }
        }

        public override async Task<OperationResult<ServiceResourceList>> GetServiceResourceListAsync(string applicationName, string serviceName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(applicationName, "applicationName");

            Dictionary<string, string> queryParameters = new Dictionary<string, string>();
            string[] resourceTypes = new string[] { RestPathBuilder.ApplicationResource, RestPathBuilder.ServiceResource};

            if (!string.IsNullOrEmpty(serviceName))
            {
                string[] resourceIds = new string[] { applicationName, serviceName };
                Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters);

                var singleItemResult = await this.GetOperationResultFromRequest<ServiceResource>(httpGatewayEndpoint, null);
                return ConvertOperationResultPagedList<ServiceResourceList, ServiceResource>(singleItemResult);
            }
            else
            {
                if (!string.IsNullOrEmpty(continuationToken))
                {
                    queryParameters.Add(RestPathBuilder.ContinuationToken, continuationToken);
                }

                string[] resourceIds = new string[] { applicationName };
                Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters);
                return await this.GetOperationPagedResultListFromRequest<ServiceResourceList>(httpGatewayEndpoint, null);
            }
        }

        public override async Task<OperationResult<ClusterHealth>> GetClusterHealthAsync(ClusterHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Dictionary<string, string> queryParameters = new Dictionary<string, string>();

            if (queryDescription.EventsFilter != null)
            {
                queryParameters.Add(RestPathBuilder.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue.ToString("D"));
            }

            if (queryDescription.NodesFilter != null)
            {
                queryParameters.Add(RestPathBuilder.NodesHealthStateFilter, queryDescription.NodesFilter.HealthStateFilterValue.ToString("D"));
            }

            if (queryDescription.ApplicationsFilter != null)
            {
                Random r = new Random((int)DateTime.Now.Ticks);
                bool add = true;
                if (queryDescription.ApplicationsFilter.HealthStateFilterValue == HealthStateFilter.Default)
                {
                    add = (r.Next(2) == 0);
                }

                if (add)
                {
                    queryParameters.Add(RestPathBuilder.ApplicationsHealthStateFilter, queryDescription.ApplicationsFilter.HealthStateFilterValue.ToString("D"));
                }
            }

            if (queryDescription.HealthStatisticsFilter != null)
            {
                queryParameters.Add(RestPathBuilder.ExcludeHealthStatistics, queryDescription.HealthStatisticsFilter.ExcludeHealthStatistics.ToString());
                queryParameters.Add(RestPathBuilder.IncludeSystemApplicationHealthStatistics, queryDescription.HealthStatisticsFilter.IncludeSystemApplicationHealthStatistics.ToString());
            }

            // TODO: 5604082 : Testability REST client: GetClusterHealth should pass ApplicationHealthPolicyMap in the body
            // and set apiversion to 2.0
            if (queryDescription.ApplicationHealthPolicyMap.Count > 0)
            {
                throw new NotImplementedException("ApplicationHealthPolicyMap is not supported in REST testability client yet.");
            }

            string[] resourceTypes = new string[] { RestPathBuilder.GetClusterHealth };
            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriByTypes(this.httpGatewayAddresses, resourceTypes, queryParameters, RestPathBuilder.ApiVersionDefault);
            return await this.GetOperationResultFromRequest<ClusterHealth>(httpGatewayEndpoint, queryDescription.HealthPolicy);
        }

        public override async Task<OperationResult<ClusterHealthChunk>> GetClusterHealthChunkAsync(ClusterHealthChunkQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Dictionary<string, string> queryParameters = new Dictionary<string, string>();
            string[] resourceTypes = new string[] { RestPathBuilder.GetClusterHealthChunk };
            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriByTypes(this.httpGatewayAddresses, resourceTypes, queryParameters, RestPathBuilder.ApiVersionDefault);
            return await this.GetOperationResultFromRequest<ClusterHealthChunk>(httpGatewayEndpoint, queryDescription);
        }

        public override async Task<OperationResult<NodeHealth>> GetNodeHealthAsync(NodeHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string[] resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.GetHealth };
            string[] resourceIds = new string[] { queryDescription.NodeName };

            Dictionary<string, string> queryParameters = new Dictionary<string, string>();

            if (queryDescription.EventsFilter != null)
            {
                queryParameters.Add(RestPathBuilder.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue.ToString("D"));
            }

            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.ApiVersionDefault);
            return await this.GetOperationResultFromRequest<NodeHealth>(httpGatewayEndpoint, queryDescription.HealthPolicy);
        }

        public override async Task<OperationResult<ApplicationHealth>> GetApplicationHealthAsync(ApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string applicationId = RestFabricClient.EscapeAndStripRootUri(queryDescription.ApplicationName);
            string[] resourceTypes = new string[] { RestPathBuilder.Applications, RestPathBuilder.GetHealth };
            string[] resourceIds = new string[] { applicationId };

            Dictionary<string, string> queryParameters = new Dictionary<string, string>();

            if (queryDescription.EventsFilter != null)
            {
                queryParameters.Add(RestPathBuilder.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue.ToString("D"));
            }

            if (queryDescription.ServicesFilter != null)
            {
                queryParameters.Add(RestPathBuilder.ServicesHealthStateFilter, queryDescription.ServicesFilter.HealthStateFilterValue.ToString("D"));
            }

            if (queryDescription.DeployedApplicationsFilter != null)
            {
                queryParameters.Add(RestPathBuilder.DeployedApplicationsHealthStateFilter, queryDescription.DeployedApplicationsFilter.HealthStateFilterValue.ToString("D"));
            }

            if (queryDescription.HealthStatisticsFilter != null)
            {
                queryParameters.Add(RestPathBuilder.ExcludeHealthStatistics, queryDescription.HealthStatisticsFilter.ExcludeHealthStatistics.ToString());
            }

            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.ApiVersionDefault);
            return await this.GetOperationResultFromRequest<ApplicationHealth>(httpGatewayEndpoint, queryDescription.HealthPolicy);
        }

        public override async Task<OperationResult<DeployedApplicationHealth>> GetDeployedApplicationHealthAsync(DeployedApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.GetApplications, RestPathBuilder.GetHealth };
            string applicationId = RestFabricClient.EscapeAndStripRootUri(queryDescription.ApplicationName.OriginalString);
            var resourceIds = new string[] { queryDescription.NodeName, applicationId };

            Dictionary<string, string> queryParameters = new Dictionary<string, string>();

            if (queryDescription.EventsFilter != null)
            {
                queryParameters.Add(RestPathBuilder.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue.ToString("D"));
            }

            if (queryDescription.DeployedServicePackagesFilter != null)
            {
                queryParameters.Add(RestPathBuilder.DeployedServicePackagesHealthStateFilter, queryDescription.DeployedServicePackagesFilter.HealthStateFilterValue.ToString("D"));
            }

            if (queryDescription.HealthStatisticsFilter != null)
            {
                queryParameters.Add(RestPathBuilder.ExcludeHealthStatistics, queryDescription.HealthStatisticsFilter.ExcludeHealthStatistics.ToString());
            }

            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.ApiVersionDefault);
            return await this.GetOperationResultFromRequest<DeployedApplicationHealth>(httpGatewayEndpoint, queryDescription.HealthPolicy);
        }

        public override async Task<OperationResult<ApplicationLoadInformation>> GetApplicationLoadAsync(string applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(applicationName, "applicationName");

            string[] resourceIds = new string[] { applicationName };
            string[] resourceTypes = new string[] { RestPathBuilder.Applications, RestPathBuilder.GetLoadInformation };
            return await this.GetOperationResultFromRequest<ApplicationLoadInformation>(resourceTypes, resourceIds, null);
        }

        public override async Task<OperationResult> StartPartitionDataLossRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            DataLossMode dataLossMode,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string serviceId = RestFabricClient.EscapeAndStripRootUri(serviceName.OriginalString);
                    string suffix = string.Format(CultureInfo.InvariantCulture, "/Faults/Services/{0}/$/GetPartitions/{1}/$/StartDataLoss", serviceId, partitionId.ToString());
                    string queryParameters = string.Format(CultureInfo.InvariantCulture, "?api-version=1.0&OperationId={0}&DataLossMode={1}", operationId.ToString(), dataLossMode.ToString());

                    Uri httpGatewayEndpoint = RestPathBuilder.BuildRequestEndpoint(this.httpGatewayAddresses, suffix, queryParameters);

                    Console.WriteLine("Sending '{0}'", httpGatewayEndpoint.ToString());
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> StartPartitionQuorumLossRestAsync(
           Guid operationId,
           Uri serviceName,
           Guid partitionId,
           QuorumLossMode quorumlossMode,
           TimeSpan quorumlossDuration,
           TimeSpan operationTimeout,
           CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string serviceId = RestFabricClient.EscapeAndStripRootUri(serviceName.OriginalString);
                    string suffix = string.Format(CultureInfo.InvariantCulture, "/Faults/Services/{0}/$/GetPartitions/{1}/$/StartQuorumLoss", serviceId, partitionId.ToString());
                    string queryParameters = string.Format(CultureInfo.InvariantCulture, "?api-version=1.0&OperationId={0}&QuorumLossMode={1}&QuorumLossDuration={2}", operationId.ToString(), quorumlossMode.ToString(), quorumlossDuration.TotalSeconds);

                    Uri httpGatewayEndpoint = RestPathBuilder.BuildRequestEndpoint(this.httpGatewayAddresses, suffix, queryParameters);

                    Console.WriteLine("Sending - '{0}'", httpGatewayEndpoint.ToString());

                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> StartPartitionRestartRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            RestartPartitionMode restartPartitionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string serviceId = RestFabricClient.EscapeAndStripRootUri(serviceName.OriginalString);
                    string suffix = string.Format(CultureInfo.InvariantCulture, "/Faults/Services/{0}/$/GetPartitions/{1}/$/StartRestart", serviceId, partitionId.ToString());
                    string queryParameters = string.Format(CultureInfo.InvariantCulture, "?api-version=1.0&OperationId={0}&RestartPartitionMode={1}", operationId.ToString(), restartPartitionMode.ToString());

                    Uri httpGatewayEndpoint = RestPathBuilder.BuildRequestEndpoint(this.httpGatewayAddresses, suffix, queryParameters);

                    Console.WriteLine("Sending - '{0}'", httpGatewayEndpoint.ToString());
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override Task<OperationResult<PartitionDataLossProgress>> GetPartitionDataLossProgressRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return Inner<PartitionDataLossProgress>("GetDataLossProgress", operationId, serviceName, partitionId, timeout, cancellationToken);
        }

        // This function is not calling Inner() because there is a test case that intentionally faults quorum loss commands, thus causing the progress result to have a State of Faulted.
        // In that case, there is an assert inside the
        public override Task<OperationResult<PartitionQuorumLossProgress>> GetPartitionQuorumLossProgressRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return Inner<PartitionQuorumLossProgress>("GetQuorumLossProgress", operationId, serviceName, partitionId, timeout, cancellationToken);
        }

        public override Task<OperationResult<PartitionRestartProgress>> GetPartitionRestartProgressRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return Inner<PartitionRestartProgress>("GetRestartProgress", operationId, serviceName, partitionId, timeout, cancellationToken);
        }

        public override async Task<OperationResult> StartNodeTransitionAsync(
            NodeTransitionDescription nodeTransitionDescription,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string suffix = string.Format(CultureInfo.InvariantCulture, "/Faults/Nodes/{0}/$/StartTransition", nodeTransitionDescription.NodeName);
                    string queryParameters = string.Format(
                        CultureInfo.InvariantCulture,
                        "?api-version=1.0&OperationId={0}&NodeTransitionType={1}&NodeInstanceId={2}",
                        nodeTransitionDescription.OperationId.ToString(),
                        nodeTransitionDescription.NodeTransitionType.ToString(),
                        nodeTransitionDescription.NodeInstanceId.ToString());
                    if (nodeTransitionDescription.NodeTransitionType == NodeTransitionType.Stop)
                    {
                        NodeStopDescription nodeStopTransitionDescription = (NodeStopDescription)nodeTransitionDescription;
                        queryParameters += string.Format(
                            CultureInfo.InvariantCulture,
                            "&StopDurationInSeconds={0}",
                            nodeStopTransitionDescription.StopDurationInSeconds);
                    }

                    Uri httpGatewayEndpoint = RestPathBuilder.BuildRequestEndpoint(this.httpGatewayAddresses, suffix, queryParameters);

                    Console.WriteLine("Sending - '{0}'", httpGatewayEndpoint.ToString());
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<NodeTransitionProgress>> GetNodeTransitionProgressAsync(
            Guid operationId,
            string nodeName,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            OperationResult<NodeTransitionProgress> result = null;
            NodeTransitionProgress progressResult = default(NodeTransitionProgress);

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    string suffix = string.Format(CultureInfo.InvariantCulture, "/Faults/Nodes/{0}/$/GetTransitionProgress", nodeName);
                    string queryParameters = string.Format(
                        CultureInfo.InvariantCulture,
                        "?api-version=1.0&OperationId={0}",
                        operationId.ToString());

                    Uri httpGatewayEndpoint = RestPathBuilder.BuildRequestEndpoint(this.httpGatewayAddresses, suffix, queryParameters);

                    Console.WriteLine("Sending - '{0}'", httpGatewayEndpoint.ToString());
                    HttpWebResponse response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet);
                    progressResult = this.GetDeserializedObject<NodeTransitionProgress>(response.GetResponseStream());
                });

            if (progressResult != null)
            {
                if (progressResult.State == TestCommandProgressState.Faulted)
                {
                    NodeTransitionProgress progress = progressResult as NodeTransitionProgress;
                    if (progress.Result.ErrorCode != 0)
                    {
                        Exception exception = InteropHelpers.TranslateError(progress.Result.ErrorCode);
                        progress.Result.Exception = exception;
                    }
                }

                result = FabricClientState.CreateOperationResultFromNativeErrorCode<NodeTransitionProgress>(progressResult);
            }
            else
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<NodeTransitionProgress>(errorCode);
            }

            return result;
        }

        public override async Task<OperationResult> CancelTestCommandAsync(
            Guid operationId,
            bool force,
            TimeSpan timeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string suffix = string.Format(CultureInfo.InvariantCulture, "/Faults/$/Cancel");
                    string queryParameters = string.Format(CultureInfo.InvariantCulture, "?api-version=1.0&OperationId={0}&Force={1}", operationId.ToString(), force);

                    Uri httpGatewayEndpoint = RestPathBuilder.BuildRequestEndpoint(this.httpGatewayAddresses, suffix, queryParameters);

                    Console.WriteLine("Sending - '{0}'", httpGatewayEndpoint.ToString());
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<TestCommandStatusList>> GetTestCommandStatusListAsync(TestCommandStateFilter stateFilter, TestCommandTypeFilter typeFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            OperationResult<TestCommandStatusList> result = null;
            TestCommandStatus[] statusResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    string suffix = string.Format(CultureInfo.InvariantCulture, "/Faults/");
                    string queryParameters = string.Format(CultureInfo.InvariantCulture, "?api-version=1.0&StateFilter={0}&TypeFilter={1}", (int)stateFilter, (int)typeFilter);

                    Uri httpGatewayEndpoint = RestPathBuilder.BuildRequestEndpoint(this.httpGatewayAddresses, suffix, queryParameters);

                    Console.WriteLine("Sending - '{0}'", httpGatewayEndpoint.ToString());
                    HttpWebResponse response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet);
                    statusResult = this.GetDeserializedObject<TestCommandStatus[]>(response.GetResponseStream());
                });

            if (statusResult != null)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<TestCommandStatusList>(new TestCommandStatusList(statusResult));
            }
            else
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<TestCommandStatusList>(errorCode);
            }

            return result;
        }

        public override async Task<OperationResult<ChaosDescription>> GetChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            ChaosDescription description = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    var resourceTypes = new[] { RestPathBuilder.Tools };
                    string toolId = RestPathBuilder.Chaos;
                    var resourceIds = new[] { toolId };

                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, null, RestPathBuilder.Api62Version);

                    HttpWebResponse response = await this.MakeRequest(httpGatewayEndpoint, HttpGet).ConfigureAwait(false);

                    description = this.GetDeserializedObject<ChaosDescription>(response.GetResponseStream());
                }).ConfigureAwait(false);

            return description != null
                ? FabricClientState.CreateOperationResultFromNativeErrorCode(description)
                : FabricClientState.CreateOperationResultFromNativeErrorCode<ChaosDescription>(errorCode);
        }

        public override async Task<OperationResult<ChaosScheduleDescription>> GetChaosScheduleAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            ChaosScheduleDescription description = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    var resourceTypes = new[] { RestPathBuilder.Tools };
                    string toolId = RestPathBuilder.ChaosSchedule;
                    var resourceIds = new[] { toolId };

                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, null, RestPathBuilder.Api62Version);

                    HttpWebResponse response = await this.MakeRequest(httpGatewayEndpoint, HttpGet).ConfigureAwait(false);

                    description = this.GetDeserializedObject<ChaosScheduleDescription>(response.GetResponseStream());
                });

            return description != null
                ? FabricClientState.CreateOperationResultFromNativeErrorCode(description)
                : FabricClientState.CreateOperationResultFromNativeErrorCode<ChaosScheduleDescription>(errorCode);
        }

        public override async Task<OperationResult> SetChaosScheduleAsync(
            string scheduleJson,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                                                        delegate
                                                        {
                                                            var resourceTypes = new[] { RestPathBuilder.Tools };
                                                            string toolId = RestPathBuilder.ChaosSchedule;
                                                            var resourcesIds = new[] { toolId };

                                                            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourcesIds, null, RestPathBuilder.Api62Version);

                                                            byte[] buffer = Encoding.ASCII.GetBytes(scheduleJson);

                                                            return MakeRequest(httpGatewayEndpoint, HttpPost, buffer);
                                                        });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> StartChaosAsync(
            ChaosParameters chaosParameters,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                                                          delegate
                                                          {
                                                              var resourceTypes = new[] { RestPathBuilder.Tools, RestPathBuilder.Start };
                                                              string toolId = RestPathBuilder.Chaos;
                                                              var resourceIds = new[] { toolId };

                                                              Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, null, RestPathBuilder.ApiVersionDefault);

                                                              byte[] buffer = this.GetSerializedObject(chaosParameters);

                                                              return this.MakeRequest(httpGatewayEndpoint, HttpPost, buffer);
                                                          }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<ChaosEventsSegment>> GetChaosEventsAsync(
            DateTime startTimeUtc,
            DateTime endTimeUtc,
            string continuationToken,
            long maxResults,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            ChaosEventsSegment report = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                                                        async delegate
                                                        {
                                                            Dictionary<string, string> queryParameters = new Dictionary<string, string>();

                                                            if (string.IsNullOrEmpty(continuationToken))
                                                            {
                                                                queryParameters.Add(RestPathBuilder.StartTimeUtc, ToFileTimeTicks(startTimeUtc).ToString());
                                                                queryParameters.Add(RestPathBuilder.EndTimeUtc, ToFileTimeTicks(endTimeUtc).ToString());
                                                            }
                                                            else
                                                            {
                                                                queryParameters.Add(RestPathBuilder.ContinuationToken, continuationToken);
                                                            }

                                                            queryParameters.Add(RestPathBuilder.MaxResults, maxResults.ToString());

                                                            string[] resourceIds = { RestPathBuilder.ChaosEvents };
                                                            string[] resourceTypes = { RestPathBuilder.Tools };

                                                            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.Api62Version);

                                                            HttpWebResponse response = await this.MakeRequest(httpGatewayEndpoint, HttpGet).ConfigureAwait(false);

                                                            report = this.GetDeserializedObject<ChaosEventsSegment>(response.GetResponseStream());
                                                        }).ConfigureAwait(false);

            return report != null
                ? FabricClientState.CreateOperationResultFromNativeErrorCode(report)
                : FabricClientState.CreateOperationResultFromNativeErrorCode<ChaosEventsSegment>(errorCode);
        }

        public override async Task<OperationResult> StopChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                                                          delegate
                                                          {
                                                              var resourceTypes = new[] { RestPathBuilder.Tools, RestPathBuilder.Stop };
                                                              string toolId = RestPathBuilder.Chaos;
                                                              var resourceIds = new[] { toolId };

                                                              Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, null, RestPathBuilder.ApiVersionDefault);

                                                              return this.MakeRequest(httpGatewayEndpoint, HttpPost);
                                                          }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<ChaosReport>> GetChaosReportAsync(
            DateTime startTimeUtc,
            DateTime endTimeUtc,
            string continuationToken,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            ChaosReport report = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                                                          async delegate
                                                              {
                                                                  Dictionary<string, string> queryParameters = new Dictionary<string, string>();

                                                                  if (string.IsNullOrEmpty(continuationToken))
                                                                  {
                                                                      queryParameters.Add(RestPathBuilder.StartTimeUtc, ToFileTimeTicks(startTimeUtc).ToString());
                                                                      queryParameters.Add(RestPathBuilder.EndTimeUtc, ToFileTimeTicks(endTimeUtc).ToString());
                                                                  }
                                                                  else
                                                                  {
                                                                      queryParameters.Add(RestPathBuilder.ContinuationToken, continuationToken);
                                                                  }

                                                                  string[] resourceIds = { RestPathBuilder.Chaos };
                                                                  string[] resourceTypes = { RestPathBuilder.Tools, RestPathBuilder.Report };

                                                                  Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.Api61Version);

                                                                  HttpWebResponse response = await this.MakeRequest(httpGatewayEndpoint, HttpGet).ConfigureAwait(false);

                                                                  report = this.GetDeserializedObject<ChaosReport>(response.GetResponseStream());
                                                              }).ConfigureAwait(false);

            return report != null
                ? FabricClientState.CreateOperationResultFromNativeErrorCode(report)
                : FabricClientState.CreateOperationResultFromNativeErrorCode<ChaosReport>(errorCode);
        }

        public override async Task<OperationResult<DeployedServicePackageHealth>> GetDeployedServicePackageHealthAsync(DeployedServicePackageHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.GetApplications, RestPathBuilder.GetServicePackages, RestPathBuilder.GetHealth };
            string applicationId = RestFabricClient.EscapeAndStripRootUri(queryDescription.ApplicationName.OriginalString);
            var resourceIds = new string[] { queryDescription.NodeName, applicationId, queryDescription.ServiceManifestName };

            Dictionary<string, string> queryParameters = new Dictionary<string, string>();

            if (queryDescription.EventsFilter != null)
            {
                queryParameters.Add(RestPathBuilder.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue.ToString("D"));
            }

            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.ApiVersionDefault);
            return await this.GetOperationResultFromRequest<DeployedServicePackageHealth>(httpGatewayEndpoint, queryDescription.HealthPolicy);
        }

        public override async Task<OperationResult<ServiceHealth>> GetServiceHealthAsync(ServiceHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string serviceId = RestFabricClient.EscapeAndStripRootUri(queryDescription.ServiceName.OriginalString);
            string[] resourceTypes = new string[] { RestPathBuilder.Services, RestPathBuilder.GetHealth };
            string[] resourceIds = new string[] { serviceId };

            Dictionary<string, string> queryParameters = new Dictionary<string, string>();

            if (queryDescription.EventsFilter != null)
            {
                queryParameters.Add(RestPathBuilder.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue.ToString("D"));
            }

            if (queryDescription.PartitionsFilter != null)
            {
                queryParameters.Add(RestPathBuilder.PartitionsHealthStateFilter, queryDescription.PartitionsFilter.HealthStateFilterValue.ToString("D"));
            }

            if (queryDescription.HealthStatisticsFilter != null)
            {
                queryParameters.Add(RestPathBuilder.ExcludeHealthStatistics, queryDescription.HealthStatisticsFilter.ExcludeHealthStatistics.ToString());
            }

            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.ApiVersionDefault);
            return await this.GetOperationResultFromRequest<ServiceHealth>(httpGatewayEndpoint, queryDescription.HealthPolicy);
        }

        public override async Task<OperationResult<PartitionHealth>> GetPartitionHealthAsync(PartitionHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string[] resourceTypes = new string[] { RestPathBuilder.Partitions, RestPathBuilder.GetHealth };
            string[] resourceIds = new string[] { queryDescription.PartitionId.ToString() };

            Dictionary<string, string> queryParameters = new Dictionary<string, string>();

            if (queryDescription.EventsFilter != null)
            {
                queryParameters.Add(RestPathBuilder.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue.ToString("D"));
            }

            if (queryDescription.ReplicasFilter != null)
            {
                queryParameters.Add(RestPathBuilder.ReplicasHealthStateFilter, queryDescription.ReplicasFilter.HealthStateFilterValue.ToString("D"));
            }

            if (queryDescription.HealthStatisticsFilter != null)
            {
                queryParameters.Add(RestPathBuilder.ExcludeHealthStatistics, queryDescription.HealthStatisticsFilter.ExcludeHealthStatistics.ToString());
            }

            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.ApiVersionDefault);
            return await this.GetOperationResultFromRequest<PartitionHealth>(httpGatewayEndpoint, queryDescription.HealthPolicy);
        }

        public override async Task<OperationResult<ReplicaHealth>> GetReplicaHealthAsync(ReplicaHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string[] resourceTypes = new string[] { RestPathBuilder.Partitions, RestPathBuilder.GetReplicas, RestPathBuilder.GetHealth };
            string[] resourceIds = new string[] { queryDescription.PartitionId.ToString(), queryDescription.ReplicaOrInstanceId.ToString(CultureInfo.InvariantCulture) };

            Dictionary<string, string> queryParameters = new Dictionary<string, string>();

            if (queryDescription.EventsFilter != null)
            {
                queryParameters.Add(RestPathBuilder.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue.ToString("D"));
            }

            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.ApiVersionDefault);
            return await this.GetOperationResultFromRequest<ReplicaHealth>(httpGatewayEndpoint, queryDescription.HealthPolicy);
        }

        public override OperationResult ReportHealth(HealthReport healthReport, HealthReportSendOptions sendOptions)
        {
            var timeout = TimeSpan.FromSeconds(30);

            Dictionary<string, string> queryParameters = null;
            if (sendOptions != null)
            {
                queryParameters = new Dictionary<string, string>();
                queryParameters.Add(RestPathBuilder.ImmediateQueryParameter, sendOptions.Immediate.ToString());
            }

            switch (healthReport.Kind)
            {
                case HealthReportKind.Application:
                    {
                        var report = healthReport as ApplicationHealthReport;
                        var resourceTypes = new string[] { RestPathBuilder.Applications, RestPathBuilder.ReportHealth };
                        var resourceIds = new string[] { report.ApplicationName.OriginalString.Substring(RestFabricClient.ApplicationUriPrefix.Length) };
                        return ReportHealthAsync(resourceTypes, resourceIds, report.HealthInformation, queryParameters, timeout, CancellationToken.None).Result;
                    }
                case HealthReportKind.Node:
                    {
                        var report = healthReport as NodeHealthReport;
                        var resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.ReportHealth };
                        var resourceIds = new string[] { report.NodeName };
                        return ReportHealthAsync(resourceTypes, resourceIds, report.HealthInformation, queryParameters, timeout, CancellationToken.None).Result;
                    }
                case HealthReportKind.Cluster:
                    {
                        var report = healthReport as ClusterHealthReport;
                        var resourceTypes = new string[] { RestPathBuilder.ReportClusterHealth };
                        var resourceIds = new string[] { };
                        return ReportHealthAsync(resourceTypes, resourceIds, report.HealthInformation, queryParameters, timeout, CancellationToken.None).Result;
                    }
                case HealthReportKind.Service:
                    {
                        var report = healthReport as ServiceHealthReport;
                        var resourceTypes = new string[] { RestPathBuilder.Services, RestPathBuilder.ReportHealth };
                        var resourceIds = new string[] { RestFabricClient.EscapeAndStripRootUri(report.ServiceName.AbsoluteUri) };
                        return ReportHealthAsync(resourceTypes, resourceIds, report.HealthInformation, queryParameters, timeout, CancellationToken.None).Result;
                    }
                case HealthReportKind.Partition:
                    {
                        var report = healthReport as PartitionHealthReport;
                        var resourceTypes = new string[] { RestPathBuilder.Partitions, RestPathBuilder.ReportHealth };
                        var resourceIds = new string[] { report.PartitionId.ToString() };
                        return ReportHealthAsync(resourceTypes, resourceIds, report.HealthInformation, queryParameters, timeout, CancellationToken.None).Result;
                    }
                case HealthReportKind.StatefulServiceReplica:
                    {
                        var report = healthReport as StatefulServiceReplicaHealthReport;
                        if (queryParameters == null)
                        {
                            queryParameters = new Dictionary<string, string>();
                        }
                        queryParameters.Add(RestPathBuilder.ServiceKindParameter, string.Format("{0}", (int)ServiceKind.Stateful));

                        var resourceTypes = new string[] { RestPathBuilder.Partitions, RestPathBuilder.GetReplicas, RestPathBuilder.ReportHealth };
                        var resourceIds = new string[] { report.PartitionId.ToString(), report.ReplicaId.ToString() };
                        return ReportHealthAsync(resourceTypes, resourceIds, report.HealthInformation, queryParameters, timeout, CancellationToken.None).Result;
                    }
                case HealthReportKind.StatelessServiceInstance:
                    {
                        var report = healthReport as StatelessServiceInstanceHealthReport;
                        if (queryParameters == null)
                        {
                            queryParameters = new Dictionary<string, string>();
                        }
                        queryParameters.Add(RestPathBuilder.ServiceKindParameter, string.Format("{0}", (int)ServiceKind.Stateless));

                        var resourceTypes = new string[] { RestPathBuilder.Partitions, RestPathBuilder.GetReplicas, RestPathBuilder.ReportHealth };
                        var resourceIds = new string[] { report.PartitionId.ToString(), report.InstanceId.ToString() };
                        return ReportHealthAsync(resourceTypes, resourceIds, report.HealthInformation, queryParameters, timeout, CancellationToken.None).Result;
                    }
                case HealthReportKind.DeployedApplication:
                    {
                        var report = healthReport as DeployedApplicationHealthReport;
                        var resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.GetApplications, RestPathBuilder.ReportHealth };
                        string applicationId = RestFabricClient.EscapeAndStripRootUri(report.ApplicationName.OriginalString);
                        var resourceIds = new string[] { report.NodeName, applicationId };
                        return ReportHealthAsync(resourceTypes, resourceIds, report.HealthInformation, queryParameters, timeout, CancellationToken.None).Result;
                    }
                case HealthReportKind.DeployedServicePackage:
                    {
                        var report = healthReport as DeployedServicePackageHealthReport;
                        var resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.GetApplications, RestPathBuilder.GetServicePackages, RestPathBuilder.ReportHealth };
                        string applicationId = RestFabricClient.EscapeAndStripRootUri(report.ApplicationName.OriginalString);
                        var resourceIds = new string[] { report.NodeName, applicationId, report.ServiceManifestName };
                        return ReportHealthAsync(resourceTypes, resourceIds, report.HealthInformation, queryParameters, timeout, CancellationToken.None).Result;
                    }
                default:
                    {
                        return FabricClientState.CreateOperationResultFromNativeErrorCode(NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_ADDRESS);
                    }
            }
        }

        private async Task<OperationResult> ReportHealthAsync(string[] resourceTypes, string[] resourceIds, HealthInformation healthInformation, Dictionary<string, string> queryParameters, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string restString = string.Empty;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.ApiVersionDefault);
                    byte[] buffer = this.GetSerializedObject(healthInformation);

                    TestabilityTrace.TraceSource.WriteInfo(TraceSource, "Command: {0} -  {1}", httpGatewayEndpoint.OriginalString, Encoding.UTF8.GetString(buffer));
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CopyImageStoreContentAsync(WinFabricCopyImageStoreContentDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.ImageStore, RestPathBuilder.Copy };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriByTypes(this.httpGatewayAddresses, resourceTypes);
                    byte[] buffer = this.GetSerializedObject(description);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<ImageStoreContentResult>> ListImageStoreContentAsync(string remoteLocation, string imageStoreConnectionString, TimeSpan timeout, CancellationToken cancellationToken)
        {
            OperationResult<ImageStoreContentResult> result = null;
            ImageStoreContentResult imageStoreContentResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.ImageStore };
                    string[] resourceIds = new string[] { remoteLocation };

                    Uri httpGatewayEndpoint = null;
                    if (!string.IsNullOrEmpty(remoteLocation))
                    {
                        httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);
                    }
                    else
                    {
                        httpGatewayEndpoint = RestPathBuilder.GetResourceUriByTypes(this.httpGatewayAddresses, resourceTypes);
                    }
                    HttpWebResponse response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet);
                    if (response.StatusCode != HttpStatusCode.NotFound)
                    {
                        imageStoreContentResult = this.GetDeserializedObject<ImageStoreContentResult>(response.GetResponseStream());
                    }
                });

            if (imageStoreContentResult == null)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<ImageStoreContentResult>(errorCode);
            }
            else
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<ImageStoreContentResult>(imageStoreContentResult);
            }

            return result;
        }

        public override async Task<OperationResult<ImageStorePagedContentResult>> ListImageStorePagedContentAsync(ImageStoreListDescription listDescription, string imageStoreConnectionString, TimeSpan timeout, CancellationToken cancellationToken)
        {
            OperationResult<ImageStorePagedContentResult> result = null;
            ImageStorePagedContentResult imageStoreContentResult = null;

            TestabilityTrace.TraceSource.WriteInfo(
                TraceSource,
                "Parameter: RemoteLocation:{0}, ContinuationToken={1}, IsRecursive={2}",
                listDescription.RemoteLocation,
                listDescription.ContinuationToken,
                listDescription.IsRecursive);

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    Uri httpGatewayEndpoint = null;
                    string[] resourceTypes = new string[] { RestPathBuilder.ImageStore };
                    string[] resourceIds = new string[] { listDescription.RemoteLocation };

                    Dictionary<string, string> queryParameters = queryParameters = new Dictionary<string, string>();
                    queryParameters.Add(RestPathBuilder.ContinuationToken, listDescription.ContinuationToken);
                    httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.ApiVersionDefault);

                    TestabilityTrace.TraceSource.WriteInfo(TraceSource, "Command: {0}", httpGatewayEndpoint.OriginalString);
                    HttpWebResponse response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet);
                    if (response.StatusCode != HttpStatusCode.NotFound)
                    {
                        imageStoreContentResult = this.GetDeserializedObject<ImageStorePagedContentResult>(response.GetResponseStream());
                    }
                });

            if (imageStoreContentResult == null)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceSource, "Result is NULL");
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<ImageStorePagedContentResult>(errorCode);
            }
            else
            {
                TestabilityTrace.TraceSource.WriteInfo(
                    TraceSource,
                    "FileCount:{0}, FolderCount:{1}, ContinuationToken:{2}",
                    imageStoreContentResult.StoreFiles.Count,
                    imageStoreContentResult.StoreFolders.Count,
                    imageStoreContentResult.ContinuationToken);
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<ImageStorePagedContentResult>(imageStoreContentResult);
            }

            return result;
        }

        public override async Task<OperationResult> DeleteImageStoreContentAsync(string remoteLocation, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                   delegate
                   {
                       string[] resourceTypes = new string[] { RestPathBuilder.ImageStore };
                       string[] resourceIds = new string[] { remoteLocation };

                       Dictionary<string, string> queryParameters = new Dictionary<string, string>();
                       queryParameters.Add(RestPathBuilder.TimeoutQueryParameter, string.Format("{0}", timeout.TotalSeconds));
                       Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters);
                       return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpDelete);
                   });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UploadChunkAsync(string remoteLocation, string sessionId, UInt64 startPosition, UInt64 endPosition, string filePath, UInt64 fileSize, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.ImageStore, RestPathBuilder.UploadChunk };
                    string[] resourceIds = new string[] { remoteLocation };
                    Dictionary<string, string> sessionIdQueryParameter = new Dictionary<string, string>() { { RestPathBuilder.SessionIdUri, sessionId.ToString() } };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, sessionIdQueryParameter);
                    TestabilityTrace.TraceSource.WriteInfo(TraceSource, "Upload chunk for {0}: ['{1}', '{2}'-'{3}'/'{4}'",
                        remoteLocation,
                        sessionId.ToString(),
                        startPosition,
                        endPosition,
                        fileSize);

                    int length = (int)(endPosition - startPosition + 1);
                    byte[] chunk = new byte[length];
                    using (FileStream streamSource = new FileStream(filePath, FileMode.Open, FileAccess.Read, FileShare.None))
                    {
                        streamSource.Seek((long)startPosition, SeekOrigin.Begin);
                        streamSource.Read(chunk, 0, length);
                    }

                    Dictionary<string, string> customizedHeaderProperties = new Dictionary<string, string>() { { "Content-Range", string.Format("bytes {0}-{1}/{2}", startPosition, endPosition, fileSize) } };

                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPut, chunk, "application/octet-stream", customizedHeaderProperties);
                });

            if (errorCode != NativeTypes.FABRIC_ERROR_CODE.S_OK)
            {
                return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
            }

            return FabricClientState.CreateOperationResultFromNativeErrorCode(NativeTypes.FABRIC_ERROR_CODE.S_OK);
        }

        public override async Task<OperationResult> DeleteUploadSessionAsync(Guid sessionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.ImageStore, RestPathBuilder.DeleteUploadSession };
                    Dictionary<string, string> sessionIdQueryParameter = new Dictionary<string, string>() { { RestPathBuilder.SessionIdUri, sessionId.ToString() } };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriByTypes(this.httpGatewayAddresses, resourceTypes, sessionIdQueryParameter);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpDelete);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<UploadSession>> ListUploadSessionAsync(Guid sessionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            OperationResult<UploadSession> result = null;
            UploadSession uploadSession = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.ImageStore, RestPathBuilder.GetUploadSession };
                    Dictionary<string, string> sessionIdQueryParameter = new Dictionary<string, string>() { { RestPathBuilder.SessionIdUri, sessionId.ToString() } };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriByTypes(this.httpGatewayAddresses, resourceTypes, sessionIdQueryParameter);
                    HttpWebResponse response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet);
                    uploadSession = this.GetDeserializedObject<UploadSession>(response.GetResponseStream());
                });

            if (uploadSession == null)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<UploadSession>(errorCode);
            }
            else
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<UploadSession>(uploadSession);
            }

            return result;
        }

        public override async Task<OperationResult<string>> GetClusterVersionAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            OperationResult<string> result = null;
            ClusterVersion clusterVersion = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(
                        this.httpGatewayAddresses,
                        RestPathBuilder.GetClusterVersion);
                    HttpWebResponse response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet);

                    clusterVersion = this.GetDeserializedObject<ClusterVersion>(response.GetResponseStream());
                });

            result = FabricClientState.CreateOperationResultFromNativeErrorCode<string>(errorCode);
            if (clusterVersion != null)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<string>(clusterVersion.Version);
            }

            return result;
        }

        public override async Task<OperationResult> CommitUploadSessionAsync(Guid sessionId, TimeSpan tiemout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.ImageStore, RestPathBuilder.CommitUploadSession };
                    Dictionary<string, string> sessionIdQueryParameter = new Dictionary<string, string>() { { RestPathBuilder.SessionIdUri, sessionId.ToString() } };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriByTypes(this.httpGatewayAddresses, resourceTypes, sessionIdQueryParameter);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<Node>> GetNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            OperationResult<Node> result = null;
            Node nodeResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.Nodes };
                    string[] resourceIds = new string[] { nodeName };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);
                    HttpWebResponse response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet);

                    nodeResult = this.GetDeserializedObject<Node>(response.GetResponseStream());
                });

            result = FabricClientState.CreateOperationResultFromNativeErrorCode<Node>(errorCode);
            if (nodeResult != null)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<Node>(nodeResult);
            }

            return result;
        }

        public override async Task<OperationResult<NodeList>> GetNodeListAsync(NodeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var queryParameters = new Dictionary<string, string>();

            if (!string.IsNullOrEmpty(queryDescription.NodeNameFilter))
            {
                queryParameters.Add(RestPathBuilder.NodeNameFilter, queryDescription.NodeNameFilter);
            }

            if (!string.IsNullOrEmpty(queryDescription.ContinuationToken))
            {
                queryParameters.Add(RestPathBuilder.ContinuationToken, queryDescription.ContinuationToken);
            }

            if (queryDescription.MaxResults.HasValue)
            {
                queryParameters.Add(RestPathBuilder.MaxResults, queryDescription.MaxResults.ToString());
            }

            if (queryDescription.NodeStatusFilter != NodeStatusFilter.Default)
            {
                queryParameters.Add(RestPathBuilder.NodeStatusFilter, queryDescription.NodeStatusFilter.ToString());
            }

            Uri httpGatewayEndpoint;

            // Have two different calls depending on whether or not nodeNameFilter is specified
            if (!string.IsNullOrEmpty(queryDescription.NodeNameFilter))
            {
                var nameFilter = new string[] { queryDescription.NodeNameFilter };

                httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(
                    this.httpGatewayAddresses,
                    new string[] { RestPathBuilder.Nodes },
                    nameFilter,
                    queryParameters,
                    RestPathBuilder.ApiVersionDefault);

                var nodeResult = await this.GetOperationResultFromRequest<Node>(httpGatewayEndpoint, null);
                var nodeList = new NodeList();
                if (nodeResult.Result != null)
                {
                    nodeList.Add(nodeResult.Result);
                }
                return new OperationResult<NodeList>(nodeResult.ErrorCode, nodeList);
            }
            else
            {
                // For optional query parameters with URL format &{0}={1}, put it into a dictionary. It will be converted to that format from the dictionary
                httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, RestPathBuilder.Nodes, queryParameters, RestPathBuilder.ApiVersionDefault);
                return await this.GetOperationPagedResultListFromRequest<NodeList>(httpGatewayEndpoint, null);
            }
        }

        public override async Task<OperationResult<ApplicationType[]>> GetApplicationTypeListAsync(string applicationTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            OperationResult<ApplicationType[]> result = null;
            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, RestPathBuilder.ApplicationTypes, applicationTypeNameFilter, RestPathBuilder.Api30Version);
            result = await this.GetOperationResultArrayFromRequest<ApplicationType>(httpGatewayEndpoint, null);
            return result;
        }

        public override async Task<OperationResult<ApplicationTypePagedList>> GetApplicationTypePagedListAsync(PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var queryParameters = new Dictionary<string, string>();

            if (queryDescription.ContinuationToken != null)
            {
                queryParameters.Add(RestPathBuilder.ContinuationToken, queryDescription.ContinuationToken);
            }

            if (queryDescription.MaxResults != 0)
            {
                queryParameters.Add(RestPathBuilder.MaxResults, queryDescription.MaxResults.ToString());
            }

            if (!string.IsNullOrEmpty(queryDescription.ApplicationTypeVersionFilter))
            {
                queryParameters.Add(RestPathBuilder.ApplicationTypeVersionFilter, queryDescription.ApplicationTypeVersionFilter);
            }

            if (queryDescription.ApplicationTypeDefinitionKindFilter != ApplicationTypeDefinitionKindFilter.Default)
            {
                queryParameters.Add(RestPathBuilder.ApplicationTypeDefinitionKindFilter, ((int)queryDescription.ApplicationTypeDefinitionKindFilter).ToString());
            }

            queryParameters.Add(RestPathBuilder.ExcludeApplicationParameters, queryDescription.ExcludeApplicationParameters.ToString());

            Uri httpGatewayEndpoint;

            // Have two different calls depending on whether or not applicationTypeNameFilter is specified
            if (!string.IsNullOrEmpty(queryDescription.ApplicationTypeNameFilter))
            {
                var nameFilter = new string[] { queryDescription.ApplicationTypeNameFilter };

                // For optional query parameters with URL format &{0}={1}, put it into a dictionary. It will be converted to that format from the dictionary
                httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(
                    this.httpGatewayAddresses,
                    new string[] { RestPathBuilder.ApplicationTypes },
                    nameFilter,
                    queryParameters,
                    RestPathBuilder.ApiVersionDefault);
            }
            else
            {
                // For optional query parameters with URL format &{0}={1}, put it into a dictionary. It will be converted to that format from the dictionary
                httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(
                    this.httpGatewayAddresses,
                    RestPathBuilder.ApplicationTypes,
                    queryParameters,
                    RestPathBuilder.ApiVersionDefault);
            }

            return await this.GetOperationPagedResultListFromRequest<ApplicationTypePagedList>(httpGatewayEndpoint, null);
        }

        public override async Task<OperationResult<Application>> GetApplicationAsync(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            OperationResult<Application> result = null;
            Application applicationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    string[] resourceTypes = new string[] { RestPathBuilder.Applications };
                    string[] resourceIds = new string[] { applicationName.OriginalString };
                    Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);
                    HttpWebResponse response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet);
                    applicationResult = this.GetDeserializedObject<Application>(response.GetResponseStream());
                });

            if (applicationResult != null)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<Application>(applicationResult);
            }
            else
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<Application>(errorCode);
            }

            return result;
        }

        public override async Task<OperationResult<Application>> GetApplicationByIdRestAsync(string applicationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(applicationId, "applicationId");

            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, RestPathBuilder.Applications, applicationId);
            return await this.GetOperationResultFromRequest<Application>(httpGatewayEndpoint, null);
        }

        // Handles both get application paged list and get application by ID
        public override async Task<OperationResult<ApplicationList>> GetApplicationPagedListAsync(ApplicationQueryDescription applicationQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Dictionary<string, string> queryParameters = new Dictionary<string, string>();

            queryParameters.Add(RestPathBuilder.ExcludeApplicationParameters, applicationQueryDescription.ExcludeApplicationParameters.ToString());

            string[] resourceTypes = new string[] { RestPathBuilder.Applications };

            if (applicationQueryDescription.ApplicationNameFilter != null)
            {
                // Single item expected (or none)
                string applicationId = RestFabricClient.EscapeAndStripRootUri(applicationQueryDescription.ApplicationNameFilter.OriginalString);
                Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, new string[] { applicationId }, queryParameters, RestPathBuilder.ApiVersionDefault);

                var singleItemResult = await this.GetOperationResultFromRequest<Application>(httpGatewayEndpoint, null);
                return ConvertOperationResultPagedList<ApplicationList, Application>(singleItemResult);
            }
            else
            {
                if (!string.IsNullOrEmpty(applicationQueryDescription.ApplicationTypeNameFilter))
                {
                    queryParameters.Add(RestPathBuilder.ApplicationTypeName, applicationQueryDescription.ApplicationTypeNameFilter);
                }

                if (applicationQueryDescription.ApplicationDefinitionKindFilter != ApplicationDefinitionKindFilter.Default)
                {
                    queryParameters.Add(RestPathBuilder.ApplicationDefinitionKindFilter, ((int)applicationQueryDescription.ApplicationDefinitionKindFilter).ToString());
                }

                if (!string.IsNullOrEmpty(applicationQueryDescription.ContinuationToken))
                {
                    queryParameters.Add(RestPathBuilder.ContinuationToken, applicationQueryDescription.ContinuationToken);
                }

                if (applicationQueryDescription.MaxResults.HasValue)
                {
                    queryParameters.Add(RestPathBuilder.MaxResults, applicationQueryDescription.MaxResults.ToString());
                }

                string[] resourceIds = new string[0];
                Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.ApiVersionDefault);
                return await this.GetOperationPagedResultListFromRequest<ApplicationList>(httpGatewayEndpoint, null);
            }
        }

        public override async Task<OperationResult<DeployedApplication[]>> GetDeployedApplicationListAsync(string nodeName, Uri applicationNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string[] resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.GetApplications };
            string[] resourceIds = null;

            if (applicationNameFilter == null)
            {
                resourceIds = new string[] { nodeName };
            }
            else
            {
                string applicationId = RestFabricClient.EscapeAndStripRootUri(applicationNameFilter.OriginalString);
                resourceIds = new string[] { nodeName, applicationId };
            }

            // List of item expected
            // Version 3.0 is the last one that's not paged.
            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, RestPathBuilder.Api30Version);

            // check if list of item or a single item expected in rest response
            if (applicationNameFilter != null)
            {
                // Single item expected
                var singleItemResult = await this.GetOperationResultFromRequest<DeployedApplication>(httpGatewayEndpoint, null);
                return ConvertOperationResultArray(singleItemResult);
            }

            // List of item expected
            return await this.GetOperationResultArrayFromRequest<DeployedApplication>(httpGatewayEndpoint, null);
        }

        public override async Task<OperationResult<DeployedApplicationPagedList>> GetDeployedApplicationPagedListAsync(
            PagedDeployedApplicationQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            var queryParameters = new Dictionary<string, string>();

            if (queryDescription.ContinuationToken != null)
            {
                queryParameters.Add(RestPathBuilder.ContinuationToken, queryDescription.ContinuationToken);
            }

            if (queryDescription.MaxResults.HasValue)
            {
                queryParameters.Add(RestPathBuilder.MaxResults, queryDescription.MaxResults.Value.ToString());
            }

            queryParameters.Add(RestPathBuilder.IncludeHealthState, queryDescription.IncludeHealthState.ToString());

            string[] resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.GetApplications };
            string[] resourceIds = null;

            // Have two different calls depending on whether or not applicationTypeNameFilter is specified
            if (queryDescription.ApplicationNameFilter == null)
            {
                resourceIds = new string[] { queryDescription.NodeName };
            }
            else
            {
                string applicationId = RestFabricClient.EscapeAndStripRootUri(queryDescription.ApplicationNameFilter.OriginalString);
                resourceIds = new string[] { queryDescription.NodeName, applicationId };
            }

            // List of item expected
            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(
                this.httpGatewayAddresses,
                resourceTypes,
                resourceIds,
                queryParameters,
                RestPathBuilder.ApiVersionDefault);

            // REST returns a single DeployedApplication object if we are getting by name
            // For purposes of testing, artificially make this into one object
            if (queryDescription.ApplicationNameFilter == null)
            {
                // List of item expected
                return await this.GetOperationPagedResultListFromRequest<DeployedApplicationPagedList>(httpGatewayEndpoint, null);
            }
            else
            {
                var result = await this.GetOperationResultFromRequest<DeployedApplication>(httpGatewayEndpoint, null);
                var deployedApplicationObject = result.Result;
                var pagedList = new DeployedApplicationPagedList();
                if (deployedApplicationObject != null)
                {
                    pagedList.Add(deployedApplicationObject);
                }
                return new OperationResult<DeployedApplicationPagedList>(result.ErrorCode, pagedList);
            }
        }

        public override async Task<OperationResult<DeployedServiceType[]>> GetDeployedServiceTypeListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string applicationId = RestFabricClient.EscapeAndStripRootUri(applicationName.OriginalString);
            string[] resourceIds = null;
            Dictionary<string, string> queryParameters = null;

            if (string.IsNullOrEmpty(serviceManifestNameFilter) && string.IsNullOrEmpty(serviceTypeNameFilter))
            {
                resourceIds = new string[] { nodeName, applicationId };
            }
            else if (!string.IsNullOrEmpty(serviceManifestNameFilter) && string.IsNullOrEmpty(serviceTypeNameFilter))
            {
                resourceIds = new string[] { nodeName, applicationId };
                queryParameters = new Dictionary<string, string>();
                queryParameters.Add(RestPathBuilder.ServiceManifestName, serviceManifestNameFilter);
            }
            else if (string.IsNullOrEmpty(serviceManifestNameFilter) && !string.IsNullOrEmpty(serviceTypeNameFilter))
            {
                resourceIds = new string[] { nodeName, applicationId, serviceTypeNameFilter };
            }
            else
            {
                resourceIds = new string[] { nodeName, applicationId, serviceTypeNameFilter };
                queryParameters = new Dictionary<string, string>();
                queryParameters.Add(RestPathBuilder.ServiceManifestName, serviceManifestNameFilter);
            }

            string[] resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.GetApplications, RestPathBuilder.GetServiceTypes };
            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.ApiVersionDefault);

            // List of item expected
            return await this.GetOperationResultArrayFromRequest<DeployedServiceType>(httpGatewayEndpoint, null);
        }

        public override async Task<OperationResult<DeployedServicePackage[]>> GetDeployedServicePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string applicationId = RestFabricClient.EscapeAndStripRootUri(applicationName.OriginalString);
            string[] resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.GetApplications, RestPathBuilder.GetServicePackages };
            string[] resourceIds = null;

            if (!string.IsNullOrEmpty(serviceManifestNameFilter))
            {
                resourceIds = new string[] { nodeName, applicationId, serviceManifestNameFilter };
            }
            else
            {
                resourceIds = new string[] { nodeName, applicationId };
            }

            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, null, RestPathBuilder.ApiVersionDefault);
            return await this.GetOperationResultArrayFromRequest<DeployedServicePackage>(httpGatewayEndpoint, null);
        }

        public override async Task<OperationResult<DeployedCodePackage[]>> GetDeployedCodePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string codePackageNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Get httpGatewayEndpoint
            string applicationId = RestFabricClient.EscapeAndStripRootUri(applicationName.OriginalString);
            string[] resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.GetApplications, RestPathBuilder.GetCodePackages };
            string[] resourceIds = new string[] { nodeName, applicationId };
            Dictionary<string, string> queryParameters = new Dictionary<string, string>();

            if (!string.IsNullOrEmpty(serviceManifestNameFilter))
            {
                queryParameters.Add(RestPathBuilder.ServiceManifestName, serviceManifestNameFilter);
            }

            if (!string.IsNullOrEmpty(codePackageNameFilter))
            {
                queryParameters.Add(RestPathBuilder.CodePackageName, codePackageNameFilter);
            }

            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters);
            return await this.GetOperationResultArrayFromRequest<DeployedCodePackage>(httpGatewayEndpoint, null);
        }

        public override async Task<OperationResult<Service>> GetServiceByIdRestAsync(string applicationId, string serviceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(applicationId, "applicationId");
            ThrowIf.NullOrEmpty(applicationId, "serviceId");

            string[] resourceTypes = new string[] { RestPathBuilder.Applications, RestPathBuilder.GetServices };
            string[] resourceIds = new string[] { applicationId, serviceId };

            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);
            return await this.GetOperationResultFromRequest<Service>(httpGatewayEndpoint, null);
        }

        public override async Task<OperationResult<ServiceList>> GetServicePagedListAsync(ServiceQueryDescription serviceQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(serviceQueryDescription.ApplicationName, "applicationName");
            string applicationId = RestFabricClient.EscapeAndStripRootUri(serviceQueryDescription.ApplicationName);
            string[] resourceTypes = new string[] { RestPathBuilder.Applications, RestPathBuilder.GetServices };
            string[] resourceIds = new string[] { applicationId };

            Dictionary<string, string> queryParameters = new Dictionary<string, string>(); ;

            if (!string.IsNullOrEmpty(serviceQueryDescription.ContinuationToken))
            {
                queryParameters.Add(RestPathBuilder.ContinuationToken, serviceQueryDescription.ContinuationToken);
            }

            if (serviceQueryDescription.MaxResults.HasValue)
            {
                queryParameters.Add(RestPathBuilder.MaxResults, serviceQueryDescription.MaxResults.ToString());
            }

            if (!string.IsNullOrEmpty(serviceQueryDescription.ServiceTypeNameFilter))
            {
                queryParameters.Add(RestPathBuilder.ServiceTypeName, serviceQueryDescription.ServiceTypeNameFilter);
            }

            TestabilityTrace.TraceSource.WriteInfo(TraceSource, "DEBUG RestFabricClient GetServiceListAsync() passing escaped application name: '{0}'", applicationId);
            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.ApiVersionDefault);
            TestabilityTrace.TraceSource.WriteInfo(TraceSource, "DEBUG RestFabricClient GetServiceListAsync() uri: '{0}', uri.original: '{1}'", httpGatewayEndpoint, httpGatewayEndpoint.OriginalString);
            return await this.GetOperationPagedResultListFromRequest<ServiceList>(httpGatewayEndpoint, null);
        }

        public override async Task<OperationResult<ServicePartitionList>> GetPartitionListRestAsync(string applicationName, string serviceName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(applicationName, "applicationName");
            ThrowIf.NullOrEmpty(serviceName, "serviceName");

            string applicationId = RestFabricClient.EscapeAndStripRootUri(applicationName);
            string serviceId = RestFabricClient.EscapeAndStripRootUri(serviceName);

            Dictionary<string, string> queryParameters = null;

            if (!string.IsNullOrEmpty(continuationToken))
            {
                queryParameters = new Dictionary<string, string>();
                queryParameters.Add(RestPathBuilder.ContinuationToken, continuationToken);
            }

            string[] resourceTypes = new string[] { RestPathBuilder.Applications, RestPathBuilder.GetServices, RestPathBuilder.GetPartitions };
            string[] resourceIds = new string[] { applicationId, serviceId };

            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.ApiVersionDefault);
            return await this.GetOperationPagedResultListFromRequest<ServicePartitionList>(httpGatewayEndpoint, null);
        }

        public override async Task<OperationResult<ServiceReplicaList>> GetReplicaListRestAsync(Uri applicationName, Uri serviceName, Guid partitionId, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(applicationName, "applicationName");
            ThrowIf.Null(serviceName, "serviceName");
            ThrowIf.Empty(partitionId, "partitionId");

            Dictionary<string, string> queryParameters = null;

            if (!string.IsNullOrEmpty(continuationToken))
            {
                queryParameters = new Dictionary<string, string>();
                queryParameters.Add(RestPathBuilder.ContinuationToken, continuationToken);
            }

            string applicationId = RestFabricClient.EscapeAndStripRootUri(applicationName.OriginalString);
            string serviceId = RestFabricClient.EscapeAndStripRootUri(serviceName.OriginalString);

            string[] resourceTypes = new string[] { RestPathBuilder.Applications, RestPathBuilder.GetServices, RestPathBuilder.GetPartitions, RestPathBuilder.GetReplicas };
            string[] resourceIds = new string[] { applicationId, serviceId, partitionId.ToString() };

            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.ApiVersionDefault);
            return await this.GetOperationPagedResultListFromRequest<ServiceReplicaList>(httpGatewayEndpoint, null);
        }

        public override async Task<OperationResult<DeployedServiceReplica[]>> GetDeployedReplicaListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, Guid? partitionIdFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string applicationId = RestFabricClient.EscapeAndStripRootUri(applicationName.OriginalString);

            string[] resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.GetApplications, RestPathBuilder.GetReplicas };
            string[] resourceIds = new string[] { nodeName, applicationId };

            Dictionary<string, string> queryParameters = new Dictionary<string, string>();
            if (partitionIdFilter.HasValue)
            {
                queryParameters.Add(RestPathBuilder.PartitionId, partitionIdFilter.Value.ToString());
            }

            if (!string.IsNullOrEmpty(serviceManifestNameFilter))
            {
                queryParameters.Add(RestPathBuilder.ServiceManifestName, serviceManifestNameFilter);
            }

            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds, queryParameters);
            return await this.GetOperationResultArrayFromRequest<DeployedServiceReplica>(httpGatewayEndpoint, null);
        }

        public override async Task<OperationResult<string>> GetClusterManifestAsync(ClusterManifestQueryDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            OperationResult<string> result = null;
            WinFabricManifestResult manifestResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    var queryParameters = new Dictionary<string, string>();

                    if (!string.IsNullOrEmpty(description.ClusterManifestVersion))
                    {
                        queryParameters[RestPathBuilder.ClusterManifestVersionFilter] = description.ClusterManifestVersion;
                    }

                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(
                        this.httpGatewayAddresses,
                        RestPathBuilder.GetClusterManifest,
                        queryParameters);
                    HttpWebResponse response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet);

                    manifestResult = (WinFabricManifestResult)RestFabricClient.JsonManifestResultSerializer.ReadObject(response.GetResponseStream());
                });

            result = FabricClientState.CreateOperationResultFromNativeErrorCode<string>(errorCode);
            if (manifestResult != null)
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<string>(manifestResult.Manifest);
            }

            return result;
        }

        public override async Task<OperationResult<NodeLoadInformation>> GetNodeLoadAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(nodeName, "nodeName");

            string[] resourceIds = new string[] { nodeName };
            string[] resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.GetLoadInformation };
            return await this.GetOperationResultFromRequest<NodeLoadInformation>(resourceTypes, resourceIds, null);
        }

        public override async Task<OperationResult<ClusterLoadInformation>> GetClusterLoadAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, RestPathBuilder.GetLoadInformation);
            return await this.GetOperationResultFromRequest<ClusterLoadInformation>(httpGatewayEndpoint, null);
        }

        public override async Task<OperationResult<ReplicaLoadInformation>> GetReplicaLoadAsync(Guid partitionId, long replicaId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(partitionId.ToString(), "partitionId");
            ThrowIf.NullOrEmpty(replicaId.ToString(), "replicaId");

            string[] resourceIds = new string[] { partitionId.ToString(), replicaId.ToString(CultureInfo.InvariantCulture) };
            string[] resourceTypes = new string[] { RestPathBuilder.Partitions, RestPathBuilder.GetReplicas, RestPathBuilder.GetLoadInformation };
            return await this.GetOperationResultFromRequest<ReplicaLoadInformation>(resourceTypes, resourceIds, null);
        }

        public override async Task<OperationResult<PartitionLoadInformation>> GetPartitionLoadAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(partitionId.ToString(), "partitionId");

            string[] resourceIds = new string[] { partitionId.ToString() };
            string[] resourceTypes = new string[] { RestPathBuilder.Partitions, RestPathBuilder.GetLoadInformation };
            return await this.GetOperationResultFromRequest<PartitionLoadInformation>(resourceTypes, resourceIds, null);
        }

        public override async Task<OperationResult> StartNodeNativeAsync(string nodeName, BigInteger instanceId, string ipAddressOrFQDN, int clusterConnectionPort, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
            delegate
            {
                string[] resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.Start };
                string[] resourceIds = new string[] { nodeName };
                Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);
                WinFabricStartNodeData argument = new WinFabricStartNodeData(ipAddressOrFQDN, clusterConnectionPort.ToString(), instanceId.ToString());
                byte[] buffer = RestFabricClient.GetSerializedObject(RestFabricClient.JsonStartNodeDataSerializer, argument);
                return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> StopNodeNativeAsync(string nodeName, BigInteger instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
            delegate
            {
                string[] resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.Stop };
                string[] resourceIds = new string[] { nodeName };
                Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);
                WinFabricNodeControlData argument = new WinFabricNodeControlData(instanceId.ToString());
                byte[] buffer = RestFabricClient.GetSerializedObject(RestFabricClient.JsonNodeControlDataSerializer, argument);
                return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RestartNodeNativeAsync(string nodeName, BigInteger instanceId, bool createFabricDump, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
            delegate
            {
                string[] resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.Restart };
                string[] resourceIds = new string[] { nodeName };
                Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);
                WinFabricRestartNodeControlData argument = new WinFabricRestartNodeControlData(instanceId.ToString(), createFabricDump.ToString());
                byte[] buffer = RestFabricClient.GetSerializedObject(RestFabricClient.JsonRestartNodeControlDataSerializer, argument);
                return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RestartDeployedCodePackageNativeAsync(string nodeName, Uri applicationName, string serviceManifestName, string servicePackageActivationId, string codePackageName, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
            delegate
            {
                string[] resourceTypes = new string[] { RestPathBuilder.Nodes, RestPathBuilder.GetApplications, RestPathBuilder.GetCodePackages, RestPathBuilder.Restart };
                string applicationId = RestFabricClient.EscapeAndStripRootUri(applicationName.OriginalString);
                string[] resourceIds = new string[] { nodeName, applicationId };
                Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);
                WinFabricCodePackageControlData argument = new WinFabricCodePackageControlData(serviceManifestName, servicePackageActivationId, codePackageName, instanceId.ToString());
                byte[] buffer = RestFabricClient.GetSerializedObject(RestFabricClient.JsonCodePackageControlDataSerializer, argument);
                return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<List<string>>> GetEventsStorePartitionEventsRestAsync(
            Guid partitionId,
            DateTime startTimeUtc,
            DateTime endTimeUtc,
            IList<string> eventsTypesFilter,
            bool excludeAnalysisEvent,
            bool skipCorrelationLookup)
        {
            List<string> partitionEvents = new List<string>();
            HttpWebResponse response = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
            async delegate
            {
                Dictionary<string, string> queryParameters = new Dictionary<string, string>();

                queryParameters.Add(RestPathBuilder.StartTimeUtc, startTimeUtc.ToString(RestPathBuilder.YearMonthDayHourMinuteSecondUtcFormat));
                queryParameters.Add(RestPathBuilder.EndTimeUtc, endTimeUtc.ToString(RestPathBuilder.YearMonthDayHourMinuteSecondUtcFormat));
                if (eventsTypesFilter != null && eventsTypesFilter.Any())
                {
                    queryParameters.Add(RestPathBuilder.EventsTypesFilter, string.Join(",", eventsTypesFilter));
                }

                queryParameters.Add(RestPathBuilder.ExcludeAnalysisEvents, excludeAnalysisEvent.ToString());
                queryParameters.Add(RestPathBuilder.SkipCorrelationLookup, skipCorrelationLookup.ToString());

                string resource = null;

                if (partitionId == Guid.Empty)
                {
                    resource = string.Format(RestPathBuilder.EventsStoreAllPartitionEvents);
                }
                else
                {
                    resource = string.Format(RestPathBuilder.EventsStorePartitionEvents, partitionId);
                }

                Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, resource, queryParameters, RestPathBuilder.Api62PreviewVersion);

                response = await this.MakeRequest(httpGatewayEndpoint, HttpGet).ConfigureAwait(false);
                using (Stream stream = response.GetResponseStream())
                {
                    StreamReader reader = new StreamReader(stream, Encoding.UTF8);
                    string json = reader.ReadToEnd();

                    JArray eventArray = JArray.Parse(json);
                    foreach (var eventInfo in eventArray)
                    {
                        partitionEvents.Add(eventInfo.ToString());
                    }
                }

            }).ConfigureAwait(false);

            return partitionEvents != null
                ? FabricClientState.CreateOperationResultFromNativeErrorCode(partitionEvents)
                : FabricClientState.CreateOperationResultFromNativeErrorCode<List<string>>(errorCode);
        }

#if !DotNetCoreClr

        //// BRS APIs.
        public override async Task<OperationResult> CreateBackupPolicyAsync(string policyJson)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
            delegate
            {
                Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses,
                    RestPathBuilder.CreateBackupPolicy, null, RestPathBuilder.FabricBrsApiVersion);
                byte[] bodyBuffer = Encoding.UTF8.GetBytes(policyJson);
                return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, bodyBuffer, DefaultContentType);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public async Task<OperationResult> CreateBackupPolicyAsync(BackupPolicy backupPolicy)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses,
                        RestPathBuilder.CreateBackupPolicy, null, RestPathBuilder.FabricBrsApiVersion);
                    byte[] bodyBuffer = this.GetSerializedObject(backupPolicy);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, bodyBuffer, DefaultContentType);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public async Task<OperationResult> UpdateBackupPolicyAsync(BackupPolicy backupPolicy)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses,
                        string.Format(RestPathBuilder.UpdateBackupPolicy, backupPolicy.Name), null, RestPathBuilder.FabricBrsApiVersion);
                    byte[] bodyBuffer = this.GetSerializedObject(backupPolicy);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, bodyBuffer, DefaultContentType);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public async Task<OperationResult> DeleteBackupPolicyAsync(string policyName)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.DeleteBackupPolicy, policyName), null, RestPathBuilder.FabricBrsApiVersion);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, null, DefaultContentType);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public async Task<OperationResult<PagedBackupEntityList>> GetBackupEnabledEntitiesByPolicyAsync(string policyName, string continuationToken = null, int maxResults = 0)
        {
            HttpWebResponse response = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    var queryParameters = new Dictionary<string, string>();

                    if (!String.IsNullOrEmpty(continuationToken))
                    {
                        queryParameters[RestPathBuilder.ContinuationToken] = continuationToken;
                    }

                    if (0 != maxResults)
                    {
                        queryParameters[RestPathBuilder.MaxResults] = maxResults.ToString();
                    }

                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.GetBackupEnabledEntities, policyName), queryParameters, RestPathBuilder.FabricBrsApiVersion);
                    response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet, null, DefaultContentType);
                });

            return GetOperationResultFromResponse<PagedBackupEntityList>(response, errorCode, null);
        }

        public async Task<OperationResult<PagedBackupPolicyDescriptionList>> GetBackupPoliciesAsync(string continuationToken = null, int maxResults = 0)
        {
            HttpWebResponse response = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    var queryParameters = new Dictionary<string, string>();

                    if (!String.IsNullOrEmpty(continuationToken))
                    {
                        queryParameters[RestPathBuilder.ContinuationToken] = continuationToken;
                    }

                    if (0 != maxResults)
                    {
                        queryParameters[RestPathBuilder.MaxResults] = maxResults.ToString();
                    }

                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, RestPathBuilder.GetBackupPolicies, queryParameters, RestPathBuilder.FabricBrsApiVersion);
                    response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet, null, DefaultContentType);
                });

            return GetOperationResultFromResponse<PagedBackupPolicyDescriptionList>(response, errorCode, null);
        }

        public async Task<OperationResult> EnableBackupProtectionAsync(BackupMapping backupMapping, string fabricUri, TimeSpan timeout)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.EnableBackup, fabricUri), null, RestPathBuilder.FabricBrsApiVersion);
                    byte[] bodyBuffer = this.GetSerializedObject(backupMapping);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, bodyBuffer, DefaultContentType);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public async Task<OperationResult> DisableBackupProtectionAsync(string fabricUri, TimeSpan timeout, CleanBackupView cleanBackupView = null)
        {
            if(cleanBackupView ==null)
            {
                cleanBackupView = new CleanBackupView();
                cleanBackupView.CleanBackup = false;
            }
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    byte[] bodyBuffer = this.GetSerializedObject(cleanBackupView);
                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.DisableBackup, fabricUri), null, RestPathBuilder.FabricBrsApiVersion);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, bodyBuffer, DefaultContentType);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public async Task<OperationResult<PagedBackupInfoList>> GetBackupsByBackupEntityAsync(BackupEntityKind entityKind, string entityName, bool latest = false,
            DateTime? startDateTimeFilter = null, DateTime? endDateTimeFilter = null, string continuationToken = null, int maxResults = 0)
        {
            HttpWebResponse response = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    var queryParameters = new Dictionary<string, string>();

                    if (latest)
                    {
                        queryParameters[RestPathBuilder.LatestQueryPatam] = "true";
                    }

                    if (!String.IsNullOrEmpty(continuationToken))
                    {
                        queryParameters[RestPathBuilder.ContinuationToken] = continuationToken;
                    }

                    if (0 != maxResults)
                    {
                        queryParameters[RestPathBuilder.MaxResults] = maxResults.ToString();
                    }

                    if (null != startDateTimeFilter)
                    {
                        queryParameters[RestPathBuilder.StartDateTimeFilter] = startDateTimeFilter.ToString();          // TODO: Verify if this format is correct
                    }

                    if (null != endDateTimeFilter)
                    {
                        queryParameters[RestPathBuilder.EndDateTimeFilter] = endDateTimeFilter.ToString();
                    }

                    Uri httpGatewayEndpoint = null;
                    switch (entityKind)
                    {
                        case BackupEntityKind.Partition:
                            httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.GetBackupByPartition, entityName), queryParameters, RestPathBuilder.FabricBrsApiVersion);
                            break;

                        case BackupEntityKind.Service:
                            httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.GetBackupByService, entityName), queryParameters, RestPathBuilder.FabricBrsApiVersion);
                            break;

                        case BackupEntityKind.Application:
                            httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.GetBackupByApplication, entityName), queryParameters, RestPathBuilder.FabricBrsApiVersion);
                            break;

                        default:
                            throw new InvalidOperationException("Invalid entitykind passed");
                    }

                    response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet, null, DefaultContentType);
                });

            return GetOperationResultFromResponse<PagedBackupInfoList>(response, errorCode, null);
        }

        public async Task<OperationResult<PagedBackupInfoList>> GetBackupsByStorageAsync(BackupByStorageQueryDescription backupByStorageDescription, string continuationToken = null, int maxResults = 0)
        {
            HttpWebResponse response = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    var queryParameters = new Dictionary<string, string>();

                    if (!String.IsNullOrEmpty(continuationToken))
                    {
                        queryParameters[RestPathBuilder.ContinuationToken] = continuationToken;
                    }

                    if (0 != maxResults)
                    {
                        queryParameters[RestPathBuilder.MaxResults] = maxResults.ToString();
                    }

                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, RestPathBuilder.GetBackupByStorage, queryParameters, RestPathBuilder.FabricBrsApiVersion);
                    byte[] bodyBuffer = this.GetSerializedObject(backupByStorageDescription);
                    response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, bodyBuffer, DefaultContentType);
                });

            return GetOperationResultFromResponse<PagedBackupInfoList>(response, errorCode, null);
        }

        public async Task<OperationResult> RestorePartitionAsync(Guid partitionId, RestoreRequest restoreDetails)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.RestorePartition, partitionId), null, RestPathBuilder.FabricBrsApiVersion);
                    byte[] bodyBuffer = this.GetSerializedObject(restoreDetails);
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, bodyBuffer, DefaultContentType);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public async Task<OperationResult<RestoreProgress>> GetRestoreProgressAsync(Guid partitionId)
        {
            HttpWebResponse response = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.GetRestoreProgress, partitionId), null, RestPathBuilder.FabricBrsApiVersion);
                    response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet, null, DefaultContentType);
                });

            return GetOperationResultFromResponse<RestoreProgress>(response, errorCode, null);
        }

        public async Task<OperationResult> BackupPartitionAsync(Guid partitionId, BackupPartitionRequest backupPartitionDetails = null)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.BackupPartition, partitionId), null, RestPathBuilder.FabricBrsApiVersion);
                    byte[] bodyBuffer = null;
                    if (null != backupPartitionDetails)
                    {
                        bodyBuffer = this.GetSerializedObject(backupPartitionDetails);
                    }
                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, bodyBuffer, DefaultContentType);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public async Task<OperationResult<BackupProgress>> GetBackupProgressAsync(Guid partitionId)
        {
            HttpWebResponse response = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.GetBackupProgress, partitionId), null, RestPathBuilder.FabricBrsApiVersion);
                    response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet, null, DefaultContentType);
                });

            return GetOperationResultFromResponse<BackupProgress>(response, errorCode, null);
        }

        public async Task<OperationResult<BackupPolicy>> GetBackupPolicyByNameAsync(string policyName)
        {
            HttpWebResponse response = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.GetBackupPolicy, policyName), null, RestPathBuilder.FabricBrsApiVersion);
                    response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet, null, DefaultContentType);
                });

            return GetOperationResultFromResponse<BackupPolicy>(response, errorCode, null);
        }

        public override async Task<OperationResult> EnableBackupProtectionAsync(string policyNameJson, string fabricUri, TimeSpan timeout)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
            delegate
            {
                Uri httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.EnableBackup, fabricUri), null, RestPathBuilder.FabricBrsApiVersion);

                byte[] bodyBuffer = Encoding.UTF8.GetBytes(policyNameJson);
                return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, bodyBuffer, DefaultContentType);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public async Task<OperationResult> SuspendBackupAsync(BackupEntityKind entityKind, string entityName)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    Uri httpGatewayEndpoint = null;
                    switch (entityKind)
                    {
                        case BackupEntityKind.Partition:
                            httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.SuspendPartitionBackup, entityName), null, RestPathBuilder.FabricBrsApiVersion);
                            break;

                        case BackupEntityKind.Service:
                            httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.SuspendServiceBackup, entityName), null, RestPathBuilder.FabricBrsApiVersion);
                            break;

                        case BackupEntityKind.Application:
                            httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.SuspendApplicationBackup, entityName), null, RestPathBuilder.FabricBrsApiVersion);
                            break;

                        default:
                            throw new InvalidOperationException("Invalid entity kind passed");
                    }

                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, null, DefaultContentType);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public async Task<OperationResult> ResumeBackupAsync(BackupEntityKind entityKind, string entityName)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                delegate
                {
                    Uri httpGatewayEndpoint = null;
                    switch (entityKind)
                    {
                        case BackupEntityKind.Partition:
                            httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.ResumePartitionBackup, entityName), null, RestPathBuilder.FabricBrsApiVersion);
                            break;

                        case BackupEntityKind.Service:
                            httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.ResumeServiceBackup, entityName), null, RestPathBuilder.FabricBrsApiVersion);
                            break;

                        case BackupEntityKind.Application:
                            httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.ResumeApplicationBackup, entityName), null, RestPathBuilder.FabricBrsApiVersion);
                            break;

                        default:
                            throw new InvalidOperationException("Invalid entity kind passed");
                    }

                    return MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, null, DefaultContentType);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public async Task<OperationResult<BackupConfigurationInfo>> GetBackupConfigurationInfoByBackupEntityAsync(BackupEntityKind entityKind, string entityName, string continuationToken = null, int maxResults = 0)
        {
            HttpWebResponse response = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    var queryParameters = new Dictionary<string, string>();

                    if (!String.IsNullOrEmpty(continuationToken))
                    {
                        queryParameters[RestPathBuilder.ContinuationToken] = continuationToken;
                    }

                    if (0 != maxResults)
                    {
                        queryParameters[RestPathBuilder.MaxResults] = maxResults.ToString();
                    }

                    Uri httpGatewayEndpoint = null;
                    switch (entityKind)
                    {
                        case BackupEntityKind.Partition:
                            httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.GetPartitionBackupConfigurationInfo, entityName), queryParameters, RestPathBuilder.FabricBrsApiVersion);
                            break;

                        case BackupEntityKind.Service:
                            httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.GetServiceBackupConfigurationInfo, entityName), queryParameters, RestPathBuilder.FabricBrsApiVersion);
                            break;

                        case BackupEntityKind.Application:
                            httpGatewayEndpoint = RestPathBuilder.GetBaseResourceUri(this.httpGatewayAddresses, string.Format(RestPathBuilder.GetApplicationBackupConfigurationInfo, entityName), queryParameters, RestPathBuilder.FabricBrsApiVersion);
                            break;

                        default:
                            throw new InvalidOperationException("Invalid entity kind passed");
                    }

                    response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet, null, DefaultContentType);
                });

            return GetOperationResultFromResponse<BackupConfigurationInfo>(response, errorCode, null);
        }

#endif

        private async Task<HttpWebResponse> MakeRequest(Uri httpGatewayEndpoint, string method, byte[] buffer = null, string contentType = null, IDictionary<String, string> customizedHeaderProperties = null)
        {
            HttpWebRequest request = (HttpWebRequest)WebRequest.Create(httpGatewayEndpoint);
            request.Method = method;
            string data = string.Empty;

            if (customizedHeaderProperties != null && customizedHeaderProperties.Count() > 0)
            {
                foreach (var property in customizedHeaderProperties)
                {
                    request.Headers.Add(property.Key, property.Value);
                    TestabilityTrace.TraceSource.WriteInfo(TraceSource, "Add customized header property: ['{0}':'{1}']", property.Key, property.Value);
                }
            }

            if (this.clusterCredentialType == CredentialType.Windows)
            {
                request.UseDefaultCredentials = true;
            }
            else if (clusterCredentialType == CredentialType.X509)
            {
                request.ClientCertificates.Add(clientCertificate);
            }
            //ClaimsCredentials can technically be used for both DSTS and AAD, but ScriptTest is only supporting AAD at this point
            else if (clusterCredentialType == CredentialType.Claims)
            {
                request.Headers.Add("Authorization", string.Format("Bearer {0}", this.bearer));
            }

            if (method == RestFabricClient.HttpPost || method == RestFabricClient.HttpPut)
            {
                request.ContentType = string.IsNullOrEmpty(contentType) ? DefaultContentType : contentType;
                if (buffer != null)
                {
                    request.ContentLength = buffer.Length;
                    using (Stream dataStream = request.GetRequestStream())
                    {
                        dataStream.Write(buffer, 0, buffer.Length);
                    }

                    data = contentType ?? Encoding.UTF8.GetString(buffer, 0, buffer.Length);
                }
                else
                {
                    request.ContentLength = 0;
                }
            }

            HttpWebResponse response = null;
            try
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceSource, "REST ran the following: '{0}'", request.RequestUri);
                response = (HttpWebResponse)await request.GetResponseAsync().ConfigureAwait(false);
            }
            catch (Exception e)
            {
                Console.WriteLine("REST failed running: '{0}'", request.RequestUri);
                Console.WriteLine("Caught: '{0}'", e);
                throw;
            }

            if (response != null && (response.StatusCode == HttpStatusCode.OK || response.StatusCode == HttpStatusCode.Accepted || response.StatusCode == HttpStatusCode.Created || response.StatusCode == HttpStatusCode.NoContent))
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceSource, "Command: '{0}', Results: {1}", httpGatewayEndpoint.OriginalString, response.StatusDescription);
            }
            else
            {
                string description = response != null && !string.IsNullOrEmpty(response.StatusDescription) ? response.StatusDescription : "Unknown";
                TestabilityTrace.TraceSource.WriteWarning(TraceSource, "Command: '{0}', Error Message: {1}", httpGatewayEndpoint.OriginalString, description);
            }

            return response;
        }

        private async Task<Tuple<HttpWebResponse, NativeTypes.FABRIC_ERROR_CODE>> GetRestResponse(Uri httpGatewayEndpoint, object objToSerializeForPostRequest)
        {
            HttpWebResponse response = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    if (objToSerializeForPostRequest == null)
                    {
                        response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet);
                    }
                    else
                    {
                        byte[] buffer = this.GetSerializedObject(objToSerializeForPostRequest);
                        response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpPost, buffer);
                    }
                });

            return new Tuple<HttpWebResponse, NativeTypes.FABRIC_ERROR_CODE>(response, errorCode);
        }

        private async Task<OperationResult<T>> GetOperationResultFromRequest<T>(string[] resourceTypes, string[] resourceIds, object objToSerializeForPostRequest)
        {
            Uri httpGatewayEndpoint = RestPathBuilder.GetResourceUriById(this.httpGatewayAddresses, resourceTypes, resourceIds);
            var result = await this.GetOperationResultFromRequest<T>(httpGatewayEndpoint, objToSerializeForPostRequest);
            return result;
        }

        private async Task<OperationResult<T>> GetOperationResultFromRequest<T>(Uri httpGatewayEndpoint, object objToSerializeForPostRequest)
        {
            OperationResult<T> result = null;
            var responseAndErrorCode = await GetRestResponse(httpGatewayEndpoint, objToSerializeForPostRequest);

            result = this.GetOperationResultFromResponse<T>(responseAndErrorCode.Item1, responseAndErrorCode.Item2);
            return result;
        }

        private async Task<OperationResult<T>> GetOperationPagedResultListFromRequest<T>(Uri httpGatewayEndpoint, object objToSerializeForPostRequest) where T : new()
        {
            var responseAndErrorCode = await GetRestResponse(httpGatewayEndpoint, objToSerializeForPostRequest);
            return this.GetOperationResultFromResponse<T>(responseAndErrorCode.Item1, responseAndErrorCode.Item2, new T());
        }

        private async Task<OperationResult<T[]>> GetOperationResultArrayFromRequest<T>(Uri httpGatewayEndpoint, object objToSerializeForPostRequest)
        {
            OperationResult<T[]> result = null;
            var responseAndErrorCode = await GetRestResponse(httpGatewayEndpoint, objToSerializeForPostRequest);

            result = this.GetOperationResultFromResponse<T[]>(responseAndErrorCode.Item1, responseAndErrorCode.Item2, new T[0]);
            return result;
        }

        private OperationResult<T> GetOperationResultFromResponse<T>(HttpWebResponse response, NativeTypes.FABRIC_ERROR_CODE errorCode, T defaultVal = default(T))
        {
            T result = defaultVal;
            if (errorCode == NativeTypes.FABRIC_ERROR_CODE.S_OK && response != null && response.StatusCode != HttpStatusCode.NoContent)
            {
                // success. Deserializer rest response
                result = this.GetDeserializedObject<T>(response.GetResponseStream());
            }

            return FabricClientState.CreateOperationResultFromNativeErrorCode<T>(errorCode, result);
        }

        private OperationResult<T[]> ConvertOperationResultArray<T>(OperationResult<T> result)
        {
            T[] resultArray = new T[0];
            if (result.ErrorCode == 0 && result.Result != null)
            {
                // result array with one item
                resultArray = new T[] { result.Result };
            }

            return new OperationResult<T[]>(result.ErrorCode, resultArray);
        }

        private OperationResult<TList> ConvertOperationResultPagedList<TList, TItem>(OperationResult<TItem> resultItem) where TList : PagedList<TItem>, new()
        {
            TList result = new TList();
            if (resultItem.ErrorCode == 0 && resultItem.Result != null)
            {
                // Add the item to the paged list
                result.Add(resultItem.Result);
            }

            return new OperationResult<TList>(resultItem.ErrorCode, result);
        }

        private static string StripRootUri(string resourceUri)
        {
            ThrowIf.NullOrEmpty(resourceUri, "resourceUri");

            int index = resourceUri.IndexOf(":/", StringComparison.Ordinal);
            if (index > 0)
            {
                return resourceUri.Substring(index + 2);
            }
            else
            {
                throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Invalid resource uri {0}", resourceUri));
            }
        }

        private static string EscapeAndStripRootUri(Uri name)
        {
            return Uri.EscapeUriString(RestFabricClient.StripRootUri(name.OriginalString));
        }

        private static string EscapeAndStripRootUri(string name)
        {
            return Uri.EscapeUriString(RestFabricClient.StripRootUri(name));
        }

        private static async Task<NativeTypes.FABRIC_ERROR_CODE> HandleExceptionAsync(Func<Task> operation)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = 0;

            try
            {
                await operation();
                errorCode = 0; // Success
            }
            catch (WebException webEx)
            {
                errorCode = ExtractFabricErrorCode(webEx);
                ReleaseAssert.AssertIf(errorCode == 0, "HRESULT cannot be 0 for exception: {0}", webEx);
            }
            catch (AggregateException e)
            {
                WebException webEx = e.InnerExceptions.Where(i => i is WebException).First() as WebException;
                errorCode = ExtractFabricErrorCode(webEx);
                ReleaseAssert.AssertIf(errorCode == 0, "HRESULT cannot be 0 for exception: {0}", webEx);
            }

            return errorCode;
        }

        private static NativeTypes.FABRIC_ERROR_CODE ExtractFabricErrorCode(WebException webEx)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_OPERATION;

            HttpWebResponse response = (HttpWebResponse)webEx.Response;
            if (response != null)
            {
                string httpStatusDescription = response.StatusDescription;
                if (!string.IsNullOrEmpty(httpStatusDescription))
                {
                    if (httpStatusDescription.StartsWith(RestFabricClient.FabricError1, StringComparison.Ordinal) ||
                        httpStatusDescription.StartsWith(RestFabricClient.FabricError2, StringComparison.Ordinal))
                    {
                        errorCode = FabricClientState.ConvertToEnum<NativeTypes.FABRIC_ERROR_CODE>(httpStatusDescription);
                    }
                    else if (httpStatusDescription.Contains(RestFabricClient.TimeoutError))
                    {
                        errorCode = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TIMEOUT;
                    }
                    else if (httpStatusDescription.Contains(RestFabricClient.BadRequestError))
                    {
                        errorCode = NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG;
                        TestabilityTrace.TraceSource.WriteWarning(TraceSource, "REST: Unexpected bad request http status description {0} original exception {1}", httpStatusDescription, webEx);
                    }
                    else if (response.StatusCode == HttpStatusCode.BadGateway
                          || response.StatusCode == HttpStatusCode.ServiceUnavailable)
                    {
                        errorCode = NativeTypes.FABRIC_ERROR_CODE.E_ABORT;
                    }
                    else if (response.StatusCode == HttpStatusCode.Forbidden)
                    {
                        errorCode = NativeTypes.FABRIC_ERROR_CODE.E_ACCESSDENIED;
                    }
                    else
                    {
                        TestabilityTrace.TraceSource.WriteWarning(TraceSource, "REST: Unknown StatusDescription http status description {0} original exception {1}", httpStatusDescription, webEx);
                    }
                }
                else
                {
                    if (response.StatusCode == HttpStatusCode.GatewayTimeout)
                    {
                        errorCode = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TIMEOUT;
                    }
                    else if (response.StatusCode == HttpStatusCode.BadGateway)
                    {
                        errorCode = NativeTypes.FABRIC_ERROR_CODE.E_ABORT;
                    }
                    else
                    {
                        TestabilityTrace.TraceSource.WriteWarning(TraceSource, "REST: Unknown StatusCode {0} exception {1}", response.StatusCode, webEx);
                    }
                }
            }
            else
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceSource, "REST: HttpWebResponse is null. Assuming E_ABORT exception {0}", webEx);
                errorCode = NativeTypes.FABRIC_ERROR_CODE.E_ABORT;
            }

            return errorCode;
        }

        private static X509Certificate2 LookUpClientCertificate(X509Credentials credentials)
        {
            var store = new X509Store(credentials.StoreName, credentials.StoreLocation);
            store.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly);
            var collection = (X509Certificate2Collection)store.Certificates;
            var ret = (X509Certificate2Collection)collection.Find(credentials.FindType, credentials.FindValue, true);

            return ret[0];
        }

        public bool ValidateCertificate(object sender, X509Certificate cert, X509Chain chain, SslPolicyErrors sslPolicyErrors)
        {
            if (sslPolicyErrors == SslPolicyErrors.RemoteCertificateNameMismatch)
            {
                X509Certificate2 cert2 = cert as X509Certificate2;
                if (cert2 != null)
                {
                    if (this.ServerCommonNames != null)
                    {
                        foreach (string commonName in this.ServerCommonNames)
                        {
                            if (cert2.Subject == commonName ||
                                cert2.GetNameInfo(X509NameType.SimpleName, false) == commonName ||
                                cert2.GetNameInfo(X509NameType.DnsFromAlternativeName, false) == commonName ||
                                cert2.GetNameInfo(X509NameType.DnsName, false) == commonName)
                            {
                                return true;
                            }
                        }
                    }
                    else if (this.ServerCertThumbprints != null)
                    {
                        return this.ServerCertThumbprints.Any(p => string.Equals(cert2.Thumbprint, p, StringComparison.OrdinalIgnoreCase));
                    }
                }
            }

            return sslPolicyErrors == SslPolicyErrors.None;
        }

        private static T[] MakeArray<T>(Stream stream, DataContractJsonSerializer serializer)
        {
            return ((IEnumerable<T>)serializer.ReadObject(stream)).ToArray();
        }

        private static string GetStoreRelativePath(string fullName, string applicationPackagePathInImageStore, int prefixIndex)
        {
            return string.IsNullOrWhiteSpace(applicationPackagePathInImageStore) ? fullName.Substring(prefixIndex) : Path.Combine(applicationPackagePathInImageStore, fullName.Substring(prefixIndex + 1));
        }

        private static void ProcessDirectory(DirectoryInfo sourceDirectory, int prefixIndex, string applicationPackagePathInImageStore, Dictionary<FileInfo, string> filesToUpload)
        {
            DirectoryInfo[] subFolders = sourceDirectory.GetDirectories();
            foreach (var subFolder in subFolders)
            {
                ProcessDirectory(subFolder, prefixIndex, applicationPackagePathInImageStore, filesToUpload);
            }

            FileInfo[] files = sourceDirectory.GetFiles();
            foreach (var file in files)
            {
                filesToUpload.Add(file, GetStoreRelativePath(file.FullName, applicationPackagePathInImageStore, prefixIndex));
            }

            string markFileName = Path.Combine(sourceDirectory.FullName, "_.dir");
            File.Create(markFileName).Dispose();
            FileInfo markFile = new FileInfo(markFileName);
            filesToUpload.Add(markFile, GetStoreRelativePath(markFile.FullName, applicationPackagePathInImageStore, prefixIndex));
        }

        private static byte[] GetSerializedObject(DataContractJsonSerializer serializer, object objectToSerialize)
        {
            using (MemoryStream ms = new MemoryStream())
            {
                serializer.WriteObject(ms, objectToSerialize);
                return ms.ToArray();
            }
        }

        private T GetDeserializedObject<T>(Stream ioStream)
        {
            var obj = this.jsonSerializer.Deserialize(ioStream, typeof(T));
            return (T)obj;
        }

        private byte[] GetSerializedObject(object objectToSerialize)
        {
            using (MemoryStream mStream = new MemoryStream())
            {
                // This is using a custom serializer.
                this.jsonSerializer.Serialize(objectToSerialize, mStream);
                return mStream.ToArray();
            }
        }

        private async Task<OperationResult<T>> Inner<T>(string progressType, Guid operationId, Uri serviceName, Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken) where T : TestCommandProgress
        {
            OperationResult<T> result = null;
            T progressResult = default(T);

            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(
                async delegate
                {
                    string serviceId = RestFabricClient.EscapeAndStripRootUri(serviceName.OriginalString);
                    string suffix = string.Format(CultureInfo.InvariantCulture, "/Faults/Services/{0}/$/GetPartitions/{1}/$/{2}", serviceId, partitionId, progressType);
                    string queryParameters = string.Format(CultureInfo.InvariantCulture, "?api-version=1.0&OperationId={0}", operationId.ToString());

                    Uri httpGatewayEndpoint = RestPathBuilder.BuildRequestEndpoint(this.httpGatewayAddresses, suffix, queryParameters);

                    Console.WriteLine("progressType - '{0}'", httpGatewayEndpoint.ToString());

                    HttpWebResponse response = await MakeRequest(httpGatewayEndpoint, RestFabricClient.HttpGet);
                    progressResult = this.GetDeserializedObject<T>(response.GetResponseStream());
                });

            // This is necessary because the rest version of these APIs do not go through System.Fabric, so the parts that System.Fabric populates (progress.Result.Exception), as opposed
            // to forwarding from native, are null here.  In order for the consumers to have a uniform view above the client switch, we need to populate progress.Result.Exception here.
            if (progressResult != null)
            {
                if (progressResult.State == TestCommandProgressState.Faulted)
                {
                    if (progressResult is PartitionDataLossProgress)
                    {
                        PartitionDataLossProgress progress = progressResult as PartitionDataLossProgress;
                        if (progress.Result.ErrorCode != 0)
                        {
                            Exception exception = InteropHelpers.TranslateError(progress.Result.ErrorCode);
                            progress.Result.Exception = exception;
                        }
                    }
                    else if (progressResult is PartitionQuorumLossProgress)
                    {
                        PartitionQuorumLossProgress progress = progressResult as PartitionQuorumLossProgress;
                        if (progress.Result.ErrorCode != 0)
                        {
                            Exception exception = InteropHelpers.TranslateError(progress.Result.ErrorCode);
                            progress.Result.Exception = exception;
                        }
                    }
                    else if (progressResult is PartitionRestartProgress)
                    {
                        PartitionRestartProgress progress = progressResult as PartitionRestartProgress;
                        if (progress.Result.ErrorCode != 0)
                        {
                            Exception exception = InteropHelpers.TranslateError(progress.Result.ErrorCode);
                            progress.Result.Exception = exception;
                        }
                    }
                    else
                    {
                        throw new InvalidOperationException("Invalid type - there is probably a missing case block");
                    }
                }

                result = FabricClientState.CreateOperationResultFromNativeErrorCode<T>(progressResult);
            }
            else
            {
                result = FabricClientState.CreateOperationResultFromNativeErrorCode<T>(errorCode);
            }

            return result;
        }

        private static long ToFileTimeTicks(DateTime datetime)
        {
            var fileTimeEpoch = new DateTime(1601, 01, 01, 0, 0, 0, DateTimeKind.Utc);
            return (datetime - fileTimeEpoch).Ticks;
        }
    }

    [Serializable]
    [DataContract]
    internal class DeactivateWrapper
    {
        public DeactivateWrapper()
        { }

        [DataMember]
        public NodeDeactivationIntent DeactivationIntent;
    }
}

#pragma warning restore 1591
