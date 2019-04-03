// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.ComponentModel.DataAnnotations;
    using System.Globalization;

    public enum ProtectedAccountKeyName
    {
         StorageAccountKey1,

         StorageAccountKey2,
    }

    public class DiagnosticsStorageAccountConfig
    {
        [Required(AllowEmptyStrings = false)]
        public string StorageAccountName { get; set; }

        public string PrimaryAccessKey { get; set; }

        public string SecondaryAccessKey { get; set; }

        public string ProtectedAccountKeyName { get; set; }

        public string BlobEndpoint { get; set; }

        public string QueueEndpoint { get; set; }

        public string TableEndpoint { get; set; }
        
        public DiagnosticsStorageAccountConfig Clone()
        {
            return new DiagnosticsStorageAccountConfig
            {
                StorageAccountName = string.Copy(this.StorageAccountName ?? string.Empty),

                PrimaryAccessKey = string.Copy(this.PrimaryAccessKey ?? string.Empty),

                SecondaryAccessKey = string.Copy(this.SecondaryAccessKey ?? string.Empty),

                ProtectedAccountKeyName = string.Copy(this.ProtectedAccountKeyName ?? string.Empty),

                BlobEndpoint = string.Copy(this.BlobEndpoint ?? string.Empty),

                QueueEndpoint = string.Copy(this.QueueEndpoint ?? string.Empty),

                TableEndpoint = string.Copy(this.TableEndpoint ?? string.Empty),
            };
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "DiagnosticsStorageAccountConfig [ StorageAccountName = {0}, ProtectedAccountKeyName = {1}, BlobEndpoint = {2}, QueueEndpoint = {3}, TableEndpoint = {4} ] ",
                this.StorageAccountName,
                this.ProtectedAccountKeyName,
                this.BlobEndpoint,
                this.QueueEndpoint,
                this.TableEndpoint);
        }
    }
}