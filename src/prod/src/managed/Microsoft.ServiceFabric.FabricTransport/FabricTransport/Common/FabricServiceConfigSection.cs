// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Linq;
    using System.Reflection;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Description;

    internal class FabricServiceConfigSection
    {
        private static readonly string TraceType = "FabricServiceConfigSection";
        private readonly Action onInitialize;
        private readonly string sectionName;
        private SettingsTypeSection exeSection;
        private ConfigurationSection configSection;

        public FabricServiceConfigSection(string sectionName, Action onInitialize)
        {
            this.sectionName = sectionName;
            this.onInitialize = onInitialize;
        }

        public bool Initialize()
        {
            var config = FabricServiceConfig.GetConfig();
            //File is not Present
            if (config == null || (IsExeSettingsFileEmpty(config) && DoesConfigurationSectionExist(config)))
            {
                AppTrace.TraceSource.WriteInfo(
                    TraceType,
                    "FabricServiceConfig instance is null .This can happen if Settings File is not Present");
                return false;
            }


            if (config.configurationSettings != null)
            {
                if (config.configurationSettings.Sections.Contains(this.sectionName))
                {
                    this.configSection = config.configurationSettings.Sections[this.sectionName];
                    this.onInitialize();
                    return true;
                }
                return false;
            }

            foreach (var section in config.Settings.Section)
            {
                if (section.Name.Equals(this.sectionName.Trim()))
                {
                    this.exeSection = section;
                    this.onInitialize();
                    return true;
                }
            }
            AppTrace.TraceSource.WriteInfo(
                TraceType,
                "Section :{0} is not present in FabricServiceConfig ",
                this.sectionName);
            return false;
        }

        private static bool DoesConfigurationSectionExist(FabricServiceConfig config)
        {
            return config.configurationSettings == null;
        }

        private static bool IsExeSettingsFileEmpty(FabricServiceConfig config)
        {
            return ((config.Settings == null) || config.Settings.Section == null);
        }

        public T GetSetting<T>(string settingName, T defaultValue)
        {
            if (this.configSection != null)
            {
                if (this.configSection.Parameters.Contains(settingName.Trim()))
                {
                    var param= this.configSection.Parameters[settingName.Trim()];
                    return CastParameter<T>(param.Value);
                }

            }
            else
            {
                foreach (var param in this.exeSection.Parameter)
                {
                    if (param.Name.Equals(settingName.Trim()))
                    {
                        return CastParameter<T>(param.Value);
                    }
                }

            }


            AppTrace.TraceSource.WriteInfo(
                TraceType,
                "Settings : {0} for Section : {1} is not present in FabricServiceConfig,so  using defaultValue",
                settingName,
                this.sectionName);
            return defaultValue;
        }

        private static T CastParameter<T>(string param)
        {
            if (typeof(T).GetTypeInfo().IsEnum)
            {
                return (T)Enum.Parse(typeof(T), param);
            }
            return (T)Convert.ChangeType(param, typeof(T));
        }

        public IList<T> GetSettingsList<T>(string settingName)
        {
            if (this.configSection != null)
            {
                if (this.configSection.Parameters.Contains(settingName.Trim()))
                {
                    var param = this.configSection.Parameters[settingName.Trim()];
                    return CastParameterAsList<T>(param.Value);
                }
            }
            else
            {
                foreach (var param in this.exeSection.Parameter)
                {
                    if (param.Name.Equals(settingName.Trim()))
                    {
                        return CastParameterAsList<T>(param.Value);
                    }
                }

            }


            AppTrace.TraceSource.WriteInfo(
                TraceType,
                "SettingsList : {0} for Section : {1} is not present in FabricServiceConfig",
                settingName,
                this.sectionName);

            return new List<T>();
        }

        private static IList<T> CastParameterAsList<T>(string param)
        {
            var array = param.Split(',').Select(s => (T)Convert.ChangeType(s, typeof(T)));
            return array.ToList<T>();
        }

        public Dictionary<string, string> GetSettingsMapFromPrefix(string settingPrefix)
        {

            var map = new Dictionary<string, string>();
            if (this.configSection != null)
            {
                var fullMatchingKeys =
                    this.configSection.Parameters.Where(currentKey => currentKey.Name.StartsWith(settingPrefix));
                foreach (var param in fullMatchingKeys)
                {
                    map.Add(param.Name.Substring(settingPrefix.Length), param.Value);
                }
            }
            else
            {
                foreach (var param in this.exeSection.Parameter)
                {
                    if (param.Name.StartsWith(settingPrefix))
                    {
                        map.Add(param.Name.Substring(settingPrefix.Length), param.Value);
                    }
                }

            }

            if (map.Count == 0)
            {
                AppTrace.TraceSource.WriteInfo(
                    TraceType,
                    "SettingsMap for Prefix : {0} for Section : {1} is not present in FabricServiceConfig",
                    settingPrefix,
                    this.sectionName);
            }

            return map;
        }
    }
}