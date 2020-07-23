// ----------------------------------------------------------------------
//  <copyright file="GetChaosSchedule.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Chaos.DataStructures;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricChaosSchedule")]
    public sealed class GetChaosSchedule : CommonCmdletBase
    {
        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                ChaosScheduleDescription description = null;

                description = clusterConnection.GetChaosScheduleAsync(
                    this.GetTimeout(),
                    this.GetCancellationToken()).Result;

                this.WriteObject(this.FormatOutput(description));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetChaosScheduleCommandErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is ChaosScheduleDescription)
            {
                var item = output as ChaosScheduleDescription;

                var itemPSObj = new PSObject(item);
                var schedulePSObj = new PSObject(item.Schedule);
                if (item.Schedule.ChaosParametersDictionary != null)
                {
                    var parametersDictionaryPSObj = new PSObject(item.Schedule.ChaosParametersDictionary);
                    parametersDictionaryPSObj.Members.Add(
                        new PSCodeMethod(
                            Constants.ToStringMethodName,
                            typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                    schedulePSObj.Properties.Add(
                        new PSNoteProperty(
                            Constants.ChaosParametersDictionaryPropertyName,
                            parametersDictionaryPSObj));
                }

                if (item.Schedule.Jobs != null)
                {
                    var jobsPropertyPSObj = new PSObject(item.Schedule.Jobs);
                    jobsPropertyPSObj.Members.Add(
                        new PSCodeMethod(
                            Constants.ToStringMethodName,
                            typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                    schedulePSObj.Properties.Add(
                        new PSNoteProperty(
                            Constants.ChaosScheduleJobsPropertyName,
                            jobsPropertyPSObj));
                }

                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                    Constants.ChaosSchedulePropertyName,
                    schedulePSObj));

                return itemPSObj;
            }

            return base.FormatOutput(output);
        }
    }
}