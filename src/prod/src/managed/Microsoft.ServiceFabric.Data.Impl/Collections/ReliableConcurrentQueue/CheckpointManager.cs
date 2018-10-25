// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.IO;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Replicator;

    internal class CheckpointManager<T>
    {
        private readonly string traceType;

        internal const int FileVersion = 1;

        private const string NewCheckpointManagerFileName = "new_checkpoint_meta" + CheckpointFileHelper.CheckpointFileExtension;

        private const string CurrentCheckpointManagerFileName = "current_checkpoint_meta" + CheckpointFileHelper.CheckpointFileExtension;

        private const string BackupCheckpointManagerFileName = "backup_checkpoint_meta" + CheckpointFileHelper.CheckpointFileExtension;

        // todo: ensure never null
        public Checkpoint<T> CurrentCheckpoint { get; private set; }

        private string directory;

        public string FileName { get; private set; }

        public string FilePath
        {
            get
            {
                return Path.Combine(this.directory, this.FileName);
            }
        }

        private IStateSerializer<T> valueSerializer;

        public CheckpointManager(Checkpoint<T> currentCheckpoint, string directory, string FileName, IStateSerializer<T> valueSerializer, string traceType)
        {
            this.traceType = traceType;
            this.CurrentCheckpoint = currentCheckpoint;
            this.directory = directory;
            this.FileName = FileName;
            this.valueSerializer = valueSerializer;
        }

        public static CheckpointManager<T> CreateCurrentCheckpointManager(
            List<IListElement<T>> listElements,
            string directory,
            IStateSerializer<T> valueSerializer,
            string traceType)
        {
            return new CheckpointManager<T>(
                new Checkpoint<T>(directory, Checkpoint<T>.GenerateFileName(), listElements, valueSerializer, traceType),
                directory,
                CurrentCheckpointManagerFileName,
                valueSerializer, 
                traceType);
        }

        public static CheckpointManager<T> CreateNewCheckpointManager(List<IListElement<T>> listElements, string directory, IStateSerializer<T> valueSerializer, string traceType)
        {
            return new CheckpointManager<T>(
                new Checkpoint<T>(directory, Checkpoint<T>.GenerateFileName(), listElements, valueSerializer, traceType),
                directory,
                NewCheckpointManagerFileName,
                valueSerializer, 
                traceType);
        }

        public static Task<ConditionalValue<CheckpointManager<T>>> TryReadCurrentFile(string directory, IStateSerializer<T> valueSerializer, string traceType)
        {
            return TryReadCheckpointFile(directory, CurrentCheckpointManagerFileName, valueSerializer, traceType);
        }

        public static Task<ConditionalValue<CheckpointManager<T>>> TryReadNewFile(string directory, IStateSerializer<T> valueSerializer, string traceType)
        {
            return TryReadCheckpointFile(directory, NewCheckpointManagerFileName, valueSerializer, traceType);
        }

        public async Task WriteAsync()
        {
            var filePath = Path.Combine(this.directory, this.FileName);
            using (var stream = FabricFile.Open(filePath, FileMode.Create, FileAccess.Write, FileShare.None, 4096, FileOptions.SequentialScan))
            {
                var versionArraySegment = new ArraySegment<byte>(BitConverter.GetBytes(FileVersion));

                if (this.CurrentCheckpoint != null)
                {
                    ArraySegment<byte> currentCheckpointFileNameArraySegment;
                    using (var writer = new InMemoryBinaryWriter(new MemoryStream()))
                    {
                        writer.Write(this.CurrentCheckpoint.FileName);
                        currentCheckpointFileNameArraySegment = new ArraySegment<byte>(writer.BaseStream.GetBuffer(), 0, (int)writer.BaseStream.Position);
                    }

                    var currentCheckpointFileNameLengthArraySegment = new ArraySegment<byte>(BitConverter.GetBytes(currentCheckpointFileNameArraySegment.Count));

                    var crc = CRC64.ToCRC64(new[] { versionArraySegment, currentCheckpointFileNameLengthArraySegment, currentCheckpointFileNameArraySegment });
                    var crcArraySegment = new ArraySegment<byte>(BitConverter.GetBytes(crc));

                    await stream.WriteAsync(versionArraySegment.Array, versionArraySegment.Offset, versionArraySegment.Count).ConfigureAwait(false);
                    await
                        stream.WriteAsync(
                            currentCheckpointFileNameLengthArraySegment.Array,
                            currentCheckpointFileNameLengthArraySegment.Offset,
                            currentCheckpointFileNameLengthArraySegment.Count).ConfigureAwait(false);
                    await
                        stream.WriteAsync(
                            currentCheckpointFileNameArraySegment.Array,
                            currentCheckpointFileNameArraySegment.Offset,
                            currentCheckpointFileNameArraySegment.Count).ConfigureAwait(false);
                    await stream.WriteAsync(crcArraySegment.Array, crcArraySegment.Offset, crcArraySegment.Count).ConfigureAwait(false);
                }
                else
                {
                    var currentCheckpointNameLengthArraySegment = new ArraySegment<byte>(BitConverter.GetBytes(0));

                    var crc = CRC64.ToCRC64(new[] { versionArraySegment, currentCheckpointNameLengthArraySegment });
                    var crcArraySegment = new ArraySegment<byte>(BitConverter.GetBytes(crc));

                    await stream.WriteAsync(versionArraySegment.Array, versionArraySegment.Offset, versionArraySegment.Count).ConfigureAwait(false);
                    await
                        stream.WriteAsync(
                            currentCheckpointNameLengthArraySegment.Array,
                            currentCheckpointNameLengthArraySegment.Offset,
                            currentCheckpointNameLengthArraySegment.Count).ConfigureAwait(false);
                    await stream.WriteAsync(crcArraySegment.Array, crcArraySegment.Offset, crcArraySegment.Count).ConfigureAwait(false);
                }
            }
        }

        public void Copy(string targetDirectory, bool overwrite)
        {
            var sourcePath = Path.Combine(this.directory, this.FileName);
            var targetPath = Path.Combine(targetDirectory, this.FileName);

            if (!FabricFile.Exists(sourcePath))
            {
                throw new ArgumentException(string.Format("Checkpoint Manager file not found in directory {0}", this.directory), "this.directory");
            }

            if (this.CurrentCheckpoint != null)
            {
                this.CurrentCheckpoint.Copy(targetDirectory, overwrite);
            }

            FabricFile.Copy(sourcePath, targetPath, overwrite);
        }

        public async Task ReplaceCurrentAsync()
        {
            if (this.FileName != NewCheckpointManagerFileName)
            {
                TestableAssertHelper.FailInvalidOperation(
                    this.traceType,
                    "CheckpointManager.ReplaceCurrentAsync",
                    "this.FileName == NewCheckpointManagerFileName; expected: {0} actual: {1}",
                    NewCheckpointManagerFileName, this.FileName);
            }

            await this.VerifyAsync().ConfigureAwait(false);

            var currentFilePath = Path.Combine(this.directory, CurrentCheckpointManagerFileName);
            var backupFilePath = Path.Combine(this.directory, BackupCheckpointManagerFileName);
            var newFilePath = Path.Combine(this.directory, NewCheckpointManagerFileName);
            CheckpointFileHelper.SafeFileReplace(currentFilePath, newFilePath, backupFilePath, this.traceType);

            this.FileName = CurrentCheckpointManagerFileName;
        }

        private async Task VerifyAsync()
        {
            // Read from disk to verify
            var res = await TryReadCheckpointFile(this.directory, this.FileName, this.valueSerializer, this.traceType).ConfigureAwait(false);
            if (!res.HasValue)
            {
                throw new InvalidDataException(string.Format("Checkpoint file '{0}' not found.", Path.Combine(this.directory, this.FileName)));
            }

            if ((this.CurrentCheckpoint == null) != (res.Value.CurrentCheckpoint == null))
            {
                TestableAssertHelper.FailInvalidData(
                    this.traceType,
                    "CheckpointManager.VerifyAsync",
                    "Disagreement on checkpoint existing.  In-Memory: {0}  On-Disk: {1}",
                    this.CurrentCheckpoint != null ? this.CurrentCheckpoint.FilePath : "null",
                    res.Value.CurrentCheckpoint != null ? res.Value.CurrentCheckpoint.FilePath : "null");
            }

            await this.CurrentCheckpoint.VerifyAsync().ConfigureAwait(false);
        }

        private static async Task<ConditionalValue<CheckpointManager<T>>> TryReadCheckpointFile(
            string directory,
            string fileName,
            IStateSerializer<T> valueSerializer, 
            string traceType)
        {
            var filePath = Path.Combine(directory, fileName);
            if (!FabricFile.Exists(filePath))
            {
                return new ConditionalValue<CheckpointManager<T>>(false, default(CheckpointManager<T>));
            }

            using (var stream = FabricFile.Open(filePath, FileMode.Open, FileAccess.Read, FileShare.Read, 4096, FileOptions.SequentialScan))
            {
                var intSegment = new ArraySegment<byte>(new byte[sizeof(int)]);

                var versionRead = await SerializationHelper.ReadIntAsync(intSegment, stream).ConfigureAwait(false);
                if (versionRead != FileVersion)
                {
                    throw new InvalidDataException(string.Format("versionRead '{0}' != FileVersion '{1}'", versionRead, FileVersion));
                }

                var nameLength = await SerializationHelper.ReadIntAsync(intSegment, stream).ConfigureAwait(false);
                if (nameLength < 0)
                {
                    throw new InvalidDataException(string.Format("nameLength '{0}' < 0", nameLength));
                }

                if (nameLength == 0)
                {
                    return new ConditionalValue<CheckpointManager<T>>(true, new CheckpointManager<T>(null, directory, fileName, valueSerializer, traceType));
                }

                var nameSegment = new ArraySegment<byte>(new byte[nameLength]);
                await SerializationHelper.ReadBytesAsync(nameSegment, nameLength, stream).ConfigureAwait(false);
                string name;

                using (var reader = new InMemoryBinaryReader(new MemoryStream(nameSegment.Array)))
                {
                    name = reader.ReadString();
                }

                var path = Path.Combine(directory, name);
                if (!FabricFile.Exists(path))
                {
                    throw new InvalidDataException(string.Format("Current checkpoint file does not exist: {0}", path));
                }

                return new ConditionalValue<CheckpointManager<T>>(
                    true,
                    new CheckpointManager<T>(new Checkpoint<T>(directory, name, valueSerializer, traceType), directory, fileName, valueSerializer, traceType));
            }
        }
    }
}