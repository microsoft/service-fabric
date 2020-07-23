// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    /// <summary>
    /// A class mocking <see cref="NamedProperty"/> for unit-testability.
    /// </summary>
    public class MockNamedPropertyWrapper : INamedPropertyWrapper
    {
        public object Value { get; set; }

        public T GetValue<T>()
        {
            return (T)Value;
        }
    }
}