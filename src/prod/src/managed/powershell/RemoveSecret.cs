// ----------------------------------------------------------------------
//  <copyright file="RemoveSecret.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{    
    using System;
    using System.Fabric.SecretStore;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Remove, "ServiceFabricSecret")]
    public sealed class RemoveSecret : CommonCmdletBase
    {
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

        [ValidateNotNullOrEmpty]
        [ValidateLength(1, Constants.SecretVersionMaxLength)]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = ParameterSetNames.SingleSecretReference)]
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
                            Version = this.Version
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
                var result = clusterConnection.RemoveSecretsAsync(
                    secretReferences,
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
                        "RemoveSecretsErrorId",
                        clusterConnection);
                    return true;
                });
            }
            catch (Exception ex)
            {
                this.ThrowTerminatingError(
                    ex,
                    "RemoveSecretsErrorId",
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