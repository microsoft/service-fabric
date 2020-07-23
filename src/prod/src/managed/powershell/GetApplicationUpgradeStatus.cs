// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricApplicationUpgrade")]
    public sealed class GetApplicationUpgradeStatus : ApplicationCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public Uri ApplicationName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var applicationUpgradeProgress = this.GetApplicationUpgradeProgress(this.ApplicationName);
            this.WriteObject(this.FormatOutput(applicationUpgradeProgress), true);
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1305:FieldNamesMustNotUseHungarianNotation", Justification = "Reviewed. Suppression is OK here for psInfo and psNote.")]
        protected override object FormatOutput(object output)
        {
            var casted = output as ApplicationUpgradeProgress;
            if (casted != null)
            {
                var upgradeDomainsPropertyPSObj = new PSObject(casted.UpgradeDomains);
                upgradeDomainsPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                var itemPSObj = new PSObject(casted);
                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.UpgradeDomainsPropertyName,
                        upgradeDomainsPropertyPSObj));

                if (casted.UpgradeDescription != null)
                {
                    var descriptionPSObj = this.ToPSObject(casted.UpgradeDescription);

                    foreach (var psInfo in descriptionPSObj.Properties)
                    {
                        var psNote = psInfo as PSNoteProperty;

                        if (psNote != null)
                        {
                            itemPSObj.Properties.Add(psNote);
                        }
                    }
                }

                Helpers.AddToPsObject(itemPSObj, casted.UnhealthyEvaluations);

                if (casted.CurrentUpgradeDomainProgress != null)
                {
                    var progressDetailsPSObj = new PSObject(casted.CurrentUpgradeDomainProgress);
                    progressDetailsPSObj.Members.Add(
                        new PSCodeMethod(
                            Constants.ToStringMethodName,
                            typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                    itemPSObj.Properties.Add(
                        new PSNoteProperty(
                            Constants.CurrentUpgradeDomainProgressPropertyName,
                            progressDetailsPSObj));
                }

                if (casted.UpgradeDomainProgressAtFailure != null)
                {
                    var progressDetailsPSObj = new PSObject(casted.UpgradeDomainProgressAtFailure);
                    progressDetailsPSObj.Members.Add(
                        new PSCodeMethod(
                            Constants.ToStringMethodName,
                            typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                    itemPSObj.Properties.Add(
                        new PSNoteProperty(
                            Constants.UpgradeDomainProgressAtFailurePropertyName,
                            progressDetailsPSObj));
                }

                return itemPSObj;
            }

            return base.FormatOutput(output);
        }
    }
}