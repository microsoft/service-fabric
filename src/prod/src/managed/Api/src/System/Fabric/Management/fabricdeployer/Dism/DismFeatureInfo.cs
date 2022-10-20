// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if !DotNetCoreClrLinux

namespace System.Fabric.FabricDeployer
{
    using System.Runtime.InteropServices;

    /// <summary>
    /// <see cref="https://msdn.microsoft.com/en-us/library/windows/desktop/hh824793(v=vs.85).aspx"/>
    /// </summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 4)]
    internal struct DismFeatureInfo
    {
        public string FeatureName;
        public DismPackageFeatureState FeatureState;
        public string DisplayName;
        public string Description;
        public DismRestartType RestartRequired;
        public IntPtr CustomProperty;
        public UInt32 CustomPropertyCount;
    }
}

#endif