// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Filters
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Linq;
    using FabricMonSvc;
    using Microsoft.ServiceFabric.Monitoring.Interfaces;

    internal class EntityFilterRepository
    {
        private const string SystemAppName = "fabric:/System";
        private readonly IServiceConfiguration config;

        public EntityFilterRepository(IServiceConfiguration config)
        {
            this.config = Guard.IsNotNull(config, nameof(config));
            this.RefreshFilterRepository();
        }

        public EntityFilter ServiceHealthFilter { get; private set; }

        public EntityFilter PartitionHealthFilter { get; private set; }

        public EntityFilter ReplicaHealthFilter { get; private set; }

        public EntityFilter DeployedApplicationHealthFilter { get; private set; }

        public EntityFilter ServicePackageHealthFilter { get; private set; }

        public EntityFilter HealthEventsFilter { get; private set; }

        internal void RefreshFilterRepository()
        {
            this.ServiceHealthFilter = this.GetEntityHealthFilter(
                this.ParseServiceEnum(), this.config.ApplicationsThatReportServiceHealth);
            this.PartitionHealthFilter = this.GetEntityHealthFilter(
                this.ParsePartitionEnum(), this.config.ApplicationsThatReportPartitionHealth);
            this.ReplicaHealthFilter = this.GetEntityHealthFilter(
                this.ParseReplicaEnum(), this.config.ApplicationsThatReportReplicaHealth);
            this.DeployedApplicationHealthFilter = this.GetEntityHealthFilter(
                this.ParseDeployedApplicationEnum(), this.config.ApplicationsThatReportDeployedApplicationHealth);
            this.ServicePackageHealthFilter = this.GetEntityHealthFilter(
                this.ParseServicePackageEnum(), this.config.ApplicationsThatReportServicePackageHealth);
            this.HealthEventsFilter = this.GetEntityHealthFilter(
                this.ParseHealthEventsEnum(), this.config.ApplicationsThatReportHealthEvents);

            this.ValidateWhitelistSetRelationship(
                this.PartitionHealthFilter,
                this.ServiceHealthFilter,
                nameof(this.config.ReportPartitionHealth),
                nameof(this.config.ReportServiceHealth),
                nameof(this.config.ApplicationsThatReportPartitionHealth),
                nameof(this.config.ApplicationsThatReportServiceHealth));

            this.ValidateWhitelistSetRelationship(
                this.ReplicaHealthFilter,
                this.PartitionHealthFilter,
                nameof(this.config.ReportReplicaHealth),
                nameof(this.config.ReportPartitionHealth),
                nameof(this.config.ApplicationsThatReportReplicaHealth),
                nameof(this.config.ApplicationsThatReportPartitionHealth));

            this.ValidateWhitelistSetRelationship(
                this.ServicePackageHealthFilter,
                this.DeployedApplicationHealthFilter,
                nameof(this.config.ReportServicePackageHealth),
                nameof(this.config.ReportDeployedApplicationHealth),
                nameof(this.config.ApplicationsThatReportServicePackageHealth),
                nameof(this.config.ApplicationsThatReportDeployedApplicationHealth));
        }

        private WhenToReportEntityHealth ParseServiceEnum()
        {
            return this.ParseEnum(
                this.config.ReportServiceHealth, 
                nameof(this.config.ReportServiceHealth),
                defaultValue: WhenToReportEntityHealth.Always);
        }

        private WhenToReportEntityHealth ParsePartitionEnum()
        {
            return this.ParseEnum(
                this.config.ReportPartitionHealth, 
                nameof(this.config.ReportPartitionHealth),
                defaultValue: WhenToReportEntityHealth.Always);
        }

        private WhenToReportEntityHealth ParseReplicaEnum()
        {
            return this.ParseEnum(
                this.config.ReportReplicaHealth, 
                nameof(this.config.ReportReplicaHealth),
                defaultValue: WhenToReportEntityHealth.Always);
        }

        private WhenToReportEntityHealth ParseDeployedApplicationEnum()
        {
            return this.ParseEnum(
                this.config.ReportDeployedApplicationHealth,
                nameof(this.config.ReportDeployedApplicationHealth),
                defaultValue: WhenToReportEntityHealth.Never);
        }

        private WhenToReportEntityHealth ParseServicePackageEnum()
        {
            return this.ParseEnum(
                this.config.ReportServicePackageHealth, 
                nameof(this.config.ReportServicePackageHealth),
                defaultValue: WhenToReportEntityHealth.Never);
        }

        private WhenToReportEntityHealth ParseHealthEventsEnum()
        {
            return this.ParseEnum(
                this.config.ReportHealthEvents,
                nameof(this.config.ReportHealthEvents),
                defaultValue: WhenToReportEntityHealth.Always);
        }

        private EntityFilter GetEntityHealthFilter(WhenToReportEntityHealth whenToReport, string appWhitelist)
        {
            if (whenToReport == WhenToReportEntityHealth.Never)
            {
                return new EntityFilter(whenToReport, false, null);
            }

            if (this.IsReportingEnabledForAllApps(appWhitelist))
            {
                return new EntityFilter(whenToReport, true, null);
            }

            var whitelist = this.GetApplicationWhiteList(appWhitelist);
            return new EntityFilter(whenToReport, false, whitelist);
        }

        private bool IsReportingEnabledForAllApps(string whitelist)
        {
            return string.IsNullOrEmpty(whitelist) 
                ? false 
                : whitelist.Equals("all", StringComparison.OrdinalIgnoreCase);
        }

        private ISet<string> GetApplicationWhiteList(string paramValue)
        {
            var setofApps = new HashSet<string>();
            setofApps.Add(SystemAppName);

            if (!string.IsNullOrEmpty(paramValue))
            {
                var parts = paramValue.Split(',', ';');
                setofApps.UnionWith(parts.Where(part => !string.IsNullOrWhiteSpace(part)));
            }

            return setofApps;
        }

        private WhenToReportEntityHealth ParseEnum(string paramValue, string configParamName, WhenToReportEntityHealth defaultValue)
        {
            if (string.IsNullOrEmpty(paramValue))
            {
                return defaultValue;
            }
            
            WhenToReportEntityHealth enumValue;
            if (!Enum.TryParse(paramValue, ignoreCase: true, result: out enumValue))
            {
                throw new ConfigurationErrorsException(
                    string.Format(
                        "Invalid value for application parameter {0}. Allowed values are {1}, {2} and {3}",
                            configParamName,
                            WhenToReportEntityHealth.Always,
                            WhenToReportEntityHealth.Never,
                            WhenToReportEntityHealth.OnWarningOrError));
            }

            return enumValue;
        }

        private void ValidateWhitelistSetRelationship(
            EntityFilter subsetFilter, 
            EntityFilter supersetFilter,
            string subsetFilterConditionParamName,
            string supersetFilterConditionParamName,
            string subsetParamName,
            string supersetParamName)
        {
            if (supersetFilter.WhenToReport == WhenToReportEntityHealth.Never
                && subsetFilter.WhenToReport != WhenToReportEntityHealth.Never)
            {
                throw new ConfigurationErrorsException(
                    string.Format(
                        "Invalid value {0} in parameter {1}. {1} must equal or more restrictive than {2}.",
                        subsetFilter.WhenToReport,
                        subsetFilterConditionParamName,
                        supersetFilterConditionParamName));
            }

            if (supersetFilter.ReportAllApps || subsetFilter.WhenToReport == WhenToReportEntityHealth.Never)
            {
                return;
            }

            if (!supersetFilter.ReportAllApps && subsetFilter.ReportAllApps)
            {
                throw new ConfigurationErrorsException(
                    string.Format(
                        "Invalid value 'All' in parameter {0}. {0} must be subset of {1}.", 
                        subsetParamName, 
                        supersetParamName));
            }

            var subsetList = subsetFilter.AppWhitelist;
            var supersetList = supersetFilter.AppWhitelist;
            if (!subsetList.IsSubsetOf(supersetList))
            {
                var deltaSet = subsetList.Except(supersetList);
                throw new ConfigurationErrorsException(
                    string.Format(
                        "{0} should be subset of {1}. Add the following to {0} or remove from {1} {2}.",
                        subsetParamName,
                        supersetParamName,
                        string.Join(",", deltaSet)));
            }
        }
    }
}