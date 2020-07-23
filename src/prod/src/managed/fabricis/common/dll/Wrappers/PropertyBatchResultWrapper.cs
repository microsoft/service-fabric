// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    internal class PropertyBatchResultWrapper : IPropertyBatchResultWrapper
    {
        private readonly PropertyBatchResult result;

        public PropertyBatchResultWrapper(PropertyBatchResult result)
        {
            this.result = result;
        }

        public Exception FailedOperationException 
        {
            get { return result.FailedOperationException; }
        }

        public int FailedOperationIndex
        {
            get { return result.FailedOperationIndex; }
        }

        public INamedPropertyWrapper GetProperty(int index)
        {
            return new NamedPropertyWrapper(result.GetProperty(index));
        }
    }
}