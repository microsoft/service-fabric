// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace MdsHelper 
{
    using System;
    using Microsoft.Cis.Monitoring.Tables;

    // This class implements the logic to write data to MDS tables
    internal class MdsTableWriter : IDisposable
    {
        // Constants
        public const int MdNoError = 0;
        public const int TableErrorInternal = -2147352564; // 0x8002000c
        public const int TableErrorCorrupt = -2147352552; // 0x80020018
        public const int TableErrorFatalCorrupt = -2147352551; // 0x80020019
        public const int TableErrorSchemaExist = -2147352538; // 0x80020026

        // Reference to the writer
        private readonly Writer writer;
        private readonly string directoryName;
        private readonly string tableName;
        private readonly TablePriority tablePriority;
        private readonly long dataDeletionAgeMinutes;
        private readonly int diskQuotaInMb;
        private readonly string[] tableFieldTags;
        private readonly TableFieldType[] tableFieldTypes;

        // MDS table handle
        private IntPtr tableHandle;
        private uint schemaHandle;

        // Whether or not the object has been disposed
        private bool disposed;

        internal MdsTableWriter(
                        string directoryName, 
                        string tableName, 
                        TablePriority tablePriority, 
                        long dataDeletionAgeMinutes,
                        int diskQuotaInMB,
                        string[] tableFieldTags,
                        TableFieldType[] tableFieldTypes)
        {
            this.directoryName = directoryName;
            this.tableName = tableName;
            this.tablePriority = tablePriority;
            this.dataDeletionAgeMinutes = dataDeletionAgeMinutes;
            this.diskQuotaInMb = diskQuotaInMB;
            this.tableFieldTags = tableFieldTags;
            this.tableFieldTypes = tableFieldTypes;

            // Create the writer
            this.writer = new Writer();

            // Create the table
            var result = this.CreateTableSafe();
            if (MdNoError != result)
            {
                this.writer.Dispose();
                var message = string.Format(
                    "Failed to create MDS table {0} in directory {1}. Error: {2}.",
                    tableName,
                    directoryName,
                    result);
                throw new InvalidOperationException(message);
            }
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            if (null != this.writer)
            {
                if (this.tableHandle != (IntPtr)0)
                {
                    this.writer.CloseTable(ref this.tableHandle);
                    this.tableHandle = (IntPtr)0;
                }

                this.writer.Dispose();
            }

            GC.SuppressFinalize(this);
        }

        internal int WriteEvent(object[] tableFields)
        {
            var result = this.writer.WriteRecordToTable(
                            this.tableHandle,
                            this.schemaHandle,
                            tableFields,
                            null);

            if (result == TableErrorCorrupt || result == TableErrorFatalCorrupt)
            {
                result = this.CreateTableSafe();
                if (result != MdNoError)
                {
                    return result;
                }

                // Try again with non-corrupt table.
                result = this.writer.WriteRecordToTable(
                            this.tableHandle,
                            this.schemaHandle,
                            tableFields,
                            null);
            }

            return result;
        }

        /// <summary>
        /// Handles table corruption while opening table. 
        /// </summary>
        /// <returns>Returns <see cref="MdNoError" /> on success, forwards MDS error code otherwise.</returns>
        private int CreateTableSafe()
        {
            int result = MdNoError;

            // Close existing open table.
            if (this.tableHandle != (IntPtr)0)
            {
                result = this.writer.CloseTable(ref this.tableHandle);
                if (result != MdNoError)
                {
                    return result;
                }
            }

            // Table might be corrupt.  In that case we recreate tables under new name.
            for (var tableNameNonce = 0; tableNameNonce < 100; tableNameNonce++)
            {
                result = this.writer.CreateTable(
                    this.tableName + tableNameNonce,
                    this.directoryName,
                    TableDisposition.TD_CreateOpen,
                    this.tablePriority,
                    this.dataDeletionAgeMinutes * 60,
                    this.diskQuotaInMb,
                    out this.tableHandle);

                if (result == TableErrorCorrupt || 
                    result == TableErrorFatalCorrupt ||
                    result == TableErrorInternal)
                {
                    continue;
                }

                if (result != MdNoError)
                {
                    return result;
                }

                // Set the table schema
                result = this.writer.AddTableSchema(
                             this.tableHandle,
                             this.tableFieldTags,
                             this.tableFieldTypes,
                             out this.schemaHandle);
                if (result == TableErrorSchemaExist || result == MdNoError)
                {
                    return MdNoError;
                }

                if (result == TableErrorCorrupt || result == TableErrorFatalCorrupt)
                {
                    this.writer.CloseTable(ref this.tableHandle);
                    this.tableHandle = (IntPtr)0;
                    continue;
                }

                this.writer.CloseTable(ref this.tableHandle);
                this.tableHandle = (IntPtr)0;
            }

            // Return Corrupt error code if table names exhausted.  Something else is likely wrong.
            return result;
        }
    }
}