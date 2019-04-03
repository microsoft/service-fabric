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
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Runtime.InteropServices;

    [Serializable]
    public class OperationResult
    {
        public OperationResult(uint errorCode)
        {
            this.ErrorCode = errorCode;
        }

        public uint ErrorCode
        {
            get;
            private set;
        }

        public void ThrowExceptionFromErrorCode()
        {
            if (this.ErrorCode != 0)
            {
                TestabilityTrace.TraceSource.WriteWarning("OperationResult", "Request failed with '{0}", this.ErrorCode);

                string errorMessage = string.Format(
                                                    CultureInfo.InvariantCulture,
                                                    "Request failed with {0}",
                                                    this.ErrorCode);

                Func<Exception, string, Exception> translateExceptionFunction = null;
                if (!InteropExceptionMap.NativeToManagedConversion.TryGetValue((int)this.ErrorCode, out translateExceptionFunction))
                {
                    TestabilityTrace.TraceSource.WriteError(
                        "OperationResult",
                        "Unknown ErrorCode {0} which cannot be mapped to any Fabric related exception",
                        this.ErrorCode);

                    throw new FabricException(Strings.StringResources.Error_Unknown, FabricErrorCode.Unknown);
                }

                // Creates an inner com exception with the native error code as well and error message. 
                // This can be improved by somehow getting the original exception thrown which was handled by the code
                throw translateExceptionFunction(new COMException(errorMessage, (int)this.ErrorCode), errorMessage);
            }
        }
    }
}

#pragma warning restore 1591