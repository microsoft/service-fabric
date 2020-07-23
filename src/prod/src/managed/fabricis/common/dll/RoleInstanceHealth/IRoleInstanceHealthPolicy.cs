// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Collections.Generic;
    using Health;

    /// <summary>
    /// Interface for a health policy. Various health policies could be applied on the role instance health observed
    /// by the watchdog before finally sending it off to the healh store.
    /// </summary>
    internal interface IRoleInstanceHealthPolicy
    {
        /// <summary>
        /// Gets the name of the health policy.
        /// </summary>
        string Name { get; }

        /// <summary>
        /// Gets the configuration keys.
        /// Config keys could be used to tweak the settings before applying the health policy
        /// on a role instance health status.
        /// </summary>
        IEnumerable<KeyValuePair<string, string>> ConfigKeys { get; }

        /// <summary>
        /// Gets the value of a particular config key.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <returns>The value of a config key</returns>
        string GetConfigValue(string key);

        /// <summary>
        /// Checks if the config key key exists.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <returns><c>true</c> if the config key exists. <c>false</c> otherwise.</returns>
        bool DoesConfigKeyExist(string key);

        /// <summary>
        /// Applies the health policy on a role instance health specified updated configuration keys.
        /// </summary>
        /// <param name="roleInstance">The role instance.</param>
        /// <param name="input">The input health state on which the policy will be applied.</param>
        /// <returns>The updated health state for the role instance</returns>
        /// <remarks>Consider wrapping the parameters in an object and passing it to this method.</remarks>
        HealthState Apply(RoleInstance roleInstance, HealthState input);
    }
}