// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Config
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using ClusterAnalysis.Common.Util;
    using Newtonsoft.Json;

    public sealed class NamedConfigManager
    {
        private IDictionary<string, IDictionary<string, string>> configStore;

        private PropertyInfo[] namedConfigProperties;

        public NamedConfigManager(IDictionary<string, IDictionary<string, string>> config)
        {
            Assert.IsNotNull(config, "config");
            this.configStore = config;
            this.namedConfigProperties = typeof(INamedConfig).GetProperties();
        }

        public static NamedConfigManager FromConfigFile(string configFullPath)
        {
            if (!File.Exists(configFullPath))
            {
                throw new FileNotFoundException(configFullPath);
            }

            var deserializedSimpleConfig = JsonConvert.DeserializeObject<List<SimpleConfig>>(File.ReadAllText(configFullPath));
            return new NamedConfigManager(
                deserializedSimpleConfig.Select(item => new { item.ConfigName, item.ConfigValues })
                    .ToDictionary(item => item.ConfigName, item => item.ConfigValues));
        }

        public TConfig GetConfiguration<TConfig>() where TConfig : INamedConfig, new()
        {
            var configurationObject = (TConfig)Activator.CreateInstance(typeof(TConfig));
            if (!this.configStore.ContainsKey(((INamedConfig)configurationObject).Name))
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "Config for Type: {0} with Name: {1} is Missing",
                        typeof(TConfig),
                        ((INamedConfig)configurationObject).Name));
            }

            var rawValuesForThisConfig = this.configStore[((INamedConfig)configurationObject).Name];

            // Get all Properties for this Config object
            var properties = typeof(TConfig).GetProperties();

            foreach (var oneProperty in properties)
            {
                // We don't want to populate fields from Named Config so we skip them.
                if (this.namedConfigProperties.Any(item => item.Name == oneProperty.Name))
                {
                    continue;
                }

                if (!rawValuesForThisConfig.ContainsKey(oneProperty.Name))
                {
                    throw new KeyNotFoundException(
                        string.Format(CultureInfo.InvariantCulture, "ConfigType: {0}, Prop Value: {1} is Missing", typeof(TConfig), oneProperty.Name));
                }

                oneProperty.SetValue(configurationObject, Convert(oneProperty.PropertyType, rawValuesForThisConfig[oneProperty.Name]), null);
            }

            return configurationObject;
        }

        private static object Convert(Type type, string value)
        {
            if (type == typeof(TimeSpan))
            {
                return TimeSpan.Parse(value);
            }

            if (type == typeof(string))
            {
                return value;
            }

            if (type == typeof(bool))
            {
                return bool.Parse(value);
            }

            if (type == typeof(int))
            {
                return int.Parse(value);
            }

            if (type == typeof(long))
            {
                return long.Parse(value);
            }

            if (type == typeof(uint))
            {
                return uint.Parse(value);
            }

            throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Convertion of Value: {0} to Type: {1} is Not supported", value, type));
        }
    }
}