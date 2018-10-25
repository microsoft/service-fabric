// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Allows a stateful replica to configure the <see cref="System.Fabric.FabricReplicator" /> when creating it 
    /// via <see cref="System.Fabric.IStatefulServicePartition.CreateReplicator(System.Fabric.IStateProvider,System.Fabric.ReplicatorSettings)" />.</para>
    /// </summary>
    public sealed class ReplicatorSettings
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.ReplicatorSettings" /> class.</para>
        /// </summary>
        public ReplicatorSettings()
        {
        }

        //
        // Note that if fields are changed in this structure then corresponding changes need to 
        // be made in prod\src\Managed\replicator\PersistentStatefulServiceReplica.cs
        //
        /// <summary>
        /// <para>Defines how long the <see cref="System.Fabric.FabricReplicator" /> waits after it transmits a message from the primary to the 
        /// secondary for the secondary to acknowledge that it has received the message.</para>
        /// </summary>
        /// <value>
        /// <para>The time needed the <see cref="System.Fabric.FabricReplicator" /> waits after it transmits a message from the primary to the 
        /// secondary for the secondary to acknowledge that it has received the message.</para>
        /// </value>
        /// <remarks>
        /// <para>Receiving a message does not necessarily that the message has been processed.</para>
        /// <para>If this timer is exceeded, then the message is retransmitted.</para>
        /// <para>The default value is 5 seconds.</para>
        /// </remarks>
        public TimeSpan? RetryInterval { get; set; }

        /// <summary>
        /// <para>Gets or sets the amount of time that the replicator waits after receiving an operation before sending back an acknowledgment. </para>
        /// </summary>
        /// <value>
        /// <para>The amount of time that the replicator waits after receiving an operation before sending back an acknowledgment.</para>
        /// </value>
        /// <remarks>
        /// <para>Other operations received and acknowledged during this time period will have their acknowledgments sent back in a single message.</para>
        /// <para>Increasing the <see cref="System.Fabric.ReplicatorSettings.BatchAcknowledgementInterval" /> value decreases latency of individual replication 
        /// operations but increases throughput of the replicator.</para>
        /// <para>Default value is 0.05 Seconds (50 milliseconds)</para>
        /// </remarks>
        public TimeSpan? BatchAcknowledgementInterval { get; set; }

        /// <summary>
        /// <para>Configures the address that this replicator will use when communicating with other Replicators.</para>
        /// </summary>
        /// <value>
        /// <para>The address that this replicator will use when communicating with other Replicators.</para>
        /// </value>
        /// <remarks>
        /// <para>String is formatted as “hostname:port”, where hostname can be FQDN or IP address. The default value is localhost:0. If replicator is running inside a container, you should try setting up <see cref="System.Fabric.ReplicatorSettings.ReplicatorListenAddress" /> and <see cref="System.Fabric.ReplicatorSettings.ReplicatorPublishAddress" />.</para>
        /// </remarks>
        public string ReplicatorAddress { get; set; }

        /// <summary>
        /// <para>Allows the service to define security credentials for securing the traffic between replicators.</para>
        /// </summary>
        /// <value>
        /// <para>The service to define security credentials for securing the traffic between replicators.</para>
        /// </value>
        public SecurityCredentials SecurityCredentials { get; set; }

        /// <summary>
        /// <para>Gets or sets the initial size of the replication queue size.</para>
        /// </summary>
        /// <value>
        /// <para>The initial size of the replication queue size.</para>
        /// </value>
        /// <remarks>
        ///   <para />
        /// </remarks>
        public long? InitialReplicationQueueSize { get; set; }

        /// <summary>
        /// <para>Gets or sets the maximum size for the replication queue.</para>
        /// </summary>
        /// <value>
        /// <para>the maximum size for the replication queue.</para>
        /// </value>
        /// <remarks>
        ///   <para />
        /// </remarks>
        public long? MaxReplicationQueueSize { get; set; }

        /// <summary>
        /// <para>Gets or sets the initial size of the copy operation queue inside <see cref="System.Fabric.FabricReplicator" />, which contains 
        /// copy <see cref="System.Fabric.IOperation" />s not yet pumped and processed by the service.</para>
        /// </summary>
        /// <value>
        /// <para>The initial size of the copy operation queue inside <see cref="System.Fabric.FabricReplicator" />, which contains copy 
        /// <see cref="System.Fabric.IOperation" />s not yet pumped and processed by the service.</para>
        /// </value>
        /// <remarks>
        /// <para>The default value is 64. Note that values for this parameter must be a power of 2.</para>
        /// </remarks>
        public long? InitialCopyQueueSize { get; set; }

        /// <summary>
        /// <para>Gets or sets the maximum size of the copy operation queue inside <see cref="System.Fabric.FabricReplicator" />, which contains 
        /// copy <see cref="System.Fabric.IOperation" />s not yet pumped and processed by the service.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum size of the copy operation queue inside <see cref="System.Fabric.FabricReplicator" />, which contains copy 
        /// <see cref="System.Fabric.IOperation" />s not yet pumped and processed by the service.</para>
        /// </value>
        /// <remarks>
        /// <para>If this queue size is reached at the secondary, operations will be buffered in the Primary’s copy queue. If that queue also fills, 
        /// then the Primary will begin seeing <see cref="System.Fabric.FabricErrorCode.ReplicationQueueFull" /> exceptions.</para>
        /// <para>The default value is 1024</para>
        /// </remarks>
        public long? MaxCopyQueueSize { get; set; }

        /// <summary>
        /// <para>Prevents the optimistic acknowledgment of operations in non-persistent services by requiring that the service calls 
        /// <see cref="System.Fabric.IOperation.Acknowledge" /> before it pumps the next operation.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the optimistic acknowledgment of operations in non-persistent services;otherwise, 
        /// <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        /// <remarks>
        /// <para>Non-persistent services which require explicit acknowledgment can set this property to True in order to prevent optimistic acknowledgment 
        /// of the operations by the Replicator. This setting has no effect for persistent services. </para>
        /// <para>The default value is false.</para>
        /// </remarks>
        public bool? RequireServiceAck { get; set; }

        // The following are Ex1 Settings
        /// <summary>
        /// <para>Gets or sets the maximum size for the replication queue memory.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum size for the replication queue memory.</para>
        /// </value>
        /// <remarks>
        /// <para>The default value is 0. This implies there is no memory limit</para>
        /// </remarks>
        public long? MaxReplicationQueueMemorySize { get; set; }

        /// <summary>
        /// <para>Typically, operations in the secondary replicator are kept in the queue to be able to catchup replicas if it is promoted to a primary. With 
        /// this flag enabled, the secondary replicator releases the operation as soon as it is acknowledged by the user service.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the secondary replicator releases the operation as soon as it is acknowledged by the user service; 
        /// otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        /// <remarks>
        /// <para>The default value is false</para>
        /// </remarks>
        public bool? SecondaryClearAcknowledgedOperations { get; set; }

        /// <summary>
        /// <para>Gets or sets the maximum size of a message that can be transmitted via the replicator. These include replication messages, copy messages 
        /// and copy context messages. The unit of representation is bytes.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum size of a message that can be transmitted via the replicator.</para>
        /// </value>
        /// <remarks>
        /// <para>The default value is 50MB</para>
        /// </remarks>
        public long? MaxReplicationMessageSize { get; set; }

        // The following are Ex2 Settings
        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        public bool? UseStreamFaultsAndEndOfStreamOperationAck { get; set; }

        // The following are Ex3 Settings
        /// <summary>
        /// <para>Defines the initial size of the primary replication operation queue inside <see cref="System.Fabric.FabricReplicator" />, which contains 
        /// replication <see cref="System.Fabric.IOperation" />s.The unit here is the number of operations in the queue.</para>
        /// </summary>
        /// <value>
        /// <para>The initial size of the primary replication operation queue inside <see cref="System.Fabric.FabricReplicator" /></para>
        /// </value>
        /// <remarks>
        /// <para>This setting is specific to the Replicator when the role of the service is Primary</para>
        /// <para>The default value is 64.  Note that values for this parameter must be a power of 2.</para>
        /// </remarks>
        public long? InitialPrimaryReplicationQueueSize { get; set; }

        /// <summary>
        /// <para>Defines the maximum size of the primary replication operation queue inside <see cref="System.Fabric.FabricReplicator" />, which contains 
        /// replication <see cref="System.Fabric.IOperation" />s</para>
        /// </summary>
        /// <value>
        /// <para>The maximum size of the primary replication operation queue inside <see cref="System.Fabric.FabricReplicator" />, which contains 
        /// replication <see cref="System.Fabric.IOperation" />s. The unit here is the number of operations in the queue.</para>
        /// </value>
        /// <remarks>
        /// <para>If this queue size is reached, then the Primary will begin seeing <see cref="System.Fabric.FabricErrorCode.ReplicationQueueFull" /> exceptions.</para>
        /// <para>The default value is 1024 Note that values for this parameter must be a power of 2.</para>
        /// <para>This setting is specific to the Replicator when the role of the service is Primary</para>
        /// </remarks>
        public long? MaxPrimaryReplicationQueueSize { get; set; }

        /// <summary>
        /// <para>Defines the maximum size of the primary replication operation queue inside <see cref="System.Fabric.FabricReplicator" />, which contains 
        /// replication <see cref="System.Fabric.IOperation" />s</para>
        /// </summary>
        /// <value>
        /// <para>. Returns the maximum size of the primary replication operation queue inside <see cref="System.Fabric.FabricReplicator" />, which contains 
        /// replication <see cref="System.Fabric.IOperation" />s The unit here is the virtual memory consumption of the queue .Returns <see cref="System.Int64" />.</para>
        /// </value>
        /// <remarks>
        /// <para>This setting is specific to the Replicator when the role of the service is Primary. The default value is 0. This implies there is no memory limit</para>
        /// </remarks>
        public long? MaxPrimaryReplicationQueueMemorySize { get; set; }

        /// <summary>
        /// <para>Defines the initial size of the secondary replication operation queue inside <see cref="System.Fabric.FabricReplicator" />, which contains 
        /// replication <see cref="System.Fabric.IOperation" />s </para>
        /// </summary>
        /// <value>
        /// <para>The initial size of the secondary replication operation queue inside <see cref="System.Fabric.FabricReplicator" />, which contains 
        /// replication <see cref="System.Fabric.IOperation" />s not yet pumped and processed by the service. The unit here is the number of operations in the queue </para>
        /// </value>
        /// <remarks>
        /// <para>This setting is specific to the Replicator when the role of the service is Secondary/Idle</para>
        /// <para>The default value is 64.  Note that values for this parameter must be a power of 2.</para>
        /// </remarks>
        public long? InitialSecondaryReplicationQueueSize { get; set; }

        /// <summary>
        /// <para>Defines the maximum size of the secondary replication operation queue inside <see cref="System.Fabric.FabricReplicator" />, which contains 
        /// replication <see cref="System.Fabric.IOperation" />s </para>
        /// </summary>
        /// <value>
        /// <para>The maximum size of the secondary replication operation queue inside <see cref="System.Fabric.FabricReplicator" />, which contains 
        /// replication <see cref="System.Fabric.IOperation" />s not yet pumped and processed by the service. The unit here is the number of operations in the queue</para>
        /// </value>
        /// <remarks>
        /// <para>If this queue size is reached, operations will be buffered in the Primary’s replication queue.  If that queue also fills, then the Primary 
        /// will begin seeing <see cref="System.Fabric.FabricErrorCode.ReplicationQueueFull" /> exceptions.</para>
        /// <para>The default value is 2048.Note that values for this parameter must be a power of 2.</para>
        /// <para>This setting is specific to the Replicator when the role of the service is Secondary/Idle</para>
        /// </remarks>
        public long? MaxSecondaryReplicationQueueSize { get; set; }

        /// <summary>
        /// <para>Defines the maximum size of the secondary replication operation queue inside <see cref="System.Fabric.FabricReplicator" />, which contains 
        /// replication <see cref="System.Fabric.IOperation" />s.</para>
        /// </summary>
        /// <value>
        /// <para>Returns the maximum size of the secondary replication operation queue inside <see cref="System.Fabric.FabricReplicator" />, which contains 
        /// replication <see cref="System.Fabric.IOperation" />s. The unit here is the virtual memory consumption of the queue.</para>
        /// </value>
        /// <remarks>
        /// <para>This setting is specific to the Replicator when the role of the service is Secondary/Idle. The default value is 0. This implies there is no memory limit</para>
        /// </remarks>
        public long? MaxSecondaryReplicationQueueMemorySize { get; set; }
        /// <summary>
        /// <para>Defines how long the primary replicator waits for receiving a quorum of acknowledgments for any pending replication operations before 
        /// processing a reconfiguration request, that could potentially result in ‘cancelling’ the pending replication operations.</para>
        /// </summary>
        /// <value>
        /// <para>Amount of time the primary replicator waits for receiving a quorum of acknowledgments for any pending replication operations when there is 
        /// a request for the primary replicator to process a reconfiguration <see cref="System.TimeSpan" />.</para>
        /// </value>
        /// <remarks>
        /// <para>The default value is 0. This implies that reconfigurations aren’t waited upon for receiving quorum on the pending replication operations. 
        /// This helps in completing reconfigurations sooner. Note that larger values for this parameter could potentially result in slower reconfigurations, 
        /// implying longer durations to fail-over a primary. </para>
        /// </remarks>
        public TimeSpan? PrimaryWaitForPendingQuorumsTimeout { get; set; }

        /// <summary>
        /// <para>Configures the listen address that this replicator will use to receieve information from other Replicators.</para>
        /// </summary>
        /// <value>
        /// <para>The listen address that this replicator will use to receive information from other Replicators.</para>
        /// </value>
        /// <remarks>
        /// <para>String is formatted as “hostname:port”, where hostname can be FQDN or IP address. The default value is localhost:0. hostname for listen address can be obtained from <see cref="System.Fabric.CodePackageActivationContext.ServiceListenAddress" /></para>
        /// </remarks>
        public string ReplicatorListenAddress { get; set; }

        /// <summary>
        /// <para>Configures the publish address that this replicator will use to send information to other Replicators.</para>
        /// </summary>
        /// <value>
        /// <para>The publish address that this replicator will use to send information to other Replicators.</para>
        /// </value>
        /// <remarks>
        /// <para>String is formatted as “hostname:port”, where hostname can be FQDN or IP address. The default value is localhost:0. hostname for publish address can be obtained from <see cref="System.Fabric.CodePackageActivationContext.ServicePublishAddress" /></para>
        /// </remarks>
        public string ReplicatorPublishAddress { get; set; }

        /// <summary>
        /// <para>Loads the <see cref="System.Fabric.ReplicatorSettings" /> object from the service configuration settings file.</para>
        /// </summary>
        /// <param name="codePackageActivationContext">
        /// <para>The current code package activation context <see cref="System.Fabric.CodePackageActivationContext" /></para>
        /// </param>
        /// <param name="configPackageName">
        /// <para>The current configuration package name</para>
        /// </param>
        /// <param name="sectionName">
        /// <para>The section within the configuration file that defines all the replicator settings</para>
        /// </param>
        /// <returns>
        /// <para>The loaded <see cref="System.Fabric.ReplicatorSettings" /> object from the service configuration settings file</para>
        /// </returns>
        /// <remarks>
        /// <para> The configuration settings file (settings.xml) within the service configuration folder generally contains all the replicator settings that is needed to pass in the <see cref="System.Fabric.ReplicatorSettings" /> object to the <see cref="System.Fabric.IStatefulServicePartition.CreateReplicator" /> method. Typically, the onus is on the service author to read the settings.xml file, parse the values and appropriately construct the <see cref="System.Fabric.ReplicatorSettings" /> object.</para>
        /// <para>With the current helper method, the service author can bypass the above process.</para>
        /// <para>The following are the parameter names that should be provided in the service configuration “settings.xml”, to be recognizable by windows fabric to perform the above parsing automatically:</para>
        /// <list type="number">
        /// <item>
        /// <description>
        /// <para>BatchAcknowledgementInterval –<see cref="System.Fabric.ReplicatorSettings.BatchAcknowledgementInterval" /> value in seconds</para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>InitialCopyQueueSize -<see cref="System.Fabric.ReplicatorSettings.InitialCopyQueueSize" /></para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>MaxCopyQueueSize -<see cref="System.Fabric.ReplicatorSettings.MaxCopyQueueSize" /></para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>MaxReplicationMessageSize -<see cref="System.Fabric.ReplicatorSettings.MaxReplicationMessageSize" /></para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>RetryInterval -<see cref="System.Fabric.ReplicatorSettings.RetryInterval" /> value in seconds</para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>RequireServiceAck -<see cref="System.Fabric.ReplicatorSettings.RequireServiceAck" /></para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>ReplicatorAddress or ReplicatorEndpoint – ReplicatorAddress should be of the form IPort. ReplicatorEndpoint must reference 
        /// a valid service endpoint resource from the service manifest -<see cref="System.Fabric.ReplicatorSettings.ReplicatorAddress" /></para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>ReplicatorListenAddress or ReplicatorEndpoint – ReplicatorListenAddress should be of the form IPort. ReplicatorEndpoint must reference 
        /// a valid service endpoint resource from the service manifest -<see cref="System.Fabric.ReplicatorSettings.ReplicatorListenAddress" /></para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>ReplicatorPublishAddress or ReplicatorEndpoint – ReplicatorPublishAddress should be of the form IPort. ReplicatorEndpoint must reference 
        /// a valid service endpoint resource from the service manifest -<see cref="System.Fabric.ReplicatorSettings.ReplicatorPublishAddress" /></para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>SecondaryClearAcknowledgedOperations -<see cref="System.Fabric.ReplicatorSettings.SecondaryClearAcknowledgedOperations" /></para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>PrimaryWaitForPendingQuorumsTimeout - <see cref="System.Fabric.ReplicatorSettings.PrimaryWaitForPendingQuorumsTimeout" /> value in seconds</para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>UseStreamFaultsAndEndOfStreamOperationAck -<see cref="System.Fabric.ReplicatorSettings.UseStreamFaultsAndEndOfStreamOperationAck" /></para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>InitialPrimaryReplicationQueueSize -<see cref="System.Fabric.ReplicatorSettings.InitialPrimaryReplicationQueueSize" /></para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>InitialSecondaryReplicationQueueSize -<see cref="System.Fabric.ReplicatorSettings.InitialSecondaryReplicationQueueSize" /></para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>MaxPrimaryReplicationQueueSize -<see cref="System.Fabric.ReplicatorSettings.MaxPrimaryReplicationQueueSize" /></para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>MaxSecondaryReplicationQueueSize -<see cref="System.Fabric.ReplicatorSettings.MaxSecondaryReplicationQueueSize" /></para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>MaxPrimaryReplicationQueueMemorySize -<see cref="System.Fabric.ReplicatorSettings.MaxPrimaryReplicationQueueMemorySize" /></para>
        /// </description>
        /// </item>
        /// <item>
        /// <description>
        /// <para>MaxSecondaryReplicationQueueMemorySize -<see cref="System.Fabric.ReplicatorSettings.MaxSecondaryReplicationQueueMemorySize" /></para>
        /// </description>
        /// </item>
        /// </list>
        /// </remarks>
        public static ReplicatorSettings LoadFrom(CodePackageActivationContext codePackageActivationContext, string configPackageName, string sectionName)
        {
            return Utility.WrapNativeSyncInvokeInMTA<ReplicatorSettings>(
                () => LoadFromPrivate(codePackageActivationContext, configPackageName, sectionName), "NativeFabricRuntime::FabricLoadReplicatorSettings");
        }

        private static ReplicatorSettings LoadFromPrivate(CodePackageActivationContext codePackageActivationContext, string configPackageName, string sectionName)
        {
            using (var pin = new PinCollection())
            {
                var nativeSectionName = pin.AddBlittable(sectionName);
                var nativeConfigPackageName = pin.AddBlittable(configPackageName);
                NativeRuntime.IFabricReplicatorSettingsResult replicatorSettingsResult =
                    NativeRuntime.FabricLoadReplicatorSettings(
                        codePackageActivationContext.NativeActivationContext,
                        nativeConfigPackageName,
                        nativeSectionName);

                return ReplicatorSettings.CreateFromNative(replicatorSettingsResult);
            }
        }

        internal IntPtr ToNative(PinCollection pin)
        {
            NativeTypes.FABRIC_REPLICATOR_SETTINGS nativeObj = new NativeTypes.FABRIC_REPLICATOR_SETTINGS();
            nativeObj.Reserved = IntPtr.Zero;

            NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS flags = NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.REPLICATOR_SETTING_NONE;
            NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX1[] ex1Settings = null;
            NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX2[] ex2Settings = null;
            NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX3[] ex3Settings = null;
            NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX4[] ex4Settings = null;

            // If any Ex1 or Ex2 or Ex3 or Ex4 settings are specified
            if (IsEx1SettingsPresent() ||
                IsEx2SettingsPresent() ||
                IsEx3SettingsPresent() ||
                IsEx4SettingsPresent())
            {
                ex1Settings = new NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX1[1];
                ex1Settings[0].Reserved = IntPtr.Zero;
                nativeObj.Reserved = pin.AddBlittable(ex1Settings);

                // If any Ex2 or Ex3 or Ex4 settings are specified
                if (IsEx2SettingsPresent() ||
                    IsEx3SettingsPresent() ||
                    IsEx4SettingsPresent())
                {
                    ex2Settings = new NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX2[1];
                    ex2Settings[0].Reserved = IntPtr.Zero;

                    ex1Settings[0].Reserved = pin.AddBlittable(ex2Settings);

                    // If any Ex3 or Ex4 settings are specified
                    if (IsEx3SettingsPresent() ||
                        IsEx4SettingsPresent())
                    {
                        ex3Settings = new NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX3[1];
                        ex3Settings[0].Reserved = IntPtr.Zero;

                        ex2Settings[0].Reserved = pin.AddBlittable(ex3Settings);

                        //If any Ex4 settings are specified
                        if (IsEx4SettingsPresent())
                        {
                            ex4Settings = new NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX4[1];
                            ex4Settings[0].Reserved = IntPtr.Zero;

                            ex3Settings[0].Reserved = pin.AddBlittable(ex4Settings);
                        }
                    }
                }
            }

            if (!string.IsNullOrEmpty(this.ReplicatorAddress))
            {
                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.REPLICATOR_ADDRESS;
                nativeObj.ReplicatorAddress = pin.AddBlittable(this.ReplicatorAddress);
            }

            if (this.RetryInterval.HasValue)
            {
                ThrowIfValueOutOfBounds((long)this.RetryInterval.Value.TotalMilliseconds, "RetryInterval");

                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_RETRY_INTERVAL;
                nativeObj.RetryIntervalMilliseconds = (uint)this.RetryInterval.Value.TotalMilliseconds;
            }

            if (this.BatchAcknowledgementInterval.HasValue)
            {
                ThrowIfValueOutOfBounds((long)this.BatchAcknowledgementInterval.Value.TotalMilliseconds, "BatchAcknowledgementInterval");

                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL;
                nativeObj.BatchAcknowledgementIntervalMilliseconds = (uint)this.BatchAcknowledgementInterval.Value.TotalMilliseconds;
            }

            if (this.SecurityCredentials != null)
            {
                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_SECURITY;

                nativeObj.SecurityCredentials = this.SecurityCredentials.ToNative(pin);
            }
            else
            {
                nativeObj.SecurityCredentials = IntPtr.Zero;
            }

            if (this.InitialReplicationQueueSize != null)
            {
                ThrowIfValueOutOfBounds(this.InitialReplicationQueueSize.Value, "InitialReplicationQueueSize");

                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE;
                nativeObj.InitialReplicationQueueSize = (uint)this.InitialReplicationQueueSize.Value;
            }

            if (this.InitialCopyQueueSize != null)
            {
                ThrowIfValueOutOfBounds(this.InitialCopyQueueSize.Value, "InitialCopyQueueSize");

                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE;
                nativeObj.InitialCopyQueueSize = (uint)this.InitialCopyQueueSize.Value;
            }

            if (this.MaxReplicationQueueSize != null)
            {
                ThrowIfValueOutOfBounds(this.MaxReplicationQueueSize.Value, "MaxReplicationQueueSize");

                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE;
                nativeObj.MaxReplicationQueueSize = (uint)this.MaxReplicationQueueSize.Value;
            }

            if (this.MaxCopyQueueSize != null)
            {
                ThrowIfValueOutOfBounds(this.MaxCopyQueueSize.Value, "MaxCopyQueueSize");

                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE;
                nativeObj.MaxCopyQueueSize = (uint)this.MaxCopyQueueSize.Value;
            }

            if (this.RequireServiceAck != null)
            {
                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK;
                nativeObj.RequireServiceAck = NativeTypes.ToBOOLEAN(this.RequireServiceAck.Value);
            }

            if (this.MaxReplicationQueueMemorySize != null)
            {
                ThrowIfValueOutOfBounds(this.MaxReplicationQueueMemorySize.Value, "MaxReplicationQueueMemorySize");

                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
                ex1Settings[0].MaxReplicationQueueMemorySize = (uint)this.MaxReplicationQueueMemorySize.Value;
            }

            if (this.SecondaryClearAcknowledgedOperations != null)
            {
                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS;
                ex1Settings[0].SecondaryClearAcknowledgedOperations = NativeTypes.ToBOOLEAN(this.SecondaryClearAcknowledgedOperations.Value);
            }

            if (this.MaxReplicationMessageSize != null)
            {
                ThrowIfValueOutOfBounds(this.MaxReplicationMessageSize.Value, "MaxReplicationMessageSize");

                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE;
                ex1Settings[0].MaxReplicationMessageSize = (uint)this.MaxReplicationMessageSize.Value;
            }

            if (this.UseStreamFaultsAndEndOfStreamOperationAck != null)
            {
                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK;
                ex2Settings[0].UseStreamFaultsAndEndOfStreamOperationAck = NativeTypes.ToBOOLEAN(this.UseStreamFaultsAndEndOfStreamOperationAck.Value);
            }

            if (this.InitialPrimaryReplicationQueueSize != null)
            {
                ThrowIfValueOutOfBounds(this.InitialPrimaryReplicationQueueSize.Value, "InitialPrimaryReplicationQueueSize");

                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE;
                ex3Settings[0].InitialPrimaryReplicationQueueSize = (uint)this.InitialPrimaryReplicationQueueSize.Value;
            }

            if (this.MaxPrimaryReplicationQueueSize != null)
            {
                ThrowIfValueOutOfBounds(this.MaxPrimaryReplicationQueueSize.Value, "MaxPrimaryReplicationQueueSize");

                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;
                ex3Settings[0].MaxPrimaryReplicationQueueSize = (uint)this.MaxPrimaryReplicationQueueSize.Value;
            }

            if (this.MaxPrimaryReplicationQueueMemorySize != null)
            {
                ThrowIfValueOutOfBounds(this.MaxPrimaryReplicationQueueMemorySize.Value, "MaxPrimaryReplicationQueueMemorySize");

                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
                ex3Settings[0].MaxPrimaryReplicationQueueMemorySize = (uint)this.MaxPrimaryReplicationQueueMemorySize.Value;
            }

            if (this.InitialSecondaryReplicationQueueSize != null)
            {
                ThrowIfValueOutOfBounds(this.InitialSecondaryReplicationQueueSize.Value, "InitialSecondaryReplicationQueueSize");

                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;
                ex3Settings[0].InitialSecondaryReplicationQueueSize = (uint)this.InitialSecondaryReplicationQueueSize.Value;
            }

            if (this.MaxSecondaryReplicationQueueSize != null)
            {
                ThrowIfValueOutOfBounds(this.MaxSecondaryReplicationQueueSize.Value, "MaxSecondaryReplicationQueueSize");

                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;
                ex3Settings[0].MaxSecondaryReplicationQueueSize = (uint)this.MaxSecondaryReplicationQueueSize.Value;
            }

            if (this.MaxSecondaryReplicationQueueMemorySize != null)
            {
                ThrowIfValueOutOfBounds(this.MaxSecondaryReplicationQueueMemorySize.Value, "MaxSecondaryReplicationQueueMemorySize");

                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
                ex3Settings[0].MaxSecondaryReplicationQueueMemorySize = (uint)this.MaxSecondaryReplicationQueueMemorySize.Value;
            }

            if (this.PrimaryWaitForPendingQuorumsTimeout.HasValue)
            {
                ThrowIfValueOutOfBounds((long)this.PrimaryWaitForPendingQuorumsTimeout.Value.TotalMilliseconds, "PrimaryWaitForPendingQuorumsTimeout");

                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT;
                ex3Settings[0].PrimaryWaitForPendingQuorumsTimeoutMilliseconds = (uint)this.PrimaryWaitForPendingQuorumsTimeout.Value.TotalMilliseconds;
            }

            if (!string.IsNullOrEmpty(this.ReplicatorListenAddress))
            {
                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_LISTEN_ADDRESS;
                ex4Settings[0].ReplicatorListenAddress = pin.AddBlittable(this.ReplicatorListenAddress);
            }

            if (!string.IsNullOrEmpty(this.ReplicatorPublishAddress))
            {
                flags |= NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_PUBLISH_ADDRESS;
                ex4Settings[0].ReplicatorPublishAddress = pin.AddBlittable(this.ReplicatorPublishAddress);
            }

            nativeObj.Flags = (uint)flags;
            return pin.AddBlittable(nativeObj);
        }

        internal static unsafe ReplicatorSettings CreateFromNative(NativeRuntime.IFabricReplicatorSettingsResult replicatorSettingsResult)
        {
            ReleaseAssert.AssertIfNot(replicatorSettingsResult != null, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "ReplicatorSettingsResult"));

            ReplicatorSettings managedReplicatorSettings = new ReplicatorSettings();

            ReleaseAssert.AssertIfNot(replicatorSettingsResult.get_ReplicatorSettings() != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "ReplicatorSettingsResult.get_ReplicatorSettings()"));
            NativeTypes.FABRIC_REPLICATOR_SETTINGS* nativeReplicatorSettings = (NativeTypes.FABRIC_REPLICATOR_SETTINGS*)replicatorSettingsResult.get_ReplicatorSettings();
            NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX1* nativeReplicatorSettingsEx1 = null;
            NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX2* nativeReplicatorSettingsEx2 = null;
            NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX3* nativeReplicatorSettingsEx3 = null;
            NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX4* nativeReplicatorSettingsEx4 = null;

            if (nativeReplicatorSettings->Reserved != IntPtr.Zero)
            {
                nativeReplicatorSettingsEx1 = (NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX1*)nativeReplicatorSettings->Reserved;

                if (nativeReplicatorSettingsEx1->Reserved != IntPtr.Zero)
                {
                    nativeReplicatorSettingsEx2 = (NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX2*)nativeReplicatorSettingsEx1->Reserved;

                    if (nativeReplicatorSettingsEx2->Reserved != IntPtr.Zero)
                    {
                        nativeReplicatorSettingsEx3 = (NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX3*)nativeReplicatorSettingsEx2->Reserved;
                    }

                    if (nativeReplicatorSettingsEx3->Reserved != IntPtr.Zero)
                    {
                        nativeReplicatorSettingsEx4 = (NativeTypes.FABRIC_REPLICATOR_SETTINGS_EX4*)nativeReplicatorSettingsEx3->Reserved;
                    }
                }
            }

            NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS flags = (NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS)nativeReplicatorSettings->Flags;

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_SECURITY))
            {
                ReleaseAssert.AssertIf(nativeReplicatorSettings->SecurityCredentials == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeReplicatorSettings->SecurityCredentials"));
                managedReplicatorSettings.SecurityCredentials = SecurityCredentials.CreateFromNative((NativeTypes.FABRIC_SECURITY_CREDENTIALS*)nativeReplicatorSettings->SecurityCredentials);
            }

            // Replicator Settings
            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.REPLICATOR_ADDRESS))
            {
                ReleaseAssert.AssertIfNot(nativeReplicatorSettings->ReplicatorAddress != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeReplicatorSettings->ReplicatorAddress"));
                managedReplicatorSettings.ReplicatorAddress = NativeTypes.FromNativeString(nativeReplicatorSettings->ReplicatorAddress);
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL))
            {
                managedReplicatorSettings.BatchAcknowledgementInterval = TimeSpan.FromMilliseconds(nativeReplicatorSettings->BatchAcknowledgementIntervalMilliseconds);
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE))
            {
                managedReplicatorSettings.InitialCopyQueueSize = nativeReplicatorSettings->InitialCopyQueueSize;
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE))
            {
                managedReplicatorSettings.MaxCopyQueueSize = nativeReplicatorSettings->MaxCopyQueueSize;
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE))
            {
                managedReplicatorSettings.InitialReplicationQueueSize = nativeReplicatorSettings->InitialReplicationQueueSize;
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE))
            {
                managedReplicatorSettings.MaxReplicationQueueSize = nativeReplicatorSettings->MaxReplicationQueueSize;
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK))
            {
                managedReplicatorSettings.RequireServiceAck = NativeTypes.FromBOOLEAN(nativeReplicatorSettings->RequireServiceAck);
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_RETRY_INTERVAL))
            {
                managedReplicatorSettings.RetryInterval = TimeSpan.FromMilliseconds(nativeReplicatorSettings->RetryIntervalMilliseconds);
            }

            // Ex1 Settings
            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE))
            {
                ReleaseAssert.AssertIfNot(nativeReplicatorSettings->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Reserved"));
                managedReplicatorSettings.MaxReplicationMessageSize = nativeReplicatorSettingsEx1->MaxReplicationMessageSize;
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE))
            {
                ReleaseAssert.AssertIfNot(nativeReplicatorSettings->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Reserved"));
                managedReplicatorSettings.MaxReplicationQueueMemorySize = nativeReplicatorSettingsEx1->MaxReplicationQueueMemorySize;
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS))
            {
                ReleaseAssert.AssertIfNot(nativeReplicatorSettings->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Reserved"));
                managedReplicatorSettings.SecondaryClearAcknowledgedOperations = NativeTypes.FromBOOLEAN(nativeReplicatorSettingsEx1->SecondaryClearAcknowledgedOperations);
            }

            // Ex2 Settings
            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK))
            {
                ReleaseAssert.AssertIfNot(nativeReplicatorSettings->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx1->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex1 Reserved"));
                managedReplicatorSettings.UseStreamFaultsAndEndOfStreamOperationAck = NativeTypes.FromBOOLEAN(nativeReplicatorSettingsEx2->UseStreamFaultsAndEndOfStreamOperationAck);
            }

            // Ex3 Settings
            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE))
            {
                ReleaseAssert.AssertIfNot(nativeReplicatorSettings->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx1->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex1 Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx2->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex2 Reserved"));
                managedReplicatorSettings.InitialPrimaryReplicationQueueSize = nativeReplicatorSettingsEx3->InitialPrimaryReplicationQueueSize;
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE))
            {
                ReleaseAssert.AssertIfNot(nativeReplicatorSettings->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx1->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex1 Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx2->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex2 Reserved"));
                managedReplicatorSettings.MaxPrimaryReplicationQueueSize = nativeReplicatorSettingsEx3->MaxPrimaryReplicationQueueSize;
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE))
            {
                ReleaseAssert.AssertIfNot(nativeReplicatorSettings->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx1->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex1 Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx2->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex2 Reserved"));
                managedReplicatorSettings.MaxPrimaryReplicationQueueMemorySize = nativeReplicatorSettingsEx3->MaxPrimaryReplicationQueueMemorySize;
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE))
            {
                ReleaseAssert.AssertIfNot(nativeReplicatorSettings->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx1->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex1 Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx2->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex2 Reserved"));
                managedReplicatorSettings.InitialSecondaryReplicationQueueSize = nativeReplicatorSettingsEx3->InitialSecondaryReplicationQueueSize;
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE))
            {
                ReleaseAssert.AssertIfNot(nativeReplicatorSettings->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx1->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex1 Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx2->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex2 Reserved"));
                managedReplicatorSettings.MaxSecondaryReplicationQueueSize = nativeReplicatorSettingsEx3->MaxSecondaryReplicationQueueSize;
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE))
            {
                ReleaseAssert.AssertIfNot(nativeReplicatorSettings->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx1->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex1 Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx2->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex2 Reserved"));
                managedReplicatorSettings.MaxSecondaryReplicationQueueMemorySize = nativeReplicatorSettingsEx3->MaxSecondaryReplicationQueueMemorySize;
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT))
            {
                managedReplicatorSettings.PrimaryWaitForPendingQuorumsTimeout = TimeSpan.FromMilliseconds(nativeReplicatorSettingsEx3->PrimaryWaitForPendingQuorumsTimeoutMilliseconds);
            }

            //Ex4 Settings
            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_LISTEN_ADDRESS))
            {
                ReleaseAssert.AssertIfNot(nativeReplicatorSettings->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx1->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex1 Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx2->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex2 Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx3->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex3 Reserved"));
                managedReplicatorSettings.ReplicatorListenAddress = NativeTypes.FromNativeString(nativeReplicatorSettingsEx4->ReplicatorListenAddress);
            }

            if (flags.HasFlag(NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_PUBLISH_ADDRESS))
            {
                ReleaseAssert.AssertIfNot(nativeReplicatorSettings->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx1->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex1 Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx2->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex2 Reserved"));
                ReleaseAssert.AssertIfNot(nativeReplicatorSettingsEx3->Reserved != IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "Replicator Settings Ex3 Reserved"));
                managedReplicatorSettings.ReplicatorPublishAddress = NativeTypes.FromNativeString(nativeReplicatorSettingsEx4->ReplicatorPublishAddress);
            }

            GC.KeepAlive(replicatorSettingsResult);
            return managedReplicatorSettings;
        }

        private void ThrowIfValueOutOfBounds(long value, string argumentName)
        {
            if (value > UInt32.MaxValue)
            {
                throw new ArgumentOutOfRangeException(argumentName);
            }
        }

        private bool IsEx1SettingsPresent()
        {
            return
                (this.MaxReplicationQueueMemorySize != null ||
                this.SecondaryClearAcknowledgedOperations != null ||
                this.MaxReplicationMessageSize != null);
        }
        private bool IsEx2SettingsPresent()
        {
            return (this.UseStreamFaultsAndEndOfStreamOperationAck != null);
        }
        private bool IsEx3SettingsPresent()
        {
            return
                (this.InitialSecondaryReplicationQueueSize != null ||
                this.MaxSecondaryReplicationQueueSize != null ||
                this.MaxSecondaryReplicationQueueMemorySize != null ||
                this.InitialPrimaryReplicationQueueSize != null ||
                this.MaxPrimaryReplicationQueueMemorySize != null ||
                this.MaxPrimaryReplicationQueueSize != null ||
                this.PrimaryWaitForPendingQuorumsTimeout != null);
        }
        private bool IsEx4SettingsPresent()
        {
            return
                (!string.IsNullOrEmpty(this.ReplicatorListenAddress)||
                 !string.IsNullOrEmpty(this.ReplicatorPublishAddress));
        }

    }
}