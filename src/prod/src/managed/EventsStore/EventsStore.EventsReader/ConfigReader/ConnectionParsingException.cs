// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.ConfigReader
{
    using System;

    internal class ConnectionParsingException : Exception
    {
        public ConnectionParsingException(string message) : base(message) { }

        public ConnectionParsingException(string message, Exception exception) : base(message, exception) { }
    }
}
