// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Collections.Generic;
    using Health;

    /// <summary>
    /// The health policy manager executes all the health policies in sequence and reports the final health to the
    /// watchdog. 
    /// 'Dynamic' is prefixed here because this manager can pick the latest config settings in clustermanfiest.xml
    /// without needing a service level upgrade.
    /// </summary>
    internal class DynamicRoleInstanceHealthPolicyManager : IRoleInstanceHealthPolicyManager
    {
        private readonly List<IRoleInstanceHealthPolicy> healthPolicies = new List<IRoleInstanceHealthPolicy>();

        public DynamicRoleInstanceHealthPolicyManager(IConfigSection configSection, string configKeyPrefix)
        {
            configSection.Validate("configSection");

            healthPolicies.Add(new RoleInstanceStatusMapHealthPolicy(configSection, configKeyPrefix));
            healthPolicies.Add(new RoleInstanceStatusMaxAllowedHealthPolicy(configSection, configKeyPrefix));
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
}