// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace System.Fabric.Management.ImageBuilder.DockerCompose
{
    using System.Globalization;
    using YamlDotNet.RepresentationModel;

    //
    // We support CPU and Memory limits. We dont support CPU reservation. We only support Memory reservation.
    //
    internal class ComposeServiceResourceGovernance : ComposeObjectDescription
    {
        public string LimitCpuShares;
        public string LimitCpus;
        public string LimitMemoryInMb;
        public string LimitMemorySwapInMb;
        public string ReservationMemoryInMb;

        public ComposeServiceResourceGovernance()
        {
            this.LimitMemoryInMb = "";
            this.LimitCpuShares = "";
            this.ReservationMemoryInMb = "";
            this.LimitMemorySwapInMb = "";
            this.LimitCpus = "";
        }

        public bool IsSpecified()
        {
            if (!String.IsNullOrEmpty(this.LimitCpuShares) ||
                !String.IsNullOrEmpty(this.LimitMemoryInMb) ||
                !String.IsNullOrEmpty(this.LimitMemorySwapInMb) ||
                !String.IsNullOrEmpty(this.ReservationMemoryInMb) ||
                !String.IsNullOrEmpty(this.LimitCpus))
            {
                return true;
            }

            return false;
        }

        public override void Parse(string traceContext, YamlNode node, HashSet<string> ignoredKeys)
        {
            var resourcesRootNode = DockerComposeUtils.ValidateAndGetMapping(traceContext, "", node);

            foreach (var childItem in resourcesRootNode.Children)
            {
                string key = childItem.Key.ToString();
                switch (childItem.Key.ToString())
                {
                    case DockerComposeConstants.ResourceLimitsKey:
                    {
                        this.ParseLimits(
                            DockerComposeUtils.GenerateTraceContext(traceContext, DockerComposeConstants.ResourceLimitsKey),
                            DockerComposeUtils.ValidateAndGetMapping(traceContext, DockerComposeConstants.ResourceLimitsKey, childItem.Value));

                        break;
                    }
                    case DockerComposeConstants.ResourceReservationsKey:
                    {
                        this.ParseReservations(
                            DockerComposeUtils.GenerateTraceContext(traceContext, DockerComposeConstants.ResourceReservationsKey),
                            DockerComposeUtils.ValidateAndGetMapping(traceContext, DockerComposeConstants.ResourceReservationsKey, childItem.Value));

                        break;
                    }
                    case DockerComposeConstants.LabelsKey:
                    {
                        // ignored
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

        private void ParseLimits(string traceContext, YamlMappingNode limitsNode)
        {
            foreach (var childItem in limitsNode.Children)
            {
                string key = childItem.Key.ToString();
                switch (childItem.Key.ToString())
                {
                    case DockerComposeConstants.CpuSharesKey:
                    {
                        this.LimitCpuShares = DockerComposeUtils.ValidateAndGetScalar(traceContext, DockerComposeConstants.CpuSharesKey, childItem.Value).ToString();
                        break;
                    }
                    case DockerComposeConstants.CpusKey:
                    {
                        this.LimitCpus = DockerComposeUtils.ValidateAndGetScalar(traceContext, DockerComposeConstants.CpusKey, childItem.Value).ToString();
                        break;
                    }
                    case DockerComposeConstants.MemoryKey:
                    {
                        this.LimitMemoryInMb = this.NormalizeToMb(
                            DockerComposeUtils.GenerateTraceContext(traceContext, DockerComposeConstants.MemoryKey),
                            DockerComposeUtils.ValidateAndGetScalar(traceContext, DockerComposeConstants.MemoryKey, childItem.Value).ToString());

                        break;
                    }
                    case DockerComposeConstants.MemorySwapKey:
                    {
                        this.LimitMemorySwapInMb = this.NormalizeToMb(
                            DockerComposeUtils.GenerateTraceContext(traceContext, DockerComposeConstants.MemorySwapKey),
                            DockerComposeUtils.ValidateAndGetScalar(traceContext, DockerComposeConstants.MemorySwapKey, childItem.Value).ToString());
                        break;
                    }
                }
            }
        }

        private void ParseReservations(string tracecontext, YamlMappingNode reservationsNode)
        {
            foreach (var childItem in reservationsNode.Children)
            {
                string key = childItem.Key.ToString();
                switch (childItem.Key.ToString())
                {
                    case DockerComposeConstants.MemoryKey:
                        {
                            this.ReservationMemoryInMb = this.NormalizeToMb(DockerComposeConstants.MemoryKey, childItem.Value.ToString());
                            break;
                        }
                }
            }
        }

        //
        // The format is <number>[Unit]
        //
        private string NormalizeToMb(string traceContext, string memoryValue)
        {
            if (String.IsNullOrEmpty(memoryValue))
            {
                return memoryValue;
            }

            if (memoryValue.EndsWith("m", StringComparison.OrdinalIgnoreCase))
            {
                return memoryValue.TrimEnd(memoryValue[memoryValue.Length - 1]);
            }
            else if (memoryValue.EndsWith("g", StringComparison.OrdinalIgnoreCase))
            {
                var memoryInGB = memoryValue.TrimEnd(memoryValue[memoryValue.Length - 1]);
                int valueInGB = Int32.Parse(memoryInGB);

                return ((Int64)valueInGB * 1024).ToString();
            }
            else
            {
                throw new FabricComposeException(
                    $"{traceContext} - Only 'm' and 'g' are supported units for memory key - specified: {memoryValue}");
            }
        }
    }
}
