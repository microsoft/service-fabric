// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Structures
{
    using System;

    [Serializable]
    public enum WinFabricServicePackageSharingScope
    {
        None = 0,
        All = 1,
        Code = 2,
        Config = 3,
        Data = 4,
    }

    [Serializable]
    public class PackageSharingPolicyDescription
    {
        public string PackageName 
        { 
            private set; 
            get; 
        }

        public WinFabricServicePackageSharingScope Scope 
        { 
            private set; 
            get; 
        }

        public PackageSharingPolicyDescription(PackageSharingPolicyDescription other)
        {
            this.PackageName = other.PackageName;
            this.Scope = other.Scope;
        }

        public PackageSharingPolicyDescription(string packageName, WinFabricServicePackageSharingScope scope)
        {
            this.PackageName = packageName;
            this.Scope = scope;
        }
    }
}


#pragma warning restore 1591