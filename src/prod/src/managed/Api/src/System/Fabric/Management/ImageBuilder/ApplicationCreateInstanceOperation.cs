// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.ImageModel;
    using System.Threading.Tasks;

    class ApplicationCreateInstanceOperation : ApplicationInstanceOperationBase
    {
        private static readonly string TraceType = "ApplicationCreateInstanceOperation";

        private Uri nameUri;

        public ApplicationCreateInstanceOperation(
            string applicationTypeName,
            string applicationTypeVersion,
            string applicationId,
            Uri nameUri,
            IDictionary<string, string> userParameters,
            ImageStoreWrapper imageStoreWrapper,
            IEnumerable<IApplicationValidator> validators)
            : base(applicationTypeName, applicationTypeVersion, applicationId, userParameters, imageStoreWrapper, validators)
        {
            this.nameUri = nameUri;
        }

        public async Task CreateInstanceAsync(string outputFolder, TimeSpan timeout, ApplicationTypeContext validatedApplicationTypeContext = null)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);

            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Starting CreateInstance. ApplicationTypeName:{0}, ApplicationTypeVersion:{1}, ApplicationId:{2}, Timeout:{3}",
                this.ApplicationTypeName,
                this.ApplicationTypeVersion,
                this.ApplicationId,
                timeoutHelper.GetRemainingTime());

            ApplicationInstanceContext appInstanceContext = await base.CreateAndSortInstanceAsync(
                1 /*ApplicationInstace version starts at 1*/,
                nameUri,
                timeoutHelper,
                validatedApplicationTypeContext);            

            this.ValidateApplicationInstance(appInstanceContext);

            if (validatedApplicationTypeContext == null)
            {
                timeoutHelper.ThrowIfExpired();

                StoreLayoutSpecification clusterManagerOutputSpecification = null;
                if (outputFolder != null)
                {
                    clusterManagerOutputSpecification = StoreLayoutSpecification.Create();
                    clusterManagerOutputSpecification.SetRoot(outputFolder);
                }

                await this.UploadInstanceAsync(
                    appInstanceContext.ApplicationInstance,
                    appInstanceContext.ApplicationPackage,
                    appInstanceContext.ServicePackages,
                    StoreLayoutSpecification.Create(),
                    clusterManagerOutputSpecification,
                    true,
                    timeoutHelper);
            }

            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Completed CreateInstance. ApplicationTypeName:{0}, ApplicationTypeVersion:{1}, ApplicationId:{2}",
                this.ApplicationTypeName,
                this.ApplicationTypeVersion,
                this.ApplicationId);
        }        
    }
}