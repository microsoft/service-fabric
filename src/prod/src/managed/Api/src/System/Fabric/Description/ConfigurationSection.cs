// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.ObjectModel;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Runtime.InteropServices;

    /// <summary>
    /// <para>Specifies a named collection of configuration properties.</para>
    /// </summary>
    public sealed class ConfigurationSection
    {
        internal ConfigurationSection()
        {
            this.Parameters = new KeyedItemCollection<string, ConfigurationProperty>(p => p.Name);
        }

        /// <summary>
        /// <para> The name of the section. </para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        public string Name { get; internal set; }

        /// <summary>
        /// <para>The key/value pair of a configuration property. </para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Collections.ObjectModel.KeyedCollection{TKey, TItem}" />.</para>
        /// </value>
        public KeyedCollection<string, ConfigurationProperty> Parameters
        {
            get;
            private set;
        }

        internal static unsafe ConfigurationSection CreateFromNative(IntPtr nativeSectionRaw)
        {
            NativeTypes.FABRIC_CONFIGURATION_SECTION nativeSection = *(NativeTypes.FABRIC_CONFIGURATION_SECTION*)nativeSectionRaw;

            ConfigurationSection section = new ConfigurationSection();
            string name = NativeTypes.FromNativeString(nativeSection.Name);

            if (string.IsNullOrEmpty(name))
            {
                throw new ArgumentException(
                    StringResources.Error_ConfigSectionNameNullOrEmpty,
                    "nativeSectionRaw");
            }

            section.Name = name;

            if (nativeSection.Parameters != IntPtr.Zero)
            {
                NativeTypes.FABRIC_CONFIGURATION_PARAMETER_LIST nativeSectionList = *(NativeTypes.FABRIC_CONFIGURATION_PARAMETER_LIST*)nativeSection.Parameters;
                for (int i = 0; i < nativeSectionList.Count; i++)
                {
                    section.Parameters.Add(ConfigurationProperty.CreateFromNative(nativeSectionList.Items + (i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_CONFIGURATION_PARAMETER)))));
                }
            }

            return section;
        }
    }
}