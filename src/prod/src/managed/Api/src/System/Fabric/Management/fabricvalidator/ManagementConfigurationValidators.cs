// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ImageStore;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.Security;

    /// <summary>
    /// This is the class handles the validation of configuration for the Management section.
    /// </summary>
    class ManagementConfigurationValidators : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.Management; }
        }

        private string StoreName { get; set; }

        private SecureString ImageStoreConnectionString { get; set; }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings) { }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {
            bool allowImageStoreConnectionStringChange = targetWindowsFabricSettings.GetParameter(
                this.SectionName, 
                FabricValidatorConstants.ParameterNames.AllowImageStoreConnectionStringChange).GetValue<bool>();

            if(allowImageStoreConnectionStringChange)
            {
                return;
            }

            this.StoreName = currentWindowsFabricSettings.StoreName;
            this.ImageStoreConnectionString = currentWindowsFabricSettings.GetParameter(this.SectionName,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString).GetSecureValue(this.StoreName);

            if (!this.CompareImageStoreConnectionString(targetWindowsFabricSettings))
            {
                WriteError("The change in ImageStoreConnectionString is not allowed.");
            }
        }

        public bool CompareImageStoreConnectionString(WindowsFabricSettings newWindowsFabricSettings)
        {
            var oldImageStoreConnectionChars = FabricValidatorUtility.SecureStringToCharArray(this.ImageStoreConnectionString);
            var newImageStoreConnectionChars = FabricValidatorUtility.SecureStringToCharArray(
                newWindowsFabricSettings.GetParameter(
                this.SectionName,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString).GetSecureValue(this.StoreName));

            if (FabricValidatorUtility.StartsWithIgnoreCase(oldImageStoreConnectionChars, FabricValidatorConstants.FileImageStoreConnectionStringPrefix))
            {
                if (!FabricValidatorUtility.StartsWithIgnoreCase(newImageStoreConnectionChars, FabricValidatorConstants.FileImageStoreConnectionStringPrefix))
                {
                    WriteError("the old image store connection string starts with file prefix but new one is not");
                    return false;
                }

                return CompareFileImageStoreConnectionString(new string(oldImageStoreConnectionChars), new string(newImageStoreConnectionChars));
            }
            else if (FabricValidatorUtility.StartsWithIgnoreCase(oldImageStoreConnectionChars, FabricValidatorConstants.XStoreImageStoreConnectionStringPrefix))
            {
                if (!FabricValidatorUtility.StartsWithIgnoreCase(newImageStoreConnectionChars, FabricValidatorConstants.XStoreImageStoreConnectionStringPrefix))
                {
                    WriteError("the old image store connection string starts with xstore prefix but new one is not");
                    return false;
                }

                return CompareXStoreImageStoreConnectionString(new string(oldImageStoreConnectionChars), new string(newImageStoreConnectionChars));
            }
            else if (FabricValidatorUtility.StartsWithIgnoreCase(oldImageStoreConnectionChars, FabricValidatorConstants.FabricImageStoreConnectionStringPrefix))
            {
                if (!FabricValidatorUtility.StartsWithIgnoreCase(newImageStoreConnectionChars, FabricValidatorConstants.FabricImageStoreConnectionStringPrefix))
                {
                    WriteError("the old image store connection string starts with fabric prefix but new one is not");
                    return false;
                }

                return CompareFabricImageStoreConnectionString(new string(oldImageStoreConnectionChars), new string(newImageStoreConnectionChars));
            }
            else if (FabricValidatorUtility.StartsWithIgnoreCase(oldImageStoreConnectionChars, FabricValidatorConstants.DefaultTag))
            {
                if (!FabricValidatorUtility.StartsWithIgnoreCase(newImageStoreConnectionChars, FabricValidatorConstants.DefaultTag))
                {
                    WriteError("the old image store connection string starts with default prefix but new one is not");
                    return false;
                }
                return true;
            }
            else
            {
                WriteError(
                    "Value for section {0} parameter {1} should start with {2}, {3} or {4}",
                    this.SectionName,
                    FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                    FabricValidatorConstants.XStoreImageStoreConnectionStringPrefix,
                    FabricValidatorConstants.FileImageStoreConnectionStringPrefix,
                    FabricValidatorConstants.FabricImageStoreConnectionStringPrefix);
                return false;
            }
        }

     
        private bool CompareFabricImageStoreConnectionString(string oldImageStoreConntionString, string newImageStoreConntionString)
        {
            if (!oldImageStoreConntionString.Equals(newImageStoreConntionString, StringComparison.InvariantCultureIgnoreCase))
            {
                WriteError("the old connection string {0} does not match the new one {1}", oldImageStoreConntionString, newImageStoreConntionString);
                return false;
            }

            return true;
        }

        private bool CompareFileImageStoreConnectionString(string oldImageStoreConntionString, string newImageStoreConntionString)
        {
            try
            {
                string oldRootUri, oldAccountName, oldAccountPassword;
                bool oldIsManagedServiceAccount;
                ImageStoreFactory.ParseFileImageStoreConnectionString(oldImageStoreConntionString, out oldRootUri, out oldAccountName, out oldAccountPassword, out oldIsManagedServiceAccount);
                string newRootUri, newAccountName, newAccountPassword;
                bool newIsManagedServiceAccount;
                ImageStoreFactory.ParseFileImageStoreConnectionString(newImageStoreConntionString, out newRootUri, out newAccountName, out newAccountPassword, out newIsManagedServiceAccount);

                if (!oldRootUri.Equals(newRootUri, StringComparison.InvariantCultureIgnoreCase))
                {
                    WriteError("the old root uri {0} does not match the new one {1}", oldRootUri, newRootUri);
                    return false;
                }

                return true;
            }
            catch (ArgumentException ae)
            {
                WriteError(ae.Message);
                return false;
            }
        }

        private bool CompareXStoreImageStoreConnectionString(string oldImageStoreConntionString, string newImageStoreConntionString)
        {
            try
            {
                string oldConnectionString, oldSecondaryConnectionString, oldBlobEndpoint, oldContainer, oldAccountName, oldEndpointSuffix;
                XStoreCommon.TryParseImageStoreUri(
                    oldImageStoreConntionString,
                    out oldConnectionString,
                    out oldSecondaryConnectionString,
                    out oldBlobEndpoint,
                    out oldContainer,
                    out oldAccountName,
                    out oldEndpointSuffix);

                string newConnectionString, newSecondaryConnectionString, newBlobEndpoint, newContainer, newAccountName, newEndpointSuffix;
                XStoreCommon.TryParseImageStoreUri(
                    newImageStoreConntionString,
                    out newConnectionString,
                    out newSecondaryConnectionString,
                    out newBlobEndpoint,
                    out newContainer,
                    out newAccountName,
                    out newEndpointSuffix);

                if (!oldAccountName.Equals(newAccountName, StringComparison.InvariantCultureIgnoreCase))
                {
                    WriteError("the old account name {0} does not match the new one {1}", oldAccountName, newAccountName);
                    return false;
                }

                if (!oldContainer.Equals(newContainer, StringComparison.InvariantCultureIgnoreCase))
                {
                    WriteError("the old container {0} does not match the new one {1}", oldContainer, newContainer);
                    return false;
                }

                if((string.IsNullOrEmpty(oldEndpointSuffix) ^ string.IsNullOrEmpty(newEndpointSuffix))
                    || (!oldEndpointSuffix.Equals(newEndpointSuffix, StringComparison.InvariantCultureIgnoreCase)))
                {
                    WriteError("the old endpoint suffix {0} does not match the new endpoint suffix {1}", oldEndpointSuffix, newEndpointSuffix);
                    return false;
                }

                return true;
            }
            catch (ArgumentException ae)
            {
                WriteError(ae.Message);
                return false;
            }
        }

        private static void WriteError(string format, params object[] args)
        {
            throw new ArgumentException(string.Format(format, args));
        }
    }
}