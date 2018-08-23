// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Common
{
    internal class FatalException : Exception
    {
        public FatalException(string message)
            : base(message)
        {
        }

        public FatalException(string message, Exception inner)
            : base(message, inner)
        {
            this.HResult = inner.HResult;
        }
    }
}