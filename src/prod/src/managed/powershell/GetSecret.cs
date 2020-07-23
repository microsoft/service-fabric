// ----------------------------------------------------------------------
//  <copyright file="GetSecret.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.SecretStore;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricSecret")]
    public sealed class GetSecret : CommonCmdletBase
    {
        [Parameter(Mandatory = false, ParameterSetName = ParameterSetNames.SecretReferenceObjects)]
        [Parameter(Mandatory = false, ParameterSetName = ParameterSetNames.SingleSecretReference)]
        public SwitchParameter IncludeValue
        {
            get;
            set;
        }

        [ValidateNotNullOrEmpty]
        [ValidateLength(1, Constants.SecretNameMaxLength)]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = ParameterSetNames.SingleSecretReference)]
        public string Name
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipeline = true, ParameterSetName = ParameterSetNames.SecretReferenceObjects)]
        public SecretReference[] SecretReference
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, ParameterSetName = ParameterSetNames.SingleSecretReference)]
        public string Version
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            SecretReference[] secretReferences;

            switch (this.ParameterSetName)
            {
                case ParameterSetNames.SingleSecretReference:
                    secretReferences = new SecretReference[]
                    {
                        new SecretReference()
                        {
                            Name = this.Name,
                            Version = this.Version == null ? string.Empty : this.Version
                        }
                    };
                    break;

                case ParameterSetNames.SecretReferenceObjects:
                    secretReferences = this.SecretReference;
                    break;

                default:
                    throw new InvalidPowerShellStateException(string.Format("Invalid parameter set name '{0}'.", this.ParameterSetName));
            }

            var clusterConnection = this.GetClusterConnection();
            try
            {
                var result = clusterConnection.GetSecretsAsync(
                    secretReferences,
                    this.IncludeValue,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Result;

                this.WriteObject(result);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        "GetSecretsErrorId",
                        clusterConnection);
                    return true;
                });
            }
            catch (Exception ex)
            {
                this.ThrowTerminatingError(
                    ex,
                    "GetSecretsErrorId",
                    clusterConnection);
            }
        }

        private class ParameterSetNames
        {
            public const string SecretReferenceObjects = nameof(SecretReferenceObjects);

            public const string SingleSecretReference = nameof(SingleSecretReference);
        }
    }
}