// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FileLogUtility
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.IO;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Replicator;

    class FileLogicalLogParser
    {
        /// <summary>
        /// Path to the FileLogicalLog.
        /// </summary>
        private string logPath;

        /// <summary>
        /// Indicates whether the records should be read as physical (or logical) records.
        /// </summary>
        private bool isPhysical;

        /// <summary>
        /// Accumulated statistics about the FileLogicalLog.
        /// </summary>
        private FileLogicalLogStats stats = new FileLogicalLogStats();

        public FileLogicalLogParser(string logPath, bool isPhysical)
        {
            this.logPath = logPath;
            this.isPhysical = isPhysical;
        }

        public async Task ParseAsync()
        {
            using (var filestream = new FileStream(logPath, FileMode.Open))
            {
                if (isPhysical)
                {
                    await this.ParsePhysicalRecordsAsync(filestream);
                }
                else
                {
                    this.ParseLogicalRecords(filestream);
                }
            }
        }

        public void DisplayStatistics()
        {
            this.stats.DisplayStatistics();
        }

        private async Task ParsePhysicalRecordsAsync(Stream stream)
        {
            while (stream.Position < stream.Length)
            {
                LogRecord record = await LogRecord.ReadNextRecordAsync(stream);
                this.stats.ProcessRecord(record);
            }
        }

        private void ParseLogicalRecords(Stream stream)
        {
            using (var reader = new BinaryReader(stream))
            {
                while (stream.Position < stream.Length)
                {
                    int recordSize = reader.ReadInt32();
                    ArraySegment<byte> recordBytes = new ArraySegment<byte>(reader.ReadBytes(recordSize));

                    LogRecord record = LogRecord.FromOperationData(new OperationData(recordBytes));
                    record.RecordLength = (uint)(recordSize - 2 * sizeof(uint)); // to compensate for RecordSize adding 2 * sizeof(uint)

                    this.stats.ProcessRecord(record);
                }
            }
        }
    }
}