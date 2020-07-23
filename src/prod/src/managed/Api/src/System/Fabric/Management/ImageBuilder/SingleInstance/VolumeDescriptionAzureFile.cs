// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Threading;

    internal class VolumeDescriptionAzureFile : VolumeDescription
    {
        private const string DockerPluginName = "sfazurefile";

        private Lazy<IEnumerable<DriverOptionType>> driverOptions;

        public VolumeDescriptionAzureFile()
            : this(null)
        {
        }

        public VolumeDescriptionAzureFile(string volumeName)
            : base(volumeName)
        {
            this.accountName = null;
            this.accountKey = null;
            this.shareName = null;

            this.driverOptions = new Lazy<IEnumerable<DriverOptionType>>(
                () =>
                {
                    var accountKeyValue = this.accountKey;
                    var accountKeyType = TryMatchStorageAccountKeySecretRef(ref accountKeyValue);

                    return new DriverOptionType[]
                    {
                        new DriverOptionType() { Name = "shareName", Value = this.shareName },
                        new DriverOptionType() { Name = "storageAccountName", Value = this.accountName },
                        new DriverOptionType() { Name = "storageAccountKey", Value = accountKeyValue, Type = accountKeyType }
                    };
                },
                LazyThreadSafetyMode.PublicationOnly);
        }

        public string accountName;
        public string accountKey;
        public string shareName;

        public override string PluginName
        {
            get
            {
                return DockerPluginName;
            }
        }

        public override IEnumerable<DriverOptionType> DriverOptions
        {
            get
            {
                return this.driverOptions.Value;
            }
        }

        public override void Validate(string codePackageName)
        {
            base.Validate(codePackageName);

            if (string.IsNullOrEmpty(this.accountName))
            {
                throw new FabricApplicationException(String.Format("required parameter 'accountName' not specified for azure file parameter in code package {0}", codePackageName));
            }

            if (string.IsNullOrEmpty(this.accountKey))
            {
                throw new FabricApplicationException(String.Format("required parameter 'accountKey' not specified for azure file parameter in code package {0}", codePackageName));
            }

            if (string.IsNullOrEmpty(this.shareName))
            {
                throw new FabricApplicationException(String.Format("required parameter 'shareName' not specified for azure file parameter in code package {0}", codePackageName));
            }
        }

        private string TryMatchStorageAccountKeySecretRef(ref string accountKeyValue)
        {
            if (ApplicationPackageGenerator.TryMatchSecretRef(ref accountKeyValue))
            {
                return EnvironmentVariableTypeType.SecretsStoreRef.ToString();
            }
            else
            {
                return EnvironmentVariableTypeType.PlainText.ToString();
            }
        }
    };
}
