// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    internal enum RollbackState
    {
        NotRollingBack, 
        RollingBackAndWillRetryAction,
        RollingBackAndWillFailAction,
        RollingBackDueToUserCancel,
        RollingBackForce // user called cancel api, specifying force
    }
}