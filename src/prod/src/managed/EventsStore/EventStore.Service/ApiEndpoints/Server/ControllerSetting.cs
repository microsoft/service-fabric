// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Server
{
    using System.Threading;

    public sealed class ControllerSetting
    {
        public ControllerSetting(EventStoreRuntime currentRuntime, CancellationToken cancellationToken)
        {
            this.CurrentRuntime = currentRuntime;
            this.CancellationToken = cancellationToken;
        }

        public EventStoreRuntime CurrentRuntime { get; private set; }

        public CancellationToken CancellationToken { get; private set; }

    }
}