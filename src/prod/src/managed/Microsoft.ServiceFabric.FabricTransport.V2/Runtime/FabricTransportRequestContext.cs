// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2.Runtime
{
    using System;

    internal class FabricTransportRequestContext
    {
        private readonly string clientId;
        private readonly Func<string, FabricTransportCallbackClient> callback;
        private FabricTransportCallbackClient callbackClient;

        public FabricTransportRequestContext(string clientId, Func<string, FabricTransportCallbackClient> getCallBack)
        {
            this.clientId = clientId;
            this.callback = getCallBack;
        }

        public string ClientId
        {
            get { return this.clientId; }
        }

        public FabricTransportCallbackClient GetCallbackClient()

        {
            if (this.callbackClient == null)
            {
                this.callbackClient = this.callback(this.clientId);
            }

            return this.callbackClient;
        }
    }
}