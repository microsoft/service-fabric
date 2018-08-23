// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.DockerCompose
{
    using System.Collections.Generic;
    using System.IO;
    using YamlDotNet.RepresentationModel;

    //
    // This is given as a root level element in the docker compose file.
    // VolumeName:
    //   driver: drivername
    //   driver_opts: 
    //     option1Key: option1Value
    //
    internal class ComposeVolumeDescription : ComposeObjectDescription
    {
        private const string LocalVolumeDriverName = "local";

        public string DriverName;

        public IDictionary<string, string> DriverOptions;

        public ComposeVolumeDescription()
        {
            this.DriverName = LocalVolumeDriverName;
            this.DriverOptions = new Dictionary<string, string>();
        }

        public override void Parse(string traceContext, YamlNode node, HashSet<string> ignoredKeys)
        {
            var rootNode = DockerComposeUtils.ValidateAndGetMapping(traceContext, "", node);

            foreach (var item in rootNode.Children)
            {
                switch (item.Key.ToString())
                {
                    case DockerComposeConstants.DriverKey:
                    {
                        this.DriverName = DockerComposeUtils.ValidateAndGetScalar(traceContext, DockerComposeConstants.DriverOptionsKey, item.Value).ToString();
                        break;
                    }
                    case DockerComposeConstants.DriverOptionsKey:
                    {
                        var driverOptions = DockerComposeUtils.ValidateAndGetMapping(traceContext, DockerComposeConstants.DriverOptionsKey, item.Value);
                        foreach (var driverOption in driverOptions)
                        {
                            this.DriverOptions.Add(driverOption.Key.ToString(), driverOption.Value.ToString());
                        }
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
        }

        public override void Validate()
        {
            if (string.IsNullOrEmpty(this.DriverName) && this.DriverOptions.Count != 0)
            {
                throw new FabricComposeException("Volume driver options cannot be specified with a null driver name");
            }
        }
    }
}