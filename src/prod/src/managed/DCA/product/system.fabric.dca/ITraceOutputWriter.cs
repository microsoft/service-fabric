// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.EtlConsumerHelper 
{
    using System;

    // This interface is implemented by the helper object that decodes ETW events
    // and writes the decoded events to text files
    public interface ITraceOutputWriter : IDisposable
    {
        // Creates a filter for ETW events
        void SetEtwEventFilter(string filter, string defaultFilter, string summaryFilter, bool isWindowsFabricEventFilter);

        void OnBootstrapTraceFileScanStart();

        void OnBootstrapTraceFileScanStop();

        bool OnBootstrapTraceFileAvailable(string fileName);
    }
}