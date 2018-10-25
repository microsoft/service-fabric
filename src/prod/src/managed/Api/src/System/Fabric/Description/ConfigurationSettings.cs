// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Collections.ObjectModel;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;

    /// <summary>
    /// <para>Describes the configuration settings.</para>
    /// </summary>
    [SuppressMessage(FxCop.Category.Naming, FxCop.Rule.TypeNamesShouldNotMatchNamespaces, Justification = "The name is kept the same to keep usage similar to System.Configuration.Configuration.")]
    [SuppressMessage(FxCop.Category.Design, FxCop.Rule.NestedTypesShouldNotBeVisible, Justification = "Class is used in the context of the parent")]
    public sealed class ConfigurationSettings
    {
        internal ConfigurationSettings()
        {
            this.Sections = new KeyedItemCollection<string, ConfigurationSection>((s) => s.Name);
        }

        /// <summary>
        /// <para>Gets the name/value pair collection of the sections. </para>
        /// </summary>
        /// <value>
        /// <para>the name/value pair collection of the sections.</para>
        /// </value>
        public KeyedCollection<string, ConfigurationSection> Sections
        {
            get;
            private set;
        }

        internal static unsafe ConfigurationSettings CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_CONFIGURATION_SETTINGS nativeSettings = *(NativeTypes.FABRIC_CONFIGURATION_SETTINGS*)nativeRaw;

            ConfigurationSettings config = new ConfigurationSettings();

            if (nativeSettings.Sections != IntPtr.Zero)
            {
                NativeTypes.FABRIC_CONFIGURATION_SECTION_LIST sectionList = *(NativeTypes.FABRIC_CONFIGURATION_SECTION_LIST*)nativeSettings.Sections;
                for (int i = 0; i < sectionList.Count; i++)
                {
                    config.Sections.Add(ConfigurationSection.CreateFromNative(sectionList.Items + (i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_CONFIGURATION_SECTION)))));
                }
            }

            return config;
        }
    }
}