// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    internal static class WindowsServerRestartConstant
    {
        public static TimeSpan FabricOperationTimeout = TimeSpan.FromMinutes(10);
        public static TimeSpan CoordinatorPollingIntervalDefault = TimeSpan.FromSeconds(10);
        public static TimeSpan MaxTimeRepairTaskActiveDefault = TimeSpan.FromMinutes(60);

        public static Uri StoreName = new Uri("Fabric:/__FabricSystem_App4294967295/WindowsServerRestartCoordinator");
        public static int MaxRetryAttempts = 50;
        public static int BackoffPeriodInMilliseconds = 5000;
    }
}