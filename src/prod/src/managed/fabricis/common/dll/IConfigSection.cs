// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Common;    

    /// <summary>
    /// A wrapper interface for handling a config section in a config store.
    /// </summary>
    internal interface IConfigSection
    {
        /// <summary>
        /// The config section name.
        /// </summary>
        string Name { get; }

        bool SectionExists { get; }

        /// <summary>
        /// Reads the configuration value.
        /// </summary>
        /// <typeparam name="T">The type of the converted result.</typeparam>
        /// <param name="keyName">Name of the key.</param>
        /// <param name="defaultValue">The default value to be used if read was not successful.</param>
        /// <returns>The configuration value or the provided default value.</returns>
        T ReadConfigValue<T>(
            string keyName,
            T defaultValue = default(T));
    }
}