// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    internal class NodeIdJsonConverter : CustomCreationJsonConverterBase<NodeId>
    {
        public NodeIdJsonConverter()
            : base(true, false)
        {
        }

        protected override NodeId PopulateInstanceUsingJObject(JObject jObject, NodeId targetObject, JsonSerializer serializer)
        {
            string nodeIdStr = (string)jObject["Id"];
            bool success = NodeId.TryParse(nodeIdStr, out targetObject);
            return targetObject;
        }

        protected override NodeId Create(Type objectType, Newtonsoft.Json.Linq.JObject jObject)
        {
            return new NodeId(0, 0);
        }

        protected override JContainer GetJContainerForInstance(object value, JsonSerializer serializer)
        {
            // Call not expected.
            throw new NotImplementedException();
        }
    }
}