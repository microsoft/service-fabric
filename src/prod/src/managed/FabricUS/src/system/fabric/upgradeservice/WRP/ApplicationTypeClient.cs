// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Query;
    using System.Fabric.WRP.Model;
    using System.Linq;
    using System.Threading.Tasks;
    using Newtonsoft.Json.Linq;

    public class ApplicationTypeClient : FabricClientApplicationWrapper
    {
        private const string ImageStoreTempPath = "ServiceFabricTemp";
        private readonly FabricClient fabricClient;
        private readonly IConfigStore configStore;
        private readonly string configSectionName;

        /// <summary>
        /// Default ImageStore copy operation timeout.
        /// 
        /// Notes: This default timeout usage is temporary till we swith to the async support for copying app packages directly from storage.
        /// </summary>
        public static readonly TimeSpan DefaultImageStoreCopyOperationTimeout = TimeSpan.FromMinutes(10);

        internal ApplicationTypeClient(
            FabricClient fabricClient,
            IConfigStore configStore,
            string configSectionName)
        {
            fabricClient.ThrowIfNull(nameof(fabricClient));
            configStore.ThrowIfNull(nameof(configStore));
            configSectionName.ThrowIfNullOrWhiteSpace(nameof(configSectionName));

            this.fabricClient = fabricClient;
            this.configStore = configStore;
            this.configSectionName = string.Copy(configSectionName);
        }

        protected ApplicationTypeClient()
        {
        }

        public override TraceType TraceType { get; protected set; } = new TraceType("ApplicationTypeClient");

        public override async Task<IFabricOperationResult> GetAsync(IOperationDescription description, IOperationContext context)
        {
            Trace.WriteInfo(TraceType, "GetAsync called");
            description.ThrowIfNull(nameof(description));
            context.ThrowIfNull(nameof(context));

            string errorMessage;
            if (!this.ValidObjectType<ApplicationTypeVersionOperationDescription>(description, out errorMessage))
            {
                throw new InvalidCastException(errorMessage);
            }

            var appTypeDescription = (ApplicationTypeVersionOperationDescription)description;
            var clusterAppTypes = await this.fabricClient.QueryManager.GetApplicationTypeListAsync(
                appTypeDescription.TypeName,
                context.GetRemainingTimeOrThrow(),
                context.CancellationToken);

            var clusterAppType = clusterAppTypes
                .FirstOrDefault(a => a.ApplicationTypeVersion == appTypeDescription.TypeVersion);

            var result = new FabricOperationResult()
            {
                OperationStatus = null,
                QueryResult = new ApplicationTypeFabricQueryResult(clusterAppType)
            };

            Trace.WriteInfo(
                TraceType,
                null == clusterAppType
                    ? "GetAsync: Application type not found. Name: {0}. Version: {1}"
                    : "GetAsync: Application type exists. Name: {0}. Version: {1}",
                appTypeDescription.TypeName,
                appTypeDescription.TypeVersion);

            if (null == clusterAppType)
            {
                if (description.OperationType == OperationType.Delete)
                {
                    result.OperationStatus = 
                        new ApplicationTypeVersionOperationStatus(appTypeDescription)
                        {
                            Status = ResultStatus.Succeeded,
                            Progress = new JObject(),
                            ErrorDetails = new JObject()
                        };

                    return result;
                }

                return null;
            }

            var status = GetResultStatus(clusterAppType.Status);
            var progress = JObject.FromObject(new { Details = clusterAppType.StatusDetails });
            var errorDetails = new JObject();
            if (status == ResultStatus.Failed)
            {
                errorDetails = JObject.FromObject(new { Details = clusterAppType.StatusDetails });
            }

            var applicationTypeOperationStatus =
                new ApplicationTypeVersionOperationStatus(appTypeDescription)
                {
                    Status = status,
                    Progress = progress,
                    ErrorDetails = errorDetails
                };

            if (status == ResultStatus.Succeeded)
            {
                applicationTypeOperationStatus.DefaultParameterList =
                    new Dictionary<string, string>(clusterAppType.DefaultParameters.AsDictionary());
            }

            result.OperationStatus = applicationTypeOperationStatus;
            return result;
        }

        public override async Task CreateAsync(IOperationDescription description, IOperationContext context)
        {
            Trace.WriteInfo(TraceType, "CreateAsync called");
            description.ThrowIfNull(nameof(description));
            context.ThrowIfNull(nameof(context));

            string errorMessage;
            if (!this.ValidObjectType<ApplicationTypeVersionOperationDescription>(description, out errorMessage))
            {
                throw new InvalidCastException(errorMessage);
            }

            var appTypeDescription = (ApplicationTypeVersionOperationDescription)description;
            appTypeDescription.TypeName.ThrowIfNullOrWhiteSpace(nameof(appTypeDescription.TypeName));
            appTypeDescription.TypeVersion.ThrowIfNullOrWhiteSpace(nameof(appTypeDescription.TypeVersion));
            appTypeDescription.AppPackageUrl.ThrowIfNullOrWhiteSpace(nameof(appTypeDescription.AppPackageUrl));

            Uri appPackageUri;
            if (!Uri.TryCreate(appTypeDescription.AppPackageUrl, UriKind.Absolute, out appPackageUri))
            {
                throw new InvalidOperationException($"{nameof(appTypeDescription.AppPackageUrl)} must have an absolute Uri value.");
            }

            Trace.WriteInfo(
                TraceType,
                "CreateAsync: Provisioning application package. Uri: {0}. Timeout: {1}", 
                appPackageUri,
                context.OperationTimeout);
            await this.fabricClient.ApplicationManager.ProvisionApplicationAsync(
                new ExternalStoreProvisionApplicationTypeDescription(
                    appPackageUri,
                    appTypeDescription.TypeName,
                    appTypeDescription.TypeVersion)
                { Async = true },
                context.OperationTimeout,
                context.CancellationToken);
            Trace.WriteInfo(TraceType, "CreateAsync: Provisioning call accepted");
        }

        public override async Task DeleteAsync(IOperationDescription description, IOperationContext context)
        {
            Trace.WriteInfo(TraceType, "DeleteAsync called");
            description.ThrowIfNull(nameof(description));
            context.ThrowIfNull(nameof(context));

            string errorMessage;
            if (!this.ValidObjectType<ApplicationTypeVersionOperationDescription>(description, out errorMessage))
            {
                throw new InvalidCastException(errorMessage);
            }

            var appTypeDescription = (ApplicationTypeVersionOperationDescription)description;
            try
            {
                Trace.WriteInfo(
                    TraceType,
                    "DeleteAsync: Unprovisioning application type. Name: {0}. Version: {1}. Timeout: {2}",
                    appTypeDescription.TypeName,
                    appTypeDescription.TypeVersion,
                    context.OperationTimeout);
                await this.fabricClient.ApplicationManager.UnprovisionApplicationAsync(
                    appTypeDescription.TypeName,
                    appTypeDescription.TypeVersion,
                    context.OperationTimeout,
                    context.CancellationToken);
                Trace.WriteInfo(TraceType, "DeleteAsync: Unprovisioning call accepted");
            }
            catch (FabricElementNotFoundException nfe)
            {
                if (nfe.ErrorCode == FabricErrorCode.ApplicationTypeNotFound)
                {
                    Trace.WriteInfo(
                        TraceType,
                        "DeleteAsync: Application type not found. Name: {0}. Version: {1}",
                        appTypeDescription.TypeName,
                        appTypeDescription.TypeVersion);
                    return;
                }

                throw;
            }
        }

        private ResultStatus GetResultStatus(ApplicationTypeStatus status)
        {
            switch (status)
            {
                case ApplicationTypeStatus.Available:
                    return ResultStatus.Succeeded;
                case ApplicationTypeStatus.Failed:
                    return ResultStatus.Failed;
                case ApplicationTypeStatus.Provisioning:
                case ApplicationTypeStatus.Unprovisioning:
                    return ResultStatus.InProgress;
            }

            Trace.WriteWarning(this.TraceType, "Invalid ApplicationTypeStatus: {0}", status);
            return ResultStatus.InProgress; // TODO: Should we create an Unknown/Invalid status?
        }
    }

    public class ApplicationTypeFabricQueryResult : IFabricQueryResult
    {
        public ApplicationTypeFabricQueryResult(ApplicationType applicationType)
        {
            this.ApplicationType = applicationType;
        }

        public ApplicationType ApplicationType { get; private set; }
    }
}