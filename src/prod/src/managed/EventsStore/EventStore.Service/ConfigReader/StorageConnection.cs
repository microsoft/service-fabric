// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ConfigReader
{
    using System.Security;

    // Represents an Azure storage connnection string
    internal class StorageConnection
    {
        internal string AccountName { get; set; }

        internal SecureString AccountKey { get; set; }

        internal SecureString ConnectionString { get; set; }
    }
}
