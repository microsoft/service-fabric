// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Threading.Tasks;

    /// <summary>
    /// Configuration object used to create ReliableStateManager.
    /// </summary>
    public class ReliableStateManagerConfiguration
    {
        private const string DefaultConfigPackageName = "Config";
        private const string DefaultReplicatorSecuritySectionName = "ReplicatorSecurityConfig";
        private const string DefaultReplicatorSettingsSectionName = "ReplicatorConfig";

        /// <summary>
        /// Create a new ReliableStateManagerConfiguration.
        /// </summary>
        /// <param name="configPackageName">Optional config package name from which to load replicator security/settings.</param>
        /// <param name="replicatorSecuritySectionName">Optional config section name from which to load replicator security settings.</param>
        /// <param name="replicatorSettingsSectionName">Optional config section name from which to load replicator settings.</param>
        /// <param name="onInitializeStateSerializersEvent">
        /// Optional callback which will fire when custom serializers should be added.
        /// Used to set the <see cref="OnInitializeStateSerializersEvent"/> property.
        /// </param>
        public ReliableStateManagerConfiguration(
            string configPackageName = DefaultConfigPackageName,
            string replicatorSecuritySectionName = DefaultReplicatorSecuritySectionName,
            string replicatorSettingsSectionName = DefaultReplicatorSettingsSectionName,
            Func<Task> onInitializeStateSerializersEvent = null)
            : this(null, configPackageName, replicatorSecuritySectionName, replicatorSettingsSectionName, onInitializeStateSerializersEvent)
        {
        }

        /// <summary>
        /// Create a new ReliableStateManagerConfiguration.
        /// </summary>
        /// <param name="replicatorSettings">Replicator settings used to initialize the ReliableStateManager.</param>
        /// <param name="onInitializeStateSerializersEvent">
        /// Optional callback which will fire when custom serializers should be added.
        /// Used to set the <see cref="OnInitializeStateSerializersEvent"/> property.
        /// </param>
        public ReliableStateManagerConfiguration(
            ReliableStateManagerReplicatorSettings replicatorSettings,
            Func<Task> onInitializeStateSerializersEvent = null)
            : this(replicatorSettings, null, null, null, onInitializeStateSerializersEvent)
        {
        }

        private ReliableStateManagerConfiguration(
            ReliableStateManagerReplicatorSettings replicatorSettings,
            string configPackageName,
            string replicatorSecuritySectionName,
            string replicatorSettingsSectionName,
            Func<Task> onInitializeStateSerializersEvent)
        {
            this.ReplicatorSettings = replicatorSettings;
            this.ConfigPackageName = configPackageName;
            this.ReplicatorSecuritySectionName = replicatorSecuritySectionName;
            this.ReplicatorSettingsSectionName = replicatorSettingsSectionName;
            this.OnInitializeStateSerializersEvent = onInitializeStateSerializersEvent ?? (() => Task.FromResult(true));
        }

        /// <summary>
        /// Gets or sets the replicator settings.
        /// </summary>
        /// <returns>The replicator settings.</returns>
        public ReliableStateManagerReplicatorSettings ReplicatorSettings { get; private set; }

        /// <summary>
        /// Gets the name of the config package in Settings.xml from which to load replicator settings and replicator
        /// security settings.
        /// </summary>
        /// <returns>The config package name.</returns>
        public string ConfigPackageName { get; private set; }

        /// <summary>
        /// Gets the replicator security settings section name.
        /// </summary>
        /// <returns>The section name.</returns>
        /// <remarks>If present in the config package specified by <see cref="ConfigPackageName"/> in Settings.xml,
        /// this section will be used to configure replicator security settings.</remarks>
        public string ReplicatorSecuritySectionName { get; private set; }

        /// <summary>
        /// Gets the replicator settings section name.
        /// </summary>
        /// <returns>The section name.</returns>
        /// <remarks>If present in the config package specified by <see cref="ConfigPackageName"/> in Settings.xml,
        /// this section will be used to configure replicator settings.</remarks>
        public string ReplicatorSettingsSectionName { get; private set; }

        /// <summary>
        /// Gets the delegate which will be called when custom serializers can be added.  
        /// When called, specify custom serializers via <see cref="IReliableStateManager.TryAddStateSerializer{T}(IStateSerializer{T})"/>
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Func<Task> OnInitializeStateSerializersEvent { get; private set; }
    }
}