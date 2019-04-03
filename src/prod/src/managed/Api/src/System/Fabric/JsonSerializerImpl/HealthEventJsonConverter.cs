// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using System.Fabric.Health;

    internal class HealthEventJsonConverter : CustomCreationJsonConverterBase<HealthEvent>
    {
        public HealthEventJsonConverter()
            : base(true, true)
        {
        }

        protected override HealthEvent Create(Type objectType, JObject jObject)
        {
            return new HealthEvent();
        }

        protected override HealthEvent PopulateInstanceUsingJObject(JObject jObject, HealthEvent targetObject, JsonSerializer serializer)
        {
            HealthEvent healthEvent = (HealthEvent)targetObject;
            serializer.Populate(jObject.CreateReader(), healthEvent);
            healthEvent.HealthInformation = new HealthInformation();
            serializer.Populate(jObject.CreateReader(), healthEvent.HealthInformation);
            return healthEvent;
        }

        protected override JContainer GetJContainerForInstance(object value, JsonSerializer serializer)
        {
            HealthEvent healthEvent = (HealthEvent)value;
            JObject jhealthEvent = this.CreateJObjectFrom(value, serializer);
            jhealthEvent.Remove("HealthInformation");
            if (healthEvent.HealthInformation == null)
            {
                return jhealthEvent;
            }

            /// Add HealthInformation's properties
            JObject jhealthInformation = JObject.FromObject(healthEvent.HealthInformation, serializer);
            foreach (var prop in jhealthInformation)
            {
                jhealthEvent.Add(prop.Key, prop.Value);
            }

            return jhealthEvent;
        }
    }
}