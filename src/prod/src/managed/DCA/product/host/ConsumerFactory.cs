// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Fabric.Dca;
    using System.Fabric.Health;
    using System.Reflection;
    
    /// <summary>
    /// Factory class to create consumers
    /// </summary>
    internal class ConsumerFactory
    {
        // Constants
        private const string TraceType = "ConsumerFactory";

        internal IDcaConsumer CreateConsumer(
                                         string consumerInstance, 
                                         ConsumerInitializationParameters initParam, 
                                         string moduleName, 
                                         string typeName)
        {
            // Load the consumer assembly
            Action<string> reportError = message =>
            {
                Utility.TraceSource.WriteError(TraceType, message);
            };

            Assembly assembly;
            try
            {
                assembly = Assembly.Load(new AssemblyName(moduleName));
            }
            catch (Exception e)
            {
                var message = string.Format(
                    "Exception occured while loading assembly {0} for creating consumer {1}. The consumer type might not be available in the runtime version you are using.",
                    moduleName,
                    consumerInstance);
                reportError(string.Format("{0} Exception information: {1}.", message, e));
                throw new InvalidOperationException(message, e);
            }

            // Get the type of the consumer object from the assembly
            Type type;
            try
            {
                type = assembly.GetType(typeName);
                if (null == type)
                {
                    throw new TypeLoadException(string.Format("Could not load type {0} from assembly {1}.", typeName, assembly.FullName));
                }
            }
            catch (Exception e)
            {
                var message = string.Format(
                    "Exception occured while retrieving type {0} in assembly {1} for creating consumer {2}. The consumer type might not be available in the runtime version you are using.",
                    typeName,
                    moduleName,
                    consumerInstance);
                reportError(string.Format("{0} Exception information: {1}.", message, e));
                throw new InvalidOperationException(message, e);
            }

            // Create the consumer object
            object consumerObject;
            try
            {
#if DotNetCoreClr
                consumerObject = Activator.CreateInstance(type, new object[] { initParam });
#else
                consumerObject = Activator.CreateInstance(
                    type, 
                    BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance, 
                    null, 
                    new object[] { initParam }, 
                    null);
#endif
            }
            catch (Exception e)
            {
                var message = string.Format(
                    "Exception occured while creating an object of type {0} (assembly {1}) for creating consumer {2}.",
                    typeName,
                    moduleName,
                    consumerInstance);
                reportError(string.Format("{0} Exception information: {1}.", message, e));
                throw new InvalidOperationException(message, e);
            }

            IDcaConsumer consumer;
            try
            {
                consumer = (IDcaConsumer)consumerObject;
            }
            catch (InvalidCastException e)
            {
                var message = string.Format(
                    "Exception occured while casting an object of type {0} (assembly {1}) to interface IDcaConsumer while creating consumer {2}",
                    typeName,
                    moduleName,
                    consumerInstance);
                reportError(string.Format("{0} Exception information: {1}.", message, e));
                throw new InvalidOperationException(message, e);
            }

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Created consumer {0}.",
                consumerInstance);
            return consumer;
        }
    }
}