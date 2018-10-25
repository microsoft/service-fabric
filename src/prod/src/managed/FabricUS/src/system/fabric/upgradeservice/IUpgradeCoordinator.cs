// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Threading.Tasks;
    using System.Collections.Generic;
    using System.Threading;

    interface IUpgradeCoordinator
    {
        string ListeningAddress { get; }

        IEnumerable<Task> StartProcessing(CancellationToken token);
    }
}