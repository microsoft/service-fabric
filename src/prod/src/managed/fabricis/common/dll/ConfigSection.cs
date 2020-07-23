// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Common;    
    using Globalization;
    using Reflection;

    /// <summary>
    /// A helper class for handling a config section from the config store.    
    /// </summary>
    internal class ConfigSection : IConfigSection
    {
        private readonly TraceType traceType;

        public ConfigSection(TraceType traceType, IConfigStore configStore, string configSectionName)
        {
            this.traceType = traceType.Validate("traceType");
            this.ConfigStore = configStore.Validate("configStore");
            this.Name = configSectionName.Validate("configSectionName");
        }

        private IConfigStore ConfigStore { get; set; }

        public string Name { get; private set; }

        public bool SectionExists
        {
            get
            {
                return this.ConfigStore.GetSections(this.Name).Contains(this.Name);
            }
        }

        /// <summary>
        /// Reads the configuration value.
        /// </summary>
        /// <typeparam name="T">The type of the converted result.</typeparam>
        /// <param name="keyName">Name of the key.</param>
        /// <param name="defaultValue">The default value to be used if read was not successful.</param>
        /// <returns>The configuration value or the provided default value.</returns>
        public T ReadConfigValue<T>(
            string keyName,
            T defaultValue = default(T))
        {
            keyName.Validate("keyName");

            T result = defaultValue;

            string configValue = this.ConfigStore.ReadUnencryptedString(this.Name, keyName);

            if (string.IsNullOrEmpty(configValue))
            {
                return result;
            }
            
            Exception ex = null;

            try
            {
                if (typeof(T).GetTypeInfo().IsEnum)
                {
                    result = (T)Enum.Parse(typeof(T), configValue, true);
                }
                else if (typeof(T) == typeof(TimeSpan))
                {
                    result = (T)(object)TimeSpan.Parse(configValue);
                }
                else
                {
                    result = (T)Convert.ChangeType(configValue, typeof(T), CultureInfo.InvariantCulture);
                }
            }
            catch (ArgumentException e)
            {
                ex = e;
            }
            catch (InvalidCastException e)
            {
                ex = e;
            }
            catch (FormatException e)
            {
                ex = e;
            }
            catch (OverflowException e)
            {
                ex = e;
            }

            if (ex != null)
            {
                traceType.WriteWarning(                        
                    "Error converting value '{0}' from config section/key name '{1}/{2}'. Returning default value: {3}. Error details: {4}",
                    configValue,
                    this.Name,
                    keyName,
                    result,
                    ex);
            }
            else
            {
                traceType.WriteNoise(                        
                    "Converted value '{0}' from config section/key name '{1}/{2}'. Returning converted value: {3}",
                    configValue,
                    this.Name,
                    keyName,
                    result);
            }
            
            return result;
        }
    }
}