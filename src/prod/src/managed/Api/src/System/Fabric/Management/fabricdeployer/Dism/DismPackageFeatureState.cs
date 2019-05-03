// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if !DotNetCoreClrLinux

namespace System.Fabric.FabricDeployer
{
    /// <summary>
    /// <see cref="http://msdn.microsoft.com/en-us/library/windows/desktop/hh824765.aspx"/>
    /// </summary>
    internal enum DismPackageFeatureState
    {
        DismStateNotPresent = 0,
        DismStateUninstallPending = 1,
        DismStateStaged = 2,
        DismStateRemoved = 3,
        DismStateInstalled = 4,
        DismStateInstallPending = 5,
        DismStateSuperseded = 6,
        DismStatePartiallyInstalled = 7,
    }
}

#endif