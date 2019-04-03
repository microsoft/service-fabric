// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using System.Collections.Generic;
    using System.Fabric.Query;
    using System.Linq;

    /// <summary>
    /// Custom converter for classes that derive from generic IList and have more public fields.
    /// TList needs to have a public or non-public constructor with zero parameters.
    /// </summary>
    /// <typeparam name="TList"></typeparam>
    /// <typeparam name="TElm"></typeparam>
    internal class CustomPagedIListJsonConverter<TList, TElm>
        : CustomCreationJsonConverterBase<TList> where TList : PagedList<TElm>, new()
    {
        private const string ItemsName = "Items";
        private const string ContinuationTokenName = "ContinuationToken";

        public CustomPagedIListJsonConverter()
            : base(true, true) // Can-read = true, Can-write = true
        {
        }

        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            if (reader.TokenType == JsonToken.Null)
            {
                return null;
            }

            if (reader.TokenType == JsonToken.StartArray)
            {
                JArray jarray = JArray.Load(reader);
                TList target = this.Create();
                return this.PopulateInstanceUsingJArray(jarray, target, serializer);
            }
            else if (reader.TokenType == JsonToken.StartObject)
            {
                // Object contains list and other objects
                JObject jObject = JObject.Load(reader);
                TList target = this.Create();
                return this.PopulateInstanceUsingJObject(jObject, target, serializer);
            }
            else
            {
                throw new FabricSerializationException("CustomPagedIListJsonConverter(): ReadJson(): Unexpected token type.");
            }
        }

        protected virtual TList PopulateInstanceUsingJArray(JArray jarray, TList targetObject, JsonSerializer serializer)
        {
            foreach (var jitem in jarray)
            {
                targetObject.Add(jitem.ToObject<TElm>(serializer));
            }

            return targetObject;
        }

        protected override TList PopulateInstanceUsingJObject(JObject jObject, TList targetObject, JsonSerializer serializer)
        {
            JToken jContinuationTokenToken;
            if (jObject.TryGetValue(ContinuationTokenName, out jContinuationTokenToken))
            {
                targetObject.ContinuationToken = jContinuationTokenToken.ToObject<string>();
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
            if(value == null)
            {
                tlist = this.Create();
            }

            JObject resultJObj = new JObject();
            resultJObj.Add(ItemsName, JArray.FromObject(tlist.ToArray(), serializer));

            if (tlist.ContinuationToken != null)
            {
                resultJObj.Add(ContinuationTokenName, JToken.FromObject(tlist.ContinuationToken));
            }

            return resultJObj;
        }

        protected override TList Create(Type objectType, JObject jObject)
        {
            // This method call is not expected.
            throw new FabricSerializationException("CustomPagedListJsonConverter(): Create(): Unexpected method call.");
        }

        private TList Create()
        {
            // Activator.CreateInstance with nonPublic is not CoreCLR compliant.
            //return (TList)Activator.CreateInstance(typeof(TList), true);
            return new TList();
        }
    }
}