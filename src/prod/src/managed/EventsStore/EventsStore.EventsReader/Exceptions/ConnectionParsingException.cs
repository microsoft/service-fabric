// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.Exceptions
{
    internal class ConnectionParsingException : EventStoreException
    {
        public ConnectionParsingException(int errorCode, string message) : base(errorCode, message)
        {
        }
    }
}
