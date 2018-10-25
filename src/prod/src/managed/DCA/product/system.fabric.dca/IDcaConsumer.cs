// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System;

    // This interface needs to be implemented by all DCA consumers
    public interface IDcaConsumer : IDisposable
    {
        // Retrieves the data sink interface exposed by the consumer
        object GetDataSink();

        void FlushData();
    }
}