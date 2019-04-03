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
    
    internal class UpgradeUpdateDescriptionJsonConverterHelper<T> where T : UpgradeUpdateDescriptionBase
    {
        private static string UpdateDescriptionName = "UpdateDescription";
        private static string UpgradeKindName = "UpgradeKind";
        
        public UpgradeUpdateDescriptionJsonConverterHelper(string[] firstLevelProperties)
        {
            this.FirstLevelProperties = firstLevelProperties;
        }

        public string[] FirstLevelProperties { get; private set; }
        
        public T PopulateInstanceUsingJObject(JObject jObject, T targetObject, JsonSerializer serializer)
        {
            serializer.Populate(jObject.CreateReader(), targetObject);

            // Native code writes "UpgradeKind" and "UpdateDescription" (RollingUpgradeUpdateDescription) as inner objects
            // Ignore "UpgradeKind", since managed class doesn't use it.

            // Read UpdateDescription and populate the fields directly into our object
            serializer.Populate(jObject[UpdateDescriptionName].CreateReader(), targetObject);

            return targetObject;
        }

        public JContainer GetJContainerForInstance(JObject input, JsonSerializer serializer)
        {
            // The result, that will have the first level fields and the rest encapsulated in an inner object.
            var result = new JObject(input);

            // Create a copy of the object to represent an inner object that encapsulates some of the object properties
            var innerClass = new JObject(input);

            foreach (var p in this.FirstLevelProperties)
            {
                innerClass.Remove(p);
            }

            // Remove all properties from outer class that do not belong to outsideProperties
            foreach (JProperty child in input.Children<JProperty>())
            {
                bool shouldExclude = true;
                foreach (var p in FirstLevelProperties)
                {
                    if (child.Name == p)
                    {
                        shouldExclude = false;
                    }
                }

                if (shouldExclude)
                {
                    result.Remove(child.Name);
                }
            }

            result.Add(UpdateDescriptionName, innerClass);

            // Add upgrade kind which is not used in managed
            var upgradeKind = UpgradeKind.Rolling;
            result.Add(UpgradeKindName, JToken.FromObject(upgradeKind, serializer));

            return result;
        }
    }


    internal class FabricUpgradeUpdateDescriptionJsonConverter : CustomCreationJsonConverterBase<FabricUpgradeUpdateDescription>
    {
        private static string[] FirstLevelProperties = new string[] { "ClusterHealthPolicy", "EnableDeltaHealthEvaluation", "ClusterUpgradeHealthPolicy", "ApplicationHealthPolicyMap" };

        private UpgradeUpdateDescriptionJsonConverterHelper<FabricUpgradeUpdateDescription> helper;

        public FabricUpgradeUpdateDescriptionJsonConverter()
            : base(true, true) // Can Read, Can write
        {
            this.helper = new UpgradeUpdateDescriptionJsonConverterHelper<FabricUpgradeUpdateDescription>(FirstLevelProperties);
        }

        protected override FabricUpgradeUpdateDescription Create(Type objectType, JObject jObject)
        {
            return new FabricUpgradeUpdateDescription();
        }

        protected override FabricUpgradeUpdateDescription PopulateInstanceUsingJObject(JObject jObject, FabricUpgradeUpdateDescription targetObject, JsonSerializer serializer)
        {
            return this.helper.PopulateInstanceUsingJObject(jObject, targetObject, serializer);
        }

        protected override JContainer GetJContainerForInstance(object value, JsonSerializer serializer)
        {
            var upgradeUpdateDesc = (FabricUpgradeUpdateDescription)value;

            var jObject = this.CreateJObjectFrom(upgradeUpdateDesc, serializer);

            return this.helper.GetJContainerForInstance(jObject, serializer);
        }
    }
}