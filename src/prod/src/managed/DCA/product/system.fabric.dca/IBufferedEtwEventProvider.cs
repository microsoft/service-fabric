// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.EtlConsumerHelper 
{
    // This interface is implemented by the helper object that decodes ETW events,
    // buffers them locally and periodically delivers the buffered events with a 
    // unique ID for each event
    public interface IBufferedEtwEventProvider : ITraceOutputWriter
    {
        // Stops delivery of decoded ETW events for the current period. Delivery 
        // of events is resumed in the next period.
        void AbortEtwEventDelivery();
    }
}