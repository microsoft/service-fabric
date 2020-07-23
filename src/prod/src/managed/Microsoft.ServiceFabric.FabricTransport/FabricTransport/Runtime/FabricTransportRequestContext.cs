// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.Runtime
{
    using System;
    using Microsoft.ServiceFabric.FabricTransport.Client;

    internal class FabricTransportRequestContext
    {
        private string clientId;
        private Func<string, FabricTransportCallbackClient> getCallBack;
        private FabricTransportCallbackClient callback;

        public FabricTransportRequestContext(string clientId, Func<string, FabricTransportCallbackClient> getCallBack)
        {
            this.clientId = clientId;
            this.getCallBack = getCallBack;
        }

        public string ClientId
        {
            get { return this.clientId; }
            set { this.clientId = value; }
        }

        public FabricTransportCallbackClient GetCallbackClient()

        {
            if (this.callback == null)
            {
                this.callback = this.getCallBack(this.clientId);
            }

            return this.callback;
        }
    }
}