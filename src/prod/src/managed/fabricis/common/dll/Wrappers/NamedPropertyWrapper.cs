// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    internal class NamedPropertyWrapper : INamedPropertyWrapper
    {
        private readonly NamedProperty namedProperty;

        public NamedPropertyWrapper(NamedProperty namedProperty)
        {
            this.namedProperty = namedProperty;
        }

        public T GetValue<T>()
        {
            return namedProperty.GetValue<T>();
        }
    }
}