// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    /// <summary>
    /// Wrapper interface for <see cref="NamedProperty"/>
    /// </summary>
    internal interface INamedPropertyWrapper
    {
        T GetValue<T>();            
    }
}