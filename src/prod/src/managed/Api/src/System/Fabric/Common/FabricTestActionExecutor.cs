// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    internal class FabricTestActionExecutor
    {
        private readonly FabricTestContext testContext;
        private const string TraceSource = "FabricTestActionExecutor";

        internal FabricTestActionExecutor(FabricTestContext testContext)
        {
            this.testContext = testContext;
        }

        public async Task RunAsync(FabricTestAction action, CancellationToken token)
        {
            FabricTestActionHandler handler = (FabricTestActionHandler)Activator.CreateInstance(action.ActionHandlerType);
            await handler.ExecuteAsync(this.testContext, action, token);
        }

        public async Task RunAsync(FabricTestAction action)
        {
            FabricTestActionHandler handler = (FabricTestActionHandler)Activator.CreateInstance(action.ActionHandlerType);
            await handler.ExecuteAsync(this.testContext, action, CancellationToken.None);
        }
    }
}