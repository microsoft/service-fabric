// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ServiceModel;
using System.ServiceModel.Channels;
using System.Reflection;
using System.Fabric;
using System.Fabric.Common;
using System.Fabric.Description;
using System.IdentityModel.Tokens;
using System.Net;
using System.Net.Security;
using System.Fabric.Strings;
using System.Fabric.Common.Tracing;
using System.IO;

namespace System.Fabric.dSTSClient
{
    [Serializable]
    public class dSTSClient
    {
        private static readonly string TraceType = "DSTSClient";

        private readonly string metadataEndpoint;
        private readonly string[] serverCommonNames;
        private readonly string[] serverThumbprints;
        private readonly bool interactive;
        private readonly string cloudServiceName;
        private readonly string[] cloudServiceDNSNames;

        internal static FabricEvents.ExtensionsEvents TraceSource = FabricEvents.ExtensionsEvents.GetEventSource(FabricEvents.Tasks.DSTSClient);

        private dSTSClient(
            string metadataEndpoint,
            string[] serverCommonNames,
            string[] serverThumbprints,
            bool interactive,
            string cloudServiceName,
            string[] cloudServiceDNSNames)
        {
            this.metadataEndpoint = metadataEndpoint;
            this.serverCommonNames = serverCommonNames;
            this.serverThumbprints = serverThumbprints;
            this.interactive = interactive;
            this.cloudServiceName = cloudServiceName;
            this.cloudServiceDNSNames = cloudServiceDNSNames;
        }

        public string GetSecurityToken()
        {
            AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(ResolveHandler);
            string token = null;

            try
            {
                token = LoadLibraryAndGetToken();
            }
            catch (TargetInvocationException e)
            {
                if (e.InnerException != null)
                {
                    throw e.InnerException;
                }
                else
                {
                    throw;
                }
            }
            finally
            {
                AppDomain.CurrentDomain.AssemblyResolve -= new ResolveEventHandler(ResolveHandler);
            }

            return token;
        }

        private string LoadLibraryAndGetToken()
        {
            Assembly module;
            var assembly = Path.Combine(FabricEnvironment.GetCodePath(), Constants.dSTSClientLibraryName);
            try
            {
                module = Assembly.LoadFrom(assembly);
            }
            catch (FileNotFoundException e)
            {
                TraceSource.WriteWarning(
                    TraceType,
                    "Unable to load assembly or one of its dependencies: {0}",
                    assembly);

                throw new FabricException(StringResources.Error_dSTSOptionNotSupported, e);
            }

            Type dSTSClient;
            dSTSClient = module.GetType(Constants.dSTSClientName, false);
            if (dSTSClient == null)
            {
                TraceSource.WriteWarning(
                    TraceType,
                    "Unable to get type : {0}",
                    Constants.dSTSClientName);

                throw new FabricException(StringResources.Error_dSTSOptionNotSupported);
            }

            MethodInfo GetSecurityTokenInfo = dSTSClient.GetMethod(Constants.GetSecurityToken, new Type[] { });
            MethodInfo CreatedSTSClientInfo = dSTSClient.GetMethod(Constants.CreatedSTSClient, BindingFlags.Static | BindingFlags.Public);

            if (GetSecurityTokenInfo == null ||
                CreatedSTSClientInfo == null)
            {
                TraceSource.WriteWarning(
                    TraceType,
                    "Unable to get methods : {0}, {1}",
                    Constants.GetSecurityToken,
                    Constants.CreatedSTSClient);

                throw new FabricException(StringResources.Error_dSTSOptionNotSupported);
            }

            object client = CreatedSTSClientInfo.Invoke(null, new object[] { metadataEndpoint, serverCommonNames, serverThumbprints, interactive, cloudServiceName, cloudServiceDNSNames });
            if (client == null)
            {
                TraceSource.WriteWarning(
                    TraceType,
                    "Method invocation failure - MethodName {0}",
                    Constants.CreatedSTSClient);

                throw new FabricException(StringResources.Error_dSTSOptionNotSupported);
            }

            return (string)GetSecurityTokenInfo.Invoke(client, null);
        }

        private Assembly ResolveHandler(object sender, ResolveEventArgs args)
        {
            // The ResolveHandler is called if the dsts client implementation binaries are not in the 
            // same location as the executable.
            string assemblyToLoad = args.Name.Substring(0, args.Name.IndexOf(",")) + ".dll";

            foreach (string assembly in Constants.dSTSKnownDlls)
            {
                if (assembly == assemblyToLoad)
                {
                    string assemblyPath = Path.Combine(FabricEnvironment.GetCodePath(), assemblyToLoad);
                    return Assembly.LoadFrom(assemblyPath);
                }
            }

            return null;
        }

        public static dSTSClient CreatedSTSClient(
            string metadataUrl,
            string[] serverCommonNames,
            bool interactive = false)
        {
            return CreatedSTSClient(metadataUrl, serverCommonNames, null, null, null, interactive);
        }

        public static dSTSClient CreatedSTSClient(
            string metadataUrl,
            string[] serverCommonNames,
            string[] serverThumbprints,
            bool interactive = false)
        {
            return CreatedSTSClient(metadataUrl, serverCommonNames, serverThumbprints, null, null, interactive);
        }

        public static dSTSClient CreatedSTSClient(
            string metadataUrl,
            string[] serverCommonNames,
            string[] serverThumbprints,
            string cloudServiceName,
            string[] cloudServiceDNSNames,
            bool interactive = false)
        {
            return new dSTSClient(metadataUrl, serverCommonNames, serverThumbprints, interactive, cloudServiceName, cloudServiceDNSNames);
        }
    }
}
