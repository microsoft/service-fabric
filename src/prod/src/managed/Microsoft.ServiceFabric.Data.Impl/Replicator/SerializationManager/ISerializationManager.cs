// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using Microsoft.ServiceFabric.Data;

    internal interface ISerializationManager
    {
        /// <summary>
        /// Get state serializer.
        /// </summary>
        /// <typeparam name="T">Type for the serialization.</typeparam>
        /// <param name="name">Name of the state provider.</param>
        /// <returns></returns>
        IStateSerializer<T> GetStateSerializer<T>(Uri name);

        /// <summary>
        /// Remove state serializer.
        /// </summary>
        /// <typeparam name="T">Type of the serializer.</typeparam>
        void RemoveStateSerializer<T>();

        /// <summary>
        /// Remove state serializer.
        /// </summary>
        /// <typeparam name="T">Type of the serializer.</typeparam>
        /// <param name="name">Name of the state provider.</param>
        void RemoveStateSerializer<T>(Uri name);

        /// <summary>
        /// Add state serializer.
        /// </summary>
        /// <typeparam name="T">Type of the serializer.</typeparam>
        /// <param name="stateSerializer">The state serializer.</param>
        bool TryAddStateSerializer<T>(IStateSerializer<T> stateSerializer);

        /// <summary>
        /// Add state serializer.
        /// </summary>
        /// <typeparam name="T">Type of the serializer.</typeparam>
        /// <param name="name">Name of the state provider.</param>
        /// <param name="stateSerializer">The state serializer.</param>
        bool TryAddStateSerializer<T>(Uri name, IStateSerializer<T> stateSerializer);
    }
}