// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Structures
{
    using System;
    using System.Globalization;
    using System.Text;

    [Serializable]
    public class WinFabricPropertyBatchResult
    {
        public WinFabricPropertyBatchResult(int batchIdentifier)
        {
            this.BatchIdentifier = batchIdentifier;
        }

        public PropertyBatchResult Result
        {
            get;
            set;
        }

        public FabricErrorCode FailedOperationErrorCode
        {
            get;
            set;
        }

        public int BatchIdentifier
        {
            get;
            private set;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat(CultureInfo.InvariantCulture, "BatchIdentifier={0}; ", this.BatchIdentifier);

            if (this.Result != null)
            {
                stringBuilder.AppendFormat(CultureInfo.InvariantCulture, "FailedOperationIndex={0}; FailedOperationErrorCode={1}", this.Result.FailedOperationIndex, this.FailedOperationErrorCode);
                if (this.Result.FailedOperationException != null)
                {
                    stringBuilder.AppendFormat(CultureInfo.InvariantCulture, "; FailedOperationException = {0}", this.Result.FailedOperationException);
                }
            }

            return stringBuilder.ToString();
        }
    }
}

#pragma warning restore 1591