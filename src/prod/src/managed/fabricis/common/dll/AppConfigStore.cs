// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Fabric;
    using System.Fabric.Common;
    using System.Linq;
    using Collections.Generic;
    using Description;
    using IO;
    using Query;

    internal sealed class AppConfigStore : IConfigStore
    {
        private readonly object packageLock = new object();
        private readonly CodePackageActivationContext activationCtx;
        private readonly string configPackageName;
        private ConfigurationPackage configPackage;

        public AppConfigStore(CodePackageActivationContext activationCtx, string configPackageName)
        {
            this.activationCtx = activationCtx;
            this.configPackageName = configPackageName;

            activationCtx.ConfigurationPackageModifiedEvent += this.OnConfigurationPackageModified;

            this.RefreshConfigPackage();
        }

        private void OnConfigurationPackageModified(object sender, PackageModifiedEventArgs<ConfigurationPackage> e)
        {
            this.RefreshConfigPackage();
        }

        private void RefreshConfigPackage()
        {
            lock (this.packageLock)
            {
                this.configPackage = this.activationCtx.GetConfigurationPackageObject(this.configPackageName);
            }
        }

        private ConfigurationSettings GetConfigSettings()
        {
            lock (this.packageLock)
            {
                return this.configPackage.Settings;
            }
        }

        public bool IgnoreUpdateFailures
        {
            get; set;
        }

        public ICollection<string> GetKeys(string sectionName, string partialKeyName)
        {
            var parameters = this.GetConfigSettings().Sections[sectionName].Parameters;
            return parameters.Where(p => p.Name.StartsWith(partialKeyName)).Select(p => p.Name).ToArray();
        }

        public ICollection<string> GetSections(string partialSectionName)
        {
            var sections = this.GetConfigSettings().Sections;
            return sections.Where(s => s.Name.StartsWith(partialSectionName)).Select(s => s.Name).ToArray();
        }

        public string ReadString(string sectionName, string keyName, out bool isEncrypted)
        {
            var parameters = this.GetConfigSettings().Sections[sectionName].Parameters;

            if (parameters.Contains(keyName))
            {
                ConfigurationProperty prop = parameters[keyName];
                isEncrypted = prop.IsEncrypted;
                return prop.Value;
            }
            else
            {
                isEncrypted = false;
                return null;
            }
        }

        public string ReadUnencryptedString(string sectionName, string keyName)
        {
            bool isEncrypted;
            string value = this.ReadString(sectionName, keyName, out isEncrypted);

            if (isEncrypted)
            {
                throw new InvalidDataException(
                    "Configuration setting {0}/{1} cannot be read because it is encrypted".ToString(sectionName, keyName));
            }

            return value;
        }
    }
}