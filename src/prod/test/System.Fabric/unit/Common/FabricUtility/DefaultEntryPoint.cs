// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.FabricUtility
{
    using System.Collections.Generic;
    using System.Fabric;
    using System.Xml.Linq;

    /// <summary>
    /// The default entry point for all tests that use the deployment code here
    /// This relies on each service type description containing an additional field that specifies the managed type that implements the service
    /// Don't care about unregistration right now
    /// </summary>
    public class DefaultEntryPoint : FabricWorkerEntryPoint
    {
        private const string ServiceTypeExtensionDelimiter = "<>";
        private const string ServiceTypeExtensionFormatString = "{0}<>{1}";

        public const string ServiceImplementationTypeExtensionName = "service_impl_type";

        private readonly List<string> registeredTypes = new List<string>();

        private FabricRuntime runtime;

        protected override void Activate(FabricRuntime runtime, CodePackageActivationContext activationContext)
        {
            this.runtime = runtime;

            var serviceTypes = activationContext.GetServiceTypes();

            string codePackageName = activationContext.CodePackageName;

            // register factories for all service types
            foreach (var item in serviceTypes)
            {
                var typeInformation = DefaultEntryPoint.GetImplementationTypeFromExtension(item.Extensions[DefaultEntryPoint.ServiceImplementationTypeExtensionName]);

                // register only service types for this code package
                if (codePackageName == typeInformation.Item1)
                {
                    Type implementationType = Type.GetType(typeInformation.Item2);

                    if (typeof(IStatelessServiceFactory).IsAssignableFrom(implementationType))
                    {
                        runtime.RegisterStatelessServiceFactory(
                            item.ServiceTypeName, 
                            (IStatelessServiceFactory)Activator.CreateInstance(implementationType));
                    }
                    else if (typeof(IStatefulServiceFactory).IsAssignableFrom(implementationType))
                    {
                        runtime.RegisterStatefulServiceFactory(
                            item.ServiceTypeName, 
                            (IStatefulServiceFactory)Activator.CreateInstance(implementationType));
                    }
                    else
                    {
                        runtime.RegisterServiceType(item.ServiceTypeName, implementationType);
                    }
                }
            }
        }

        protected override void Deactivate()
        {
        }

        public static string CreateExtensionValueFromTypeInformation(string codePackageName, Type serviceTypeImplementation)
        {
            return string.Format(ServiceTypeExtensionFormatString, codePackageName, serviceTypeImplementation.AssemblyQualifiedName);
        }

        /// <summary>
        /// Return codepackagename and service implementation type string
        /// </summary>
        /// <param name="extensionValue"></param>
        /// <returns></returns>
        private static Tuple<string, string> GetImplementationTypeFromExtension(string extensionValue)
        {
            // Get rid of BOM
            if (extensionValue[0] == 0xfeff)
            {
                extensionValue = extensionValue.Substring(1);
            }

            // the actual extension data is stored inside the extension element
            string innerText = XElement.Parse(extensionValue).Value;

            string[] fields = innerText.Split(new string[] { DefaultEntryPoint.ServiceTypeExtensionDelimiter } , StringSplitOptions.None);
            if (fields.Length != 2)
            {
                LogHelper.Log("Warning: Failed to parse extension: {0}", extensionValue);
                throw new InvalidOperationException();
            }

            return Tuple.Create(fields[0], fields[1]);
        }
    }
}