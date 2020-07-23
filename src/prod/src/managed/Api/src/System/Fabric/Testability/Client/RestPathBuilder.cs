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
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Text;

    internal static class RestPathBuilder
    {
        public const string Nodes = "Nodes";
        public const string Applications = "Applications";
        public const string ApplicationDefinitionKindFilter = "ApplicationDefinitionKindFilter";
        public const string ApplicationTypeName = "ApplicationTypeName";
        public const string ApplicationTypeVersionFilter = "ApplicationTypeVersion";
        public const string ApplicationTypeDefinitionKindFilter = "ApplicationTypeDefinitionKindFilter";
        public const string ExcludeApplicationParameters = "ExcludeApplicationParameters";
        public const string GetApplications = "$/GetApplications";
        public const string GetServicePackages = "$/GetServicePackages";
        public const string GetCodePackages = "$/GetCodePackages";
        public const string GetServiceTypes = "$/GetServiceTypes";
        public const string ServiceTypeName = "ServiceTypeName";
        public const string GetServices = "$/GetServices";
        public const string Services = "Services";
        public const string TestCommands = "TestCommands";
        public const string ServiceManifestName = "ServiceManifestName";
        public const string CodePackageName = "CodePackageName";
        public const string ServiceKindParameter = "ServiceKind";
        public const string Partitions = "Partitions";
        public const string PartitionId = "PartitionId";
        public const string CreateService = "$/GetServices/$/Create";
        public const string CreateServiceFromTemplate = "$/GetServices/$/CreateFromTemplate";
        public const string ApplicationTypes = "ApplicationTypes";
        public const string GetPartitions = "$/GetPartitions";
        public const string GetReplicas = "$/GetReplicas";
        //V2
        public const string Resources = "Resources";
        public const string ApplicationResource = Resources + "/Applications";
        public const string ServiceResource = "/Services";
        public const string ReplicaResource = "/Replicas";

        public const string Create = "$/Create";
        public const string Upgrade = "$/Upgrade";
        public const string Update = "$/Update";
        public const string MoveNextUpgradeDomain = "$/MoveToNextUpgradeDomain";
        public const string GetUpgradeProgress = "$/GetUpgradeProgress";
        public const string Delete = "$/Delete";
        public const string NodeStateRemoved = "$/RemoveNodeState";
        public const string Deactivate = "$/Deactivate";
        public const string Activate = "$/Activate";
        public const string UpdateUpgrade = "$/UpdateUpgrade";
        public const string Provision = "$/Provision";
        public const string Unprovision = "$/Unprovision";
        public const string GetApplicationManifest = "$/GetApplicationManifest";
        public const string GetClusterManifest = "$/GetClusterManifest";
        public const string GetClusterVersion = "$/GetClusterVersion";
        public const string GetLoadInformation = "$/GetLoadInformation";
        public const string GetDescription = "$/GetDescription";
        public const string GetHealth = "$/GetHealth";
        public const string GetClusterHealth = "$/GetClusterHealth";
        public const string GetClusterHealthChunk = "$/GetClusterHealthChunk";
        public const string ApiVersionUri = "api-version";
        public const string Api20Version = "2.0";
        public const string Api30Version = "3.0";
        public const string Api40Version = "4.0";
        public const string Api61Version = "6.1";
        public const string Api62Version = "6.2";
        public const string Api63Version = "6.3";
        public const string Api64Version = "6.4";
        public const string ApiVersionDefault = Api64Version;
        public const string Api62PreviewVersion = "6.2-preview";
        public const string FirstQueryStringFormat = "?{0}={1}";
        public const string QueryStringFormat = "&{0}={1}";
        public const string ReportHealth = "$/ReportHealth";
        public const string ReportClusterHealth = "$/ReportClusterHealth";
        public const string ContinuationToken = "ContinuationToken";
        public const string MaxResults = "MaxResults";
        public const string EventsHealthStateFilter = "EventsHealthStateFilter";
        public const string NodesHealthStateFilter = "NodesHealthStateFilter";
        public const string ReplicasHealthStateFilter = "ReplicasHealthStateFilter";
        public const string PartitionsHealthStateFilter = "PartitionsHealthStateFilter";
        public const string ServicesHealthStateFilter = "ServicesHealthStateFilter";
        public const string ApplicationsHealthStateFilter = "ApplicationsHealthStateFilter";
        public const string DeployedApplicationsHealthStateFilter = "DeployedApplicationsHealthStateFilter";
        public const string DeployedServicePackagesHealthStateFilter = "DeployedServicePackagesHealthStateFilter";
        public const string ClusterManifestVersionFilter = "ClusterManifestVersion";
        public const string ForceRemove = "ForceRemove";
        public const string Start = "$/Start";
        public const string Stop = "$/Stop";
        public const string Restart = "$/Restart";
        public const string ImageStore = "ImageStore";
        public const string Copy = "$/Copy";
        public const string UploadChunk = "$/UploadChunk";
        public const string DeleteUploadSession = "$/DeleteUploadSession";
        public const string CommitUploadSession = "$/CommitUploadSession";
        public const string GetUploadSession = "$/GetUploadSession";
        public const string SessionIdUri = "session-id";
        public const string TimeoutQueryParameter = "timeout";
        public const string ImmediateQueryParameter = "Immediate";
        public const string IncludeHealthState = "IncludeHealthState";
        public const string ExcludeHealthStatistics = "ExcludeHealthStatistics";
        public const string IncludeSystemApplicationHealthStatistics = "IncludeSystemApplicationHealthStatistics";

        public const string StartPartitionDataLoss = "$/StartPartitionDataLoss";
        public const string StartPartitionQuorumLoss = "$/StartPartitionQuorumLoss";
        public const string StartPartitionRestart = "$/StartPartitionRestart";
        public const string GetProgress = "$/GetProgress";
        public const string GetTestCommands = "$/GetTestCommands";
        public const string Cancel = "$/Cancel";
        public const string TestCommandStateFilter = "TestCommandStateFilter";
        public const string TestCommandTypeFilter = "TestCommandTypeFilter";

        public const string FabricBrsApiVersion = ApiVersionDefault;
        public const string CreateBackupPolicy = "BackupRestore/BackupPolicies/$/Create";
        public const string UpdateBackupPolicy = "BackupRestore/BackupPolicies/{0}/$/Update";
        public const string DeleteBackupPolicy = "BackupRestore/BackupPolicies/{0}/$/Delete";
        public const string GetBackupPolicy = "BackupRestore/BackupPolicies/{0}";
        public const string GetBackupPolicies = "BackupRestore/BackupPolicies";
        public const string GetBackupEnabledEntities = "BackupRestore/BackupPolicies/{0}/$/GetBackupEnabledEntities";
        public const string EnableBackup = "{0}/$/EnableBackup";
        public const string DisableBackup = "{0}/$/DisableBackup";
        public const string GetApplicationBackupConfigurationInfo = "Applications/{0}/$/GetBackupConfigurationInfo";
        public const string GetServiceBackupConfigurationInfo = "Services/{0}/$/GetBackupConfigurationInfo";
        public const string GetPartitionBackupConfigurationInfo = "Partitions/{0}/$/GetBackupConfigurationInfo";
        public const string GetBackupByApplication = "Applications/{0}/$/GetBackups";
        public const string GetBackupByStorage = "BackupRestore/$/GetBackups";
        public const string GetBackupByService = "Services/{0}/$/GetBackups";
        public const string GetBackupByPartition = "Partitions/{0}/$/GetBackups";
        public const string RestorePartition = "Partitions/{0}/$/Restore";
        public const string GetRestoreProgress = "Partitions/{0}/$/GetRestoreProgress";
        public const string BackupPartition = "Partitions/{0}/$/Backup";
        public const string GetBackupProgress = "Partitions/{0}/$/GetBackupProgress";
        public const string SuspendApplicationBackup = "Applications/{0}/$/SuspendBackup";
        public const string SuspendServiceBackup = "Services/{0}/$/SuspendBackup";
        public const string SuspendPartitionBackup = "Partitions/{0}/$/SuspendBackup";
        public const string ResumeApplicationBackup = "Applications/{0}/$/ResumeBackup";
        public const string ResumeServiceBackup = "Services/{0}/$/ResumeBackup";
        public const string ResumePartitionBackup = "Partitions/{0}/$/ResumeBackup";
        public const string LatestQueryPatam = "latest";
        public const string StartDateTimeFilter = "StartDateTimeFilter";
        public const string EndDateTimeFilter = "EndDateTimeFilter";

        // These must match corresponding values in src\httpgateway\Constants.cpp
        // TestCommandTypeDataLossString
        public const string DataLossProgress = "DataLoss";

        // TestCommandTypeQuorumLossString
        public const string QuorumLossProgress = "QuorumLoss";

        // TestCommandTypeRestartPartitionString
        public const string RestartPartitionProgress = "RestartPartition";

        public const string Tools = "Tools";
        public const string Chaos = "Chaos";
        public const string ChaosSchedule = "Chaos/Schedule";
        public const string ChaosEvents = "Chaos/Events";
        public const string Report = "$/Report";
        public const string StartTimeUtc = "StartTimeUtc";
        public const string EndTimeUtc = "EndTimeUtc";
        public const string EventsTypesFilter = "EventsTypesFilter";
        public const string ExcludeAnalysisEvents = "ExcludeAnalysisEvents";
        public const string SkipCorrelationLookup = "SkipCorrelationLookup";
        public const string EventsStoreAllPartitionEvents = "EventsStore/Partitions/Events";
        public const string EventsStorePartitionEvents = "EventsStore/Partitions/{0}/$/Events";
        public const string YearMonthDayHourMinuteSecondUtcFormat = "yyyy-MM-ddTHH:mm:ssZ";

        public const string NodeStatusFilter = "NodeStatusFilter";
        public const string NodeNameFilter = "NodeNameFilter";

        public static Uri GetBaseResourceUri(IList<string> httpGatewayAddresses, string resource)
        {
            return RestPathBuilder.GetBaseResourceUri(httpGatewayAddresses, resource, null);
        }

        public static Uri GetBaseResourceUri(IList<string> httpGatewayAddresses, string resource, Dictionary<string, string> queryParameters)
        {
            return GetBaseResourceUri(httpGatewayAddresses, resource, queryParameters, RestPathBuilder.ApiVersionDefault);
        }

        public static Uri GetBaseResourceUri(IList<string> httpGatewayAddresses, string resource, Dictionary<string, string> queryParameters, string apiVersion)
        {
            ThrowIf.Null(httpGatewayAddresses, "httpGatewayAddresses");

            int index = RandomUtility.NextFromTimestamp(httpGatewayAddresses.Count);
            string endpoint = httpGatewayAddresses[index];
            endpoint = RestPathBuilder.RemoveExtraCharacter(endpoint);

            StringBuilder sb = new StringBuilder();
            sb.Append(endpoint);
            sb.Append("/");
            sb.Append(resource);

            sb.Append(string.Format(CultureInfo.InvariantCulture, RestPathBuilder.FirstQueryStringFormat, RestPathBuilder.ApiVersionUri, apiVersion));

            if (queryParameters != null)
            {
                foreach (var key in queryParameters.Keys)
                {
                    sb.Append(string.Format(CultureInfo.InvariantCulture, RestPathBuilder.QueryStringFormat, key, queryParameters[key]));
                }
            }

            return new Uri(sb.ToString());
        }

        public static Uri GetResourceUriById(IList<string> httpGatewayAddresses, string resourceType, string resourceId, string apiVersion = RestPathBuilder.ApiVersionDefault)
        {
            string[] resourceTypes = new string[] { resourceType };
            string[] resourceIds = string.IsNullOrEmpty(resourceId) ? new string[0] : new string[] { resourceId };
            return RestPathBuilder.GetResourceUriById(httpGatewayAddresses, resourceTypes, resourceIds, null, apiVersion);
        }

        public static Uri GetResourceUriById(IList<string> httpGatewayAddresses, string[] resourceTypes, string[] resourceIds, string apiVersion = RestPathBuilder.ApiVersionDefault)
        {
            return RestPathBuilder.GetResourceUriById(httpGatewayAddresses, resourceTypes, resourceIds, null, apiVersion);
        }

        public static Uri GetResourceUriById(IList<string> httpGatewayAddresses, string[] resourceTypes, string[] resourceIds, Dictionary<string, string> queryParameters)
        {
            return RestPathBuilder.GetResourceUriById(httpGatewayAddresses, resourceTypes, resourceIds, queryParameters, RestPathBuilder.ApiVersionDefault);
        }

        public static Uri GetResourceUriById(IList<string> httpGatewayAddresses, string[] resourceTypes, string[] resourceIds, Dictionary<string, string> queryParameters, string apiVersion)
        {
            ThrowIf.Null(resourceTypes, "resourceTypes");
            ThrowIf.Null(resourceIds, "resourceIds");
            ThrowIf.Null(httpGatewayAddresses, "httpGatewayAddresses");

            int index = RandomUtility.NextFromTimestamp(httpGatewayAddresses.Count);
            string endpoint = httpGatewayAddresses[index];
            endpoint = RestPathBuilder.RemoveExtraCharacter(endpoint);

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

            sb.Append(string.Format(CultureInfo.InvariantCulture, RestPathBuilder.FirstQueryStringFormat, RestPathBuilder.ApiVersionUri, apiVersion));

            if (queryParameters != null)
            {
                foreach (var key in queryParameters.Keys)
                {
                    sb.Append(string.Format(CultureInfo.InvariantCulture, RestPathBuilder.QueryStringFormat, key, queryParameters[key]));
                }
            }

            return new Uri(sb.ToString());
        }

        public static Uri GetResourceUriByTypes(IList<string> httpGatewayAddresses, string[] resourceTypes)
        {
            return RestPathBuilder.GetResourceUriByTypes(httpGatewayAddresses, resourceTypes, null);
        }

        public static Uri GetResourceUriByTypes(IList<string> httpGatewayAddresses, string[] resourceTypes, Dictionary<string, string> queryParameters)
        {
            return GetResourceUriByTypes(httpGatewayAddresses, resourceTypes, queryParameters, RestPathBuilder.ApiVersionDefault);
        }

        public static Uri GetResourceUriByTypes(IList<string> httpGatewayAddresses, string[] resourceTypes, Dictionary<string, string> queryParameters, string apiVersion)
        {
            ThrowIf.Null(resourceTypes, "resourceTypes");
            ThrowIf.Null(httpGatewayAddresses, "httpGatewayAddresses");

            int index = RandomUtility.NextFromTimestamp(httpGatewayAddresses.Count);
            string endpoint = httpGatewayAddresses[index];
            endpoint = RestPathBuilder.RemoveExtraCharacter(endpoint);

            StringBuilder sb = new StringBuilder();
            sb.Append(endpoint);

            foreach (var partUri in resourceTypes)
            {
                sb.Append("/");
                sb.Append(partUri);
            }

            sb.Append(string.Format(CultureInfo.InvariantCulture, RestPathBuilder.FirstQueryStringFormat, RestPathBuilder.ApiVersionUri, apiVersion));

            if (queryParameters != null)
            {
                foreach (var key in queryParameters.Keys)
                {
                    sb.Append(string.Format(CultureInfo.InvariantCulture, RestPathBuilder.QueryStringFormat, key, queryParameters[key]));
                }
            }

            return new Uri(sb.ToString());
        }

        public static Uri BuildRequestEndpoint(IList<string> httpGatewayAddresses, string suffix, string queryParameters)
        {
            ThrowIf.Null(httpGatewayAddresses, "resourceTypes");
            ThrowIf.Null(suffix, "resourceIds");
            ThrowIf.Null(queryParameters, "httpGatewayAddresses");

            int index = RandomUtility.NextFromTimestamp(httpGatewayAddresses.Count);
            string endpoint = httpGatewayAddresses[index];
            endpoint = RestPathBuilder.RemoveExtraCharacter(endpoint);

            StringBuilder sb = new StringBuilder();
            sb.Append(endpoint);
            sb.Append(suffix);
            sb.Append(queryParameters);

            return new Uri(sb.ToString());
        }

        private static string RemoveExtraCharacter(string endpoint)
        {
            if (endpoint.LastIndexOf("/", StringComparison.Ordinal) == (endpoint.Length - 1))
            {
                endpoint = endpoint.Substring(0, endpoint.Length - 1);
            }

            return endpoint;
        }
    }
}


#pragma warning restore 1591
