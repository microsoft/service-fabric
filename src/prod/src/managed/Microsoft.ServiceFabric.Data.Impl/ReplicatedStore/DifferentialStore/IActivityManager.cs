// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Threading.Tasks;

    /// <summary>
    /// Manages activities for a given entity.
    /// </summary>
    internal interface IActivityManager
    {
        void AcquireActivity();

        void ReleaseActivity();

        Task WaitForActivities();

        void AbandonActivities();
    }
}