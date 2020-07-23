// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Health;
    using System.Fabric.InfrastructureService;
    using System.Globalization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Unit test class for testing various health policies
    /// </summary>
    [TestClass]
    internal class RoleInstanceStatusPolicyTest
    {
        private const string SectionName = "IS1";

        private static readonly TraceType TraceType = new TraceType(typeof(RoleInstanceStatusPolicyTest).Name);

        /// <summary>
        /// A simple test to ensure that the health policy keys conform to a well known format
        /// so that it is easy for us to separate keys by a sort of namespace.
        /// </summary>
        [TestMethod]
        public void GetConfigKeysTest()
        {
            var configSection = new ConfigSection(TraceType, new MockConfigStore(), SectionName);

            string rootPrefix = "Test.";

            var policies = new List<IRoleInstanceHealthPolicy>
            {
                new RoleInstanceStatusMapHealthPolicy(configSection, rootPrefix),
                new RoleInstanceStatusMaxAllowedHealthPolicy(configSection, rootPrefix)
            };
            
            foreach (var policy in policies)
            {
                var prefix = BaseRoleInstanceHealthPolicy.GetFullKeyName(
                    rootPrefix,
                    policy.Name,
                    string.Empty);

                foreach (var key in policy.ConfigKeys)
                {
                    bool conformsToFormat = key.Key.StartsWith(prefix, StringComparison.OrdinalIgnoreCase);

                    string message = string.Format(CultureInfo.CurrentCulture, "{{ {0} => {1} }}, conforms to format: {2}", key.Key, key.Value, conformsToFormat);

                    Assert.IsTrue(conformsToFormat, message);
                }
            }
        }

        /// <summary>
        /// Tests if the user can update a <see cref="RoleInstanceState"/> value to map to a 
        /// different <see cref="HealthState"/> value.
        /// </summary>
        [TestMethod]
        public void MapTest()
        {
            var configStore = new MockConfigStore();
            var configSection = new ConfigSection(TraceType, configStore, SectionName);

            string rootPrefix = "Test.";

            var policy = new RoleInstanceStatusMapHealthPolicy(configSection, rootPrefix);
            var ri = new RoleInstance("1", RoleInstanceState.Unhealthy, DateTime.UtcNow);

            string key = BaseRoleInstanceHealthPolicy.GetFullKeyName(
                rootPrefix,
                policy.Name,
                RoleInstanceState.Unhealthy);

            var defaultValue = policy.GetConfigValue(key);
            
            HealthState output = policy.Apply(ri, HealthState.Ok);
            Assert.AreEqual(output.ToString(), defaultValue, "Health state with default mapping");

            const HealthState NewHealth = HealthState.Warning;
            
            configStore.AddKeyValue(SectionName, key, NewHealth.ToString());            

            output = policy.Apply(ri, HealthState.Ok);

            Assert.AreEqual(output, NewHealth, "New health state with updated mapping");
        }

        /// <summary>
        /// Tests if the user can set a max allowed role instance health status in order to suppress
        /// error/warning health status and show them as Ok (if required in any circumstance).
        /// This is needed if we detect a bug in our health determination and want to not alarm the operators
        /// with incorrect health status such as Unhealthy/Unknown.
        /// </summary>
        [TestMethod]
        public void MaxAllowedValueTest()
        {
            var configStore = new MockConfigStore();
            var configSection = new ConfigSection(TraceType, configStore, SectionName);

            string rootPrefix = "Test.";

            var policy = new RoleInstanceStatusMaxAllowedHealthPolicy(configSection, rootPrefix);
            var ri = new RoleInstance("1", RoleInstanceState.Unhealthy, DateTime.UtcNow);

            string maxAllowedKeyName = BaseRoleInstanceHealthPolicy.GetFullKeyName(
                rootPrefix,
                policy.Name,
                "HealthState");

            configStore.UpdateKey(SectionName, maxAllowedKeyName, HealthState.Error.ToString());

            var inputHealthState = HealthState.Unknown;
            HealthState output = policy.Apply(ri, inputHealthState);


            Assert.AreEqual(output, HealthState.Error, string.Format(CultureInfo.CurrentCulture, "Output health state clipped down from '{0}' to max allowed state '{1}'", inputHealthState, output));

            inputHealthState = HealthState.Warning;
            output = policy.Apply(ri, inputHealthState);

            Assert.AreEqual(output, inputHealthState, "Output health state not clipped since it is below max allowed state");

            inputHealthState = HealthState.Unknown;
            string oldValue = configStore.GetValue(SectionName, maxAllowedKeyName);
            configStore.UpdateKey(SectionName, maxAllowedKeyName, HealthState.Warning.ToString());
            output = policy.Apply(ri, inputHealthState);

            Assert.AreEqual(output, HealthState.Warning, string.Format(CultureInfo.CurrentCulture, "Output health state clipped down from '{0}' to max allowed state '{1}'", inputHealthState, output));

            // revert to old value            
            configStore.UpdateKey(SectionName, maxAllowedKeyName, oldValue);            
            output = policy.Apply(ri, inputHealthState);

            Assert.AreEqual(output.ToString(), oldValue, string.Format(CultureInfo.CurrentCulture, "Output health state clipped down from '{0}' to max allowed state '{1}'", inputHealthState, output));
        }

        /// <summary>
        /// Tests combinations of invalid key name/value input which can typically occur due to a user error
        /// while setting the clustermanifest.xml.
        /// </summary>
        [TestMethod]
        public void InvalidKeysTest()
        {
            var configStore = new MockConfigStore();
            var configSection = new ConfigSection(TraceType, configStore, SectionName);

            string rootPrefix = "Test.";

            var policy = new RoleInstanceStatusMaxAllowedHealthPolicy(configSection, rootPrefix);
            var ri = new RoleInstance("1", RoleInstanceState.Unhealthy, DateTime.UtcNow);

            string maxAllowedKeyName = BaseRoleInstanceHealthPolicy.GetFullKeyName(
                rootPrefix,
                policy.Name,
                "HealthState");

            #region typo in key name

            string nonExistentKeyName = BaseRoleInstanceHealthPolicy.GetFullKeyName(
                rootPrefix,
                policy.Name,
                "HealthStatt"); // note the type from "HealthState". user has typed this in his clustermanifest.xml

            // set a default value to config store
            configStore.UpdateKey(SectionName, maxAllowedKeyName, HealthState.Error.ToString());

            // now update config store with an invalid key
            configStore.UpdateKey(SectionName, nonExistentKeyName, HealthState.Unknown.ToString());

            // fetch the correct key
            var maxAllowedHealthState = configStore.GetValue(SectionName, maxAllowedKeyName);

            const HealthState InputHealthState = HealthState.Unknown;
            HealthState output = policy.Apply(ri, InputHealthState);

            Assert.AreEqual(
                output, 
                HealthState.Error,
                string.Format(CultureInfo.CurrentCulture, "Output health state clipped down from '{0}' to default max allowed state '{1}'", InputHealthState, output));

            // now user realizes that he has made a typo in his clustermanifest.xml
            configStore.UpdateKey(SectionName, maxAllowedKeyName, HealthState.Unknown.ToString());
            output = policy.Apply(ri, InputHealthState);

            Assert.AreEqual(output, HealthState.Unknown, string.Format(CultureInfo.CurrentCulture, "Output health state not clipped down from '{0}' to max allowed state '{1}'", InputHealthState, output));

            #endregion typo in key name

            #region typo in key value
            
            configStore.ClearKeys(); // revert to defaults
            string defaultMaxHealthState = policy.GetConfigValue(maxAllowedKeyName);

            Assert.AreEqual(defaultMaxHealthState, HealthState.Error.ToString());

            // In this case, we know the default value as 'Error'. 
            // Ideally, we need to parse the enum and get the next higher enum to update the key with
            configStore.UpdateKey(SectionName, maxAllowedKeyName, "Wurning");

            output = policy.Apply(ri, InputHealthState);

            // verify that Wurning didn't apply due to the typo. Since input was higher (Unknown) than the default, the output is also Unknown
            // i.e. no clipping applied

            Assert.AreEqual(
                output,
                InputHealthState,
                string.Format(
                    CultureInfo.CurrentCulture, 
                    "Output health state not altered from '{0}' to default max allowed state '{1}' as it may not be safe to apply default since input was already higher than default", 
                    InputHealthState, 
                    defaultMaxHealthState));

            // provide a lower health state. Since there is an error in parsing, a warning should be returned
            output = policy.Apply(ri, HealthState.Ok);

            // verify that input of Ok is converted to Warning so that user observes that something is wrong and fixes his typo

            Assert.AreEqual(
                output,
                HealthState.Warning,
                string.Format(
                    CultureInfo.CurrentCulture,
                    "Output health state altered from '{0}' to '{1}' as there was an error applying policy",
                    HealthState.Ok,
                    output));

            // user realizes typo and fixes it
            configStore.UpdateKey(SectionName, maxAllowedKeyName, HealthState.Warning.ToString());

            output = policy.Apply(ri, InputHealthState);

            // verify that the new max allowed policy is in effect
            Assert.AreEqual(
                output,
                HealthState.Warning,
                string.Format(CultureInfo.CurrentCulture, "Output health state clipped down from '{0}' to max allowed state '{1}'", InputHealthState, output));

            #endregion typo in key value
        }
    }
}