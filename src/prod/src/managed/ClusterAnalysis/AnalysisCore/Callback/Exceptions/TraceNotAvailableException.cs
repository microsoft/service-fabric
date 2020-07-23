// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Callback.Exceptions
{
    using System;

    public sealed class TraceNotAvailableException : Exception
    {
        public TraceNotAvailableException(string message) : base(message)
        {
        }

        public TraceNotAvailableException(string message, Exception exception) : base(message, exception)
        {
        }
    }
}