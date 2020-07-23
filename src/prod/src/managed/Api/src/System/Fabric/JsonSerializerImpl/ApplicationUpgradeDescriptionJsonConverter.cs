// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using System.Fabric.Description;

    internal class ApplicationUpgradeDescriptionJsonConverter : CustomCreationJsonConverterBase<ApplicationUpgradeDescription>
    {
        public ApplicationUpgradeDescriptionJsonConverter()
            : base(true, true) // Can Read, Can write
        {
        }

        protected override ApplicationUpgradeDescription Create(Type objectType, Newtonsoft.Json.Linq.JObject jObject)
        {
            return new ApplicationUpgradeDescription();
        }

        protected override ApplicationUpgradeDescription PopulateInstanceUsingJObject(JObject jObject, ApplicationUpgradeDescription targetObject, JsonSerializer serializer)
        {
            serializer.Populate(jObject.CreateReader(), targetObject);
            targetObject.UpgradePolicyDescription = serializer.Deserialize<UpgradePolicyDescription>(jObject.CreateReader());
            return targetObject;
        }

        protected override JContainer GetJContainerForInstance(object value, JsonSerializer serializer)
        {
            ApplicationUpgradeDescription applicationUpgradeDescription = (ApplicationUpgradeDescription)value;

            // Add ApplicationUpgradeDescription's properties
            JObject jApplicationUpgradeDescription = this.CreateJObjectFrom(applicationUpgradeDescription, serializer);

            if (applicationUpgradeDescription.UpgradePolicyDescription != null)
            {
                // Add UpgradePolicyDescription's properties
                var jUpgradePolicy = JObject.FromObject(applicationUpgradeDescription.UpgradePolicyDescription, serializer);
                foreach (var prop in jUpgradePolicy)
                {
                    if (prop.Key == "UpgradeReplicaSetCheckTimeoutInSeconds")
                    {
                        var timeoutInSec = (((RollingUpgradePolicyDescription)applicationUpgradeDescription.UpgradePolicyDescription).UpgradeReplicaSetCheckTimeout.Ticks / TimeSpan.TicksPerSecond);
                        jApplicationUpgradeDescription.Add(prop.Key, JToken.FromObject(timeoutInSec));
                    }
                    else
                    {
                        jApplicationUpgradeDescription.Add(prop.Key, prop.Value);
                    }
                }
            }

            return jApplicationUpgradeDescription;
        }
    }
}