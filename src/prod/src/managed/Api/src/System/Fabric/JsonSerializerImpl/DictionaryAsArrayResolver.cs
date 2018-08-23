// ----------------------------------------------------------------------
//  <copyright file="DictionaryAsArrayResolver.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------
namespace System.Fabric.JsonSerializerImpl
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using Chaos.DataStructures;
    using Newtonsoft.Json.Serialization;

    // serializes dictionary<,> as an array as opposed to the default option of a collection.
    public sealed class DictionaryAsArrayResolver : DefaultContractResolver
    {
        protected override JsonContract CreateContract(Type objectType)
        {
            if (objectType.GetInterfaces().Any(i => i == typeof(IDictionary<string, ChaosParameters>) || 
            (i.IsGenericType && i.GetGenericTypeDefinition() == typeof(IDictionary<string, ChaosParameters>))))
            {
                return this.CreateArrayContract(objectType);
            }

            return base.CreateContract(objectType);
        }
    }
}