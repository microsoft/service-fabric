// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Concurrent;
    using Microsoft.ServiceFabric.Data;

    internal class SerializationManager : ISerializationManager
    {
        /// <summary>
        /// Serializer dictionary.
        /// </summary>
        private readonly ConcurrentDictionary<NamedType, object> serializerDictionary =
            new ConcurrentDictionary<NamedType, object>(1, 1);

        public IStateSerializer<T> GetStateSerializer<T>(Uri name)
        {
            var namedType = new NamedType(name, typeof(T));

            object result;

            var hasValue = this.serializerDictionary.TryGetValue(namedType, out result);

            if (true == hasValue)
            {
                return (IStateSerializer<T>) result;
            }

            namedType = new NamedType(null, typeof(T));

            hasValue = this.serializerDictionary.TryGetValue(namedType, out result);

            if (true == hasValue)
            {
                return (IStateSerializer<T>) result;
            }

            if (BuiltInTypeSerializer<T>.IsSupportedType())
            {
                return BuiltInTypeSerializerBuilder.MakeBuiltInTypeSerializer<T>();
            }

            return new DataContractStateSerializer<T>();
        }

        public void RemoveStateSerializer<T>()
        {
            this.RemoveStateSerializer<T>(null);
        }

        public void RemoveStateSerializer<T>(Uri name)
        {
            var namedType = new NamedType(name, typeof(T));

            object removedSerializer;

            var hasValue = this.serializerDictionary.TryRemove(namedType, out removedSerializer);

            if (false == hasValue)
            {
                throw new ArgumentException();
            }
        }

        public bool TryAddStateSerializer<T>(IStateSerializer<T> stateSerializer)
        {
            return this.TryAddStateSerializer<T>(null, stateSerializer);
        }

        public bool TryAddStateSerializer<T>(Uri name, IStateSerializer<T> stateSerializer)
        {
            // TODO: Add Support for prefix matching.
            var namedType = new NamedType(name, typeof(T));

            return this.serializerDictionary.TryAdd(namedType, stateSerializer);
        }
    }
}