// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if !DotNetCoreClrLinux

namespace System.Fabric.FabricDeployer
{
    using System.IO;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Helper class for common Dism operations.
    /// Dism - <see cref="https://msdn.microsoft.com/en-us/windows/hardware/commercialize/manufacture/desktop/what-is-dism"/>
    /// </summary>
    internal static class DismHelper
    {
        /// <summary>
        /// Obtained from dismapi.h for the string representation of an online Dism image.
        /// </summary>
        private const string DismOnlineImagePath = "DISM_{53BFAE52-B167-4E2F-A258-0A37B57FF845}";
        
        /// <summary>
        /// Gets the state of the feature.
        /// </summary>
        public static DismPackageFeatureState GetFeatureState(string featureName)
        {
            featureName.Validate("featureName");

            var logFile = Path.GetTempFileName();

            DismSession session = null;
            IntPtr pFeatureInfo = IntPtr.Zero;

            try
            {
                DeployerTrace.WriteInfo("Dism initializing...");
                Execute(() => DismNativeMethods.DismInitialize(DismLogLevel.LogErrorsWarnings, logFile, null));

                DeployerTrace.WriteInfo("Dism initializing completed, opening online session...");
                Execute(() => DismNativeMethods.DismOpenSession(DismOnlineImagePath, null, null, out session));

                if (session == null)
                {
                    throw new InvalidOperationException("NativeMethods.DismOpenSession: session out parameter is null. Not continuing further.");
                }

                DeployerTrace.WriteInfo("Dism opening online session completed, getting feature info...");
                Execute(() => DismNativeMethods.DismGetFeatureInfo(session, featureName, string.Empty, DismPackageIdentifier.None, out pFeatureInfo));

                if (pFeatureInfo == IntPtr.Zero)
                {
                    throw new InvalidOperationException("NativeMethods.DismGetFeatureInfo pFeatureInfo out parameter is Zero. Not continuing further.");
                }

                var featureInfo = (DismFeatureInfo) Marshal.PtrToStructure(pFeatureInfo, typeof (DismFeatureInfo));

                DeployerTrace.WriteInfo("Dism feature {0} is in state: {1}", featureName, featureInfo.FeatureState);
                return featureInfo.FeatureState;
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteWarning("Error detecting if {0} feature is present. Exception: {1}", featureName, ex);
                throw;
            }
            finally
            {
                try
                {
                    if (session != null)
                    {
                        DeployerTrace.WriteInfo("Dism closing session...");
                        Execute(() => DismNativeMethods.DismCloseSession(session.DangerousGetHandle()));
                        DeployerTrace.WriteInfo("Dism closing session completed");
                    }
                    else
                    {
                        DeployerTrace.WriteInfo("Dism session is null, closing session not required");
                    }

                    DeployerTrace.WriteInfo("Dism shutting down...");
                    Execute(DismNativeMethods.DismShutdown);
                    DeployerTrace.WriteInfo("Dism shutting down completed");

                    if (File.Exists(logFile))
                    {
                        // log file isn't too big      
                        string text = File.ReadAllText(logFile);
                        DeployerTrace.WriteInfo("Dism log file content: {0}{1}", Environment.NewLine, text);

                        File.Delete(logFile);
                    }
                }
                catch (Exception ex)
                {
                    DeployerTrace.WriteWarning("Error while cleaning up Dism state. Ignoring and trying to continue. Exception: {0}", ex);
                }
            }                        
        }

        private static void Execute(Func<int> nativeFunc)
        {
            int error = nativeFunc();

            if (error == DismNativeMethods.ERROR_SUCCESS)
            {
                return;
            }
            
            var ex = Marshal.GetExceptionForHR(error);
            throw ex;
        }        
    }
}

#endif