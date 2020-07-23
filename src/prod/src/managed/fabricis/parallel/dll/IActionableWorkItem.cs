// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;

    internal interface IActionableWorkItem
    {
        IList<IAction> Actions { get; set; }

        ActionType PendingActions { get; }

        ActionType AllowedActions { get; }

        void AllowActions(TraceType traceType, ActionType allowedActions);

        void DenyActions(TraceType traceType, ActionType deniedActions);
    }
}