// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections;
    using System.Fabric.Description;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.New, "ServiceFabricApplication")]
    public sealed class NewApplication : ApplicationCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public Uri ApplicationName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1)]
        public string ApplicationTypeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 2)]
        public string ApplicationTypeVersion
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public Hashtable ApplicationParameter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public long? MaximumNodes
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public long? MinimumNodes
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string[] Metrics
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.CreateApplicationInstance(
                this.ApplicationName,
                this.ApplicationTypeName,
                this.ApplicationTypeVersion,
                this.ApplicationParameter,
                this.MaximumNodes,
                this.MinimumNodes,
                this.Metrics);
        }

        protected override object FormatOutput(object output)
        {
            if (output is ApplicationDescription)
            {
                var item = output as ApplicationDescription;

                var parametersPropertyPSObj = new PSObject(item.ApplicationParameters);
                parametersPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                var itemPSObj = new PSObject(item);
                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.ApplicationParametersPropertyName,
                        parametersPropertyPSObj));

                return itemPSObj;
            }

            return base.FormatOutput(output);
        }
    }
}