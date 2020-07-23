// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if !DotNetCoreClrLinux

namespace System.Fabric.FabricDeployer
{
    /// <summary>
    /// <see cref="https://msdn.microsoft.com/en-us/library/windows/desktop/hh824781.aspx"/>
    /// </summary>
    internal enum DismPackageIdentifier
    {
        None = 0,
        Name = 1,
        Path = 2,
    }
}

#endif