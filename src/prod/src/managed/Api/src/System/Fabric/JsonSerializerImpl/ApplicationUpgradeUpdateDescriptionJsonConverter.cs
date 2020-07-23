// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Collections.Generic;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using System.Fabric.Description;

    internal class ApplicationUpgradeUpdateDescriptionJsonConverter : CustomCreationJsonConverterBase<ApplicationUpgradeUpdateDescription>
    {
        private static string[] FirstLevelProperties = new string[] { "Name", "ApplicationHealthPolicy" };
        private UpgradeUpdateDescriptionJsonConverterHelper<ApplicationUpgradeUpdateDescription> helper;

        public ApplicationUpgradeUpdateDescriptionJsonConverter()
            : base(true, true) // Can Read, Can write
        {
            this.helper = new UpgradeUpdateDescriptionJsonConverterHelper<ApplicationUpgradeUpdateDescription>(FirstLevelProperties);
        }

        protected override ApplicationUpgradeUpdateDescription Create(Type objectType, JObject jObject)
        {
            return new ApplicationUpgradeUpdateDescription();
        }

        protected override ApplicationUpgradeUpdateDescription PopulateInstanceUsingJObject(JObject jObject, ApplicationUpgradeUpdateDescription targetObject, JsonSerializer serializer)
        {
            return this.helper.PopulateInstanceUsingJObject(jObject, targetObject, serializer);
        }

        protected override JContainer GetJContainerForInstance(object value, JsonSerializer serializer)
        {
            var upgradeUpdateDesc = (ApplicationUpgradeUpdateDescription)value;

            var jObject = this.CreateJObjectFrom(upgradeUpdateDesc, serializer);

            return this.helper.GetJContainerForInstance(jObject, serializer);
        }
    }
}