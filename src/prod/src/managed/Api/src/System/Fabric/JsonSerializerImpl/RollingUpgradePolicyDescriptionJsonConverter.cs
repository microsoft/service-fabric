// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using System.Fabric.Description;
    using System.Management.Automation.Language;

    internal class RollingUpgradePolicyDescriptionJsonConverter : CustomCreationJsonConverterBase<RollingUpgradePolicyDescription>
    {
        public RollingUpgradePolicyDescriptionJsonConverter()
            : base(true, false) // Can Read, Can't write
        {
        }

        protected override RollingUpgradePolicyDescription Create(Type objectType, Newtonsoft.Json.Linq.JObject jObject)
        {
            return null;
        }

        protected override RollingUpgradePolicyDescription PopulateInstanceUsingJObject(JObject jObject, RollingUpgradePolicyDescription targetObject, JsonSerializer serializer)
        {
            // This is required property in json.
            RollingUpgradeMode jRollingUpgradeMode = jObject["RollingUpgradeMode"].ToObject<RollingUpgradeMode>();

            switch (jRollingUpgradeMode)
            {
                case RollingUpgradeMode.Monitored:
                    return jObject.ToObject<MonitoredRollingUpgradePolicyDescription>(serializer);

                case RollingUpgradeMode.UnmonitoredAuto:
                    return PopulateInstanceUsingJObjectInner(jObject, jRollingUpgradeMode, serializer);

                case RollingUpgradeMode.UnmonitoredManual:
                    return PopulateInstanceUsingJObjectInner(jObject, jRollingUpgradeMode, serializer);

                default:
                    throw new FabricSerializationException(string.Format("RollingUpgradePolicyDescriptionJsonConverter: PopulateInstanceUsingJObject(): Invalid RollingUpgradeMode: {0}.", jRollingUpgradeMode));
            }
        }

        private RollingUpgradePolicyDescription PopulateInstanceUsingJObjectInner(JObject jObject, RollingUpgradeMode jRollingUpgradeMode, JsonSerializer serializer)
        {
            RollingUpgradePolicyDescription targetObject = new RollingUpgradePolicyDescription();
            targetObject.UpgradeMode = jRollingUpgradeMode;
            targetObject.ForceRestart = jObject["ForceRestart"].ToObject<bool>();
            
            //UpgradeReplicaSetCheckTimeoutInSeconds
            int upgradeReplicaSetCheckTimeoutInSeconds = jObject["UpgradeReplicaSetCheckTimeoutInSeconds"].ToObject<int>();
            targetObject.UpgradeReplicaSetCheckTimeout = TimeSpan.FromSeconds(upgradeReplicaSetCheckTimeoutInSeconds);

            return targetObject;
        }

        protected override JContainer GetJContainerForInstance(object value, Newtonsoft.Json.JsonSerializer serializer)
        {
            throw new NotImplementedException();
        }
    }
}