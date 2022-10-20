// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Serialization;
    using System.Fabric.Chaos.DataStructures;
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Description;
    using System.Fabric.Query;
    using System.Fabric.Testability.Client.Structures;
    using System.Reflection;
    using System.Diagnostics;
    using System.Linq;
    using System.Fabric.Health;

    internal class CustomContractResolver : DefaultContractResolver
    {
        public static readonly CustomContractResolver Instance = new CustomContractResolver();
        private Dictionary<Type, JsonConverter> listTypes = new Dictionary<Type, JsonConverter>();

        public CustomContractResolver()
        {
            /// Adding converters for types
            this.listTypes.Add(typeof(ApplicationTypeList), new CustomIListJsonConverter<ApplicationTypeList, ApplicationType>());
            this.listTypes.Add(typeof(ApplicationTypePagedList), new CustomPagedIListJsonConverter<ApplicationTypePagedList, ApplicationType>());
            this.listTypes.Add(typeof(ApplicationParameterList), new CustomIListJsonConverter<ApplicationParameterList, ApplicationParameter>());
            this.listTypes.Add(typeof(ApplicationList), new CustomPagedIListJsonConverter<ApplicationList, Application>());
            this.listTypes.Add(typeof(ServiceList), new CustomPagedIListJsonConverter<ServiceList, Service>());
            this.listTypes.Add(typeof(ServiceTypeList), new CustomIListJsonConverter<ServiceTypeList, ServiceType>());
            this.listTypes.Add(typeof(ServiceReplicaList), new CustomPagedIListJsonConverter<ServiceReplicaList, Replica>());
            this.listTypes.Add(typeof(ServicePartitionList), new CustomPagedIListJsonConverter<ServicePartitionList, Partition>());
            this.listTypes.Add(typeof(DeployedApplicationList), new CustomIListJsonConverter<DeployedApplicationList, DeployedApplication>());
            this.listTypes.Add(typeof(DeployedApplicationPagedList), new CustomPagedIListJsonConverter<DeployedApplicationPagedList, DeployedApplication>());
            this.listTypes.Add(typeof(DeployedCodePackageList), new CustomIListJsonConverter<DeployedCodePackageList, DeployedCodePackage>());
            this.listTypes.Add(typeof(DeployedServiceTypeList), new CustomIListJsonConverter<DeployedServiceTypeList, DeployedServiceType>());
            this.listTypes.Add(typeof(DeployedServicePackageList), new CustomIListJsonConverter<DeployedServicePackageList, DeployedServicePackage>());
            this.listTypes.Add(typeof(DeployedServiceReplicaList), new CustomIListJsonConverter<DeployedServiceReplicaList, DeployedServiceReplica>());
            this.listTypes.Add(typeof(NodeList), new CustomPagedIListJsonConverter<NodeList, Node>());
            this.listTypes.Add(typeof(ProvisionedFabricCodeVersionList), new CustomIListJsonConverter<ProvisionedFabricCodeVersionList, ProvisionedFabricCodeVersion>());
            this.listTypes.Add(typeof(ProvisionedFabricConfigVersionList), new CustomIListJsonConverter<ProvisionedFabricConfigVersionList, ProvisionedFabricConfigVersion>());
            this.listTypes.Add(typeof(ServiceGroupMemberTypeList), new CustomIListJsonConverter<ServiceGroupMemberTypeList, ServiceGroupMemberType>());
            this.listTypes.Add(typeof(ServiceGroupMemberMemberList), new CustomIListJsonConverter<ServiceGroupMemberMemberList, ServiceGroupMemberMember>());
            this.listTypes.Add(typeof(ServiceGroupMemberList), new CustomIListJsonConverter<ServiceGroupMemberList, ServiceGroupMember>());
            this.listTypes.Add(typeof(NodeHealthStateChunkList), new CustomHealthStateChunkListJsonConverter<NodeHealthStateChunkList, NodeHealthStateChunk>());
            this.listTypes.Add(typeof(ApplicationHealthStateChunkList), new CustomHealthStateChunkListJsonConverter<ApplicationHealthStateChunkList, ApplicationHealthStateChunk>());
            this.listTypes.Add(typeof(ServiceHealthStateChunkList), new CustomHealthStateChunkListJsonConverter<ServiceHealthStateChunkList, ServiceHealthStateChunk>());
            this.listTypes.Add(typeof(PartitionHealthStateChunkList), new CustomHealthStateChunkListJsonConverter<PartitionHealthStateChunkList, PartitionHealthStateChunk>());
            this.listTypes.Add(typeof(ReplicaHealthStateChunkList), new CustomHealthStateChunkListJsonConverter<ReplicaHealthStateChunkList, ReplicaHealthStateChunk>());
            this.listTypes.Add(typeof(DeployedApplicationHealthStateChunkList), new CustomHealthStateChunkListJsonConverter<DeployedApplicationHealthStateChunkList, DeployedApplicationHealthStateChunk>());
            this.listTypes.Add(typeof(DeployedServicePackageHealthStateChunkList), new CustomHealthStateChunkListJsonConverter<DeployedServicePackageHealthStateChunkList, DeployedServicePackageHealthStateChunk>());

            this.listTypes.Add(typeof(ApplicationResourceList), new CustomPagedIListJsonConverter<ApplicationResourceList, ApplicationResource>());
            this.listTypes.Add(typeof(ServiceResourceList), new CustomPagedIListJsonConverter<ServiceResourceList, ServiceResource>());
        }

        protected override List<MemberInfo> GetSerializableMembers(Type objectType)
        {
            BindingFlags searchFlags = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance;

            /// Add properties only. Fields are ignored in serialization.
            List<PropertyInfo> propertyInfos = new List<PropertyInfo>(objectType.GetProperties(searchFlags));
            List<MemberInfo> propertyInfosResult = new List<MemberInfo>();

            foreach (var propInfo in propertyInfos)
            {
                propertyInfosResult.Add(GetPropertyInfoFromDeclaringType(propInfo, objectType));
            }

            return propertyInfosResult;
        }

        private static PropertyInfo GetPropertyInfoFromDeclaringType(PropertyInfo propertyInfo, Type objectType)
        {
            if (propertyInfo.DeclaringType == objectType)
            {
                return propertyInfo;
            }

            /// If declaring type is different (base class) then get PropertyInfo from that type.
            /// That would initialize setters and getters correctly.
            Type declaringType = propertyInfo.DeclaringType;
            const BindingFlags bindingAttr = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic;
            Type[] paramTypes = propertyInfo.GetIndexParameters().Select(p => p.ParameterType).ToArray();
            return declaringType.GetProperty(propertyInfo.Name, bindingAttr, null, propertyInfo.PropertyType, paramTypes, null);
        }

        protected override JsonProperty CreateProperty(MemberInfo member, MemberSerialization memberSerialization)
        {
            JsonProperty property = base.CreateProperty(member, MemberSerialization.Fields);

            if (member.MemberType == MemberTypes.Property)
            {
                var propertyInfo = member as PropertyInfo;

                // if property is not readonly then set "Writable" to true.
                if (propertyInfo.CanRead && propertyInfo.CanWrite && property.Readable && !property.Writable)
                {
                    property.Writable = true;
                }

                // if property does not have a setter (readonly). Then mark it as not used in deserialization.
                if (!propertyInfo.CanWrite)
                {
                    property.Writable = false;
                }
            }
            else if (member.MemberType == MemberTypes.Field && !property.HasMemberAttribute)
            {
                // By default fields are not serialized/deserialized unless they have "JsonProperty" attribute or "DataMember" attrribute.
                property.Ignored = true;
            }

            property = HandleJsonAttribute(member, property);
            return property;
        }

        protected override JsonContract CreateContract(Type objectType)
        {
            JsonContract contract = base.CreateContract(objectType);
            if (this.listTypes.ContainsKey(objectType))
            {
                contract.Converter = listTypes[objectType];
            }

            if (objectType == typeof(byte[]))
            {
                contract = base.CreateArrayContract(objectType);
            }

            if (objectType.GetInterfaces().Any(i => i == typeof(IDictionary<string, ChaosParameters>) ||
            (i.IsGenericType && i.GetGenericTypeDefinition() == typeof(IDictionary<string, ChaosParameters>))))
            {
                return this.CreateArrayContract(objectType);
            }

            return contract;
        }

        private JsonProperty HandleJsonAttribute(MemberInfo member, JsonProperty jsonProperty)
        {
            var attributes = member.GetCustomAttributes(typeof(JsonCustomizationAttribute), inherit: true);
            JsonCustomizationAttribute jsonAttr = null;
            if (attributes != null && attributes.Length > 0)
            {
                jsonAttr = attributes[0] as JsonCustomizationAttribute;
            }

            return HandleJsonAttribute(jsonAttr, jsonProperty);
        }

        private JsonProperty HandleJsonAttribute(JsonCustomizationAttribute jsonAttr, JsonProperty jsonProperty)
        {
            if (jsonAttr == null)
            {
                return jsonProperty;
            }

            if (jsonAttr.IsIgnored)
            {
                jsonProperty.Ignored = true;
                return jsonProperty;
            }

            // It has JsonCustomizationAttribute and not marked for "IsIgnored"
            jsonProperty.HasMemberAttribute = true;

            if (!string.IsNullOrWhiteSpace(jsonAttr.PropertyName))
            {
                jsonProperty.PropertyName = jsonAttr.PropertyName;
            }

            // Change json property's order of appearance
            int? order = jsonAttr.GetAppearanceOrder();
            if (order.HasValue)
            {
                jsonProperty.Order = order;
            }

            if(jsonAttr.ReCreateMember)
            {
                jsonProperty.ObjectCreationHandling = ObjectCreationHandling.Replace;
            }

            // Change json property's default value handling
            if (jsonAttr.IsDefaultValueIgnored)
            {
                jsonProperty.DefaultValueHandling = DefaultValueHandling.Ignore;
            }

            return jsonProperty;
        }


    }
}
