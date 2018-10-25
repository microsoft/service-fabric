// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace System.Fabric.Common
{
#if DotNetCoreClrLinux
    /// <summary>
    /// Type of Linux package manager. This is retrieved through FabricEnvironment.
    /// </summary>
    public enum LinuxPackageManagerType : int
    {
        /// <summary>
        /// Unknown. Returned when Linux distro doesn't have a known package manager.
        /// </summary>
        Unknown = 0,

        /// <summary>
        /// Debian package manager used by Ubuntu and other debian-based distros.
        /// </summary>
        Deb = 1,

        /// <summary>
        /// RPM package manager used by Redhat, CentOS, Fedora, etc.
        /// </summary>
        Rpm = 2,
    }
#endif
}