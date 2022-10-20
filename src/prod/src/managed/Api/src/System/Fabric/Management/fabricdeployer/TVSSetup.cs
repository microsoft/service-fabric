// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using System.Collections.ObjectModel;
    using System.IO;

    internal class TVSSetup
    {
        internal static bool SetupInfrastructureForClaimsAuth(bool isRunningAsAdmin)
        {
            //
            // No special setup is needed for claims based authentication.
            // Dsts binaries have a dependency on Microsoft.IdentityModel.dll,
            // which is carried as a part of the service fabric runtime.
            //
            return true;
        }
    }
}
