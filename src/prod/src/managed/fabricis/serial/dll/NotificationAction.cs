// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    /// <summary>
    /// The actions that could be taken on a <see cref="IManagementNotificationContext"/>
    /// </summary>
    internal enum NotificationAction
    {
        SignalReady,
        SignalError,
    }
}