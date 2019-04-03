// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Collections.Generic;
    using Globalization;
    using Health;
    using Linq;

    /// <summary>
    /// A base class for common functionality among all health policies.
    /// </summary>
    internal abstract class BaseRoleInstanceHealthPolicy : IRoleInstanceHealthPolicy
    {
        private readonly IDictionary<string, string> defaultConfigKeys = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

        private readonly IConfigSection configSection;
        private readonly string configKeyPrefix;

        protected BaseRoleInstanceHealthPolicy(IConfigSection configSection, string configKeyPrefix)
        {
            this.configSection = configSection.Validate("configSection");
            this.configKeyPrefix = configKeyPrefix.Validate("configKeyPrefix");

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
            string configValue = configSection.ReadConfigValue<string>(key);

            string value;

            if (string.IsNullOrEmpty(configValue))
            {
                value = defaultConfigKeys[key];
                
                Trace.WriteNoise(
                    RoleInstanceHealthConstants.TraceType,
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
            return GetFullKeyName(this.configKeyPrefix, this.Name, keyName);
        }

        internal static string GetFullKeyName<T>(string prefix, string componentName, T keyName)
        {
            // e.g. Azure.Health.RoleInstanceStatusMap.ReadyRole
            //      ^prefix      ^componentName        ^keyName
            return string.Format(CultureInfo.InvariantCulture, RoleInstanceHealthConstants.HealthKeyFormat, prefix, componentName, keyName);
        }
    }
}