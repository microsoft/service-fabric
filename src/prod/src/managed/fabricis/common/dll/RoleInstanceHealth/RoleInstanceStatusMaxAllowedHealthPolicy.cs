// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Health;

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
        
        public RoleInstanceStatusMaxAllowedHealthPolicy(IConfigSection configSection, string configKeyPrefix)
            : base(configSection, configKeyPrefix)
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
                        RoleInstanceHealthConstants.TraceType,
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
                        RoleInstanceHealthConstants.TraceType,
                        "Updating health state to '{0}' due to error while getting value '{1}' for config key '{2}' provided to watchdog health config policy '{3}'",
                        input,
                        value,
                        maxAllowedKeyName,
                        Name);
                }
                else
                {
                    Trace.WriteWarning(
                        RoleInstanceHealthConstants.TraceType,
                        "Error while getting value '{0}' for config key '{1}' provided to watchdog health config policy '{2}'",
                        value,
                        maxAllowedKeyName,
                        Name);
                }
            }

            return input;
        }
    }
}