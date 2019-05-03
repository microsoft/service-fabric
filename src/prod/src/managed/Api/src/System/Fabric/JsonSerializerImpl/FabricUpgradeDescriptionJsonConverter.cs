// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using System.Fabric.Description;

    internal class FabricUpgradeDescriptionJsonConverter : CustomCreationJsonConverterBase<FabricUpgradeDescription>
    {
        public FabricUpgradeDescriptionJsonConverter()
            : base(true, true) // Can Read, Can write
        {
        }

        protected override FabricUpgradeDescription Create(Type objectType, Newtonsoft.Json.Linq.JObject jObject)
        {
            return new FabricUpgradeDescription();
        }

        protected override FabricUpgradeDescription PopulateInstanceUsingJObject(JObject jObject, FabricUpgradeDescription targetObject, JsonSerializer serializer)
        {
            serializer.Populate(jObject.CreateReader(), targetObject);
            targetObject.UpgradePolicyDescription = serializer.Deserialize<UpgradePolicyDescription>(jObject.CreateReader());
            return targetObject;
        }

        protected override JContainer GetJContainerForInstance(object value, JsonSerializer serializer)
        {
            FabricUpgradeDescription fabricUpgradeDescription = (FabricUpgradeDescription)value;
            JObject jUpgradeDescription = this.CreateJObjectFrom(value, serializer);

            if (fabricUpgradeDescription.UpgradePolicyDescription != null)
            {
                // Add UpgradePolicyDescription's properties
                JObject jUpgradePolicy = JObject.FromObject(fabricUpgradeDescription.UpgradePolicyDescription, serializer);
                foreach (var prop in jUpgradePolicy)
                {
                    if (prop.Key == "UpgradeReplicaSetCheckTimeoutInSeconds")
                    {
                        var timeoutInSec = (((RollingUpgradePolicyDescription)fabricUpgradeDescription.UpgradePolicyDescription).UpgradeReplicaSetCheckTimeout.Ticks / TimeSpan.TicksPerSecond);
                        jUpgradeDescription.Add(prop.Key, JToken.FromObject(timeoutInSec));
                    }
                    else
                    {
                        jUpgradeDescription.Add(prop.Key, prop.Value);
                    }
                }
            }

            return jUpgradeDescription;
        }
    }
}