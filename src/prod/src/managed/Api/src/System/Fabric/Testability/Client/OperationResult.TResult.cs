// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client
{
    using System;
    using System.Fabric.Interop;
    using System.Collections.Generic;
    using System.Linq;

    [Serializable]
    public class OperationResult<TResult> : OperationResult
    {
        private readonly TResult result;

        public OperationResult(uint errorCode)
            : base(errorCode)
        {
            this.result = default(TResult);
        }

        public OperationResult(uint errorCode, TResult result)
            : base(errorCode)
        {
            this.result = result;
        }

        public TResult Result
        {
            get
            {
                this.ThrowExceptionFromErrorCode();

                return this.result;
            }
        }

        internal static OperationResult<TResult[]> GetOperationResultArray(NativeTypes.FABRIC_ERROR_CODE errorCode, IEnumerable<TResult> resultList)
        {
            TResult[] resultArray = resultList == null ? null : resultList.ToArray();
            return FabricClientState.CreateOperationResultFromNativeErrorCode<TResult[]>(errorCode, resultArray);
        }
    }
}


#pragma warning restore 1591