// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.IO;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Json;
    using System.Text;

    /// <summary>
    /// <para>Class for code package debug parameters.</para>
    /// </summary>
    [DataContract]
    public class CodePackageDebugParameters
    {
        [DataMember]
        internal string ServiceManifestName;

        [DataMember]
        internal string CodePackageName;

        [DataMember]
        internal string ConfigPackageName;

        [DataMember]
        internal string DataPackageName;

        [DataMember]
        internal string EntryPointType;

        [DataMember]
        internal string DebugExePath;

        [DataMember]
        internal string DebugArguments;

        [DataMember]
        internal string CodePackageLinkFolder;

        [DataMember]
        internal string ConfigPackageLinkFolder;

        [DataMember]
        internal string DataPackageLinkFolder;

        [DataMember]
        internal string LockFile;

        [DataMember]
        internal string WorkingFolder;

        [DataMember]
        internal string DebugParametersFile;

        [DataMember]
        internal string EnvironmentBlock;

        [DataMember]
        internal ContainerDebugParameters ContainerDebugParams;

        ///  DisableReliableCollectionReplicationMode specifies whether stateful service should run in replication mode or not.
        ///  If value is true, service will run as stateless service with standalone reliable collections. Otherwise, stateful service will replication enabled for reliable collections.
        [DataMember]
        internal bool DisableReliableCollectionReplicationMode;

        internal CodePackageDebugParameters(
            string serviceManifestName,
            string codePackageName,
            string configPackageName,
            string dataPackageName,
            string entryPointType,
            string debugExePath,
            string debugArguments,
            string codePackageLinkFolder,
            string configPackageLinkFolder,
            string dataPackageLinkFolder,
            string lockFile,
            string workingFolder,
            string debugParametersFile,
            string environmentBlock,
            ContainerDebugParameters containerDebugParams,
            bool disableReliableCollectionReplicationMode)
        {
            this.ServiceManifestName = serviceManifestName;
            this.CodePackageName = codePackageName;
            this.ConfigPackageName = configPackageName;
            this.DataPackageName = dataPackageName;
            this.EntryPointType = entryPointType;
            this.DebugExePath = debugExePath;
            this.DebugArguments = debugArguments;
            this.CodePackageLinkFolder = codePackageLinkFolder;
            this.ConfigPackageLinkFolder = configPackageLinkFolder;
            this.DataPackageLinkFolder = dataPackageLinkFolder;
            this.LockFile = lockFile;
            this.WorkingFolder = workingFolder;
            this.DebugParametersFile = debugParametersFile;
            this.EnvironmentBlock = environmentBlock;
            this.ContainerDebugParams = containerDebugParams;
            this.DisableReliableCollectionReplicationMode = disableReliableCollectionReplicationMode;
        }

        internal static List<CodePackageDebugParameters> CreateFrom(string debugParameters)
        {
            var serializer = new DataContractJsonSerializer(typeof(CodePackageDebugParameters[]));
            CodePackageDebugParameters[] parameterObj = null;
            byte[] debugParams = Encoding.UTF8.GetBytes(debugParameters);
            using (var strm = new MemoryStream(debugParams))
            {
                parameterObj = serializer.ReadObject(strm) as CodePackageDebugParameters[];
            }

            var debugParameterObjs = new List<CodePackageDebugParameters>();
            if (parameterObj != null)
            {
                for (int i = 0; i < parameterObj.Length; i++)
                {
                    debugParameterObjs.Add(parameterObj[i]);
                }
            }
            return debugParameterObjs;
        }

        /// <summary>
        ///   <para>
        /// Returns JSON string representing CodePackageDebugParameters passed in.
        /// </para>
        /// </summary>
        /// <param name="debugParameters">
        ///   <para>Array of CodePackageDebugParameters that need to be serialized into JSON. </para>
        /// </param>
        /// <returns>
        ///   <para>String representation of the debug parameters.</para>
        /// </returns>
        public static string GetDebugParameters(CodePackageDebugParameters[] debugParameters)
        {
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(CodePackageDebugParameters[]));
            using (MemoryStream stream = new MemoryStream())
            {
                serializer.WriteObject(stream, debugParameters);
                return Encoding.UTF8.GetString(stream.ToArray());
            }
        }
    }

    /// <summary>
    /// <para>Class for container debug parameters.</para>
    /// </summary>
    [DataContract]
    public class ContainerDebugParameters
    {
        [DataMember]
        internal string[] Entrypoint;

        [DataMember]
        internal string[] Volumes;

        [DataMember]
        internal string[] EnvVars;

        [DataMember]
        internal string[] Labels;

        internal ContainerDebugParameters(
            string[] entryPoints,
            string[] volumes,
            string[] envVars,
            string[] labels)
        {
            this.Entrypoint = entryPoints;
            this.EnvVars = envVars;
            this.Volumes = volumes;
            this.Labels = labels;
        }

        internal ContainerDebugParameters()
        {
        }
    }
}