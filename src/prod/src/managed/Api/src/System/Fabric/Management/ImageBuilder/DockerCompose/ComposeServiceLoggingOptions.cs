// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.DockerCompose
{
    using System.Collections.Generic;
    using YamlDotNet.RepresentationModel;

    internal class ComposeServiceLoggingOptions : ComposeObjectDescription
    {
        public string DriverName;

        public Dictionary<string, string> DriverOptions;

        public ComposeServiceLoggingOptions()
        {
            this.DriverOptions = new Dictionary<string, string>();
        }

        public bool IsSpecified()
        {
            return !string.IsNullOrEmpty(this.DriverName);
        }

        public override void Parse(string traceContext, YamlNode node, HashSet<string> ignoredKeys)
        {
            var loggingNode = DockerComposeUtils.ValidateAndGetMapping(traceContext, "", node);

            foreach (var item in loggingNode.Children)
            {
                var key = item.Key.ToString();
                switch (key)
                {
                    case DockerComposeConstants.DriverKey:
                    {
                        this.DriverName = DockerComposeUtils.ValidateAndGetScalar(
                            traceContext, 
                            DockerComposeConstants.DriverKey, 
                            item.Value).ToString();

                        break;
                    }
                    case DockerComposeConstants.LoggingDriverOptionsKey:
                    {
                        var driverOptions = DockerComposeUtils.ValidateAndGetMapping(traceContext, DockerComposeConstants.LoggingDriverOptionsKey, item.Value);
                        foreach (var driverOption in driverOptions.Children)
                        {
                            this.DriverOptions.Add(driverOption.Key.ToString(), driverOption.Value.ToString());
                        }
                        break;
                    }
                    default:
                    {
                        ignoredKeys.Add(key);
                        break;
                    }
                }
            }
        }

        public override void Validate()
        {
            // No-op. Validation done during parsing. We dont fail validation if there are keys that we dont recognize.
        }
    }
}