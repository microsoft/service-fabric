// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.TraceAccessLayer.Exceptions
{
    using System;
    using System.Runtime.Serialization;

    /// <summary>
    /// Exception when a filter condition is already present
    /// </summary>
    public class FilterConditionAlreadyExist : Exception
    {
        public FilterConditionAlreadyExist(string message) : base(message)
        {
        }

        public FilterConditionAlreadyExist(string message, Exception innerException) : base(message, innerException)
        {
        }

        protected FilterConditionAlreadyExist(SerializationInfo info, StreamingContext context) : base(info, context)
        {
        }
    }
}