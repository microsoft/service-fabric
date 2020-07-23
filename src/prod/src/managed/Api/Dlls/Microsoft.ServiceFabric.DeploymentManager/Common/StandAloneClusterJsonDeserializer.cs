// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    public class StandAloneClusterJsonDeserializer : JsonConverter
    {
        private static Dictionary<Type, Func<Type, object>> supportedTypesForCustomDeserealiser = new Dictionary<Type, Func<Type, object>>()
        {
            { typeof(IUserConfig), new Func<Type, object>(CreateInstanceWithNoParameters) },
            { typeof(IAdminConfig), new Func<Type, object>(CreateAdminConfigWithNullParameters) },
            { typeof(IClusterTopology), new Func<Type, object>(CreateInstanceWithNoParameters) },
            { typeof(ITraceLogger), new Func<Type, object>(CreateInstanceWithNoParameters) },
            { typeof(IClusterManifestBuilderActivator), new Func<Type, object>(CreateInstanceWithNoParameters) },
            { typeof(IUpgradeStateActivator), new Func<Type, object>(CreateInstanceWithNoParameters) },
            { typeof(ClusterUpgradeStateBase), new Func<Type, object>(CreateInstanceWithNoParameters) },
            { typeof(MultiphaseClusterUpgradeState), new Func<Type, object>(CreateInstanceWithNoParameters) },
            { typeof(ClusterManifestTypeInfrastructure), new Func<Type, object>(CreateClusterManifestWindowsServerType) }
        };

        public static Dictionary<Type, Func<Type, object>> SupportedTypesForCustomDeserealiser
        {
            get { return supportedTypesForCustomDeserealiser; }
        }

        public override bool CanWrite
        {
            get { return false; }
        }

        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            throw new NotImplementedException("Unnecessary because CanWrite is false. The type will skip the converter.");
        }

        public override bool CanConvert(Type objectType)
        {
            return StandAloneClusterJsonDeserializer.SupportedTypesForCustomDeserealiser.ContainsKey(objectType);
        }

        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            JObject obj = JObject.Load(reader);
            object target = StandAloneClusterJsonDeserializer.CreateFromTypeInformation(obj, objectType);       
            serializer.Populate(obj.CreateReader(), target);
            return target;
        }

        private static object CreateFromTypeInformation(JObject obj, Type objectType)
        {
            JToken token = obj["$type"];

            /* Explicit check for ClusterManifestTypeInfrastructure because $type is not serialised in JSON. $type is defined in JSON for its class member 'Item'. ClusterManifestTypeInfrastructure
             *  needs to be initilaised before 'Item' can be read.
             */
            if (objectType == typeof(ClusterManifestTypeInfrastructure))
            {
                return StandAloneClusterJsonDeserializer.SupportedTypesForCustomDeserealiser[objectType](objectType);
            }

            ReleaseAssert.AssertIf(token == null, "StandAloneClusterJsonDeserializer: target cannot be created from $type info because $type is not specified in JSON and cannot be understood by the custom deserealiser.");
            string type = token.ToString().Split(',')[0];
            return StandAloneClusterJsonDeserializer.SupportedTypesForCustomDeserealiser[objectType](Type.GetType(type));
        }

        private static object CreateInstanceWithNoParameters(Type type)
        {
            return Activator.CreateInstance(type);
        }

        private static object CreateAdminConfigWithNullParameters(Type type)
        {
            return new StandaloneAdminConfig(null, null, null, false);
        }

        private static object CreateClusterManifestWindowsServerType(Type type)
        {
            return new ClusterManifestTypeInfrastructure() { Item = new ClusterManifestTypeInfrastructureWindowsServer() };
        }
    }
}