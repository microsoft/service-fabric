// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Health;

    /// <summary>
    /// A health policy that allows users to map each particular value of <see cref="RoleInstanceState"/> to a
    /// <see cref="HealthState"/> in clustermanifest.xml.    
    /// </summary>
    internal class RoleInstanceStatusMapHealthPolicy : BaseRoleInstanceHealthPolicy
    {
        public RoleInstanceStatusMapHealthPolicy(IConfigSection configSection, string configKeyPrefix)
            : base(configSection, configKeyPrefix)
        {
            // apply default mapping
            foreach (var status in (RoleInstanceState[])Enum.GetValues(typeof(RoleInstanceState)))
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
                        RoleInstanceHealthConstants.TraceType,
                        "Updating health state to '{0}' due to unknown config key '{1}' provided to watchdog health config policy '{2}'",
                        input,
                        key,
                        Name);                    
                }
                else
                {
                    Trace.WriteWarning(
                        RoleInstanceHealthConstants.TraceType,
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
                    RoleInstanceHealthConstants.TraceType,
                    "Updating health state to '{0}' due to error while getting value '{1}' for config key '{2}' provided to watchdog health config policy '{3}'",
                    input,
                    value,
                    key,
                    Name);
            }
            else
            {
                Trace.WriteWarning(
                    RoleInstanceHealthConstants.TraceType,
                    "Error while getting value '{0}' for config key '{1}' provided to watchdog health config policy '{2}'",
                    value,
                    key,
                    Name);                                        
            }

            return input;
        }

        private static HealthState GetHealthState(RoleInstanceState status)
        {
            HealthState value;

            switch (status)
            {
                case RoleInstanceState.ReadyRole:
                    value = HealthState.Ok;
                    break;

                case RoleInstanceState.Unhealthy:
                    value = HealthState.Error;
                    break;

                default:
                    value = HealthState.Warning;
                    break;
            }

            return value;
        }
    }
}