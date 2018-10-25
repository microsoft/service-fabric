// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;

    /// <summary>
    /// NOTE!!! - WARNING- Do not change formatting of this class by moving names of the variables to new lines as it fails parsing in the config validator perl script
    /// </summary>
    internal static class ReliableStateManagerNativeRunConfigurationNames
    {
        public static readonly string SectionName = "NativeRunConfiguration";
        
        public static readonly Tuple<string, bool> EnableNativeReliableStateManager = new Tuple<string, bool>("EnableNativeReliableStateManager", false);

        public static readonly Tuple<string, bool> Test_LoadEnableNativeReliableStateManager = new Tuple<string, bool>("Test_LoadEnableNativeReliableStateManager", false);
    }
}