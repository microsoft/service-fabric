// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Collections.Generic;
    using Common;
    using Health;
    using Globalization;
    using Linq;
    using Microsoft.WindowsAzure.ServiceRuntime.Management;        

    /// <summary>
    /// A base class for common functionality among all health policies.
    /// </summary>
    internal abstract class BaseRoleInstanceHealthPolicy : IRoleInstanceHealthPolicy
    {
        private readonly IDictionary<string, string> defaultConfigKeys = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

        private readonly IConfigStore configStore;

        private readonly string configSectionName;

        protected BaseRoleInstanceHealthPolicy(IConfigStore configStore, string configSectionName)
        {
            configStore.ThrowIfNull("configStore");
            configSectionName.ThrowIfNullOrWhiteSpace("configSectionName");

            this.configStore = configStore;
            this.configSectionName = configSectionName;

            Name = this.GetType().Name.Replace("HealthPolicy", string.Empty);            
        }

        public string Name { get; private set; }

        public IEnumerable<KeyValuePair<string, string>> ConfigKeys
        {
            get 
            {
                return defaultConfigKeys.Keys
                    .Select(key => new KeyValuePair<string, string>(key, GetConfigValue(key)));
            }
        }

        public string GetConfigValue(string key)
        {
            string configValue = configStore.ReadUnencryptedString(configSectionName, key);

            string value;

            if (string.IsNullOrEmpty(configValue))
            {
                value = defaultConfigKeys[key];
                
                Trace.WriteNoise(
                    HealthConstants.TraceType,
                    "Config key '{0}' not defined in config store in watchdog health config policy '{1}'. Using default value of '{2}'",
                    key,
                    Name,
                    value);
            }
            else
            {
                value = configValue;
            }

            return value;
        }

        public bool DoesConfigKeyExist(string key)
        {
            return defaultConfigKeys.ContainsKey(key);
        }

        public abstract HealthState Apply(            
            RoleInstance roleInstance,
            HealthState input);

        protected void AddConfigKey(string key, string value)
        {
            defaultConfigKeys.Add(key, value);
        }

        protected string GetFullKeyName<T>(T keyName)
        {
            return string.Format(CultureInfo.InvariantCulture, HealthConstants.HealthKeyFormat, Name, keyName);            
        }
    }

    /// <summary>
    /// A health policy that allows users to map each particular value of <see cref="ManagementRoleInstanceStatus"/> to a
    /// <see cref="HealthState"/> in clustermanifest.xml.    
    /// </summary>
    internal class RoleInstanceStatusMapHealthPolicy : BaseRoleInstanceHealthPolicy
    {
        public RoleInstanceStatusMapHealthPolicy(IConfigStore configStore, string configSectionName)
            : base(configStore, configSectionName)
        {
            // apply default mapping
            foreach (var status in (ManagementRoleInstanceStatus[])Enum.GetValues(typeof(ManagementRoleInstanceStatus)))
            {
                string key = GetFullKeyName(status);
                HealthState value = GetHealthState(status);
                
                AddConfigKey(key, value.ToString());                
            }
        }
        
        public override HealthState Apply(RoleInstance roleInstance, HealthState input)
        {
            string key = GetFullKeyName(roleInstance.Status);

            if (!DoesConfigKeyExist(key))
            {
                if (input < HealthState.Warning)
                {
                    input = HealthState.Warning;
                    Trace.WriteWarning(
                        HealthConstants.TraceType,
                        "Updating health state to '{0}' due to unknown config key '{1}' provided to watchdog health config policy '{2}'",
                        input,
                        key,
                        Name);                    
                }
                else
                {
                    Trace.WriteWarning(
                        HealthConstants.TraceType,
                        "Unknown config key '{0}' provided in watchdog health config policy '{1}'",
                        key,
                        Name);                    
                }

                return input;
            }

            HealthState output;
            
            string value = GetConfigValue(key);
            bool success = Enum.TryParse(value, true, out output);

            if (success)
            {
                return output;
            }

            // we come here because of an Enum.TryParse error.
            if (input < HealthState.Warning)
            {
                input = HealthState.Warning;

                Trace.WriteWarning(
                    HealthConstants.TraceType,
                    "Updating health state to '{0}' due to error while getting value '{1}' for config key '{2}' provided to watchdog health config policy '{3}'",
                    input,
                    value,
                    key,
                    Name);
            }
            else
            {
                Trace.WriteWarning(
                    HealthConstants.TraceType,
                    "Error while getting value '{0}' for config key '{1}' provided to watchdog health config policy '{2}'",
                    value,
                    key,
                    Name);                                        
            }

            return input;
        }

        private static HealthState GetHealthState(ManagementRoleInstanceStatus status)
        {
            HealthState value;

            switch (status)
            {
                case ManagementRoleInstanceStatus.ReadyRole:
                    value = HealthState.Ok;
                    break;

                case ManagementRoleInstanceStatus.Unhealthy:
                    value = HealthState.Error;
                    break;

                default:
                    value = HealthState.Warning;
                    break;
            }

            return value;
        }
    }

    /// <summary>
    /// A health policy that allows suppression of health results to a certain health state. E.g. if we notice a bug in our
    /// health system where all role instances are now suddenly reported as unhealthy. This policy allows an operator to
    /// quickly suppress the 'unhealthy' values being reported by clipping (or allowing) a max value of just 'Warning' or 'Ok'.
    /// This way, end users don't have to suddenly see 'Red' in their alerting systems till the 
    /// buggy health policy system is fixed. 
    /// This policy is more like an emergency switch.
    /// </summary>
    internal class RoleInstanceStatusMaxAllowedHealthPolicy : BaseRoleInstanceHealthPolicy
    {
        private const HealthState DefaultMaxAllowedHealthState = HealthState.Error;

        private const string HealthStateKeyName = "HealthState";

        private readonly string maxAllowedKeyName;
        
        public RoleInstanceStatusMaxAllowedHealthPolicy(IConfigStore configStore, string configSectionName)
            : base(configStore, configSectionName)
        {
            maxAllowedKeyName = GetFullKeyName(HealthStateKeyName);

            AddConfigKey(maxAllowedKeyName, DefaultMaxAllowedHealthState.ToString());            
        }

        public override HealthState Apply(
            RoleInstance roleInstance, 
            HealthState input)
        {
            HealthState maxAllowedHealthState;

            string value = GetConfigValue(maxAllowedKeyName);
            bool success = Enum.TryParse(value, out maxAllowedHealthState);

            if (success)
            {
                // E.g. if input is Error and maxAllowedHealthState is Warning, then clamp (or restrict the return value to Warning)
                if (input > maxAllowedHealthState)
                {
                    Trace.WriteInfo(
                        HealthConstants.TraceType,
                        "Reducing input health state from '{0}' to '{1}' due to setting provided in config key '{2}' of watchdog health config policy '{3}'",
                        input,
                        maxAllowedHealthState,
                        maxAllowedKeyName,
                        Name);

                    input = maxAllowedHealthState;
                }
            }
            else
            {
                if (input < HealthState.Warning)
                {
                    input = HealthState.Warning;

                    Trace.WriteWarning(
                        HealthConstants.TraceType,
                        "Updating health state to '{0}' due to error while getting value '{1}' for config key '{2}' provided to watchdog health config policy '{3}'",
                        input,
                        value,
                        maxAllowedKeyName,
                        Name);
                }
                else
                {
                    Trace.WriteWarning(
                        HealthConstants.TraceType,
                        "Error while getting value '{0}' for config key '{1}' provided to watchdog health config policy '{2}'",
                        value,
                        maxAllowedKeyName,
                        Name);
                }
            }

            return input;
        }
    }

    /// <summary>
    /// The health policy manager executes all the health policies in sequence and reports the final health to the
    /// watchdog. 
    /// 'Dynamic' is prefixed here because this manager can pick the latest config settings in clustermanfiest.xml
    /// without needing a service level upgrade.
    /// </summary>
    internal class DynamicRoleInstanceHealthPolicyManager : IRoleInstanceHealthPolicyManager
    {
        private readonly List<IRoleInstanceHealthPolicy> healthPolicies = new List<IRoleInstanceHealthPolicy>();

        public DynamicRoleInstanceHealthPolicyManager(WindowsAzureInfrastructureCoordinator coordinator)
        {
            coordinator.ThrowIfNull("coordinator");

            healthPolicies.Add(new RoleInstanceStatusMapHealthPolicy(coordinator.ConfigStore, coordinator.ConfigSectionName));
            healthPolicies.Add(new RoleInstanceStatusMaxAllowedHealthPolicy(coordinator.ConfigStore, coordinator.ConfigSectionName));
        }

        public HealthState Execute(RoleInstance roleInstance, HealthState input)
        {
            foreach (var healthPolicy in healthPolicies)
            {
                input = healthPolicy.Apply(roleInstance, input);
            }

            return input;
        }
    }

    internal class RoleInstanceHealthWatchdogFactory : IHealthWatchdogFactory
    {
        private readonly WindowsAzureInfrastructureCoordinator coordinator;

        public RoleInstanceHealthWatchdogFactory(WindowsAzureInfrastructureCoordinator coordinator)
        {
            coordinator.ThrowIfNull("coordinator");

            this.coordinator = coordinator;
        }

        public IHealthWatchdog Create()
        {
            var watchdog = new RoleInstanceHealthWatchdog(coordinator);
            return watchdog;
        }
    }

    /// <summary>
    /// A watchdog specific to reporting role instance health. 
    /// First reports the health by invoking the management client API. Then invokes the policy executor
    /// which in turn applies all the policies and returns the final health state of a role instance to the
    /// watchdog. The watchdog then reports the final health state to the Windows Fabric health store.
    /// </summary>
    internal class RoleInstanceHealthWatchdog : IHealthWatchdog
    {
        private const string ServiceNamePrefix = "fabric:/System/";

        private readonly WindowsAzureInfrastructureCoordinator coordinator;
        private readonly FabricClient.HealthClient healthClient = new FabricClient().HealthManager;
        private readonly IRoleInstanceHealthPolicyManager healthPolicyManager;
        
        public RoleInstanceHealthWatchdog(WindowsAzureInfrastructureCoordinator coordinator)
        {
            coordinator.ThrowIfNull("coordinator");

            this.coordinator = coordinator;

            healthPolicyManager = new DynamicRoleInstanceHealthPolicyManager(coordinator);
        }

        /// <summary>
        /// Reports health of all the Azure roleinstances corresponding to the Windows Fabric nodes
        /// to the Windows Fabric health store.
        /// </summary>
        /// <param name="validityPeriod">Serves as an indication to the watchdog as to how long the reported health is valid for.</param>
        public void ReportHealth(TimeSpan validityPeriod)
        {            
            // start with an initial assumption that health is okay.
            const HealthState InputHealthState = HealthState.Ok;

            try
            {
                IList<RoleInstance> roleInstances = GetRoleInstances();

                HealthState serviceHealthState = roleInstances == null 
                    ? HealthState.Warning 
                    : GetRoleInstanceHealth(roleInstances, InputHealthState, healthPolicyManager);

                string description = GetServiceHealthDescription(roleInstances, serviceHealthState);

                // Specify a time to live. In case infrastructure service is down, the health report shows up as warning (or error)
                var hi = new HealthInformation(
                    HealthConstants.HealthReportSourceId,
                    HealthConstants.HealthReportReplicaHealthWatchdogStatus,
                    serviceHealthState)
                {
                    Description = description,
                    TimeToLive = validityPeriod != TimeSpan.Zero ? validityPeriod : TimeSpan.MaxValue,
                    RemoveWhenExpired = true
                };

                // Construct this name ourselves since there is currently a bug where the service Uri which 
                // could have been passed in all the way from ServiceFactory's CreateReplica is an empty string.
                var serviceUri = new Uri(ServiceNamePrefix + coordinator.ConfigSectionName);

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
                    HealthConstants.TraceType,
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

        private IList<RoleInstance> GetRoleInstances()
        {
            IList<RoleInstance> roleInstances = null;

            try
            {
                roleInstances = coordinator.ManagementClient.GetRoleInstances();
            }
            catch (Exception ex)
            {
                // no choice but to catch everything since we don't know what to expect from management client
                Trace.WriteWarning(
                    HealthConstants.TraceType,
                    "Unable to get role instance info from management client. Error(s): {0}",
                    ex.GetMessage());
            }

            return roleInstances;
        }

        private HealthState GetRoleInstanceHealth(
            IList<RoleInstance> roleInstances, 
            HealthState inputHealthState,
            IRoleInstanceHealthPolicyManager healthPolicyManager)
        {
            var serviceHealthState = HealthState.Ok;

            foreach (RoleInstance roleInstance in roleInstances)
            {
                var outputHealthState = inputHealthState;

                try
                {
                    outputHealthState = healthPolicyManager.Execute(roleInstance, inputHealthState);
                }
                catch (Exception ex)
                {
                    // no choice but to catch everything since we don't know what to expect from each policy implementation
                    string text = ex.GetMessage();

                    Trace.WriteWarning(
                        HealthConstants.TraceType,
                        "Unable to apply health policies on role instance '{0}'. Error(s): {1}",
                        roleInstance.Id,
                        text);

                    outputHealthState = HealthState.Warning;
                    serviceHealthState = HealthState.Warning;
                }
                finally
                {
                    AddReport(roleInstance, outputHealthState);
                }
            }

            return serviceHealthState;
        }

        /// <summary>
        /// Adds a health report to the Windows Fabric health system.
        /// </summary>
        /// <param name="roleInstance">The role instance.</param>
        /// <param name="healthState">State of the health.</param>
        private void AddReport(RoleInstance roleInstance, HealthState healthState)
        {
            string nodeName = roleInstance.Id.TranslateRoleInstanceToNodeName();

            // this one is purely for informational purposes            
            AddReport(nodeName, HealthConstants.HealthReportRoleInstanceLastUpdateTimePropertyName, roleInstance.LastUpdateTime.ToString("o"), HealthState.Ok);
            
            AddReport(nodeName, HealthConstants.HealthReportRoleInstanceStatusPropertyName, roleInstance.Status.ToString(), healthState);
        }

        private void AddReport(string nodeName, string key, string value, HealthState healthState)
        {
            // Don't specify a time to live here. Because if infrastructure service is down, then
            // all role instances (nodes in WinFab terms) are reported as down in the health report. 
            // i.e. it appears as though the whole cluster is down. In that case, WinFab upgrades 
            // etc. won't work since they perform checks on health status.
            // Instead, we are specifying a time to live only at the service health level
            var hi = new HealthInformation(HealthConstants.HealthReportSourceId, key, healthState)
            {
                Description = value
            };

            var healthReport = new NodeHealthReport(nodeName, hi);
            healthClient.ReportHealth(healthReport);            
        }
    }
}