// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Globalization;
    using System.IO;
    using System.Fabric.Common.Tracing;
    using Data;

    /// <summary>
    /// Operation data with state provider id.
    /// </summary>
    /// <dataformat>
    ///                     UserOperationData   Context
    /// OperationData:      [0]...[n]           [vX]...[v2][Header]
    /// Header:             [Version|StateProviderID|IsStateProviderDataNull]
    /// </dataformat>
    /// <devnote>
    /// Native and managed use different format.
    /// </devnote>
    internal class NamedOperationData : OperationData
    {
        /// <summary>
        /// Gets named operation data from operation data.
        /// </summary>
        public static NamedOperationData Deserialize(
            OperationData operationData,
            string traceType,
            int currentVersion)
        {
            try
            {
                List<ArraySegment<byte>> copiedBytes = new List<ArraySegment<byte>>();

                int deserializedVersion = BitConverter.ToInt32(operationData[0].Array, 0);
                if (deserializedVersion != currentVersion)
                {
                    string errorMessage = string.Format(
                        CultureInfo.InvariantCulture,
                        SR.Error_Unsupported_Version_Deserialization,
                        deserializedVersion,
                        currentVersion);
                    FabricEvents.Events.NamedOperationDataDeserializeError(traceType, errorMessage);
                    throw new NotSupportedException(errorMessage);
                }

                // state provider id
                long stateProviderId = BitConverter.ToInt64(operationData[1].Array, 0);

                for (int i = 2; i < operationData.Count; i++)
                {
                    copiedBytes.Add(operationData[i]);
                }

                return new NamedOperationData(new OperationData(copiedBytes), stateProviderId, traceType);
            }
            catch (Exception e)
            {
                StateManagerStructuredEvents.TraceException(
                    traceType,
                    "NamedOperationData.Deserialize",
                    string.Empty,
                    e);
                throw;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="NamedOperationContext"/> class.
        /// </summary>
        public NamedOperationData(
            OperationData operationData,
            long stateProviderId,
            string traceType)
        {
            Utility.Assert(
                stateProviderId != DynamicStateManager.EmptyStateProviderId,
                "{0}: NamedOperationData: Id cannot be empty in named operation data",
                traceType);

            this.OperationData = operationData;
            this.StateProviderId = stateProviderId;
            this.TraceType = traceType;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="NamedOperationContext"/> class with serialize.
        /// </summary>
        public NamedOperationData(
            OperationData operationData, 
            long stateProviderId,
            int version,
            string traceType)
        {
            // #9894884: Native changed Version to metadataCount, keep the old format for now.
            Utility.Assert(
                stateProviderId != DynamicStateManager.EmptyStateProviderId,
                "{0}: NamedOperationData: Id cannot be empty in named operation data",
                traceType);

            this.OperationData = operationData;
            this.StateProviderId = stateProviderId;
            this.Version = version;
            this.TraceType = traceType;

            this.Serialize();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="NamedOperationContext"/> class by deserialization from OperationData.
        /// </summary>
        public NamedOperationData(
            OperationData operationData,
            int version,
            string traceType)
        {
            // State Manager always at least creates OperationData of one buffer to keeps dispatching information.
            Utility.Assert(
                operationData != null,
                "{0}: NamedOperationData: Null named operation data during create",
                traceType);

            Utility.Assert(
                operationData.Count > 0,
                "{0}: NamedOperationData: Named operation data should have atleast one buffer during create, Count: {1}.",
                traceType, operationData.Count);

            this.DeserializeContext(operationData, version, traceType);
        }

        /// <summary>
        /// Gets or sets operation data.
        /// </summary>
        internal OperationData OperationData { get; private set; }

        /// <summary>
        /// Gets or sets state provider id.
        /// </summary>
        internal long StateProviderId { get; private set; }

        /// <summary>
        /// Gets or sets Version.
        /// </summary>
        internal int Version { get; private set; }

        /// <summary>
        /// Gets or sets TraceType.
        /// </summary>
        internal string TraceType { get; private set; }

        /// <summary>
        /// Serialize the OperationData.
        /// </summary>
        internal void Serialize()
        {
            if (this.OperationData != null)
            {
                foreach (var bytes in this.OperationData)
                {
                    this.Add(bytes);
                }
            }

            OperationData contextSPtr = this.CreateContext();

            Utility.Assert(
                contextSPtr.Count > 0,
                "{0}: Serialize: Named operation context should have atleast one buffer during serialize, Count: {1}.",
                this.TraceType, contextSPtr.Count);

            foreach (var bytes in contextSPtr)
            {
                this.Add(bytes);
            }
        }

        /// <summary>
        /// Create the header for the OperationData
        /// </summary>
        internal OperationData CreateContext()
        {
            OperationData context;

            const int streamSize = sizeof(int) // version
                             + sizeof(long) // stateProviderId
                             + sizeof(bool); // boolean indicating if the state provider data is null.

            using (InMemoryBinaryWriter writer = new InMemoryBinaryWriter(new MemoryStream(streamSize)))
            {
                writer.Write(this.Version);
                writer.Write(this.StateProviderId);
                writer.Write(this.OperationData == null);

                Utility.Assert(
                    writer.BaseStream.Position == streamSize,
                    "{0}: CreateContext: Size is not correct, Position: {1}.",
                    this.TraceType, writer.BaseStream.Position);

                context = new OperationData(new ArraySegment<byte>(
                    writer.BaseStream.GetBuffer(),
                    0,
                    checked((int)writer.BaseStream.Position)));
            }

            return context;
        }

        /// <summary>
        /// Deserialize the header for the OperationData
        /// </summary>
        internal void DeserializeContext(
            OperationData operationData,
            int version,
            string traceType)
        {
            Utility.Assert(
                operationData != null,
                "{0}: DeserializeContext: Null named operation data during create",
                this.TraceType);

            Utility.Assert(
                operationData.Count > 0,
                "{0}:DeserializeContext: Named operation data should have atleast one buffer during create",
                this.TraceType);

            var headerBufferSPtr = operationData[operationData.Count - 1];
            bool isStateProviderDataNull;

            using (InMemoryBinaryReader reader = new InMemoryBinaryReader(new MemoryStream(
                headerBufferSPtr.Array, 
                headerBufferSPtr.Offset,
                headerBufferSPtr.Count)))
            {
                this.Version = reader.ReadInt32();

                if (this.Version != version)
                {
                    FabricEvents.Events.OnApplyVersionError(traceType, this.Version, version);
                    throw new NotSupportedException(
                        string.Format(
                            CultureInfo.InvariantCulture,
                            "Unsupported version {0} on deserialization, current version is {1}",
                            this.Version,
                            version));
                }

                this.StateProviderId = reader.ReadInt64();
                isStateProviderDataNull = reader.ReadBoolean();
            }

            // Remove state manager metadata from data.
            operationData.RemoveAt(operationData.Count - 1);

            this.OperationData = isStateProviderDataNull ? null : operationData;
        }
    }
}
