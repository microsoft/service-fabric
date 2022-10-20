// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Callback.Exceptions
{
    using System;

    public sealed class CallbackStoreBookmarkMissingException : Exception
    {
        public CallbackStoreBookmarkMissingException(string message) : base(message)
        {
        }

        public CallbackStoreBookmarkMissingException(string message, Exception innerException) : base(message, innerException)
        {
        }
    }
}