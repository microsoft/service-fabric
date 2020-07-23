// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.InfrastructureWrapper
{
    using System;

    internal interface IEntryPoint
    {
        event EventHandler Finished;

        ErrorCode OnStart();

        void OnRun();

        void OnStop();
    }
}