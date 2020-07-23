// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Collections.Generic;
    using Globalization;
    using Health;

    /// <summary>
    /// A watchdog specific to reporting role instance health. 
    /// First reports the health by invoking the management client API. Then invokes the policy executor
    /// which in turn applies all the policies and returns the final health state of a role instance to the
    /// watchdog. The watchdog then reports the final health state to the Windows Fabric health store.
    /// </summary>
    internal class RoleInstanceHealthWatchdog : IRoleInstanceHealthWatchdog
    {
        private const string ServiceNamePrefix = "fabric:/System/";

        private readonly IConfigSection configSection;
        private readonly string configKeyPrefix;
        private readonly IHealthClient healthClient;
        private readonly IRoleInstanceHealthPolicyManager healthPolicyManager;
        
        public RoleInstanceHealthWatchdog(IConfigSection configSection, string configKeyPrefix, IHealthClient healthClient)
        {
            this.configSection = configSection.Validate("configSection");
            this.configKeyPrefix = configKeyPrefix.Validate("configKeyPrefix");
            this.healthClient = healthClient.Validate("healthClient");

            healthPolicyManager = new DynamicRoleInstanceHealthPolicyManager(configSection, configKeyPrefix);
        }

        public void ReportHealth(IList<RoleInstance> roleInstances, TimeSpan validityPeriod)
        {            
            // start with an initial assumption that health is okay.
            const HealthState InputHealthState = HealthState.Ok;

            try
            {
                HealthState serviceHealthState = (roleInstances == null || roleInstances.Count == 0)
                    ? HealthState.Warning 
                    : GetRoleInstanceHealth(roleInstances, InputHealthState, healthPolicyManager);

                string description = GetServiceHealthDescription(roleInstances, serviceHealthState);

                // Specify a time to live. In case infrastructure service is down, the health report shows up as warning (or error)
                var hi = new HealthInformation(
                    Constants.HealthReportSourceId,
                    RoleInstanceHealthConstants.HealthReportReplicaHealthWatchdogStatus,
                    serviceHealthState)
                {
                    Description = description,
                    TimeToLive = validityPeriod != TimeSpan.Zero ? validityPeriod : TimeSpan.MaxValue,
                    RemoveWhenExpired = true
                };

                // Construct this name ourselves since there is currently a bug where the service Uri which 
                // could have been passed in all the way from ServiceFactory's CreateReplica is an empty string.
                var serviceUri = new Uri(ServiceNamePrefix + configSection.Name);

                // We'll roll up the health to the service level bypassing the replica and partition health.
                // Since only 1 replica is actively doing something and there is only a single partition of
                // this service, rolling up health higher is preferred since it can catch the user's eyes a
                // little bit faster.
                var healthReport = new ServiceHealthReport(serviceUri, hi);

                healthClient.ReportHealth(healthReport);
            }
            catch (Exception ex)
            {
                Trace.WriteWarning(
                    RoleInstanceHealthConstants.TraceType,
                    "Unable to report health from role instance health watchdog. Error(s): {0}",
                    ex.GetMessage());                
            }
        }

        private static string GetServiceHealthDescription(IList<RoleInstance> roleInstances, HealthState serviceHealthState)
        {
            string description;

            if (roleInstances != null && serviceHealthState != HealthState.Ok)
            {
                description = string.Format(
                    CultureInfo.InvariantCulture,
                    "{0} role instances detected, but error while reporting health on them",
                    roleInstances.Count);
            }
            else if (roleInstances != null)
            {
                description = string.Format(
                    CultureInfo.InvariantCulture,
                    "{0} role instances detected",
                    roleInstances.Count);
            }
            else
            {
                description = "Error getting role instances";
            }

            return description;
        }

        private HealthState GetRoleInstanceHealth(
            IList<RoleInstance> roleInstances, 
            HealthState inputHealthState,
            IRoleInstanceHealthPolicyManager manager)
        {
            bool enableTracing = this.configSection.ReadConfigValue($"{this.configKeyPrefix}Health.TracingEnabled", false);
            bool enableReportHealth = this.configSection.ReadConfigValue($"{this.configKeyPrefix}Health.ReportHealthEnabled", true);

            var serviceHealthState = HealthState.Ok;

            foreach (RoleInstance roleInstance in roleInstances)
            {
                var outputHealthState = inputHealthState;

                try
                {
                    outputHealthState = manager.Execute(roleInstance, inputHealthState);
                }
                catch (Exception ex)
                {
                    // no choice but to catch everything since we don't know what to expect from each policy implementation
                    string text = ex.GetMessage();

                    Trace.WriteWarning(
                        RoleInstanceHealthConstants.TraceType,
                        "Unable to apply health policies on role instance '{0}'. Error(s): {1}",
                        roleInstance.Id,
                        text);

                    outputHealthState = HealthState.Warning;
                    serviceHealthState = HealthState.Warning;
                }
                finally
                {
                    AddReport(roleInstance, outputHealthState, enableTracing, enableReportHealth);
                }
            }

            return serviceHealthState;
        }

        /// <summary>
        /// Adds a health report to the Windows Fabric health system.
        /// </summary>
        /// <param name="roleInstance">The role instance.</param>
        /// <param name="healthState">State of the health.</param>
        private void AddReport(RoleInstance roleInstance, HealthState healthState, bool enableTracing, bool enableReportHealth)
        {
            string nodeName = roleInstance.Id.TranslateRoleInstanceToNodeName();

            if (enableReportHealth)
            {
                // this one is purely for informational purposes
                AddReport(nodeName, RoleInstanceHealthConstants.HealthReportRoleInstanceLastUpdateTimePropertyName, roleInstance.LastUpdateTime.ToString("o"), HealthState.Ok);
                AddReport(nodeName, RoleInstanceHealthConstants.HealthReportRoleInstanceStatusPropertyName, roleInstance.Status.ToString(), healthState);
            }

            if (enableTracing && (roleInstance.Status != RoleInstanceState.ReadyRole))
            {
                Trace.WriteInfo(
                    RoleInstanceHealthConstants.TraceType,
                    "Role instance {0}: {1} (last updated at {2:O})",
                    roleInstance.Id,
                    roleInstance.Status,
                    roleInstance.LastUpdateTime);
            }
        }

        private void AddReport(string nodeName, string key, string value, HealthState healthState)
        {
            // Don't specify a time to live here. Because if infrastructure service is down, then
            // all role instances (nodes in WinFab terms) are reported as down in the health report. 
            // i.e. it appears as though the whole cluster is down. In that case, WinFab upgrades 
            // etc. won't work since they perform checks on health status.
            // Instead, we are specifying a time to live only at the service health level
            var hi = new HealthInformation(Constants.HealthReportSourceId, key, healthState)
            {
                Description = value
            };

            var healthReport = new NodeHealthReport(nodeName, hi);
            healthClient.ReportHealth(healthReport);            
        }
    }
}