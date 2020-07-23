// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    internal static class Constants
    {
        public const int MaxRetryAttempts = 50;

        public const int BackoffPeriodInMilliseconds = 5000;

        /// <remarks>
        /// Why was this strange looking name used? 
        /// Writes to Fabric:/System are not allowed under managed fabric client (because we don't
        /// want users to use Fabric:/System related names for their apps).
        /// See: FabricClientImpl.cpp, bool FabricClientImpl::GetFabricUri
        /// </remarks>
        public const string StoreName = "Fabric:/__FabricSystem_App4294967295";

        public const string HealthReportSourceId = "System.InfrastructureService";

        /// <summary>
        /// The property name for showing the coordinator's job processing status as health info.
        /// </summary>
        public const string CoordinatorStatus = "CoordinatorStatus";

        /// <summary>
        /// A constant for the coordinator mode.
        /// </summary>
        public const string CoordinatorModeKey = "CoordinatorMode";
    }
}