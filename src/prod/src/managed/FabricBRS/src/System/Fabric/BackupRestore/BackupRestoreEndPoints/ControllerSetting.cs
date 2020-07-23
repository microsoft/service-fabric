// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints
{
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Threading;

    public class ControllerSetting
    {
        internal StatefulService StatefulService;
        internal CancellationToken CancellationToken;
    
        public ControllerSetting(StatefulService statefulService, CancellationToken cancellationToken)
        {
            this.StatefulService = statefulService;
            this.CancellationToken = cancellationToken;
        }
    }
}