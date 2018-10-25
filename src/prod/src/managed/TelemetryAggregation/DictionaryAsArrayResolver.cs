// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryAggregation
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
#if DotNetCoreClrLinux
    using System.Reflection;
#endif
    using Newtonsoft.Json.Serialization;

    // this class enables serializing and deserializing a dictionary with keys which are not strings
    public class DictionaryAsArrayResolver : DefaultContractResolver
    {
        protected override JsonContract CreateContract(Type objectType)
        {
#if !DotNetCoreClrLinux
            if (objectType.GetInterfaces().Any(i => i == typeof(IDictionary<,>) ||
                (i.IsGenericType &&
                 i.GetGenericTypeDefinition() == typeof(IDictionary<,>))))
#else
            if (objectType.GetInterfaces().Any(i => i == typeof(IDictionary<,>) ||
                (i.GetTypeInfo().IsGenericType &&
                 i.GetTypeInfo().GetGenericTypeDefinition() == typeof(IDictionary<,>))))
#endif
                {
                return this.CreateArrayContract(objectType);
            }

            return base.CreateContract(objectType);
        }
    }
}