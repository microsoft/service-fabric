// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using System.Collections.Generic;
    using System.Linq;

    /// <summary>
    /// TList needs to have a public or non-public constructor with zero parameters.
    /// </summary>
    /// <typeparam name="TList"></typeparam>
    /// <typeparam name="TElm"></typeparam>
    internal class CustomIListJsonConverter<TList, TElm> : CustomCreationJsonConverterBase<TList> where TList : IList<TElm>
        
    {
        public CustomIListJsonConverter()
            : base(true, true) // Can-read = true, Can-write = true
        {
        }

        private TList Create(Type objectType, JArray jObject)
        {
            return (TList)Activator.CreateInstance(typeof(TList), true);
        }

        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            if (reader.TokenType == JsonToken.Null)
            {
                return null;
            }

            JArray jarray = JArray.Load(reader);
            TList target = Create(objectType, jarray);
            return PopulateInstanceUsingJArray(jarray, target, serializer);
        }

        // Derived class can override it to do any customization.
        protected virtual TList PopulateInstanceUsingJArray(JArray jarray, TList targetObject, JsonSerializer serializer)
        {
            foreach (var jitem in jarray)
            {
                targetObject.Add(jitem.ToObject<TElm>(serializer));
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
                tlist = (TList)Activator.CreateInstance(typeof(TList), true);
            }

            return JArray.FromObject(tlist.ToArray(), serializer);
        }

        protected override TList Create(Type objectType, JObject jObject)
        {
            // This method call is not expected.
            throw new FabricSerializationException("CustomIListJsonConverter(): Create(): Unexpected method call.");
        }
    }
}