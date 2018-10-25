// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Common.Tracing;
    using System.Threading;
    using System.Threading.Tasks;

    internal abstract class FabricTestActionHandler
    {
        internal abstract Task ExecuteAsync(FabricTestContext testContext, object action, CancellationToken cancellationToken);
    }

    internal abstract class FabricTestActionHandler<TAction> : FabricTestActionHandler where TAction : FabricTestAction
    {
        private CancellationTokenSource cancellationTokenSource;

        protected const string TraceType = "ActionSource";

        protected FabricEvents.ExtensionsEvents ActionTraceSource
        {
            get
            {
                return TestabilityTrace.TraceSource;
            }
        }

        protected string ResultTraceString { get; set; }

        internal override async Task ExecuteAsync(FabricTestContext testContext, object action, CancellationToken cancellationToken)
        {
            TAction castAction = action as TAction;
            ThrowIf.Null(castAction, "action");

            this.cancellationTokenSource = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);

            this.cancellationTokenSource.Token.ThrowIfCancellationRequested();
            ActionTraceSource.WriteInfo(TraceType, "StartingAction\t\t{0}", castAction.GetType().Name);
            await this.ExecuteActionAsync(testContext, castAction, this.cancellationTokenSource.Token);
            ActionTraceSource.WriteInfo(TraceType, "StartingAction\t\t{0}", castAction.GetType().Name);
        }

        protected abstract Task ExecuteActionAsync(FabricTestContext testContext, TAction action, CancellationToken cancellationToken);
    }
}