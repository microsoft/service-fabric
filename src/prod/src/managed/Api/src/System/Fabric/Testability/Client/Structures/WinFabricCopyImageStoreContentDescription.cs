// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Testability.Client.Structures
{
    using System;
    using System.Fabric.Testability.Common;
    using System.Runtime.Serialization;
    using System.Fabric.Description;
    using System.Fabric.Health;

    /// <summary>
    /// A WinFabricCopyImageStoreContentDescription contains information to perform image store copy operation
    /// This class is currently used by testability only. This might be changed after implementing the image store APIs in FabricClient and PW (#7202139)
    /// </summary>
    [Serializable]
    [DataContract]
    public class WinFabricCopyImageStoreContentDescription
    {
        /// <summary>
        /// Initializes a new instance of the WinFabricCopyImageStoreContentDescription
        /// </summary>
        /// <param name="remoteSource">
        /// Get the source relative path in image store
        /// </param>
        /// <param name="remoteDestination">
        /// Get the destination relative path in image store
        /// </param>
        public WinFabricCopyImageStoreContentDescription(
            string remoteSource,
            string remoteDestination)
        {
            ThrowIf.Null(remoteSource, "remoteSource");
            ThrowIf.Null(remoteDestination, "remoteDestination");

            this.RemoteSource = remoteSource;
            this.RemoteDestination = remoteDestination;
        }

        /// <summary>
        /// Get the source relative path in image store
        /// </summary>
        /// <value>
        /// <para>The source image store relative path</para>
        /// </value>
        [DataMember(Name = "RemoteSource", IsRequired = true)]
        public string RemoteSource
        {
            get;
            private set;
        }

        /// <summary>
        /// Get the destination relative in image store
        /// </summary>
        /// <value>
        /// <para>The destination image store relative path</para>
        /// </value>
        [DataMember(Name = "RemoteDestination", IsRequired = true)]
        public string RemoteDestination
        {
            get;
            private set;
        }

        /// <summary>
        /// Get the list of files which will be skipped during the copy process
        /// </summary>
        /// <value>
        /// <para>A list of files to be skipped over during the copy process</para>
        /// </value>
        [DataMember(EmitDefaultValue = false)]
        public string[] SkipFiles
        {
            get;
            set;
        }

        /// <summary>
        /// Get the 
        /// </summary>
        /// <value>
        /// <para>The </para>
        /// </value>
        [DataMember(EmitDefaultValue = false)]
        public WinFabricImageStoreCopyFlags CopyFlag
        {
            get;
            set;
        }

        /// <summary>
        /// Get the
        /// </summary>
        /// <value>
        /// <para>A flag</para>
        /// </value>
        [DataMember(EmitDefaultValue = false)]
        public bool CheckMarkFile
        {
            get;
            set;
        }
    }

    /// <summary>
    /// Copy flags for image store copying operation
    /// </summary>
    [Flags]
    [Serializable]
    public enum WinFabricImageStoreCopyFlags
    {
        /// <summary>
        /// Invalid copy flag
        /// </summary>
        Invalid = 0,

        /// <summary>
        /// Mirror(atomic) copy but skip if destination already exists.
        /// </summary>
        AtomicCopySkipIfExists = 1,

        /// <summary>
        /// Mirror(atomic) copy.
        /// </summary>
        AtomicCopy = 2,
    }
}