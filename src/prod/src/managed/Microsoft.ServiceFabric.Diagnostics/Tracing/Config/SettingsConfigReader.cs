// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Diagnostics.Tracing.Config
{
    using System;
    using System.Fabric.Description;

    internal class SettingsConfigReader
    {
        private const string TraceLevelKey = "Level";
        private const string IncludeEventKey = "IncludeEvent";
        private const string ExcludeEventKey = "ExcludeEvent";
        private const string KeywordsKey = "Keywords";
        private readonly ConfigurationSection configSection;

        internal SettingsConfigReader(ConfigurationSection configSection)
        {
            this.configSection = configSection;
        }

        internal string ReadIncludeEventKey()
        {
            return this.GetValueForParameter(IncludeEventKey);
        }

        internal string ReadExcludeEventKey()
        {
            return this.GetValueForParameter(ExcludeEventKey);
        }

        internal string ReadTraceLevelKey()
        {
            return this.GetValueForParameter(TraceLevelKey);
        }

        internal string ReadKeywordsKey()
        {
            return this.GetValueForParameter(KeywordsKey);
        }

        private string GetValueForParameter(string key)
        {
            if (this.configSection != null && this.configSection.Parameters.Contains(key))
            {
                return this.configSection.Parameters[key].Value;
            }

            return null;
        }
    }
}