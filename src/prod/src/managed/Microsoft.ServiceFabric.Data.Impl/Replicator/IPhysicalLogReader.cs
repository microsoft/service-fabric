// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using Microsoft.ServiceFabric.Data;

    internal interface IPhysicalLogReader : IDisposable
    {
        ulong EndingRecordPosition { get; }

        bool IsValid { get; }

        ulong StartingRecordPosition { get; }

        IAsyncEnumerator<LogRecord> GetLogRecords(string readerName, LogManager.LogReaderType readerType);
    }
}