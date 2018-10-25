// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// <para>
    /// These are constants used by ServiceFabric for various OS Related resources.
    /// </para>
    /// </summary>
    public static class FabricConstants
    {
        /// <summary>
        /// The registry key path used by Fabric. This is going to be deprecated soon, and no new dependency should be taken on this constant.
        /// </summary>
        public const string FabricRegistryKeyPath = "Software\\Microsoft\\Service Fabric";
    }
}