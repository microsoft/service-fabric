// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    internal class EtlToCsvFileWriterConfigReader : IEtlToCsvFileWriterConfigReader
    {
        private const string DisableDtrCompressionValueName = "DisableDtrCompression";
        private const bool DisableDtrCompressionByDefault = false;

        public bool IsDtrCompressionDisabledGlobally()
        {
            return Utility.GetUnencryptedConfigValue(
                        ConfigReader.DiagnosticsSectionName,
                        DisableDtrCompressionValueName,
                        DisableDtrCompressionByDefault);
        }

        public TimeSpan GetDtrDeletionAge()
        {
            return TimeSpan.FromMinutes(Utility.GetUnencryptedConfigValue(
                ConfigReader.DiagnosticsSectionName,
                Utility.TestOnlyDtrDeletionAgeMinutesValueName,
                (int)TimeSpan.FromDays(1).TotalMinutes));
        }
    }
}