// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions.Steps
{
    internal enum StepStateNames
    {
        None,

        IntentSaved,
        LookingUpState,

        // Space intentionally left
        PerformingActions,
        MidPerformingActions, // for test only

        // Space intentionally left
        CompletedSuccessfully,
        Failed,

        All
    }
}