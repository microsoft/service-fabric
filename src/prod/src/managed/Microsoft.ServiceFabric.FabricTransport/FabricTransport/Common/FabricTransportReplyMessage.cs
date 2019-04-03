// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport
{
    internal class FabricTransportReplyMessage
    {
        private readonly byte[] reply;

        public FabricTransportReplyMessage(bool isException, byte[] reply)
        {
            this.IsException = isException;
            this.reply = reply;
        }

        public bool IsException { get; private set; }

        public byte[] GetBody()
        {
            return this.reply;
        }
    }
}