// ----------------------------------------------------------------------
//  <copyright file="SetSecret.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.SecretStore;
    using System.Management.Automation;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Text;

    [Cmdlet(VerbsCommon.Set, "ServiceFabricSecret")]
    public sealed class SetSecret : CommonCmdletBase
    {
        [ValidateNotNull]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = ParameterSetNames.SingleInsecureSecret)]
        public string InsecureValue
        {
            get;
            set;
        }

        [ValidateNotNullOrEmpty]
        [ValidateLength(1, Constants.SecretNameMaxLength)]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = ParameterSetNames.SingleInsecureSecret)]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = ParameterSetNames.SingleSecureSecret)]
        public string Name
        {
            get;
            set;
        }

        [ValidateNotNull]
        [ValidateCount(1, int.MaxValue)]
        [Parameter(Mandatory = true, ValueFromPipeline = true, ParameterSetName = ParameterSetNames.SecretObjects)]
        public Secret[] Secret
        {
            get;
            set;
        }

        [ValidateNotNull]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = ParameterSetNames.SingleSecureSecret)]
        public SecureString SecureValue
        {
            get;
            set;
        }

        [ValidateNotNullOrEmpty]
        [ValidateLength(1, Constants.SecretVersionMaxLength)]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = ParameterSetNames.SingleInsecureSecret)]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = ParameterSetNames.SingleSecureSecret)]
        public string Version
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            Secret[] secrets;

            switch (this.ParameterSetName)
            {
                case ParameterSetNames.SecretObjects:
                    secrets = this.Secret;
                    break;

                case ParameterSetNames.SingleInsecureSecret:
                    secrets = new Secret[]
                      {
                        new Secret()
                        {
                            Name = this.Name,
                            Version = this.Version,
                            Value = ToSecureString(this.InsecureValue)
                        }
                      };
                    break;

                case ParameterSetNames.SingleSecureSecret:
                    secrets = new Secret[]
                    {
                        new Secret()
                        {
                            Name = this.Name,
                            Version = this.Version,
                            Value = this.SecureValue
                        }
                    };
                    break;

                default:
                    throw new InvalidPowerShellStateException(string.Format("Invalid parameter set name '{0}'.", this.ParameterSetName));
            }

            var clusterConnection = this.GetClusterConnection();
            try
            {
                var result = clusterConnection.SetSecretsAsync(
                    secrets,
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
                        "SetSecretsErrorId",
                        clusterConnection);
                    return true;
                });
            }
            catch (Exception ex)
            {
                this.ThrowTerminatingError(
                    ex,
                    "SetSecretsErrorId",
                    clusterConnection);
            }
        }

        private static SecureString ToSecureString(string value)
        {
            return NativeTypes.FromNativeToSecureString(Marshal.StringToHGlobalUni(value));
        }

        private class ParameterSetNames
        {
            public const string SecretObjects = nameof(SecretObjects);

            public const string SingleInsecureSecret = nameof(SingleInsecureSecret);

            public const string SingleSecureSecret = nameof(SingleSecureSecret);
        }
    }
}
