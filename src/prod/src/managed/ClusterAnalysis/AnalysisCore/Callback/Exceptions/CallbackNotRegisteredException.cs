// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Callback.Exceptions
{
    using System;

    public class CallbackNotRegisteredException : Exception
    {
        public CallbackNotRegisteredException(string message) : base(message)
        {
        }

        public CallbackNotRegisteredException(string message, Exception innerException) : base(message, innerException)
        {
        }
    }
}