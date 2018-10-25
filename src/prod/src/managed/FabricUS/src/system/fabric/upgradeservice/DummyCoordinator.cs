// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    /// <summary>
    /// Dummy upgrade coordinator used when 
    /// upgrade service is being deleted as part of fabric upgrade
    /// </summary>
    internal sealed class DummyCoordinator : IUpgradeCoordinator
    {
        public string ListeningAddress
        {
            get
            {
                return String.Empty;
            }
        }

        public IEnumerable<Task> StartProcessing(CancellationToken token)
        {
            return new List<Task>() {Task.FromResult(0)};
        }
    }
}