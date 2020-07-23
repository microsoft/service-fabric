// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using System.Collections.Generic;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Linq;

    /// <summary>
    /// Custom converter for classes that derive from generic IList and have more public fields.
    /// TList needs to have a public or non-public constructor with zero parameters.
    /// </summary>
    /// <typeparam name="TList"></typeparam>
    /// <typeparam name="TElm"></typeparam>
    internal class CustomHealthStateChunkListJsonConverter<TList, TElm>
        : CustomCreationJsonConverterBase<TList> where TList : HealthStateChunkList<TElm>, new()
    {
        private const string ItemsName = "Items";
        private const string TotalCountName = "TotalCount";

        public CustomHealthStateChunkListJsonConverter()
            : base(true, true) // Can-read = true, Can-write = true
        {
        }

        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            if (reader.TokenType == JsonToken.Null)
            {
                return null;
            }

            if (reader.TokenType == JsonToken.StartObject)
            {
                // Object contains list and other objects
                JObject jObject = JObject.Load(reader);
                TList target = this.Create();
                return this.PopulateInstanceUsingJObject(jObject, target, serializer);
            }
            else
            {
                throw new FabricSerializationException("CustomHealthStateChunkListJsonConverter(): ReadJson(): Unexpected token type.");
            }
        }
        
        protected override TList PopulateInstanceUsingJObject(JObject jObject, TList targetObject, JsonSerializer serializer)
        {
            JToken jContinuationTokenToken;
            if (jObject.TryGetValue(TotalCountName, out jContinuationTokenToken))
            {
                targetObject.TotalCount = jContinuationTokenToken.ToObject<long>();
            }

            JToken jItemsToken;
            if (jObject.TryGetValue(ItemsName, out jItemsToken))
            {
                List<TElm> items = new List<TElm>(); 
                serializer.Populate(jItemsToken.CreateReader(), items);

                foreach(var item in items)
                {
                    targetObject.Add(item);
                }
            }

            return targetObject;
        }

        // Creates a JSON array for the given instance
        protected override JContainer GetJContainerForInstance(object value, JsonSerializer serializer)
        {
            var tlist = (TList)value;

            // Return empty JSON array for null value.
            if (value == null)
            {
                tlist = this.Create();
            }

            JObject resultJObj = new JObject();
            resultJObj.Add(ItemsName, JArray.FromObject(tlist.ToArray(), serializer));

            resultJObj.Add(TotalCountName, JToken.FromObject(tlist.TotalCount));
            
            return resultJObj;
        }

        protected override TList Create(Type objectType, JObject jObject)
        {
            // This method call is not expected.
            throw new FabricSerializationException("CustomHealthStateChunkListJsonConverter(): Create(): Unexpected method call.");
        }

        private TList Create()
        {
            return new TList();
        }
    }
}