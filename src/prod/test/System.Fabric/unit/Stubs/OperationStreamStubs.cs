// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Stubs
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;

    class OperationStreamStubBase : IOperationStream
    {
        public virtual Task<IOperation> GetOperationAsync(CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }
    }

    class OperationStubBase : IOperation
    {

        public virtual void Acknowledge()
        {
            throw new NotImplementedException();
        }

        public virtual OperationType OperationType
        {
            get { throw new NotImplementedException(); }
        }

        public virtual long SequenceNumber
        {
            get { throw new NotImplementedException(); }
        }

        public virtual long AtomicGroupId
        {
            get { throw new NotImplementedException(); }
        }

        public virtual OperationData Data
        {
            get { throw new NotImplementedException(); }
        }
    }
}