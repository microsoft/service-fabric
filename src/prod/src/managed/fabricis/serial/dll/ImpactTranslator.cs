// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Fabric.Description;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using IImpactDetail = Microsoft.WindowsAzure.ServiceRuntime.Management.Public.IImpactDetail;
using ResourceImpactEnum = Microsoft.WindowsAzure.ServiceRuntime.Management.Public.ResourceImpactEnum;

namespace System.Fabric.InfrastructureService
{
    /// <summary>
    /// Translates MR job impact from <see cref="IImpactDetail"/> to <see cref="NodeTask"/>
    /// based on a set of configurable policies.
    /// </summary>
    internal class ImpactTranslator
    {
        /// <summary>
        /// Summary of the overall impact to the node.
        /// </summary>
        private enum ImpactCategory
        {
            /// <summary>
            /// No observable impact.
            /// </summary>
            NoImpact = 0,

            /// <summary>
            /// Execution of the VM freezes, or the network is unavailable.
            /// </summary>
            Freeze = 1,

            /// <summary>
            /// Restart of the OS, Service Fabric, or the application.
            /// </summary>
            Restart = 2,

            /// <summary>
            /// Data stored on the VM's disks will be destroyed.
            /// </summary>
            DataDestructive = 3,

            /// <summary>
            /// Do not process this impact automatically.
            /// </summary>
            Unhandled = 4,
        }

        /// <summary>
        /// Default mappings from [ResourceType].[ResourceImpactEnum] to [ImpactCategory]
        /// </summary>
        private static readonly IReadOnlyDictionary<string, ImpactCategory> DefaultResourceImpactMap = new ReadOnlyDictionary<string, ImpactCategory>(
            new Dictionary<string, ImpactCategory>(StringComparer.OrdinalIgnoreCase)
            {
                { "Compute.None", ImpactCategory.NoImpact },
                { "Compute.Freeze", ImpactCategory.Freeze },
                { "Compute.Reset", ImpactCategory.Restart },
                { "Compute.Unknown", ImpactCategory.DataDestructive },

                { "Disk.None", ImpactCategory.NoImpact },
                { "Disk.Freeze", ImpactCategory.Freeze },
                { "Disk.Reset", ImpactCategory.DataDestructive },
                { "Disk.Unknown", ImpactCategory.DataDestructive },

                { "Network.None", ImpactCategory.NoImpact },
                { "Network.Freeze", ImpactCategory.Freeze },
                { "Network.Reset", ImpactCategory.Restart },
                { "Network.Unknown", ImpactCategory.DataDestructive },

                { "OS.None", ImpactCategory.NoImpact },
                { "OS.Freeze", ImpactCategory.Freeze },
                { "OS.Reset", ImpactCategory.Restart },
                { "OS.Unknown", ImpactCategory.DataDestructive },
            });

        /// <summary>
        /// Enables or disables processing of IImpactDetail for a particular job type.
        /// Default is false.
        /// </summary>
        /// <example>
        /// WindowsAzure.SerialJobImpact.UseImpactDetail.PlatformUpdateJob=true
        /// </example>
        internal const string ConfigKeyFormatUseImpactDetail = "WindowsAzure.SerialJobImpact.UseImpactDetail.{0}"; // 0 = JobType

        /// <summary>
        /// Sets the mapping of a particular resource impact.
        /// Default: see DefaultResourceImpactMap.
        /// </summary>
        /// <example>
        /// WindowsAzure.SerialJobImpact.ResourceMap.Compute.Reset=DataDestructive
        /// </example>
        internal const string ConfigKeyFormatResourceImpactMap = "WindowsAzure.SerialJobImpact.ResourceMap.{0}.{1}"; // 0 = Resource type; 1 = ResourceImpactEnum

        /// <summary>
        /// Determines which ImpactCategory should actually be used if the impact is determined to be
        /// ImpactCategory.Freeze but the EstimatedImpactDurationInSeconds is unknown (-1).
        /// Default is Restart.
        /// </summary>
        /// <example>
        /// WindowsAzure.SerialJobImpact.UnknownDurationFreezeMapping=Restart
        /// </example>
        internal const string ConfigKeyUnknownDurationFreezeMapping = "WindowsAzure.SerialJobImpact.UnknownDurationFreezeMapping";

        /// <summary>
        /// Determines the EstimatedImpactDurationInSeconds at which any impact category (above NoImpact)
        /// is escalated to DataDestructive.
        /// Default is 7200 sec (2 hours).
        /// </summary>
        /// <example>
        /// WindowsAzure.SerialJobImpact.DataDestructiveDurationThreshold=3600
        /// </example>
        internal const string ConfigKeyDataDestructiveDurationThreshold = "WindowsAzure.SerialJobImpact.DataDestructiveDurationThreshold";
        private const int DefaultDataDestructiveDurationThreshold = 2 * 60 * 60; // impact of >= 2 hours is data destructive

        /// <summary>
        /// Determines the EstimatedImpactDurationInSeconds at which any impact category (above NoImpact)
        /// is escalated to Restart.
        /// Default is 0 sec.
        /// </summary>
        /// <example>
        /// WindowsAzure.SerialJobImpact.RestartDurationThreshold=10
        /// </example>
        internal const string ConfigKeyRestartDurationThreshold = "WindowsAzure.SerialJobImpact.RestartDurationThreshold";
        private const int DefaultRestartDurationThreshold = int.MinValue; // an impact of any duration is a restart

        /// <summary>
        /// Determines if impact detail processing is enabled by default.
        /// </summary>
        private const bool DefaultUseImpactDetail = false;

        private static readonly TraceType Tracer = new TraceType("AzureImpactTranslator");
        private readonly IConfigSection configSection;

        public ImpactTranslator(IConfigSection configSection)
        {
            configSection.Validate("configSection");
            this.configSection = configSection;
        }

        /// <summary>
        /// Determines whether the ImpactTranslator should be used for the given job type.
        /// </summary>
        /// <param name="jobType">The job type.</param>
        /// <returns>true if the ImpactTranslator should be used for this job type, false otherwise.</returns>
        public bool UseImpactDetail(JobType jobType)
        {
            string configKey = String.Format(CultureInfo.InvariantCulture, ConfigKeyFormatUseImpactDetail, jobType);
            return this.configSection.ReadConfigValue(configKey, DefaultUseImpactDetail);
        }

        /// <summary>
        /// Translates the impact details to a CM node task.
        /// </summary>
        /// <param name="impactDetail">The impact details to be translated.</param>
        /// <returns>The node task to be sent to the CM.</returns>
        public NodeTask TranslateImpactDetailToNodeTask(IImpactDetail impactDetail)
        {
            impactDetail.Validate("impactDetail");
            ImpactCategory category = TranslateImpactDetailToCategory(impactDetail);
            return TranslateImpactCategoryToNodeTask(category);
        }

        /// <summary>
        /// Gets the key used for lookups in DefaultResourceImpactMap.
        /// </summary>
        private static string GetDefaultResourceImpactMapKey(string resourceType, ResourceImpactEnum impact)
        {
            return string.Format(CultureInfo.InvariantCulture, "{0}.{1}", resourceType, impact);
        }

        /// <summary>
        /// Translates the ResourceImpactEnum for a single resource type into ImpactCategory,
        /// based on configuration or DefaultResourceImpactMap.
        /// </summary>
        private ImpactCategory TranslateResourceImpactToCategory(
            string resourceType,
            ResourceImpactEnum impact)
        {
            // Look up the default category in the map
            ImpactCategory defaultCategory;
            if (!DefaultResourceImpactMap.TryGetValue(GetDefaultResourceImpactMapKey(resourceType, impact), out defaultCategory))
            {
                // The default map is expected to contain a mapping for Unknown; fall back to that
                defaultCategory = DefaultResourceImpactMap[GetDefaultResourceImpactMapKey(resourceType, ResourceImpactEnum.Unknown)];
            }

            string configKey = String.Format(CultureInfo.InvariantCulture, ConfigKeyFormatResourceImpactMap, resourceType, impact);
            return this.configSection.ReadConfigValue(configKey, defaultCategory);
        }

        /// <summary>
        /// Returns the more severe of the two impact categories (like Math.Max for ImpactCategory).
        /// </summary>
        /// <returns></returns>
        private static ImpactCategory EscalateImpactCategory(ImpactCategory oldCategory, ImpactCategory newCategory)
        {
            if (oldCategory < newCategory)
            {
                return newCategory;
            }
            else
            {
                return oldCategory;
            }
        }

        /// <summary>
        /// Translates IImpactDetail to an ImpactCategory.
        /// </summary>
        private ImpactCategory TranslateImpactDetailToCategory(IImpactDetail impactDetail)
        {
            ImpactCategory currentCategory = ImpactCategory.NoImpact;

            // Translate each resource
            currentCategory = ApplyResourceImpact(currentCategory, "Compute", impactDetail.ComputeImpact);
            currentCategory = ApplyResourceImpact(currentCategory, "Disk", impactDetail.DiskImpact);
            currentCategory = ApplyResourceImpact(currentCategory, "Network", impactDetail.NetworkImpact);
            currentCategory = ApplyResourceImpact(currentCategory, "OS", impactDetail.OsImpact);

            // Freeze impact of unknown duration should be escalated to a restart
            currentCategory = ApplyUnknownDurationFreezeRule(currentCategory, impactDetail);

            // If there is any impact, escalate based on estimated impact duration
            if (currentCategory != ImpactCategory.NoImpact)
            {
                currentCategory = ApplyImpactDurationThreshold(
                    currentCategory,
                    impactDetail,
                    ConfigKeyDataDestructiveDurationThreshold,
                    DefaultDataDestructiveDurationThreshold,
                    ImpactCategory.DataDestructive);

                currentCategory = ApplyImpactDurationThreshold(
                    currentCategory,
                    impactDetail,
                    ConfigKeyRestartDurationThreshold,
                    DefaultRestartDurationThreshold,
                    ImpactCategory.Restart);
            }

            return currentCategory;
        }

        /// <summary>
        /// Escalates the impact category based on the impact to a particular resource type.
        /// </summary>
        /// <param name="oldCategory">The current impact category.</param>
        /// <param name="resourceType">The resource type name.</param>
        /// <param name="impact">The impact to the given resource.</param>
        /// <returns>The new impact category.</returns>
        private ImpactCategory ApplyResourceImpact(
            ImpactCategory oldCategory,
            string resourceType,
            ResourceImpactEnum impact)
        {
            ImpactCategory newCategory = TranslateResourceImpactToCategory(resourceType, impact);
            newCategory = EscalateImpactCategory(oldCategory, newCategory);

            if (oldCategory != newCategory)
            {
                Tracer.WriteNoise(
                    "Overall impact changed from {0} to {1} due to resource impact {2}={3}",
                    oldCategory,
                    newCategory,
                    resourceType,
                    impact);
            }

            return newCategory;
        }

        /// <summary>
        /// Escalates a freeze of unknown duration, according to configuration.
        /// </summary>
        /// <param name="oldCategory">The current impact category.</param>
        /// <param name="impactDetail">The impact detail.</param>
        /// <returns>The new impact category.</returns>
        private ImpactCategory ApplyUnknownDurationFreezeRule(
            ImpactCategory oldCategory,
            IImpactDetail impactDetail)
        {
            ImpactCategory newCategory = oldCategory;

            if ((oldCategory == ImpactCategory.Freeze) && (impactDetail.EstimatedImpactDurationInSeconds < 0))
            {
                newCategory = this.configSection.ReadConfigValue(ConfigKeyUnknownDurationFreezeMapping, ImpactCategory.Restart);

                if (oldCategory != newCategory)
                {
                    Tracer.WriteNoise(
                        "Overall impact changed from {0} to {1} due to freeze of unknown duration",
                        oldCategory,
                        newCategory);
                }
            }

            return newCategory;
        }

        /// <summary>
        /// Escalates the impact category if the estimated impact duration equals or exceeds the given threshold.
        /// </summary>
        /// <param name="oldCategory">The current impact category.</param>
        /// <param name="impactDetail">The impact detail.</param>
        /// <param name="configKeyName">The key of the configuration setting containing the duration threshold value.</param>
        /// <param name="defaultThresholdInSeconds">The default duration threshold value.</param>
        /// <param name="escalateToCategory">The category to escalate to if the threshold is exceeded.</param>
        /// <returns>The new impact category.</returns>
        private ImpactCategory ApplyImpactDurationThreshold(
            ImpactCategory oldCategory,
            IImpactDetail impactDetail,
            string configKeyName,
            int defaultThresholdInSeconds,
            ImpactCategory escalateToCategory)
        {
            ImpactCategory newCategory = oldCategory;

            int thresholdInSeconds = this.configSection.ReadConfigValue(configKeyName, defaultThresholdInSeconds);
            if (impactDetail.EstimatedImpactDurationInSeconds >= thresholdInSeconds)
            {
                newCategory = EscalateImpactCategory(oldCategory, escalateToCategory);

                if (oldCategory != newCategory)
                {
                    Tracer.WriteNoise(
                        "Overall impact changed from {0} to {1} due to impact duration of {2} sec (threshold = {3} sec)",
                        oldCategory,
                        newCategory,
                        impactDetail.EstimatedImpactDurationInSeconds,
                        thresholdInSeconds);
                }
            }

            return newCategory;
        }

        private static NodeTask TranslateImpactCategoryToNodeTask(ImpactCategory category)
        {
            switch (category)
            {
                case ImpactCategory.NoImpact:
                case ImpactCategory.Freeze:
                    return NodeTask.Invalid;

                case ImpactCategory.Restart:
                    return NodeTask.Restart;

                case ImpactCategory.DataDestructive:
                    return NodeTask.Relocate;
            }

            throw new InvalidOperationException(String.Format(CultureInfo.InvariantCulture, "Unknown impact category: {0}", category));
        }
    }
}