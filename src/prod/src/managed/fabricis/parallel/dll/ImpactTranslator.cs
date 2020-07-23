// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Fabric.Repair;
using System.Globalization;
using RD.Fabric.PolicyAgent;

namespace System.Fabric.InfrastructureService.Parallel
{
    /// <summary>
    /// Translates MR job impact from <see cref="AffectedResourceImpact"/> to <see cref="NodeImpactLevel"/>
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
            /// The VM is being removed from the topology of the cluster.
            /// </summary>
            Decommission = 4,

            /// <summary>
            /// Do not process this impact automatically.
            /// </summary>
            Unhandled = 5,
        }

        /// <summary>
        /// Default mappings from [ResourceType].[Impact] to [ImpactCategory]
        /// </summary>
        private static readonly IReadOnlyDictionary<string, ImpactCategory> DefaultResourceImpactMap = new ReadOnlyDictionary<string, ImpactCategory>(
            new Dictionary<string, ImpactCategory>(StringComparer.OrdinalIgnoreCase)
            {
                { "Compute.None", ImpactCategory.NoImpact },
                { "Compute.Freeze", ImpactCategory.Freeze },
                { "Compute.Reset", ImpactCategory.Restart },
                { "Compute.Initialize", ImpactCategory.NoImpact }, // TODO: Review new impact
                { "Compute.Wipe", ImpactCategory.DataDestructive }, // TODO: Review new impact

                { "Disk.None", ImpactCategory.NoImpact },
                { "Disk.Freeze", ImpactCategory.Freeze },
                { "Disk.Reset", ImpactCategory.DataDestructive },
                { "Disk.Initialize", ImpactCategory.NoImpact }, // TODO: Review new impact
                { "Disk.Wipe", ImpactCategory.DataDestructive }, // TODO: Review new impact

                { "Network.None", ImpactCategory.NoImpact },
                { "Network.Freeze", ImpactCategory.Freeze },
                { "Network.Reset", ImpactCategory.Restart },
                { "Network.Initialize", ImpactCategory.NoImpact }, // TODO: Review new impact
                { "Network.Wipe", ImpactCategory.DataDestructive }, // TODO: Review new impact

                { "OS.None", ImpactCategory.NoImpact },
                { "OS.Freeze", ImpactCategory.Freeze },
                { "OS.Reset", ImpactCategory.Restart },
                { "OS.Initialize", ImpactCategory.NoImpact }, // TODO: Review new impact
                { "OS.Wipe", ImpactCategory.DataDestructive }, // TODO: Review new impact

                { "ApplicationConfig.None", ImpactCategory.NoImpact },
                { "ApplicationConfig.Freeze", ImpactCategory.NoImpact }, // TODO: Review new impact
                { "ApplicationConfig.Reset", ImpactCategory.NoImpact }, // TODO: Review new impact - this is an actual config update
                { "ApplicationConfig.Initialize", ImpactCategory.NoImpact }, // TODO: Review new impact
                { "ApplicationConfig.Wipe", ImpactCategory.DataDestructive }, // TODO: Review new impact
            });

        private const string ConfigKeyPrefix = Constants.ConfigKeys.ConfigKeyPrefix + "ImpactTranslator.";

        /// <summary>
        /// Sets the mapping of a particular resource impact.
        /// Default: see DefaultResourceImpactMap.
        /// </summary>
        /// <example>
        /// Azure.ImpactTranslator.ResourceMap.Compute.Reset=DataDestructive
        /// </example>
        internal const string ConfigKeyFormatResourceImpactMap = ConfigKeyPrefix + "ResourceMap.{0}.{1}"; // 0 = Resource type; 1 = ResourceImpactEnum

        /// <summary>
        /// Determines which ImpactCategory should actually be used if the impact is determined to be
        /// ImpactCategory.Freeze but the EstimatedImpactDurationInSeconds is unknown (-1).
        /// Default is Restart.
        /// </summary>
        /// <example>
        /// Azure.ImpactTranslator.UnknownDurationFreezeMapping=Restart
        /// </example>
        internal const string ConfigKeyUnknownDurationFreezeMapping = ConfigKeyPrefix + "UnknownDurationFreezeMapping";

        /// <summary>
        /// Determines the EstimatedImpactDurationInSeconds at which any impact category (above NoImpact)
        /// is escalated to DataDestructive.
        /// Default is 604800 sec (7 days), so this feature is effectively disabled by default.
        /// </summary>
        /// <example>
        /// Azure.ImpactTranslator.DataDestructiveDurationThreshold=7200
        /// </example>
        internal const string ConfigKeyDataDestructiveDurationThreshold = ConfigKeyPrefix + "DataDestructiveDurationThreshold";
        private const int DefaultDataDestructiveDurationThreshold = 604800 ; // impact of >= 7 days is data destructive

        /// <summary>
        /// Determines the EstimatedImpactDurationInSeconds at which any impact category (above NoImpact)
        /// is escalated to Restart.
        /// Default is 10 sec.
        /// </summary>
        /// <example>
        /// Azure.ImpactTranslator.RestartDurationThreshold=10
        /// </example>
        internal const string ConfigKeyRestartDurationThreshold = ConfigKeyPrefix + "RestartDurationThreshold";
        private const int DefaultRestartDurationThreshold = 10; // an impact of >= 10 seconds is a restart

        private readonly TraceType tracer;
        private readonly IConfigSection configSection;

        public ImpactTranslator(CoordinatorEnvironment environment)
        {
            environment.Validate("environment");
            this.configSection = environment.Config;
            this.tracer = environment.CreateTraceType("ImpactTranslator");
        }

        /// <summary>
        /// Translates the impact details to an RM node impact level.
        /// </summary>
        /// <param name="impactDetail">The impact details to be translated.</param>
        /// <returns>The node impact level to be sent to the RM.</returns>
        public NodeImpactLevel TranslateImpactDetailToNodeImpactLevel(ImpactActionEnum jobType, AffectedResourceImpact impactDetail)
        {
            impactDetail.Validate("impactDetail");

            ImpactCategory category;
            if (IsInstanceRemoval(jobType, impactDetail))
            {
                // Handle instance removal as a special case
                category = ImpactCategory.Decommission;
                tracer.WriteInfo("Overall impact set to {0} due to instance removal", category);
            }
            else
            {
                category = TranslateImpactDetailToCategory(impactDetail);
            }

            return TranslateImpactCategoryToNodeImpactLevel(category);
        }

        private bool IsInstanceRemoval(ImpactActionEnum jobType, AffectedResourceImpact impactDetail)
        {
            bool isInstanceRemovalImpact =
                jobType == ImpactActionEnum.TenantUpdate &&
                CoordinatorHelper.AllImpactsEqual(impactDetail, Impact.Wipe);

            bool isInstanceRemovalLegacy = false;
            if (impactDetail.ListOfImpactTypes != null)
            {
                isInstanceRemovalLegacy = impactDetail.ListOfImpactTypes.Contains(ImpactTypeEnum.InstanceRemoval);
            }

            if (isInstanceRemovalImpact != isInstanceRemovalLegacy)
            {
                tracer.WriteWarning(
                    "Mismatch between IsInstanceRemoval result from resource impacts ({0}) and ImpactTypeEnum ({1}); using {0}",
                    isInstanceRemovalImpact,
                    isInstanceRemovalLegacy);
            }

            return isInstanceRemovalImpact;
        }

        /// <summary>
        /// Gets the key used for lookups in DefaultResourceImpactMap.
        /// </summary>
        private static string GetDefaultResourceImpactMapKey(string resourceType, string impactSuffix)
        {
            return string.Format(CultureInfo.InvariantCulture, "{0}.{1}", resourceType, impactSuffix);
        }

        private static string GetDefaultResourceImpactMapKey(string resourceType, Impact impact)
        {
            return GetDefaultResourceImpactMapKey(resourceType, impact.ToString());
        }

        /// <summary>
        /// Translates the ResourceImpactEnum for a single resource type into ImpactCategory,
        /// based on configuration or DefaultResourceImpactMap.
        /// </summary>
        private ImpactCategory TranslateResourceImpactToCategory(
            string resourceType,
            Impact impact)
        {
            // Look up the default category in the map
            ImpactCategory defaultCategory;
            if (!DefaultResourceImpactMap.TryGetValue(GetDefaultResourceImpactMapKey(resourceType, impact), out defaultCategory))
            {
                defaultCategory = ImpactCategory.Unhandled;
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
        /// Translates AffectedResourceImpact to an ImpactCategory.
        /// </summary>
        private ImpactCategory TranslateImpactDetailToCategory(AffectedResourceImpact impactDetail)
        {
            ImpactCategory currentCategory = ImpactCategory.NoImpact;

            // Translate each resource
            currentCategory = ApplyResourceImpact(currentCategory, "Compute", impactDetail.ComputeImpact);
            currentCategory = ApplyResourceImpact(currentCategory, "Disk", impactDetail.DiskImpact);
            currentCategory = ApplyResourceImpact(currentCategory, "Network", impactDetail.NetworkImpact);
            currentCategory = ApplyResourceImpact(currentCategory, "OS", impactDetail.OSImpact);
            currentCategory = ApplyResourceImpact(currentCategory, "ApplicationConfig", impactDetail.ApplicationConfigImpact);

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
            Impact impact)
        {
            ImpactCategory newCategory = TranslateResourceImpactToCategory(resourceType, impact);
            newCategory = EscalateImpactCategory(oldCategory, newCategory);

            if (oldCategory != newCategory)
            {
                tracer.WriteNoise(
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
            AffectedResourceImpact impactDetail)
        {
            ImpactCategory newCategory = oldCategory;

            if ((oldCategory == ImpactCategory.Freeze) && (impactDetail.EstimatedImpactDurationInSeconds < 0))
            {
                newCategory = this.configSection.ReadConfigValue(ConfigKeyUnknownDurationFreezeMapping, ImpactCategory.Restart);

                if (oldCategory != newCategory)
                {
                    tracer.WriteNoise(
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
            AffectedResourceImpact impactDetail,
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
                    tracer.WriteNoise(
                        "Overall impact changed from {0} to {1} due to impact duration of {2} sec (threshold = {3} sec)",
                        oldCategory,
                        newCategory,
                        impactDetail.EstimatedImpactDurationInSeconds,
                        thresholdInSeconds);
                }
            }

            return newCategory;
        }

        private static NodeImpactLevel TranslateImpactCategoryToNodeImpactLevel(ImpactCategory category)
        {
            switch (category)
            {
                case ImpactCategory.NoImpact:
                case ImpactCategory.Freeze:
                    return NodeImpactLevel.None;

                case ImpactCategory.Restart:
                    return NodeImpactLevel.Restart;

                case ImpactCategory.DataDestructive:
                    return NodeImpactLevel.RemoveData;

                case ImpactCategory.Decommission:
                    return NodeImpactLevel.RemoveNode;
            }

            throw new InvalidOperationException(String.Format(CultureInfo.InvariantCulture, "Unknown impact category: {0}", category));
        }
    }
}