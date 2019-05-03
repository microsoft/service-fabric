// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting.ContainerActivatorService
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric;
    using System.IO;
    using System.Net;
    using System.Text;
    using System.Threading.Tasks;
    using Newtonsoft.Json.Linq;
    using Microsoft.ServiceFabric.ContainerServiceClient;
    using Microsoft.ServiceFabric.ContainerServiceClient.Config;

    internal class ContainerImageDownloader
    {
        #region Private Members

        private const string TraceType = "ContainerImageDownloader";

        private readonly ContainerActivatorService activator;
        private readonly ConcurrentDictionary<string, object> imageCache;
        private readonly TimeSpan downloadProgressStatusInterval;

        // CRYPT_E_ASN1_BADTAG = -2146881269 (0x8009310b) is a windows error code that is not defined
        // and successfully mapped by service fabric resulting in an unreadable error (ASN1 bad tag value met)
        // back to the user.
        private readonly int CRYPT_E_ASN1_BADTAG = -2146881269;

        private string cachedToken = "";
        private WebClient webClient;

        private enum CredentialType
        {
            AppManifestCredentials,
            ClusterManifestDefaultCredentials,
            TokenCredentials
        }

        #endregion

        // This has to be in sync with the ServiceFabricServiceModel.xsd EnvironmentVariableType attribute Type
        // and Hosting Constants SecretsStoreRef and Encrypted.
        enum Type { SecretsStoreRef, PlainText, Encrypted };

        public ContainerImageDownloader(ContainerActivatorService activator)
        {
            this.activator = activator;
            this.imageCache = new ConcurrentDictionary<string, object>();
            this.downloadProgressStatusInterval = TimeSpan.FromSeconds(15);
            webClient = new WebClient();

            #if !DotNetCoreClr
                ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;
            #endif
        }

        public Task DownloadImagesAsync(
            List<ContainerImageDescription> images,
            TimeSpan timeout)
        {
            var imageDownloadTasks = new List<Task>(images.Count);

            foreach (var image in images)
            {
                imageDownloadTasks.Add(this.DownloadImageAsync(image, timeout));
            }

            return Task.WhenAll(imageDownloadTasks);
        }

        public async Task DownloadImageAsync(ContainerImageDescription imageDescription, TimeSpan timeout, bool skipCacheCheck = false)
        {
            var timeoutHelper = new TimeoutHelper(timeout);

            var imageTags = imageDescription.ImageName.Split(':');
            bool isLatest = false;
            if (imageTags.Length < 2 || String.IsNullOrEmpty(imageTags[1]))
            {
                imageDescription.ImageName = String.Concat(imageDescription.ImageName, ":latest");
                isLatest = true;
            }

            if (!skipCacheCheck && !isLatest)
            {
                if (this.IsImageCached(imageDescription.ImageName))
                {
                    HostingTrace.Source.WriteInfo(
                        TraceType,
                        "Container image download skipped as it is cached: ImageName={0}, Time={1}ms .", imageDescription.ImageName, 0);
                    return;
                }

                if (await this.IsImagePresentLocallyAsync(imageDescription.ImageName, timeoutHelper.RemainingTime))
                {
                    HostingTrace.Source.WriteInfo(
                        TraceType,
                        "Container image download skipped as it is present locally: ImageName={0}, Time={1}ms .", imageDescription.ImageName, 0);
                    this.AddImageToCache(imageDescription.ImageName);

                    return;
                }
            }

            CredentialType currentCredentialAttempt;
            if (imageDescription.UseTokenAuthenticationCredentials)
            {
                currentCredentialAttempt = CredentialType.TokenCredentials;
            }
            else if (imageDescription.UseDefaultRepositoryCredentials)
            {
                currentCredentialAttempt = CredentialType.ClusterManifestDefaultCredentials;
            }
            else
            {
                currentCredentialAttempt = CredentialType.AppManifestCredentials;
            }

            try
            {
                AuthConfig authConfig = this.GetAuthConfig(imageDescription.RepositoryCredential, imageDescription.ImageName, currentCredentialAttempt);

                var downloadProgressStream = await Utility.ExecuteWithRetryOnTimeoutAsync(
                    (operationTimeout) =>
                    {
                        return this.ImageOperation.CreateImageAsync(
                            imageDescription.ImageName,
                            authConfig,
                            operationTimeout);
                    },
                    $"CreateImageAsync_{imageDescription.ImageName}",
                    TraceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    timeoutHelper.RemainingTime);

                await this.TrackDownloadProgressAsync(
                    imageDescription.ImageName, downloadProgressStream, timeoutHelper.RemainingTime);

                if (!await this.IsImagePresentLocallyAsync(imageDescription.ImageName, timeoutHelper.RemainingTime))
                {
                    throw new Exception(
                        $"Container image download failed - ImageName={imageDescription.ImageName}. " +
                        $"Please check if container image OS is compatible with host OS or if you have sufficient disk space on the machine. " +
                        $"Try to pull the image manually on the machine using docker CLI for further error details. ");
                }

                this.AddImageToCache(imageDescription.ImageName);
            }
            catch (ContainerApiException dex)
            {
                var errMsgBuilder = new StringBuilder();

                errMsgBuilder.AppendFormat(
                    "Container image download failed with authorization attempt type {0} for ImageName={1}. DockerRequest returned StatusCode={2} with ResponseBody={3}.",
                    currentCredentialAttempt.ToString(),
                    imageDescription.ImageName,
                    dex.StatusCode,
                    dex.ResponseBody);

                if (dex.StatusCode == HttpStatusCode.NotFound)
                {
                    errMsgBuilder.Append(
                        "Please check if image is present in repository or repository credentials provided are correct.");
                }

                HostingTrace.Source.WriteWarning(TraceType, errMsgBuilder.ToString());
                throw new FabricException(errMsgBuilder.ToString(), FabricErrorCode.InvalidOperation);
            }
            catch (Exception ex)
            {
                var errMsgBuilder = new StringBuilder();

                // ASN1 BADTAG is an esoteric error message so lets return a more readable/useful error message.
                if (ex.HResult == CRYPT_E_ASN1_BADTAG)
                {
                    errMsgBuilder.AppendFormat(
                        "Container image download failed with authorization attempt type={0} for ImageName={1} due to an error decrypting the repository credential password. Is the PasswordEncrypted flag set correctly in ApplicationManifest.xml? Exception={2}.",
                        currentCredentialAttempt.ToString(),
                        imageDescription.ImageName,
                        ex.ToString());
                }
                else
                {
                    errMsgBuilder.AppendFormat(
                        "Container image download failed with authorization attempt type={0} for ImageName={1} with unexpected error. Exception={2}.",
                        currentCredentialAttempt.ToString(),
                        imageDescription.ImageName,
                        ex.ToString());
                }

                HostingTrace.Source.WriteWarning(TraceType, errMsgBuilder.ToString());
                throw new FabricException(errMsgBuilder.ToString(), FabricErrorCode.InvalidOperation);
            }
        }

        private async Task TrackDownloadProgressAsync(
            string imageName,
            Stream progressStream,
            TimeSpan timeout)
        {
            var timeoutHelper = new TimeoutHelper(timeout);

            using (var reader = new StreamReader(progressStream, new UTF8Encoding(false)))
            {
                var sw = Stopwatch.StartNew();
                var nextProgressStatusInterval = this.downloadProgressStatusInterval;

                var progress = await reader.ReadLineAsync();
                while (progress != null)
                {
                    if (timeoutHelper.HasRemainingTime == false)
                    {
                        var errMessage = string.Format(
                            "Container image download timed out. ImageName={0}, LastProgress={1}.",
                            imageName,
                            progress);

                        HostingTrace.Source.WriteWarning(TraceType, errMessage);
                        throw new FabricException(errMessage, FabricErrorCode.OperationTimedOut);
                    }

                    if (sw.Elapsed >= nextProgressStatusInterval)
                    {
                        HostingTrace.Source.WriteInfo(
                            TraceType,
                            "Container image download progress: ImageName={0}, CurrentProgress=[{1}].",
                            imageName,
                            progress);

                        nextProgressStatusInterval += this.downloadProgressStatusInterval;
                    }

                    progress = await reader.ReadLineAsync();
                }

                HostingTrace.Source.WriteInfo(
                    TraceType,
                    "Container image download succeeded: ImageName={0}, Time={1}ms .",
                    imageName,
                    sw.ElapsedMilliseconds);
            }
        }

        public Task DeleteImagesAsync(
            List<string> images,
            TimeSpan timeout)
        {
            var imageDeleteTasks = new List<Task>(images.Count);

            foreach (var imageName in images)
            {
                imageDeleteTasks.Add(this.DeleteImageAsync(imageName, timeout));
            }

            return Task.WhenAll(imageDeleteTasks);
        }

        private ImageOperation ImageOperation
        {
            get
            {
                return this.activator.Client.ImageOperation;
            }
        }

        private async Task DeleteImageAsync(
            string imageName,
            TimeSpan timeout)
        {
            try
            {
                await Utility.ExecuteWithRetryOnTimeoutAsync(
                    (operationTimeout) =>
                    {
                        return this.activator.Client.ImageOperation.DeleteImageAsync(
                            imageName,
                            operationTimeout);
                    },
                    $"DeleteImageAsync_{imageName}",
                    TraceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    timeout);

                this.RemoveImageFromCache(imageName);
            }
            catch (ContainerApiException dex)
            {
                var errMessage = string.Format(
                    "Container image deletion failed for ImageName={0}. DockerRequest returned StatusCode={1} with ResponseBody={2}.",
                    imageName,
                    dex.StackTrace,
                    dex.ResponseBody);

                HostingTrace.Source.WriteError(TraceType, errMessage);
                throw new FabricException(errMessage, FabricErrorCode.InvalidOperation);
            }
            catch (Exception ex)
            {
                var errMessage = string.Format(
                    "Container image deletion failed for ImageName={0} with unexpected error. {1}.",
                    imageName,
                    ex.ToString());

                HostingTrace.Source.WriteError(TraceType, errMessage);
                throw new FabricException(errMessage, FabricErrorCode.InvalidOperation);
            }
        }

        private void AddImageToCache(string image)
        {
            this.imageCache.TryAdd(image, null);
        }

        private void RemoveImageFromCache(string image)
        {
            object toIgnore;
            this.imageCache.TryRemove(image, out toIgnore);
        }

        private bool IsImageCached(string image)
        {
            return this.imageCache.ContainsKey(image);
        }

        private async Task<bool> IsImagePresentLocallyAsync(string imageName, TimeSpan timeout)
        {
            try
            {
                await Utility.ExecuteWithRetryOnTimeoutAsync(
                    (operationTimeout) =>
                    {
                        return this.ImageOperation.GetImageHistoryAsync(
                            imageName,
                            operationTimeout);
                    },
                    $"GetImageHistoryAsync_{imageName}",
                    TraceType,
                    HostingConfig.Config.DockerRequestTimeout,
                    timeout);
            }
            catch (ContainerApiException dex)
            {
                if (dex.StatusCode == HttpStatusCode.NotFound)
                {
                    return false;
                }

                var errMessage = string.Format(
                    "GetImageHistory failed for ImageName={0}. DockerRequest returned StatusCode={1} with ResponseBody={2}.",
                    imageName,
                    dex.StatusCode,
                    dex.ResponseBody);

                HostingTrace.Source.WriteError(TraceType, errMessage);
                throw new FabricException(errMessage, FabricErrorCode.InvalidOperation);
            }
            catch (Exception ex)
            {
                var errMessage = string.Format(
                    "GetImageHistory failed for ImageName={0} with unexpected error. Exception={1}.",
                    imageName,
                    ex.ToString());

                HostingTrace.Source.WriteError(TraceType, errMessage);
                throw new FabricException(errMessage, FabricErrorCode.InvalidOperation);
            }

            return true;
        }

        private AuthConfig GetAuthConfig(RepositoryCredentialDescription credentials, string imageName, CredentialType credentialType)
        {
            var authConfig = new AuthConfig();
            string username = "";
            string password = "";

            if (credentialType == CredentialType.TokenCredentials)
            {
                // Docker knows to do token authentication if the user name is a GUID of all zeroes
                username = Guid.Empty.ToString();

                if (!string.IsNullOrEmpty(cachedToken))
                {
                    password = cachedToken;
                }
                else if (!string.IsNullOrEmpty(HostingConfig.Config.ContainerRepositoryCredentialTokenEndPoint))
                {
                    if (HostingConfig.Config.ContainerRepositoryCredentialTokenEndPoint.Contains(HostingConfig.Config.DefaultMSIEndpointForTokenAuthentication))
                    {
                        // They are using Azure Container Registry
                        // get a refresh token from the azure container registry via MSI
                        password = GetRefreshTokenMSI(imageName);
                    }
                    else
                    {
                        password = GetGenericRefreshToken();
                    }
                }
                else
                {
                    // no container endpoint specified, try MSI
                    password = GetRefreshTokenMSI(imageName);
                }
            }
            else if (credentialType == CredentialType.AppManifestCredentials)
            {
                username = credentials.AccountName;

                bool isencrypted = (credentials.IsPasswordEncrypted || credentials.Type.Equals(Type.Encrypted));

                if (isencrypted && !string.IsNullOrEmpty(credentials.Password))
                {
                    password = Utility.GetDecryptedValue(credentials.Password);
                }
                else
                {
                    password = credentials.Password;
                }
            }
            else if (credentialType == CredentialType.ClusterManifestDefaultCredentials)
            {
                username = HostingConfig.Config.DefaultContainerRepositoryAccountName;

                bool isencrypted = (HostingConfig.Config.IsDefaultContainerRepositoryPasswordEncrypted || HostingConfig.Config.DefaultContainerRepositoryPasswordType.Equals(Type.Encrypted));

                if (isencrypted && !string.IsNullOrEmpty(HostingConfig.Config.DefaultContainerRepositoryPassword))
                {
                    password = Utility.GetDecryptedValue(HostingConfig.Config.DefaultContainerRepositoryPassword);
                }
                else
                {
                    password = HostingConfig.Config.DefaultContainerRepositoryPassword;
                }
            }

            if (!string.IsNullOrEmpty(username) && !string.IsNullOrEmpty(password))
            {
                authConfig.Username = username;
                authConfig.Password = password;
                authConfig.Email = credentials.Email;
            }

            return authConfig;
        }

        private string GetGenericRefreshToken()
        {
            try
            {
                string downString = webClient.DownloadString(HostingConfig.Config.ContainerRepositoryCredentialTokenEndPoint);

                JObject jsonObj = JObject.Parse(downString);
                JToken jsonToken = jsonObj.SelectToken("access_token");

                string token = jsonToken.Value<String>();
                cachedToken = token;

                return token;
            }
            catch (WebException ex)
            {
                var errMessage = string.Format(
                    "GetGenericRefreshToken failed with WebExceptionStatus code: {0}. Exception={0}.",
                    ex.Status,
                    ex.InnerException);

                HostingTrace.Source.WriteError(TraceType, errMessage);
                throw ex;
            }
        }

        private string GetRefreshTokenMSI(string imageName)
        {
            try
            {
                int imageSubStrIndex = imageName.IndexOf('/');
                if (imageSubStrIndex == -1)
                {
                    var errMessage = string.Format(
                        "Could not find '/' in image name {0}.  Format should be \"ContainerRegistryUri/ImageName\"",
                        imageName);

                    HostingTrace.Source.WriteError(TraceType, errMessage);
                    throw new FabricException(errMessage, FabricErrorCode.InvalidOperation);
                }

                webClient.Headers.Clear();
                webClient.Headers["Metadata"] = "true";

                string msiDownloadJson = webClient.DownloadString(HostingConfig.Config.DefaultMSIEndpointForTokenAuthentication);

                JObject msiJsonObj = JObject.Parse(msiDownloadJson);
                JToken msiAccessTokenJson = msiJsonObj.SelectToken("access_token");
                string msiToken = msiAccessTokenJson.Value<String>();

                // Now lets get the refresh token so we can access the ACR
                webClient.Headers.Clear();
                webClient.Headers["Content-Type"] = "application/x-www-form-urlencoded";

                string acrEndPoint = imageName.Substring(0, imageSubStrIndex);
                string parameters = string.Format("grant_type=access_token&service={0}&access_token={1}", acrEndPoint, msiToken);

                string httpUploadString = webClient.UploadString(string.Format("https://{0}/oauth2/exchange", acrEndPoint), parameters);

                JObject jsonObj = JObject.Parse(httpUploadString);
                JToken jsonToken = jsonObj.SelectToken("refresh_token");
                cachedToken = jsonToken.Value<String>();

                return cachedToken;
            }
            catch (WebException ex)
            {
                var errMessage = string.Format(
                    "GetRefreshTokenMSI failed with WebExceptionStatus code: {0}. Exception={0}.",
                    ex.Status,
                    ex.InnerException);

                HostingTrace.Source.WriteError(TraceType, errMessage);
                throw ex;
            }
        }
    }
}
