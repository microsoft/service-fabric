// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace BSDockerVolumePlugin
{
    using System;
    using System.Fabric;
    using System.IO;

    internal static class Utilities
    {
        internal static void EnsureFolder(string folderPath)
        {
            if (!Directory.Exists(folderPath))
            {
                Directory.CreateDirectory(folderPath);
            }
        }

        internal static BSDockerVolumePluginSupportedOs GetOperatingSystem(ICodePackageActivationContext cpac)
        {
            var cpo = cpac.GetConfigurationPackageObject(Constants.ConfigurationPackageName);
            var section = cpo.Settings.Sections[Constants.SectionName];
            var paramValue = section.Parameters[Constants.ParameterName].Value;
            return (BSDockerVolumePluginSupportedOs)Enum.Parse(typeof(BSDockerVolumePluginSupportedOs), paramValue);
        }
    }
}
