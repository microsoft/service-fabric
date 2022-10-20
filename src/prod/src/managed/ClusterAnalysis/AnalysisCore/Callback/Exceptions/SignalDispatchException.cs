// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Callback.Exceptions
{
    using System;

    public sealed class SignalDispatchException : Exception
    {
        public SignalDispatchException(string message) : base(message)
        {
        }

        public SignalDispatchException(string message, Exception innerException) : base(message, innerException)
        {
        }
    }
}