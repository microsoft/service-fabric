// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader
{
    /// <summary>
    /// Writer for the output of EventsReaderExe.
    /// </summary>
    internal interface IResultWriter
    {
        void Write(string serializedData);
    }
}