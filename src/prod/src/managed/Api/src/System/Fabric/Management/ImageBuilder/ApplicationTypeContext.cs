// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Linq;

    internal class ApplicationTypeContext
    {
        private static readonly string TraceType = "ApplicationTypeContext";        

        public ApplicationTypeContext(ApplicationManifestType appManifestType, string appPath)
        {
            this.ApplicationManifest = appManifestType;
            this.ServiceManifests = new Collection<ServiceManifest>();

            this.BuildLayoutSpecification = BuildLayoutSpecification.Create();
            this.BuildLayoutSpecification.SetRoot(appPath);

            this.InitializeAndValidateApplicationParameters(appManifestType.Parameters);
        }

        public ApplicationManifestType ApplicationManifest
        {
            get;
            private set;
        }

        public Collection<ServiceManifest> ServiceManifests
        {
            get;
            private set;
        }

        public IDictionary<string, string> ApplicationParameters
        {
            get;
            private set;
        }

        public BuildLayoutSpecification BuildLayoutSpecification
        {
            get;
            private set;
        }

        public string GetApplicationManifestFileName()
        {
            return this.BuildLayoutSpecification.GetApplicationManifestFile();
        }

        public string GetServiceManifestFileName(string serviceManifestName)
        {
            return this.BuildLayoutSpecification.GetServiceManifestFile(serviceManifestName);
        }

        public IEnumerable<ServiceManifestType> GetServiceManifestTypes()
        {
            return this.ServiceManifests.Select(serviceManifest => serviceManifest.ServiceManifestType);
        }

        private void InitializeAndValidateApplicationParameters(ApplicationManifestTypeParameter[] parameters)
        {
            Dictionary<string, string> validatedParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            if (parameters != null)
            {
                foreach (ApplicationManifestTypeParameter parameter in parameters)
                {
                    if (validatedParameters.ContainsKey(parameter.Name))
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            this.GetApplicationManifestFileName(),
                            StringResources.ImageBuilderError_DuplicateParametersFound,
                            parameter.Name);
                    }

                    validatedParameters.Add(parameter.Name, parameter.DefaultValue);
                }
            }

            this.ApplicationParameters = validatedParameters;
        }
    }
}