// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.DockerCompose
{
    using System.Collections.Generic;
    using System.Linq;

#if !DotNetCoreClrLinux
    using System.Text.RegularExpressions;
#endif

    using YamlDotNet.RepresentationModel;

    //
    // Data Volumes can be specified for either mounting a volume in the host into the container or 
    // for mounting a shared network volume on the container. 
    // This can be given in one of the following formats,
    // 1. Mount path on the Host : Mount path on the container 
    // 2. Mount path on the Host : Mount path on the container : ro - to specify mounting as read only.
    // 3. Name of volume : Mount path on the container, and then mapping the name of volume to a volume driver.
    // We donot support just specifying a path in the container and mapping a default engine created volume.

    // Windows vs Linux:
    // Colon ':' is the separation between the Host and container volume. Windows absolute paths contain ':' as a part 
    // of the drive letter. So parsing windows and linux volume paths should be done differently.
    // The parsing logic we use for windows is lifted from Docker code(thanks to 'jhoward') - 
    // https://github.com/docker/docker/blob/master/volume/volume_windows.go 
    //

    internal class ComposeServiceVolume : ComposeObjectDescription
    {
#if !DotNetCoreClrLinux
        /// WINDOWS ONLY 

        //
        // Absolute path to a directory, can contain spaces. Just specifying root of the drive is not allowed.
        //
        private static string hostDirectoryPathRegEx = "[a-z]:\\\\(?:[^\\\\/:*?\"<>|\\r\\n]+\\\\?)*";

        //
        // Driver name can be any name that doesnt contain invalid ntfs filename characters.
        //
        private static string driverNameRegEx = "([^\\\\/:*?\"<>|\\r\\n]+)";

        //
        // Host volume can be either a host directory or a driver.
        //
        private static string directoryPathOrDriverNameRegEx = "(" + hostDirectoryPathRegEx + '|' + driverNameRegEx + ")";
        private static string volumeSourceRegEx = "((?<source>" + directoryPathOrDriverNameRegEx + "):)";

        //
        // Container path can be a directory or a drive path.
        //
        private static string volumeDestinationRegEx = "(?<destination>([a-z]:(?:\\\\[^\\\\/:*?\"<>\\r\\n]+)*\\\\?))";

        //
        // Access mode.
        //
        private static string modeRegEx = "(:(?<mode>(?i)ro|rw))?";

        //
        // Regular expression used for parsing windows volume mapping.
        //
        private static string windowsVolumeRegEx = volumeSourceRegEx + volumeDestinationRegEx + modeRegEx;

        /// END WINDOWS ONLY 
#endif
        public string ContainerLocation;

        public string HostLocation;

        public bool ReadOnly;

        public ComposeServiceVolume()
        {
            this.ReadOnly = false;
        }

        public override void Parse(string traceContext, YamlNode node, HashSet<string> ignoredKeys)
        {
            this.ParseMapping(
                traceContext,
                node.ToString());
        }

        public override void Validate()
        {
            // Validation performed during parsing.
        }

        private void ParseMapping(string traceContext, string volumeMapping)
        {

#if !DotNetCoreClrLinux
            this.ParseMappingForWindows(traceContext, volumeMapping);
#else
            this.ParseMappingForLinux(traceContext, volumeMapping);
#endif
        }

#if !DotNetCoreClrLinux
        private void ParseMappingForWindows(string traceContext, string volumeMapping)
        {
            Regex windowsVolume = new Regex(windowsVolumeRegEx);
            Match match = windowsVolume.Match(volumeMapping);

            var group = match.Groups["source"];
            if (group.Success)
            {
                this.HostLocation = group.Value;
            }
            group = match.Groups["destination"];
            if (group.Success)
            {
                this.ContainerLocation = group.Value;
            }
            group = match.Groups["mode"];
            if (group.Success)
            {
                if (group.Value == DockerComposeConstants.ReadOnly)
                {
                    this.ReadOnly = true;
                }
            }

            if (string.IsNullOrEmpty(this.ContainerLocation))
            {
                throw new FabricComposeException(string.Format("{0} - Unable to parse Volume - {1}", traceContext, volumeMapping));
            }

        }
#endif

        private void ParseMappingForLinux(string traceContext, string volumeMapping)
        {
            var indexOfContainerPath = volumeMapping.IndexOf(DockerComposeConstants.SemiColonDelimiter);
            if (indexOfContainerPath == -1)
            {
                throw new FabricComposeException(string.Format("{0} - Unable to parse Volume - {1}", traceContext, volumeMapping));
            }

            this.ContainerLocation = volumeMapping.Substring(
                indexOfContainerPath + 1,
                (volumeMapping.Length - (indexOfContainerPath + 1)));
            this.HostLocation = volumeMapping.Substring(0, indexOfContainerPath);

            var lastIndex = this.ContainerLocation.LastIndexOf(DockerComposeConstants.SemiColonDelimiter);
            if (lastIndex == -1)
            {
                return;
            }

            var accessMode = this.ContainerLocation.Substring(lastIndex + 1,
                this.ContainerLocation.Length - (lastIndex + 1));
            if (accessMode == DockerComposeConstants.ReadOnly)
            {
                this.ReadOnly = true;
                this.ContainerLocation = this.ContainerLocation.Substring(0, lastIndex);
            }
        }
    }
}