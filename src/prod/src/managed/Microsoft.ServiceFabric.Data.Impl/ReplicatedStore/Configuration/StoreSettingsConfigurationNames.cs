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
    internal static class StoreSettingsConfigurationNames
    {
        public static readonly string SectionName = "ReliableStateProvider";

        public static readonly Tuple<string, int> SweepThreshold = new Tuple<string, int>("SweepThreshold", 0);

        public static readonly Tuple<string, bool> EnableStrict2PL = new Tuple<string, bool>("EnableStrict2PL", false);
    }
}