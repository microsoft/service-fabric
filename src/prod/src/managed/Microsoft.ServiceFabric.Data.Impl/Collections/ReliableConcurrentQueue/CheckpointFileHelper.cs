// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.IO;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Replicator;

    // todo: comment on disk and memory overhead
    internal static class CheckpointFileHelper
    {
        /// <summary>
        /// The currently supported key checkpoint file version.
        /// </summary>
        public const string CheckpointFileExtension = ".sfc";

        internal const string CopyFileName = "copy" + CheckpointFileExtension;

        // Once a chunk has exceeded this size, it will be written
        internal const int ChunkWriteThreshold = 32 * 1024; // 32KB

        internal const int MetadataMarker = 541;

        internal static readonly ArraySegment<byte> MetadataMarkerSegment = new ArraySegment<byte>(BitConverter.GetBytes(MetadataMarker));

        internal const int BeginFrameMarker = 17;

        internal static readonly ArraySegment<byte> BeginChunkMarkerSegment = new ArraySegment<byte>(BitConverter.GetBytes(BeginFrameMarker));

        internal const int EndFramesMarker = 293;

        internal static readonly ArraySegment<byte> EndFramesMarkerSegment = new ArraySegment<byte>(BitConverter.GetBytes(EndFramesMarker));

        internal static string GetCopyFilePath(string directory)
        {
            return Path.Combine(directory, CopyFileName);
        }

        internal static IEnumerable<string> GetCheckpointFiles(string workDirectory)
        {
            return FabricDirectory.GetFiles(workDirectory, "*" + CheckpointFileExtension, SearchOption.TopDirectoryOnly);
        }

        public static bool TryDeleteCopyFile(string directory, string traceType)
        {
            var filePath = GetCopyFilePath(directory);

            if (!FabricFile.Exists(filePath))
            {
                return false;
            }

            FabricFile.Delete(filePath);
            if (FabricFile.Exists(filePath))
            {
                TestableAssertHelper.FailInvalidData(traceType, "CheckpointFileHelper.TryDeleteCopyFile", "!FabricFile.Exists(filePath).  filePath: {0}", filePath);
            }
            return true;
        }

        public static async Task<DataStore<T>> ReadCopyFileAsync<T>(string directory, IStateSerializer<T> valueSerializer, RCQMode queueMode, string traceType)
        {
            var copy = new Checkpoint<T>(directory, CopyFileName, valueSerializer, traceType);
            await copy.ReadAsync().ConfigureAwait(false);
            // do not ReleaseRef on the copy file; it will be deleted later.  Keeping the file around may enable
            // a DR scenario where the primary copies the last copy of its state to a secondary and is then lost,
            // and the new secondary crashes in between deleting the copy file and making the copied state the new
            // checkpoint.
            return new DataStore<T>(copy.ListElements, queueMode, traceType);
        }

        public static async Task<bool> WriteFrameAsync(ArraySegment<byte> frame, Stream stream, string traceType)
        {
            bool complete;

            var marker = BitConverter.ToInt32(frame.Array, frame.Offset);
            if (marker == BeginFrameMarker)
            {
                complete = false;
            }
            else if (marker == EndFramesMarker)
            {
                complete = true;
            }
            else
            {
                var exc = new InvalidDataException(string.Format("CheckpointFileHelper.WriteFrameAsync : marker = {0}", marker));
                FabricEvents.Events.ReliableConcurrentQueue_ExceptionError(traceType, exc.ToString());
                throw exc;
            }

            await stream.WriteAsync(frame.Array, frame.Offset, frame.Count).ConfigureAwait(false);
            return complete;
        }

        private static async Task CopyListElementsFrameAsync(Stream inputStream, Stream outputStream, string traceType)
        {
            var listElementsCountSegment = new ArraySegment<byte>(new byte[sizeof(int)]);
            var listElementsBytesSegment = new ArraySegment<byte>(new byte[sizeof(int)]);
            var crcSegment = new ArraySegment<byte>(new byte[sizeof(ulong)]);

            var listElementsCount = await SerializationHelper.ReadIntAsync(listElementsCountSegment, inputStream).ConfigureAwait(false);
            if (listElementsCount < 0)
            {
                var exc = new InvalidDataException(string.Format("CheckpointFileHelper.CopyListElementsFrameAsync : Unexpected listElementsCount: {0}", listElementsCount));
                FabricEvents.Events.ReliableConcurrentQueue_ExceptionError(traceType, exc.ToString());
                throw exc;
            }

            var listElementsBytes = await SerializationHelper.ReadIntAsync(listElementsBytesSegment, inputStream).ConfigureAwait(false);
            if (listElementsBytes < 0)
            {
                var exc = new InvalidDataException(string.Format("CheckpointFileHelper.CopyListElementsFrameAsync : Unexpected listElementsBytes: {0}", listElementsBytes));
                FabricEvents.Events.ReliableConcurrentQueue_ExceptionError(traceType, exc.ToString());
                throw exc;
            }

            var listElementsSegment = new ArraySegment<byte>(new byte[listElementsBytes]);
            await SerializationHelper.ReadBytesAsync(listElementsSegment, listElementsBytes, inputStream).ConfigureAwait(false);
            await SerializationHelper.ReadBytesAsync(crcSegment, sizeof(ulong), inputStream).ConfigureAwait(false);
            var readCRC = BitConverter.ToUInt64(crcSegment.Array, crcSegment.Offset);

            var calcCrc = CRC64.ToCRC64(new[] { listElementsCountSegment, listElementsBytesSegment, listElementsSegment });
            if (calcCrc != readCRC)
            {
                var exc = new InvalidDataException(string.Format("CheckpointFileHelper.CopyListElementsFrameAsync => CRC mismatch.  Read: {0} Calculated: {1}", readCRC, calcCrc));
                FabricEvents.Events.ReliableConcurrentQueue_ExceptionError(traceType, exc.ToString());
                throw exc;
            }

            await outputStream.WriteAsync(listElementsCountSegment.Array, listElementsCountSegment.Offset, listElementsCountSegment.Count).ConfigureAwait(false);
            await outputStream.WriteAsync(listElementsBytesSegment.Array, listElementsBytesSegment.Offset, listElementsBytesSegment.Count).ConfigureAwait(false);
            await outputStream.WriteAsync(listElementsSegment.Array, listElementsSegment.Offset, listElementsSegment.Count).ConfigureAwait(false);
            await outputStream.WriteAsync(crcSegment.Array, crcSegment.Offset, crcSegment.Count).ConfigureAwait(false);
        }

        // return true if there will be no more frames to copy (complete)
        public static async Task<bool> CopyNextFrameAsync(Stream inputStream, Stream outputStream, string traceType)
        {
            try
            {
                var intSegment = new ArraySegment<byte>(new byte[sizeof(int)]);

                var marker = await SerializationHelper.ReadIntAsync(intSegment, inputStream).ConfigureAwait(false);
                if (marker == BeginFrameMarker)
                {
                    // write the marker
                    await outputStream.WriteAsync(intSegment.Array, intSegment.Offset, intSegment.Count).ConfigureAwait(false);

                    // copy the bytes for this frame
                    await CopyListElementsFrameAsync(inputStream, outputStream, traceType).ConfigureAwait(false);
                    return false;
                }
                else if (marker == EndFramesMarker)
                {
                    // write the end marker, and the copy is complete
                    await outputStream.WriteAsync(intSegment.Array, intSegment.Offset, intSegment.Count).ConfigureAwait(false);
                    return true;
                }
                else
                {
                    var exc = new InvalidDataException(string.Format("CheckpointFileHelper.CopyNextFrameAsync => Marker : {0}", marker));
                    FabricEvents.Events.ReliableConcurrentQueue_ExceptionError(traceType, exc.ToString());
                    throw exc;
                }
            }
            catch (Exception ex)
            {
                FabricEvents.Events.ReliableConcurrentQueue_CopyNextFrameError("ReliableConcurrentQueueCheckpointFile.CopyNextFrameAsync@", ex.ToString());
                throw;
            }
        }

        public static void SafeFileReplace(string currentFilePath, string newFilePath, string backupFilePath, string traceType)
        {
            if (FabricFile.Exists(backupFilePath))
            {
                FabricFile.Delete(backupFilePath);
            }
            if (FabricFile.Exists(backupFilePath))
            {
                TestableAssertHelper.FailInvalidData(traceType, "CheckpointFileHelper.SafeFileReplace", "!FabricFile.Exists(backupFilePath) : {0}", backupFilePath);
            }

            // Previous replace could have failed in the middle before the next metadata table file got renamed to current.
            if (!FabricFile.Exists(currentFilePath))
            {
                FabricFile.Move(newFilePath, currentFilePath);
            }
            else
            {
                FabricFile.Replace(newFilePath, currentFilePath, backupFilePath, ignoreMetadataErrors: false);
            }

            if (FabricFile.Exists(backupFilePath))
            {
                FabricFile.Delete(backupFilePath);
            }
            if (FabricFile.Exists(backupFilePath))
            {
                TestableAssertHelper.FailInvalidData(traceType, "CheckpointFileHelper.SafeFileReplace", "!FabricFile.Exists(backupFilePath) : {0}", backupFilePath);
            }
        }
    }
}