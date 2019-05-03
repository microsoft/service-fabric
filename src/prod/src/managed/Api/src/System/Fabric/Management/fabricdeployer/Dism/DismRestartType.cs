// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if !DotNetCoreClrLinux

namespace System.Fabric.FabricDeployer
{
    /// <summary>
    /// <see cref="http://msdn.microsoft.com/en-us/library/windows/desktop/hh824749.aspx"/>
    /// </summary>
    internal enum DismRestartType
    {
        No = 0,
        Possible = 1,
        Required = 2,
    }
}

#endif