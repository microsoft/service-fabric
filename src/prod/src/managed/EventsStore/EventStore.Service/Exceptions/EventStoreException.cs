// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.Exceptions
{
    using System;

    internal class EventStoreException : Exception
    {
        public int ErrorCode { get; }

        public EventStoreException(int errorCode, string message) : base(message)
        {
            this.ErrorCode = errorCode;
        }
    }
}