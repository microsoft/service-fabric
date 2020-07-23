// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Fabric.Testability.Common;
    using System.Threading.Tasks;
    using System.Fabric.Interop;

    public abstract class FabricRequest : IRequest
    {
        private Predicate<IRetryContext> processResult;

        protected FabricRequest(IFabricClient fabricClient, TimeSpan timeout)
        {
            ThrowIf.Null(fabricClient, "fabricClient");

            this.FabricClient = fabricClient;
            this.Timeout = timeout;

            this.SucceedErrorCodes = new List<uint>();
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.S_OK);

            this.RetryErrorCodes = new List<uint>();
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TIMEOUT);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.E_ABORT);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_GATEWAY_NOT_REACHABLE);

            this.ProcessResult = FabricRequestProcessor.SuccessRetryErrorCodeProcessor(this);
        }

        protected IFabricClient FabricClient
        {
            get;
            private set;
        }

        public TimeSpan Timeout
        {
            get;
            private set;
        }

        public OperationResult OperationResult
        {
            get;
            protected set;
        }

        public Predicate<IRetryContext> ProcessResult
        {
            get
            {
                return this.processResult;
            }

            set
            {
                ThrowIf.Null(value, "value");
                this.processResult = value;
            }
        }

        public IList<uint> SucceedErrorCodes
        {
            get;
            private set;
        }

        public IList<uint> RetryErrorCodes
        {
            get;
            private set;
        }

        public abstract Task PerformCoreAsync(CancellationToken cancellationToken);

        public abstract override string ToString();

        public async Task PerformAsync(CancellationToken cancellationToken)
        {
            try
            {
                cancellationToken.ThrowIfCancellationRequested();
                await this.PerformCoreAsync(cancellationToken);
            }
            catch (Exception ex)
            {
                // Note: The OutOfMemoryException, StackOverflowException and ThreadAbortException classes always return null for the value of the Data property.
                if (ex.Data != null)
                {
                    ex.Data.Add("MS.Test.WinFabric.FabricRequest.PerformAsync", this.ToString());
                }

                throw;
            }
        }
    }
}


#pragma warning restore 1591