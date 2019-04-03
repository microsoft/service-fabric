// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if !DotNetCoreClrLinux

namespace System.Fabric.FabricDeployer
{
    /// <summary>
    /// <see cref="https://msdn.microsoft.com/en-us/library/windows/desktop/hh824757(v=vs.85).aspx"/>
    /// </summary>
    internal enum DismLogLevel
    {
        LogErrors = 0,
        LogErrorsWarnings = 1,
        LogErrorsWarningsInfo = 2,
    }
}

#endif