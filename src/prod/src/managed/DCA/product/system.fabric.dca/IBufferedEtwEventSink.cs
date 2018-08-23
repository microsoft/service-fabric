// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.EtlConsumerHelper 
{
    // This interface is implemented by the user of the BufferedEtwEventProvider object
    public interface IBufferedEtwEventSink
    {
        // Method invoked when a period of buffered event delivery begins
        void OnEtwEventDeliveryStart();
        
        // Method invoked when a period of buffered event delivery ends
        void OnEtwEventDeliveryStop();

        // Method invoked when a buffered event is delivered
        void OnEtwEventAvailable(DecodedEtwEvent etwEvent, string nodeUniqueEventId);
    }
}