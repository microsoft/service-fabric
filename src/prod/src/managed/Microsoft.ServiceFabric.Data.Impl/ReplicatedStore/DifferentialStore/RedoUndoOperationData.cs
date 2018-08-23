// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.IO;
    using System.Fabric.Data.Common;
    using System.Text;

    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Contains redo or undo information for store modifications.
    /// </summary>
    internal class RedoUndoOperationData : OperationData
    {
        #region Instance Members

        /// <summary>
        /// Serialized size of the RedoUndoOperationData's metadata.
        /// </summary>
        private const int SerializedMetadataSize =
            sizeof(int) + // value segment count
            sizeof(int); // new value segment count

        /// <summary>
        /// Value bytes. May be null.
        /// </summary>
        private ArraySegment<byte>[] valueBytes;

        /// <summary>
        /// New value bytes. May be null.
        /// </summary>
        private ArraySegment<byte>[] newValueBytes;

        #endregion

        /// <summary>
        /// Proper constructor.
        /// </summary>
        /// <param name="newValueBytes">New value bytes for the operation.</param>
        /// <param name="valueBytes">Value bytes for the operation.</param>
        /// <param name="traceType">trace Id</param>
        public RedoUndoOperationData(ArraySegment<byte>[] valueBytes, ArraySegment<byte>[] newValueBytes, string traceType)
        {
            this.valueBytes = valueBytes;
            this.newValueBytes = newValueBytes;

            this.Intialize(traceType);
        }

        #region Instance Properties

        /// <summary>
        /// Value that was modified.
        /// </summary>
        public ArraySegment<byte>[] ValueBytes
        {
            get { return this.valueBytes; }
        }

        /// <summary>
        /// New value for modification.
        /// </summary>
        public ArraySegment<byte>[] NewValueBytes
        {
            get { return this.newValueBytes; }
        }

        #endregion

        /// <summary>
        /// Deserializes a set of bytes into a redo/undo operation.
        /// </summary>
        /// <param name="operationData">Bytes to deserialize.</param>
        /// <param name="traceType">trace Id</param>
        /// <returns></returns>
        public static RedoUndoOperationData FromBytes(OperationData operationData, string traceType)
        {
            // Check arguments.
            if (operationData == null)
            {
                throw new ArgumentNullException(SR.Error_OperationData);
            }

            // OperationData must contain at least the metadata and key (values may be null).
            if (operationData.Count == 0)
            {
                throw new ArgumentException(SR.Error_OperationData);
            }

            // Array segment zero contains the metadata
            var metadataBytes = operationData[0];
            using (var stream = new MemoryStream(metadataBytes.Array, metadataBytes.Offset, metadataBytes.Count))
            using (var reader = new BinaryReader(stream, Encoding.Unicode))
            {
                // Read how many value and new value segments there are
                var valueSegmentCount = reader.ReadInt32();
                var newValueSegmentCount = reader.ReadInt32();

                // Check the actual number of segments matches the expected number (metadata, key, and count for the value segments)
                var expectedSegmentCount = 1 + valueSegmentCount + newValueSegmentCount;
                if (operationData.Count != expectedSegmentCount)
                {
                    throw new InvalidDataException(
                        string.Format(
                            System.Globalization.CultureInfo.CurrentCulture,
                            SR.Error_MetadataOperationData_ExpectedSegments,
                            operationData.Count,
                            expectedSegmentCount));
                }

                // Read values.
                var startSegmentIndex = 1;
                var valueBytes = GetValueBytes(operationData, startSegmentIndex, valueSegmentCount);

                startSegmentIndex += valueSegmentCount;
                var newValueBytes = GetValueBytes(operationData, startSegmentIndex, newValueSegmentCount);

                return new RedoUndoOperationData(valueBytes, newValueBytes, traceType);
            }
        }

        #region IOperationData Members

        private void Intialize(string traceId)
        {
            var enumerator = this.GetArraySegmentEnumerator(traceId);
            while (enumerator.MoveNext())
            {
                this.Add(enumerator.Current);
            }
        }

        /// <summary>
        /// Enumerates the buffers for this operation.
        /// </summary>
        /// <returns></returns>
        private IEnumerator<ArraySegment<byte>> GetArraySegmentEnumerator(string traceId)
        {
            using (var stream = new MemoryStream(SerializedMetadataSize))
            using (var writer = new BinaryWriter(stream, Encoding.Unicode))
            {
                // Write how many value and new value segments there are.
                var valueBytesCount = this.valueBytes != null ? this.valueBytes.Length : 0;
                writer.Write(valueBytesCount);

                var newValueBytesCount = this.newValueBytes != null ? this.newValueBytes.Length : 0;
                writer.Write(newValueBytesCount);

                // Verify we are writing the same bytes as the initially created memory stream buffer
                Diagnostics.Assert(
                    (int) stream.Position == SerializedMetadataSize,
                    traceId,
                    "RedoUndoOperationData.SerializedMetadataSize ({0}) needs to be updated.  Metadata size has changed to {1} bytes.",
                    SerializedMetadataSize, (int) stream.Position);

                // Return the metadata array segment
                yield return new ArraySegment<byte>(stream.GetBuffer(), 0, (int) stream.Position);
            }

            // Value bytes
            if (this.valueBytes != null)
            {
                foreach (var valueByteSegment in this.valueBytes)
                {
                    Diagnostics.Assert(valueByteSegment.Array != null, traceId, "An ArraySegment for serialized value is null.");
                    yield return valueByteSegment;
                }
            }

            // New Value bytes
            if (this.newValueBytes != null)
            {
                foreach (var newValueByteSegment in this.newValueBytes)
                {
                    Diagnostics.Assert(newValueByteSegment.Array != null, traceId, "An ArraySegment for serialized value is null.");
                    yield return newValueByteSegment;
                }
            }
        }

        #endregion

        /// <summary>
        /// Get the list of ArraySegments that represent the value's bytes.
        /// </summary>
        /// <param name="operationData">OperationData with the full list of byte array segments.</param>
        /// <param name="startSegmentIndex">Starting index within the segments for this value's bytes.</param>
        /// <param name="segmentCount">Number of segments for this value's bytes.</param>
        /// <returns></returns>
        private static ArraySegment<byte>[] GetValueBytes(OperationData operationData, int startSegmentIndex, int segmentCount)
        {
            ArraySegment<byte>[] valueBytes = null;

            if (segmentCount > 0)
            {
                valueBytes = new ArraySegment<byte>[segmentCount];
                for (var i = 0; i < segmentCount; i++)
                {
                    valueBytes[i] = operationData[startSegmentIndex + i];
                }
            }

            return valueBytes;
        }
    }
}