// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using System.Collections.Generic;

    /// <summary>
    /// A class mocking <see cref="PropertyBatchResult"/> for unit-testability.
    /// </summary>
    internal class MockPropertyBatchResultWrapper : IPropertyBatchResultWrapper
    {
        public MockPropertyBatchResultWrapper()
        {
            NamedProperties = new List<INamedPropertyWrapper>();
            FailedOperationIndex = -1;
        }

        public List<INamedPropertyWrapper> NamedProperties { get; private set; }

        public Exception FailedOperationException { get; set; }

        public int FailedOperationIndex { get; set; }
        
        public INamedPropertyWrapper GetProperty(int index)
        {
            return NamedProperties[index];
        }
    }
}