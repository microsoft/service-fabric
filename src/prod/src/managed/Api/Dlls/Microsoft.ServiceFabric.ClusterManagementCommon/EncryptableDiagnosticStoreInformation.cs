// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;

    internal class EncryptableDiagnosticStoreInformation
    {
        [JsonConverter(typeof(StringEnumConverter))]
        public DiagnosticStoreType StoreType { get; set; }

        public int DataDeletionAgeInDays { get; set; }

        public string Connectionstring { get; set; }

        public bool IsEncrypted { get; set; }

        internal void Verify()
        {
            if (this.StoreType != DiagnosticStoreType.FileShare
                    && this.StoreType != DiagnosticStoreType.AzureStorage)
            {
                throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_BPAUnsupportedStoreType);
            }

            this.Connectionstring.ThrowValidationExceptionIfNullOrEmpty("Properties/DiagnosticsStore/ConnectionString");
        }
    }
}