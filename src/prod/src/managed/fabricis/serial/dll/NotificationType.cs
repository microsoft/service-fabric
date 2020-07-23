// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    /// <summary>
    /// Represents a 1:1 mapping with the concrete classes of type ManagementNotification in the Windows Azure management protocol (MR) SDK.
    /// This exists with the intent of mocking out the MR SDK via <see cref="IManagementClient"/>.
    /// </summary>
    internal enum NotificationType
    {
        Unknown,
        StartJobStep,
        CompleteJobStep,
    }
}