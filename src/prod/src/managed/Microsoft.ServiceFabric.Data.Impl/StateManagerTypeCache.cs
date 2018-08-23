// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Reflection;
    using Microsoft.ServiceFabric.Data.Collections;
    using Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue;

    /// <summary>
    /// Cache the concrete wrapper type corresponding to a given IReliableCollection interface in order to avoid 
    /// reflection in subsequent calls to e.g. GetOrAddAsync
    /// </summary>
    internal class StateManagerTypeCache
    {
        /// <summary>
        /// Type template map for yielding default interface implementations
        /// </summary>
        private readonly Dictionary<Type, Type> defaultTypeTemplateMap = new Dictionary<Type, Type>
        {
            {typeof(IReliableDictionary<,>), typeof(DistributedDictionary<,>)},
            {typeof(IReliableDictionary2<,>), typeof(DistributedDictionary<,>)},
            {typeof(IReliableQueue<>), typeof(DistributedQueue<>)},
            {typeof(IReliableConcurrentQueue<>), typeof(ReliableConcurrentQueue<>)}
        };

        /// <summary>
        /// Type cache to avoid subsequent calls to MakeGenericType
        /// </summary>
        private readonly ConcurrentDictionary<Type, Type> typeCache = new ConcurrentDictionary<Type, Type>();

        /// <summary>
        /// Get the wrapper type which corresponds to a given IReliableState interface
        /// </summary>
        /// <param name="type">The desired interface type</param>
        /// <returns>The wrapper type to instantiate</returns>
        public Type GetConcreteType(Type type)
        {
            Type wrapperType = null;
            if (!this.typeCache.TryGetValue(type, out wrapperType))
            {
                if (!type.GetTypeInfo().IsGenericType)
                {
                    throw new ArgumentException(SR.Error_GenericType_Expected);
                }

                var interfaceTypeTemplate = type.GetGenericTypeDefinition();
                Type wrapperTypeTemplate = null;
                if (!this.defaultTypeTemplateMap.TryGetValue(interfaceTypeTemplate, out wrapperTypeTemplate))
                {
                    throw new ArgumentException(SR.Error_UnreliableType);
                }

                wrapperType = wrapperTypeTemplate.MakeGenericType(type.GetGenericArguments());
                this.typeCache.TryAdd(type, wrapperType);
            }

            return wrapperType;
        }
    }
}