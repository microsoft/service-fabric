// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{    
    using Microsoft.Win32;
    using Net;
    using RD.Fabric.PolicyAgent;
    using Threading;
    using Threading.Tasks;
    using System.Net.Http;
    using System.Net.Http.Headers;

    /// <summary>
    /// This particular implementation gets documents etc. by looking up the registry for an endpoint
    /// to the wiresever. (More like a proxy to the real service)
    /// </summary>
    internal class PolicyAgentServiceWrapper : IPolicyAgentServiceWrapper, IAzureModeDetector
    {
        private const string VersionsUriRegKey =
            @"HKEY_LOCAL_MACHINE\Software\Microsoft\Windows Azure\DeploymentManagement";

        private const string VersionsUriRegValue = "VersionsUri";

        private const string MRZeroSdkControllerPath = "mrzerosdk";

        private const string WebContentTypeKey = "Content-Type";

        private const string WebContentTypeValue = "octet-stream";

        private const string WebContentLengthKey = "Content-Length";

        private readonly TraceType traceType;
        
        private readonly Uri zeroSdkUri;

        private readonly IActivityLogger activityLogger;

        private delegate void WriteTraceDelegate(string format, params object[] args);

        private readonly WriteTraceDelegate traceWriteConditionalWarning;

        public PolicyAgentServiceWrapper(CoordinatorEnvironment environment, IActivityLogger activityLogger, Uri zeroSdkUri = null, bool silentErrors = false)
        {
            this.traceType = environment.Validate("environment").CreateTraceType("ZeroSdkClient");
            this.activityLogger = activityLogger.Validate("activityLogger");
            this.traceWriteConditionalWarning = silentErrors ? new WriteTraceDelegate(traceType.WriteInfo) : new WriteTraceDelegate(traceType.WriteWarning);
            this.zeroSdkUri = zeroSdkUri ?? GetMRZeroSdkUri(environment.Config);
        }

        public async Task<AzureMode> DetectModeAsync()
        {
            try
            {
                byte[] responseBytes = await GetDocumentFromWireServerAsync().ConfigureAwait(false);
                var doc = responseBytes.GetBondObjectFromPayload<PolicyAgentDocumentForTenant>();

                // The ZeroSDK document should always be present. The presence or absense of the
                // JobInfo property indicates whether the tenant is in parallel or serial mode.
                if (doc.JobInfo == null)
                {
                    return AzureMode.Serial;
                }

                return AzureMode.Parallel;
            }
            catch (Exception e)
            {
                traceType.WriteWarning("Failed to detect Azure mode: {0}", e);
                return AzureMode.Unknown;
            }
        }

        public async Task<IPolicyAgentDocumentForTenant> GetDocumentAsync(Guid activityId, CancellationToken cancellationToken)
        {            
            var startTime = DateTimeOffset.UtcNow;

            try
            {
                byte[] responseBytes = await GetDocumentFromWireServerAsync().ConfigureAwait(false);                
                var doc = responseBytes.GetBondObjectFromPayload<PolicyAgentDocumentForTenant>();

                traceType.WriteInfo("Successfully got policy agent document ({0} bytes)", responseBytes.Length);
                activityLogger.LogOperation(activityId, startTime);

                // Absence of the JobInfo property indicates that the tenant is not configured in parallel mode.
                if (doc.JobInfo == null)
                {
                    traceType.WriteWarning("Policy agent document does not contain job info.");
                    return null;
                }

                return new PolicyAgentDocumentForTenantWrapper(doc);
            }
            catch (Exception ex)
            {
                string text = "Unable to get policy document. Error: {0}".ToString(ex);
                traceWriteConditionalWarning("{0}", text);
                activityLogger.LogOperation(activityId, startTime, OperationResult.Failure, ex);
                throw new ManagementException(text, ex);
            }
        }

        public async Task<byte[]> GetDocumentRawAsync(Guid activityId, CancellationToken cancellationToken)
        {
            var startTime = DateTimeOffset.UtcNow;

            try
            {
                byte[] responseBytes = await GetDocumentFromWireServerAsync().ConfigureAwait(false);

                traceType.WriteInfo("Successfully got raw policy agent document ({0} bytes)", responseBytes.Length);
                activityLogger.LogOperation(activityId, startTime);

                return responseBytes;
            }
            catch (Exception ex)
            {
                string text = "Unable to get policy document. Error: {0}".ToString(ex);
                traceWriteConditionalWarning("{0}", text);
                activityLogger.LogOperation(activityId, startTime, OperationResult.Failure, ex);
                throw new ManagementException(text, ex);
            }
        }

        public async Task PostPolicyAgentRequestAsync(Guid activityId, PolicyAgentRequest request, CancellationToken cancellationToken)
        {
            request.Validate("request");

            traceType.WriteInfo("Posting request: {0}", request.ToJson());

            var startTime = DateTimeOffset.UtcNow;

            try
            {
                byte[] payload = request.GetPayloadFromBondObject();

                await PostDocumentToWireServerAsync(payload).ConfigureAwait(false);
                activityLogger.LogOperation(activityId, startTime);
            }
            catch (Exception ex)
            {
                string text = "Unable to post policy document. Request: {0}{1}Error: {2}".ToString(
                    request.ToJson(),
                    Environment.NewLine,
                    ex);
                traceType.WriteWarning("{0}", text);
                activityLogger.LogOperation(activityId, startTime, OperationResult.Failure, ex);
                throw new ManagementException(text, ex);
            }
        }

        private Uri GetMRZeroSdkUri(IConfigSection configSection)
        {
            try
            {
                string uriString = configSection.ReadConfigValue<string>(Constants.ConfigKeys.MRZeroSdkUri);

                Uri uri = string.IsNullOrWhiteSpace(uriString) 
                    ? GetUriFromRegistry() 
                    : new Uri(uriString, UriKind.Absolute);

                // using original string since a regular ToString() will not display the port no. which can be confusing while looking up traces
                traceType.WriteInfo("Tenant policy agent endpoint: {0}", uri.OriginalString); 
                return uri;
            }
            catch (Exception ex)
            {
                traceWriteConditionalWarning("Error while getting tenant policy agent endpoint. Exception: {0}", ex);
                throw;
            }
        }

        private Uri GetUriFromRegistry()
        {
            string versionsUriString = (string)Registry.GetValue(VersionsUriRegKey, VersionsUriRegValue, null);

            if (string.IsNullOrWhiteSpace(versionsUriString))
            {
                string message = "Unable to get tenant policy agent endpoint from registry; verify that tenant settings match InfrastructureService configuration";
                traceWriteConditionalWarning(message);
                throw new ManagementException(message);
            }

            Uri versionsUri = new Uri(versionsUriString, UriKind.Absolute);

            Uri uri = new UriBuilder(versionsUri.Scheme, versionsUri.Host, versionsUri.Port, MRZeroSdkControllerPath).Uri;
            return uri;
        }

        private async Task<byte[]> GetDocumentFromWireServerAsync()
        {            
            using (var client = new HttpClient())
            {
                using (HttpResponseMessage responseMessage = await client.GetAsync(zeroSdkUri).ConfigureAwait(false))
                {
                    if (!responseMessage.IsSuccessStatusCode)
                    {
                        string message =
                            "Get document failed with HTTP status code {0}{1}Response message: {2}".ToString(
                                responseMessage.StatusCode, Environment.NewLine, responseMessage.ToJson());
                        traceWriteConditionalWarning("{0}", message);

                        if (responseMessage.StatusCode == HttpStatusCode.GatewayTimeout ||
                            responseMessage.StatusCode == HttpStatusCode.RequestTimeout)
                        {
                            throw new TimeoutException(message);
                        }

                        if (responseMessage.StatusCode == HttpStatusCode.NoContent)
                        {
                            traceWriteConditionalWarning("Get document returned no content which is unexpected, returning null.");
                            return null;
                        }

                        throw new ManagementException(message);
                    }

                    byte[] payload = await responseMessage.Content.ReadAsByteArrayAsync().ConfigureAwait(false);

                    if (payload == null)
                    {
                        const string message = "Get document failed to get payload";

                        traceWriteConditionalWarning(message);
                        throw new ManagementException(message);
                    }

                    traceType.WriteNoise("Get document response stream payload obtained. Bytes: {0}", payload.Length);

                    return payload;
                }
            }
        }

        private async Task PostDocumentToWireServerAsync(byte[] payload)
        {
            using (var client = new HttpClient())
            {
                HttpContent content = new ByteArrayContent(payload);

                // for some reason, just Add throws an exception, whereas TryAddwithoutValidation works
                bool added1 = content.Headers.TryAddWithoutValidation(WebContentTypeKey, WebContentTypeValue);
                bool added2 = content.Headers.TryAddWithoutValidation(WebContentLengthKey, payload.Length.ToString());

                client.DefaultRequestHeaders
                    .Accept
                    .Add(new MediaTypeWithQualityHeaderValue("application/octet-stream"));

                traceType.WriteNoise(
                    "Added headers before post document, url: {0}{1}" +
                    "Content headers: {2}, TryAddWithoutValidation status: {3}, {4}{5}" +
                    "Request headers: {6}",
                    zeroSdkUri.OriginalString, Environment.NewLine,
                    content.Headers.ToJson(), added1, added2, Environment.NewLine,
                    client.DefaultRequestHeaders.ToJson());

                using (HttpResponseMessage responseMessage = await client.PostAsync(zeroSdkUri, content).ConfigureAwait(false))
                {                    
                    if (responseMessage.IsSuccessStatusCode)
                    {
                        traceType.WriteInfo("Post document returned HTTP status code: {0}", responseMessage.StatusCode);
                        return;
                    }

                    string message =
                        "Post document failed with HTTP status code {0}{1}Response message: {2}".ToString(
                            responseMessage.StatusCode, Environment.NewLine, responseMessage.ToJson());
                    traceType.WriteError("{0}", message);

                    if (responseMessage.StatusCode == HttpStatusCode.GatewayTimeout ||
                        responseMessage.StatusCode == HttpStatusCode.RequestTimeout)
                    {
                        throw new TimeoutException(message);
                    }

                    throw new ManagementException(message);                    
                }
            }
        }
    }
}