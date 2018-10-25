// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    internal class VolumeMountJsonConverter : JsonConverter
    {
        private static Dictionary<int,Func<string,VolumeDescription>> IndependentVolumeDescriptionCreator = new Dictionary<int,Func<string,VolumeDescription>>()
        {
            { (int)VolumeProvider.AzureFile, CreateNewVolumeDescriptionAzureFile },
            { (int)VolumeProvider.ServiceFabricVolumeDisk, CreateNewVolumeDescriptionVolumeDisk },
        };
        private static Dictionary<int,Func<string,VolumeDescription>> ApplicationScopedVolumeDescriptionCreator = new Dictionary<int,Func<string,VolumeDescription>>()
        {
            { (int)ApplicationScopedVolumeProvider.ServiceFabricVolumeDisk, CreateNewVolumeDescriptionVolumeDisk },
        };

        private static VolumeDescription CreateNewVolumeDescriptionAzureFile(string defaultVolumeName)
        {
            return new VolumeDescriptionAzureFile(defaultVolumeName);
        }

        private static VolumeDescription CreateNewVolumeDescriptionVolumeDisk(string defaultVolumeName)
        {
            return new VolumeDescriptionVolumeDisk(defaultVolumeName);
        }

        public override bool CanConvert(Type objectType)
        {
            return typeof(VolumeMount).IsAssignableFrom(objectType);
        }

        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            var properties = JObject.Load(reader);
            var isIndependentVolumeMount = typeof(IndependentVolumeMount).IsAssignableFrom(objectType);
            var isApplicationScopedVolume = typeof(ApplicationScopedVolume).IsAssignableFrom(objectType);

            if (!(isIndependentVolumeMount || isApplicationScopedVolume))
            {
                throw new JsonSerializationException(
                    String.Format(
                        "Deserialization failure. Expected object of type {0} or {1}, but found object of type {2} instead.",
                        typeof(IndependentVolumeMount),
                        typeof(ApplicationScopedVolume),
                        objectType));
            }

            string volumeDescriptionPropertyName = isIndependentVolumeMount ? "volumeDescription" : "creationParameters";
            var name = this.GetPropertyValueSafe<string>(properties, "name");
            var readOnly = this.GetPropertyValueSafe<bool>(properties, "readOnly");
            var destinationPath = this.GetPropertyValueSafe<string>(properties, "destinationPath");

            VolumeDescription volumeDescription = null;
            JObject volumeDescriptionJObject = null;
            JToken volumeDescriptionJToken = null;
            if (properties.TryGetValue(volumeDescriptionPropertyName, out volumeDescriptionJToken))
            {
                volumeDescriptionJObject = volumeDescriptionJToken as JObject;
            }
            if (volumeDescriptionJObject != null)
            {
                volumeDescription = this.GetVolumeDescription(
                    serializer,
                    volumeDescriptionJObject,
                    isIndependentVolumeMount ?
                      IndependentVolumeDescriptionCreator :
                      ApplicationScopedVolumeDescriptionCreator,
                    isIndependentVolumeMount ? null : name);
            }

            return isIndependentVolumeMount ?
                (VolumeMount) (new IndependentVolumeMount(
                    name,
                    destinationPath,
                    readOnly,
                    volumeDescription)) :
                (VolumeMount) (new ApplicationScopedVolume(
                    name,
                    destinationPath,
                    readOnly,
                    volumeDescription));
        }

        private VolumeDescription GetVolumeDescription(
            JsonSerializer serializer,
            JObject jObject,
            Dictionary<int, Func<string, VolumeDescription>> volumeDescriptionCreator,
            string defaultVolumeName)
        {
            JToken jToken;
            if (!jObject.TryGetValue("kind", out jToken))
            {
                return null;
            }
            var provider = jToken.Value<int>();

            Func<string, VolumeDescription> creator;
            if (!volumeDescriptionCreator.TryGetValue(provider, out creator))
            {
                return null;
            }

            VolumeDescription volumeDescription = creator(defaultVolumeName);
            serializer.Populate(jObject.CreateReader(), volumeDescription);
            return volumeDescription;
        }

        private T GetPropertyValueSafe<T>(JObject properties, string propertyName)
        {
            JToken property;
            if (properties.TryGetValue(propertyName, out property))
            {
                return property.Value<T>();
            }
            return default(T);
        }

        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            throw new NotImplementedException();
        }
    };
}
