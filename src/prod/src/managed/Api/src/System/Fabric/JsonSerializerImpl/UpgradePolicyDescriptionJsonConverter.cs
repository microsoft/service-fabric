// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using System.Fabric.Description;

    internal class UpgradePolicyDescriptionJsonConverter : CustomCreationJsonConverterBase<UpgradePolicyDescription>
    {
        public UpgradePolicyDescriptionJsonConverter()
            : base(true, false) // Can Read, Can't write
        {
        }

        protected override UpgradePolicyDescription Create(Type objectType, Newtonsoft.Json.Linq.JObject jObject)
        {
            return null;
        }

        protected override UpgradePolicyDescription PopulateInstanceUsingJObject(JObject jObject, UpgradePolicyDescription targetObject, Newtonsoft.Json.JsonSerializer serializer)
        {
            // UpgradeKind is required property.
            UpgradeKind jUpgradeKind = jObject["UpgradeKind"].ToObject<UpgradeKind>();
            if (jUpgradeKind == UpgradeKind.Rolling)
            {
                return jObject.ToObject<RollingUpgradePolicyDescription>(serializer);
            }

            throw new FabricSerializationException(string.Format("UpgradePolicyDescriptionJsonConverter: PopulateInstanceUsingJObject(): Invalid UpgradeKind: {0}.", jUpgradeKind));
        }

        protected override JContainer GetJContainerForInstance(object value, JsonSerializer serializer)
        {
            throw new NotImplementedException();
        }
    }
}