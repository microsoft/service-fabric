// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    /// <summary>
    /// A factory class for encapsulating creation of <see cref="IInfrastructureCoordinator"/>.
    /// </summary>
    internal interface IInfrastructureCoordinatorFactory
    {
        IInfrastructureCoordinator Create();
    }
}