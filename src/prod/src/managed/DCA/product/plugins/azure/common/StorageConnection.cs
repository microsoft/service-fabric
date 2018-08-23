// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Linq;
    using System.Security;

    // Represents an Azure storage connnection string
    public class StorageConnection
    {
        public bool IsDsms { get; set; }

        public string DsmsSourceLocation { get; set; }

        public string DsmsEndpointSuffix { get; set; }

        public string DsmsAutopilotServiceName { get; set; }

        internal bool IsHttps { get; set;  }

        internal string AccountName { get; set; }

        internal SecureString AccountKey { get; set; }

        internal bool UseDevelopmentStorage { get; set; }

        internal SecureString ConnectionString { get; set; }

        internal Uri BlobEndpoint { get; set; }

        internal Uri QueueEndpoint { get; set; }

        internal Uri TableEndpoint { get; set; }

        internal Uri FileEndpoint { get; set; }

        internal StorageConnection()
        {
        }

        internal StorageConnection(string dsmsSourceLocation, string dsmsEndpointSuffix, string dsmsAutopilotServiceName)
        {
            if (string.IsNullOrWhiteSpace(dsmsSourceLocation))
            {
                throw new ArgumentNullException(nameof(dsmsSourceLocation));
            }

            if (string.IsNullOrWhiteSpace(dsmsEndpointSuffix))
            {
                throw new ArgumentNullException(nameof(dsmsEndpointSuffix));
            }

            // if it is empty means that AutopilotServiceName was
            // specified but its content was empty
            // null is valid and means no AutopilotServiceName specified
            if (dsmsAutopilotServiceName != null && dsmsAutopilotServiceName.Trim() == string.Empty)
            {
                throw new ArgumentNullException(nameof(dsmsAutopilotServiceName));
            }

            this.IsHttps = true;
            this.IsDsms = true;
            this.DsmsSourceLocation = dsmsSourceLocation;
            this.DsmsEndpointSuffix = dsmsEndpointSuffix;
            this.DsmsAutopilotServiceName = dsmsAutopilotServiceName;
            this.AccountName = this.GetDsmsStorageAccountName(dsmsSourceLocation);
        }

        private string GetDsmsStorageAccountName(string path)
        {
            if (string.IsNullOrEmpty(path))
            {
                return null;
            }

            var parts = path.Split('/');
            var accountName = parts.Last();
            return accountName;
        }
    }
}