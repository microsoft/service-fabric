// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Exceptions
{
    using System;
    using System.Runtime.Serialization;

    public class SignalAlreadyPresentException : Exception
    {
        public SignalAlreadyPresentException()
        {
        }

        public SignalAlreadyPresentException(string message) : base(message)
        {
        }

        public SignalAlreadyPresentException(string message, Exception innerException) : base(message, innerException)
        {
        }

        protected SignalAlreadyPresentException(SerializationInfo info, StreamingContext context) : base(info, context)
        {
        }
    }
}