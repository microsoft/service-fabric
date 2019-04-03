// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    /// <summary>
    /// Factory interface for creating <see cref="IManagementClient"/> objects.
    /// </summary>
    internal interface IManagementClientFactory
    {
        IManagementClient Create();
    }
}