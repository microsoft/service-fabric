// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Hosting
{
    using System;

    internal interface IFabricHostEntryPoint
    {
        /// <summary>
        /// The number of code packages that are currently active
        /// </summary>
        int ActiveCodePackageCount { get; }

        /// <summary>
        /// Called when the object has been instantiated in its own app domain
        /// All internal initialization can be done here
        /// <param name="hostEntryPointManagerUniqueId">A unique identifier for the host entry point manager. Useful for creating trace file names etc</param>
        /// <param name="logDirectory">the log directory for this activation context</param>
        /// <param name="workDirectory">the work directory for this activation context</param>
        /// <param name="appDomainConfigFilePath">config file for appDomain</param>
        /// </summary>
        void Start(string logDirectory, string workDirectory, string appDomainConfigFilePath, string hostEntryPointManagerUniqueId);

        /// <summary>
        /// Activate a code package
        /// All service types associated with the code package should be registered at this point
        /// </summary>
        /// <param name="activationContextId">a unique identifier for this particular activation request. the same value will be passed into Deactivate</param>
        /// <param name="codePackageNameToActivate">the name of the code package to activate</param>
        /// <param name="nativeCodePackageActivationContext">the code package activation context as obtained from native code</param>
        /// <param name="nativeFabricRuntime">the raw IFabricRuntime that should be used for all operations pertaining to this code package</param>
        void ActivateCodePackage(
            string activationContextId,
            string codePackageNameToActivate,
            IntPtr nativeCodePackageActivationContext,
            IntPtr nativeFabricRuntime);

        /// <summary>
        /// Deactivate a code package
        /// The code package id is the same as passed in to the Activate call
        /// The same IFabricRuntime should be used to unregister the factory that was used in the activate call
        /// The IFabricRuntime should be released at this point
        /// </summary>
        /// <param name="activationContextId"></param>
        void DeactivateCodePackage(string activationContextId);

        /// <summary>
        /// Called before the appdomain hosting this entrypoint is unloaded
        /// Any active code packages should now be unloaded
        /// </summary>
        void Stop();
    }
}