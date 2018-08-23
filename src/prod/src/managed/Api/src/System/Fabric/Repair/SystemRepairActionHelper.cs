// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    internal static class SystemRepairActionHelper
    {
        public const string ManualRepairAction = "System.Manual";

        public static string GetActionString(SystemNodeRepairAction action)
        {
            return string.Format("System.{0}", action);
        }
    }
}