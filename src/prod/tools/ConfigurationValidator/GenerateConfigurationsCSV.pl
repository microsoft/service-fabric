#!perl -w
use strict;
use File::Find;
use WFFileIO;

my $FabricRoot = shift;
my $DestFile = shift;
my $LinuxDestFile = shift;
my $PublicDestFile = shift;
my $ProdRoot = "$FabricRoot";
my $ErrorFile = "GenerateConfigurationsCSV.err";

sub FixupNativeToManagedPropertyValue
{
    my $rawPropertyDefaultValue = shift;

    my $propertyDefaultValue = $rawPropertyDefaultValue;
    if (substr($propertyDefaultValue, 0, 21) eq "TimeSpan::FromSeconds")
    {
        $propertyDefaultValue = substr($propertyDefaultValue, 22, index($propertyDefaultValue, ')') - 22);
    $propertyDefaultValue = "TimeSpan.FromSeconds(".$propertyDefaultValue.")";
    }
    elsif (substr($propertyDefaultValue, 0, 29) eq "Common::TimeSpan::FromSeconds")
    {
        $propertyDefaultValue = substr($propertyDefaultValue, 30, index($propertyDefaultValue, ')') - 30);
    $propertyDefaultValue = "TimeSpan.FromSeconds(".$propertyDefaultValue.")";
    }
    elsif (substr($propertyDefaultValue, 0, 29) eq "Common::TimeSpan::FromMinutes")
    {
        $propertyDefaultValue = substr($propertyDefaultValue, 30, index($propertyDefaultValue, ')') - 30);        
    $propertyDefaultValue = "TimeSpan.FromMinutes(".$propertyDefaultValue.")";
    }
    elsif (substr($propertyDefaultValue, 0, 34) eq "Common::TimeSpan::FromMilliseconds")
    {
        $propertyDefaultValue = substr($propertyDefaultValue, 35, index($propertyDefaultValue, ')') - 35);
    $propertyDefaultValue = "TimeSpan.FromMilliseconds(".$propertyDefaultValue.")";        
    }
    elsif (substr($propertyDefaultValue, 0, 22) eq "Common::TimeSpan::Zero")
    {
        $propertyDefaultValue = "TimeSpan.Zero";
    }
    elsif (substr($propertyDefaultValue, 0, 26) eq "Common::TimeSpan::MaxValue")
    {
        $propertyDefaultValue = "TimeSpan.MaxValue";
    }
    elsif ($propertyDefaultValue eq "")
    {
        $propertyDefaultValue = "\"\"";
    }
    
    return $propertyDefaultValue;
}

my @EntryParserFormats =
    (
        "PUBLIC_CONFIG_ENTRY\\((.*?), (.*?), (.*?), (.*?), (.*?)(, .*)?\\)",
        "TEST_CONFIG_ENTRY\\((.*?), (.*?), (.*?), (.*?), (.*?)(, .*)?\\)",
        "INTERNAL_CONFIG_ENTRY\\((.*?), (.*?), (.*?), (.*?), (.*?)(, .*)?\\)",
        "DEPRECATED_CONFIG_ENTRY\\((.*?), (.*?), (.*?), (.*?), (.*?)(, .*)?\\)"
    );

my @GroupParserFormats =
    (
        "PUBLIC_CONFIG_GROUP\\((.*?), (.*?), (.*?), (.*?)(, .*)?\\)",
        "TEST_CONFIG_GROUP\\((.*?), (.*?), (.*?), (.*?)(, .*)?\\)",
        "INTERNAL_CONFIG_GROUP\\((.*?), (.*?), (.*?), (.*?)(, .*)?\\)",
        "DEPRECATED_CONFIG_GROUP\\((.*?), (.*?), (.*?), (.*?), (.*?)(, .*)?\\)"
    );
    

my @InvalidEntryParserFormats =
    (
        "PUBLIC_CONFIG_ENTRY\\(\\S*\$",
        "TEST_CONFIG_ENTRY\\(\\S*\$",
        "INTERNAL_CONFIG_ENTRY\\(\\S*\$",
        "DEPRECATED_CONFIG_ENTRY\\(\\S*\$"
    );
    
my $ManagedConfigNameParserFormat =
    "(.*?) Tuple<(.*?), (.*?)> (.*?) = new Tuple<(.*?), (.*?)>\\(\"(.*?)\", (.*?)\\);";

my $ReplicationParserFormat =
        "RE_CONFIG_PROPERTIES\\((.*?), (.*?), (.*), (.*), (.*), (.*)\\)";

my $ReplicationInternalParserFormat =
        "RE_INTERNAL_CONFIG_PROPERTIES\\((.*?), (.*?), (.*), (.*), (.*), (.*)\\)";

my $TransactionalReplicatorParserFormat =
        "TR_CONFIG_PROPERTIES\\((.*?)\\)";

my $TransactionalReplicatorInternalParserFormat =
        "TR_INTERNAL_CONFIG_PROPERTIES\\((.*?)\\)";

my %TRConfigurationDescription =
    (
            "CheckpointThresholdInMB" => "A checkpoint will be initiated when the log usage exceeds this value.",
            "MaxAccumulatedBackupLogSizeInMB" => "Max accumulated size (in MB) of backup logs in a given backup log chain. An incremental backup request will fail if the incremental backup would generate a backup log that results in the total backup logs since the last full backup to be greater thanthis size. In such cases user is required to take a full backup.",
            "SlowApiMonitoringDuration" => "Specify duration for api before warning health event is fired.",
            "SlowLogIODuration" => "Specify duration for log I/O operation before it is considered to be slow.",
            "SlowLogIOCountThreshold" => "Number of slow log I/O operations required within SlowLogIOTimeThreshold before a health report is fired",
            "SlowLogIOTimeThreshold" => "Slow log I/O operations are considered within this time time threshold",
            "SlowLogIOHealthReportTTL" => "Specify time to live for slow log I/O health report",
            "MinLogSizeInMB" => "Minimum size of the transactional log. The log will not be allowed to truncate to a size below this setting. 0 indicates that the replicator will determine the minimum log size according to other settings. Increasing this value increases the possibility of doing partial copies and incremental backups since chances of relevant log records being truncated is lowered.",
            "TruncationThresholdFactor" => "Determines at what size of the log truncation will be triggered. Truncation threshold is determined by MinLogSizeInMB multiplied by TruncationThresholdFactor. TruncationThresholdFactor must be greater than 1. MinLogSizeInMB * TruncationThresholdFactor must be less than MaxStreamSizeInMB.",
            "ThrottlingThresholdFactor" => "Determines at what size of the log the replica will start being throttled. Throttling threshold is determined by the maximum value of (MinLogSizeInMB * ThrottlingThresholdFactor) and (CheckpointThresholdInMB * ThrottlingThresholdFactor). ThrottlingThresholdInMB must be greater than TruncationThresholdInMB. TruncationThresholdInMB must be less than MaxStreamSizeInMB.",
            "MaxRecordSizeInKB" => "Maximum size of a log stream record.",
            "MaxWriteQueueDepthInKB" => "DEPRECATED",
            "MaxMetadataSizeInKB" => "DEPRECATED",
            "CopyBatchSizeInKb" => "Specifies the size of the replication data between primary and secondary replicas. If zero the primary sends one record per operation to the secondary. Otherwise the primary replica aggregates log records until the config value is reached",
            "ReadAheadCacheSizeInKb" => "Specifies the size to read ahead during log traversal",
            "ProgressVectorMaxEntries" => "Specifies the max number of non stutter progress vector entries that must be present in the progress vectors. Older entries may be trimmed by the replicator. Default value is 0 that disables the trimming.",
            "EnableSecondaryCommitApplyAcknowledgement" => "DEPRECATED",
            "MaxStreamSizeInMB" => "DEPRECATED",
            "OptimizeForLocalSSD" => "DEPRECATED",
            "OptimizeLogForLowerDiskUsage" => "DEPRECATED",
            "SerializationVersion" => "It indicates the version of the code. Currently it should be either 0 or 1.",
            "Test_LoggingEngine" => "DEPRECATED",
            "Test_LogMinDelayIntervalMilliseconds" => "Minimum delay when flushing log",
            "Test_LogMaxDelayIntervalMilliseconds" => "Maximum delay when flushing log",
            "Test_LogDelayRatio" => "Represents percent of flushes to be delayed in log",
            "Test_LogDelayProcessExitRatio" => "Represents percent of slow flushes after which the process crashes to induce false progress",
			"EnableIncrementalBackupsAcrossReplicas" => "Enables incremental backups to be take by different primary replicas belonging to the same partition. If this configuration is disabled and incremental backup can only be taken on a primary replica which took the previous backup at the same epoch.",
            "LogTruncationIntervalSeconds" => "Configurable interval at which log truncation will be initiated on each replica"
    );

my %REConfigurationDescription =
    (
            "InitialPrimaryReplicationQueueSize" => "This value defines the initial size for the queue which maintains the replication operations on the primary. Note that it must be a power of 2.",
            "MaxPrimaryReplicationQueueSize" => "This is the maximum number of operations that could exist in the primary replication queue. Note that it must be a power of 2.",
            "MaxPrimaryReplicationQueueMemorySize" => "This is the maximum value of the primary replication queue in bytes.",
            "InitialSecondaryReplicationQueueSize" => "This value defines the initial size for the queue which maintains the replication operations on the secondary. Note that it must be a power of 2.",
            "MaxSecondaryReplicationQueueSize" => "This is the maximum number of operations that could exist in the secondary replication queue. Note that it must be a power of 2.",
            "MaxSecondaryReplicationQueueMemorySize" => "This is the maximum value of the secondary replication queue in bytes.",
            "InitialReplicationQueueSize" => "Deprecated. See Specific configs for Primary and Secondary Replication Queues",
            "MaxReplicationQueueSize" => "Deprecated. See Specific configs for Primary and Secondary Replication Queues",
            "MaxReplicationQueueMemorySize" => "Deprecated. See Specific configs for Primary and Secondary Replication Queues",
            "InitialCopyQueueSize" => "This value defines the initial size for the queue which maintains copy operations.  Note that it must be a power of 2.",
            "MaxCopyQueueSize" => "This is the maximum value defines the initial size for the queue which maintains replication operations.  Note that it must be a power of 2.  If during runtime the queue grows to this size operations will be throttled between the primary and secondary replicators.",
            "RetryInterval" => "When an operation is lost or rejected this timer determines how often the replicator will retry sending the operation.",
            "BatchAcknowledgementInterval" => "Determines the amount of time that the replicator waits after receiving an operation before sending back an acknowledgement. Other operations received during this time period will have their acknowledgements sent back in a single message-> reducing network traffic but potentially reducing the throughput of the replicator.",
            "MaxPendingAcknowledgements" => "Maximum number of outstanding operation acknowledgements.  Together with the BatchAckInterval controls replicator operation throttling.",
            "EnableReplicationOperationHeaderInBody" => "Bool that indicates if it is allowed to place the replication operation header in the body of the transport message instead of the header.",
            "MaxReplicationMessageSize" => "Maximum message size of replication operations. Default is 50MB.",
            "RequireServiceAck" => "Bool which controls whether the Windows Fabric Replicator will optimistically acknowledge operations on behalf of services. Setting this to false allows the replicator to optimistically acknowledge operations on behalf of the service. While true tells the replicator to require service ack via the Operation.Ack() API before the acknowledgement can be sent back to the primary service. Note that this flag is not respected for persistent services which always require services to acknowledge operations.",
            "SecondaryClearAcknowledgedOperations" => "Bool which controls if the operations on the secondary replicator are cleared once they are ACK'd by the state provider. Setting this to true may result in additional copy operations during the build of an idle replica",
            "UseStreamFaultsAndEndOfStreamOperationAck" => "Bool which indicates to Windows Fabric Replicator if the state provider intends to use the OperationStream2 interface. If this is enabled the state provider must report fault on the OperationStream2 interface if it cannot apply any more operations that are pumped from the stream.The Windows Fabric Replicator will also dispatch an End of Stream operation in the secondary copy and replication streams.",
            "ReplicatorAddress" => "The endpoint in form of a string -'IP:Port' which is used by the Windows Fabric Replicator to establish connections with other replicas in order to send/receive operations.",
            "ReplicatorListenAddress" => "The endpoint in form of a string -'IP:Port' which is used by the Windows Fabric Replicator to receive operations from other replicas.",
            "ReplicatorPublishAddress" => "The endpoint in form of a string -'IP:Port' which is used by the Windows Fabric Replicator to send operations to other replicas.",
            "QueueHealthMonitoringInterval" => "This value determines the time period used by the Replicator to monitor any warning/error health events in the replication operation queues. A value of '0' disables health monitoring",
            "QueueHealthWarningAtUsagePercent" => "This value determines the replication queue usage(in percentage) after which we report warning about high queue usage. We do so after a grace interval of QueueHealthMonitoringInterval. If the queue usage falls below this percentage in the grace interval, the warning is not reported.",
            "SlowApiMonitoringInterval" => "This value determines the time period used by the Replicator to monitor GetNextCopyState and GetNextCopyContext async api's. A value of '0' disables health monitoring. A warning health report is generated if the async api takes longer than this duration",
            "CompleteReplicateThreadCount" => "The maximum number of parallel threads that can be used by the Windows Fabric Replicator to quorum complete Replication operations on the primary",
            "AllowMultipleQuorumSet" => "Bool which controls whether the Windows Fabric Replicator considers replicas in the previous and current configuration to compute quorum during reconfigurations. Setting this to false could result in higher chances of dataloss during failures",
            "TraceInterval" => "Determines the tracing interval of the replicator's progress (if there was any progress from the previous trace).",
            "PrimaryWaitForPendingQuorumsTimeout" => "Specify timespan in seconds. Defines how long the primary replicator waits for receiving a quorum of acknowledgements for any pending replication operations before processing a reconfiguration request, that could potentially result in ‘cancelling’ the pending replication operationsl"
    );

my @invalidConfigEntries = ();


sub GetTransactionalReplicatorConfigurations
{   
    return VerifyManagedConfigDefaultValuesMatchesWithTheCode( 
        "TransactionalReplicator,InitialCopyQueueSize,uint,64,Static,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"This value defines the initial size for the queue which maintains copy operations.  Note that it must be a power of 2.\"" . "\n" .
        "TransactionalReplicator,MaxCopyQueueSize,uint,16384,Static,PUBLIC,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"This is the maximum value defines the initial size for the queue which maintains replication operations.  Note that it must be a power of 2.  If during runtime the queue grows to this size operations will be throttled between the primary and secondary replicators.\"" . "\n" .
        "TransactionalReplicator,BatchAcknowledgementInterval,TimeSpan,Common::TimeSpan::FromMilliseconds(15),Static,PUBLIC,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Specify timespan in seconds. Determines the amount of time that the replicator waits after receiving an operation before sending back an acknowledgement. Other operations received during this time period will have their acknowledgements sent back in a single message-> reducing network traffic but potentially reducing the throughput of the replicator.\"" . "\n" .
        "TransactionalReplicator,MaxReplicationMessageSize,uint,52428800,Static,PUBLIC,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Maximum message size of replication operations. Default is 50MB.\"" . "\n" . 
        "TransactionalReplicator,ReplicatorAddress,wstring,\"localhost:0\",Static,PUBLIC,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"The endpoint in form of a string -'IP:Port' which is used by the Windows Fabric Replicator to establish connections with other replicas in order to send/receive operations.\"" .  "\n" .
        "TransactionalReplicator,ReplicatorListenAddress,wstring,,Static,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"The endpoint in form of a string -'IP:Port' which is used by the Windows Fabric Replicator to receive operations from other replicas.\"" .  "\n" .
        "TransactionalReplicator,ReplicatorPublishAddress,wstring,,Static,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"The endpoint in form of a string -'IP:Port' which is used by the Windows Fabric Replicator to send operations to other replicas.\"" .  "\n" .
        "TransactionalReplicator,InitialPrimaryReplicationQueueSize,uint,64,Static,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"This value defines the initial size for the queue which maintains the replication operations on the primary. Note that it must be a power of 2.\"" . "\n" .
        "TransactionalReplicator,MaxPrimaryReplicationQueueSize,uint,8192,Static,PUBLIC,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"This is the maximum number of operations that could exist in the primary replication queue. Note that it must be a power of 2.\"" . "\n" .
        "TransactionalReplicator,MaxPrimaryReplicationQueueMemorySize,uint,0,Static,PUBLIC,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"This is the maximum value of the primary replication queue in bytes.\"" . "\n" .
        "TransactionalReplicator,InitialSecondaryReplicationQueueSize,uint,64,Static,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"This value defines the initial size for the queue which maintains the replication operations on the secondary. Note that it must be a power of 2.\"" . "\n" .
        "TransactionalReplicator,MaxSecondaryReplicationQueueSize,uint,16384,Static,PUBLIC,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"This is the maximum number of operations that could exist in the secondary replication queue. Note that it must be a power of 2.\"" . "\n" .
        "TransactionalReplicator,MaxSecondaryReplicationQueueMemorySize,uint,0,Static,PUBLIC,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"This is the maximum value of the secondary replication queue in bytes.\"" . "\n" .
        "TransactionalReplicator,RetryInterval,TimeSpan,Common::TimeSpan::FromSeconds(5),Static,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Specify timespan in seconds. When an operation is lost or rejected this timer determines how often the replicator will retry sending the operation.\"" . "\n" .
        "TransactionalReplicator,SecondaryClearAcknowledgedOperations,bool,false,Static,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Bool which controls if the operations on the secondary replicator are cleared once they are acknowledged to the primary(flushed to the disk). Settings this to TRUE can result in additional disk reads on the new primary, while catching up replicas after a failover.\"" . "\n" .
        "TransactionalReplicator,MaxStreamSizeInMB,int,1024,NotAllowed,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"DEPRECATED\"" . "\n" .
        "TransactionalReplicator,MaxMetadataSizeInKB,int,4,NotAllowed,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"DEPRECATED\"" . "\n" .
        "TransactionalReplicator,MaxRecordSizeInKB,int,1024,NotAllowed,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Maximum size of a log stream record.\"" . "\n" .
        "TransactionalReplicator,CheckpointThresholdInMB,int,50,Static,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"A checkpoint will be initiated when the log usage exceeds this value.\"" . "\n" .
        "TransactionalReplicator,MaxAccumulatedBackupLogSizeInMB,int,800,Static,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Max accumulated size (in MB) of backup logs in a given backup log chain. An incremental backup requests will fail if the incremental backup would generate a backup log that would cause the accumulated backup logs since the relevant full backup to be larger than this size. In such cases, user is required to take a full backup.\"" . "\n" .   
        "TransactionalReplicator,OptimizeForLocalSSD,bool,false,NotAllowed,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"DEPRECATED\"". "\n" .
        "TransactionalReplicator,MaxWriteQueueDepthInKB,int,0,NotAllowed,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"DEPRECATED\"" . "\n" .
        "TransactionalReplicator,SharedLogId,string,,NotAllowed,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Shared log identifier. This is a guid and should be unique for each shared log.\"" . "\n" .
        "TransactionalReplicator,SharedLogPath,string,,NotAllowed,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Path to the shared log. If this value is empty then the default shared log is used.\"" . "\n" .
        "TransactionalReplicator,OptimizeLogForLowerDiskUsage,bool,true,NotAllowed,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"DEPRECATED \"" . "\n" .
        "TransactionalReplicator,Test_LoggingEngine,string,\"ktl\",NotAllowed,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"DEPRECATED \"" . "\n" .
        "TransactionalReplicator,SlowApiMonitoringDuration,TimeSpan,Common::TimeSpan::FromSeconds(300),Static,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Specify duration for api before warning health event is fired.\"" . "\n" .
        "TransactionalReplicator,MinLogSizeInMB,int,0,Static,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Minimum size of the transactional log. The log will not be allowed to truncate to a size below this setting. 0 indicates that the replicator will determine the minimum log size according to other settings. Increasing this value increases the possibility of doing partial copies and incremental backups since chances of relevant log records being truncated is lowered.\"" . "\n" .
        "TransactionalReplicator,TruncationThresholdFactor,int,2,Static,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Determines at what size of the log, truncation will be triggered. Truncation threshold is determined by MinLogSizeInMB multiplied by TruncationThresholdFactor. TruncationThresholdFactor must be greater than 1. MinLogSizeInMB * TruncationThresholdFactor must be less than MaxStreamSizeInMB.\"" . "\n" .
        "TransactionalReplicator,ThrottlingThresholdFactor,int,4,Static,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Determines at what size of the log, the replica will start being throttled. Throttling threshold is determined by Max((MinLogSizeInMB * ThrottlingThresholdFactor),(CheckpointThresholdInMB * ThrottlingThresholdFactor)). ThrottlingThresholdInMB must be greater than TruncationThresholdInMB. TruncationThresholdInMB must be less than MaxStreamSizeInMB.\"" . "\n" .
        "TransactionalReplicator,Test_LogMinDelayIntervalMilliseconds,int,0,Static,TEST,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Minimum delay when flushing log\"" . "\n" .
        "TransactionalReplicator,Test_LogMaxDelayIntervalMilliseconds,int,0,Static,TEST,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Maximum delay when flushing log\"" . "\n" .
        "TransactionalReplicator,Test_LogDelayRatio,double,0,Static,TEST,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Represents percent of flushes to be delayed in log\"" . "\n" .
        "TransactionalReplicator,Test_LogDelayProcessExitRatio,double,0,Static,TEST,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Represents percent of slow flushes after which the process crashes to induce false progress\"" . "\n" .
        "TransactionalReplicator,ProgressVectorMaxEntries,uint,0,Static,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"This value indicates the max number of entries that must be present in the progress vector. Older entries may be trimmed by the replicator. Default value is 0 that disables the trimming.\"" . "\n" .
		"TransactionalReplicator,EnableIncrementalBackupsAcrossReplicas,bool,false,Static,INTERNAL,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Enables incremental backups to be take by different primary replicas belonging to the same partition. If this configuration is disabled and incremental backup can only be taken on a primary replica which took the previous backup at the same epoch." . "\n" .
        "TransactionalReplicator,LogTruncationIntervalSeconds,int,0,Static,PUBLIC,ReliableStateManagerReplicatorSettingsConfigurationNames.cs,\"Configurable interval at which log truncation will be initiated on each replica.\"" . "\n" . 
        "",    
        "ReliableStateManagerReplicatorSettingsConfigurationNames.cs");
}

sub GetTStoreConfigurations
{   
    return VerifyManagedConfigDefaultValuesMatchesWithTheCode(

         "ReliableStateProvider,SweepThreshold,int,0,Static,INTERNAL,TStoreConstants.cs,\"State providers cache read items in memory. Once a threshold of cached items is reached, items can be swept from the cache via an LRU algorithm. 0 (the default) tells Service Fabric to automatically determine the maximum number of items to cache before sweeping. -1 disables sweeping (all items are cached and none are evicted). Other valid cached item limits are between 1 and 5000000, inclusive. Enumerations are not cached.\"" . "\n" .
         "ReliableStateProvider,EnableStrict2PL,bool,false,Static,INTERNAL,TStoreConstants.cs,\"This configurations is to enable or disable Strict 2PL. When Strict 2PL is enabled, read locks will be released at the beginning of commit without waiting for the commit to become stable. When Strict 2PL is disabled, Rigourous 2PL is used. This will cause read locks to be released when the commit is stable.\"" . "\n" .
        
         "",

         "TStoreConstants.cs");
}

sub GetNativeRunConfigurations
{   
   return VerifyManagedConfigDefaultValuesMatchesWithTheCode(

         "NativeRunConfiguration,EnableNativeReliableStateManager,bool,false,Static,PUBLIC,ReliableStateManagerNativeRunConfigurationNames.cs,\"Enables native reliable state manager\"" . "\n" .
         "NativeRunConfiguration,Test_LoadEnableNativeReliableStateManager,bool,false,Static,INTERNAL,ReliableStateManagerNativeRunConfigurationNames.cs,\"Enable loading of the setting that enables native reliable state manager\"" . "\n" .
         
         "",

         "ReliableStateManagerNativeRunConfigurationNames.cs");
}

sub GetDCAConfigurations
{
    return VerifyManagedConfigDefaultValuesMatchesWithTheCode( 
        "LocalLogStore,LocalLogDeletionEnabled,bool,false,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "LocalLogStore,LogDeletionAgeInDays,int,7,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticFileStore,IsEnabled,bool,false,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticFileStore,StoreConnectionString,string,,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticFileStore,UploadIntervalInMinutes,int,5,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticFileStore,LogContainerName,string,,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticFileStore,CrashDumpContainerName,string,,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticFileStore,LogDeletionAgeInDays,int,7,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticFileStore,LogFilter,string,,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticFileStore,IsAppLogCollectionEnabled,bool,false,Dynamic,DEPRECATED,No Source File (deployer only) ,\"Deprecated. Please use application manifest to specify application diagnostics settings.\"" . "\n" .
        "DiagnosticFileStore,AppLogDirectoryQuotaInMB,int,1024,Dynamic,DEPRECATED,No Source File (converted to Management.MonitoringAgentDirectoryQuota in ManagementConfig.h) ,\"Deprecated. Please use application manifest to specify application diagnostics settings.\"" . "\n" .
        "DiagnosticFileStore,TestOnlyLogDeletionAgeInMinutes,int,0,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticTableStore,IsEnabled,bool,false,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticTableStore,StoreConnectionString,string,,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticTableStore,UploadIntervalInMinutes,int,5,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticTableStore,TableName,string,,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticTableStore,LogDeletionAgeInDays,int,7,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticTableStore,LogFilter,string,,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticTableStore,TestOnlyLogDeletionAgeInMinutes,int,0,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "DiagnosticTableStore,UploadConcurrencyCount,int,4,Dynamic,DEPRECATED,src\\Managed\\DCA\\Settings.cs,\"Deprecated. Please use the new producer-consumer model instead.\"" . "\n" .
        "Diagnostics,ConsumerInstances,string,,Dynamic,PUBLIC,src\\Managed\\DCA\\Settings.cs,\"The list of DCA consumer instances\"" . "\n" .
        "Diagnostics,ProducerInstances,string,,Dynamic,PUBLIC,src\\Managed\\DCA\\Settings.cs,\"The list of DCA producer instances\"" . "\n" .
        "Diagnostics,ConfigurationChangeReactionTimeInSeconds,int,5,Dynamic,INTERNAL,src\\Managed\\DCA\\Settings.cs,\"Time to wait before reacting to configuration change\"" . "\n" .
        "Diagnostics,AppInstanceDataEtlFlushIntervalInSeconds,int,60,Dynamic,INTERNAL,src\\Managed\\DCA\\Settings.cs,\"Interval at which to flush ETL file ETW session for application instance related events\"" . "\n" .
        "Diagnostics,AppInstanceDataEtlReadIntervalInMinutes,int,3,Dynamic,INTERNAL,src\\Managed\\DCA\\Settings.cs,\"Interval (in minutes) at which to read ETL files containing application instance related events\"" . "\n" .
        "Diagnostics,TestOnlyAppInstanceDataEtlReadIntervalInSeconds,int,0,Dynamic,TEST,src\\Managed\\DCA\\Settings.cs,\"Interval (in seconds) at which to read ETL files containing application instance related events\"" . "\n" .
        "Diagnostics,TestOnlyAppDataDeletionIntervalInSeconds,int,10800,Dynamic,TEST,src\\Managed\\DCA\\Settings.cs,\"Interval at which we delete old data related to application instances\"" . "\n" .
        "Diagnostics,AppEtwTraceDeletionAgeInDays,int,3,Dynamic,PUBLIC,src\\Managed\\DCA\\Settings.cs,\"Number of days after which we delete old ETL files containing application ETW traces\"" . "\n" .
        "Diagnostics,TestOnlyAppEtwTraceDeletionAgeInMinutes,int,0,Dynamic,TEST,src\\Managed\\DCA\\Settings.cs,\"Number of minutes after which we delete old ETL files containing application ETW traces\"" . "\n" .
        "Diagnostics,AppDiagnosticStoreAccessRequiresImpersonation,bool,true,Dynamic,PUBLIC,src\\Managed\\DCA\\Settings.cs,\"Whether or not impersonation is required when accessing diagnostic stores on behalf of the application\"" . "\n" .
        "Diagnostics,TestOnlyDtrDeletionAgeInMinutes,int,1440,Dynamic,TEST,src\\Managed\\DCA\\Settings.cs,\"Number of minutes after which we delete DTR files that are located on the node\"" . "\n" .
        "Diagnostics,MaxDiskQuotaInMB,int,65536,Dynamic,PUBLIC,src\\Managed\\DCA\\Settings.cs,\"Disk quota in MB for Windows Fabric log files\"" . "\n" .
        "Diagnostics,DiskFullSafetySpaceInMB,int,1024,Dynamic,PUBLIC,src\\Managed\\DCA\\Settings.cs,\"Remaining disk space in MB to protect from use by DCA\"" . "\n" .
        "Diagnostics,TestOnlyOldDataDeletionIntervalInSeconds,int,30,Dynamic,TEST,src\\Managed\\DCA\\Settings.cs,\"Interval (in seconds) at which we perform deletion of old files, if necessary\"" . "\n" .
        "Diagnostics,TestOnlyFlushDataOnDispose,bool,false,Dynamic,TEST,src\\Managed\\DCA\\Settings.cs,\"Controls whether all plugins should perform a flush operation on dispose.\"" . "\n" .
        "Diagnostics,PluginFlushTimeoutInSeconds,int,180,Dynamic,INTERNAL,src\\Managed\\DCA\\Settings.cs,\"Timeout (in seconds) for a plugin to complete its flush operations on dispose\"" . "\n" .
        "Diagnostics,ApplicationLogsFormatVersion,int,0,Dynamic,PUBLIC,src\\Managed\\DCA\\Settings.cs,\"Version for application logs format. Supported values are 0 and 1. Version 1 includes more fields from the ETW event record than version 0.\"" . "\n" .
        "Diagnostics,ClusterId,string,L\"\",Dynamic,PUBLIC,src\\Managed\\DCA\\Settings.cs,\"The unique id of the cluster. This is generated when the cluster is created.\"" . "\n" .
        "Diagnostics,EnableTelemetry,bool,true,Dynamic,PUBLIC,src\\Managed\\DCA\\Settings.cs,\"This is going to enable or disable telemetry.\"" . "\n" .
        "Diagnostics,TestOnlyTelemetryPushTime,string,L\"\",Dynamic,TEST,src\\Managed\\DCA\\Settings.cs,\"Defines the specific time of the day to push telemetry.\"" . "\n" .
		"Diagnostics,TestOnlyTelemetryDailyPushes,int,3,Dynamic,TEST,src\\Managed\\DCA\\Settings.cs,\"Defines how many times to send telemetry per day.\"" . "\n" .
        "Diagnostics,EnableCircularTraceSession,bool,false,Static,PUBLIC,src\\Managed\\DCA\\Settings.cs,\"Flag indicates whether circular trace sessions should be used.\"" . "\n" .
        "",
        "");
}

sub GetTraceConfigurations
{
    return VerifyManagedConfigDefaultValuesMatchesWithTheCode(
        "Trace/Etw,Level,int,4,Dynamic,PUBLIC,Trace.cpp,\"Trace etw level\"" . "\n" .
        "Trace/Etw,Filters,string,L\"\",Dynamic,INTERNAL,Trace.cpp,\"Trace etw filters\"" . "\n" .
        "Trace/Etw,Sampling,int,0,Dynamic,INTERNAL,Trace.cpp,\"Trace etw sampling interval\"" . "\n" .
        "Trace/Etw,TraceProvisionalFeatureStatus,bool,true,Dynamic,INTERNAL,Trace.cpp,\"Determine if Provisional Trace feature is enable or not\"" . "\n" .
        "Trace/File,Level,int,4,Dynamic,INTERNAL,Trace.cpp,\"Trace file level\"" . "\n" .
        "Trace/File,Filters,string,L\"\",Dynamic,INTERNAL,Trace.cpp,\"Trace file filters\"" . "\n" .
        "Trace/File,Path,string,Fabric.trace,Dynamic,INTERNAL,Trace.cpp,\"Trace file path\"" . "\n" .
        "Trace/File,Option,,L\"\",Dynamic,INTERNAL,Trace.cpp,\"Trace file options\"" . "\n" .
        "Trace/Console,Filters,string,L\"\",Dynamic,INTERNAL,Trace.cpp,\"Trace console filters\"" . "\n" .
        "Trace/Console,Level,int,0,Dynamic,INTERNAL,Trace.cpp,\"Trace console level\"" . "\n" .
        "",
        "");
}

sub GetCounterConfigurations
{
    return VerifyManagedConfigDefaultValuesMatchesWithTheCode(
        "PerformanceCounterLocalStore,IsEnabled,bool,true,Dynamic,PUBLIC,Setup\\FabricDeployer\\DeployerCommon\\PerformanceCounters.cs,\"Flag indicates whether performance counter collection on local node is enabled\"" . "\n" .
        "PerformanceCounterLocalStore,SamplingIntervalInSeconds,int,60,Dynamic,PUBLIC,Setup\\FabricDeployer\\DeployerCommon\\PerformanceCounters.cs,\"Sampling interval for performance counters being collected\"" . "\n" .
        "PerformanceCounterLocalStore,Counters,string,,Dynamic,PUBLIC,Setup\\FabricDeployer\\DeployerCommon\\PerformanceCounters.cs,\"Comma-separated list of performance counters to collect\"" . "\n" .
        "PerformanceCounterLocalStore,TestOnlyCounterFilePath,string,,Dynamic,TEST,Setup\\FabricDeployer\\DeployerCommon\\PerformanceCounters.cs,\"Path to the folder where the performance counter data is collected\"" . "\n" .
        "PerformanceCounterLocalStore,TestOnlyCounterFileNamePrefix,string,,Dynamic,TEST,Setup\\FabricDeployer\\DeployerCommon\\PerformanceCounters.cs,\"Prefix used in the names of the files containing performance counter data\"" . "\n" .
        "PerformanceCounterLocalStore,TestOnlyIncludeMachineNameInCounterFileName,bool,false,Dynamic,TEST,Setup\\FabricDeployer\\DeployerCommon\\PerformanceCounters.cs,\"Flag indicates whether the machine name is included in the names of the files containing performance counter data\"" . "\n" .
        "PerformanceCounterLocalStore,MaxCounterBinaryFileSizeInMB,int,1,Dynamic,PUBLIC,Setup\\FabricDeployer\\DeployerCommon\\PerformanceCounters.cs,\"Maximum size (in MB) for each performance counter binary file\"" . "\n" .
        "PerformanceCounterLocalStore,NewCounterBinaryFileCreationIntervalInMinutes,int,10,Dynamic,PUBLIC,Setup\\FabricDeployer\\DeployerCommon\\PerformanceCounters.cs,\"Maximum interval (in seconds) after which a new performance counter binary file is created\"" . "\n" .
        "",
        "");
}

sub GetHttpGatewayConfigurations
{
    return
        "";
}


sub GetCommonConfigurations
{
    return VerifyManagedConfigDefaultValuesMatchesWithTheCode(
        "Common,DebugBreakEnabled,bool,false,Static,TEST,src\\Common\\AssertWF.h,\"\"" . "\n" .
        "Common,TestAssertEnabled,bool,false,Static,TEST,src\\Common\\AssertWF.h,\"\"" . "\n" .
        "",
        "");
}

sub GetSetupConfigurations
{
    return VerifyManagedConfigDefaultValuesMatchesWithTheCode( 
        "Setup,FabricDataRoot,string,\"\",NotAllowed,PUBLIC,Setup\\FabricDeployer\\Constants.cs,\"The Windows Fabric data root directory\"" . "\n" .
        "Setup,FabricLogRoot,string,\"\",NotAllowed,PUBLIC,Setup\\FabricDeployer\\Constants.cs,\"The windows fabric log root directory.\"" . "\n" .
        "Setup,NodesToBeRemoved,string,\"\",Dynamic,PUBLIC,Setup\\FabricDeployer\\Constants.cs,\"The nodes which should be removed as part of configuration upgrade. (Only for Standalone Deployments)\"" . "\n" .
        "Setup,ServiceRunAsAccountName,string,\"\",NotAllowed,PUBLIC,Setup\\FabricDeployer\\Constants.cs,\"The account name under which to run fabric host service.\"" . "\n" .
        "Setup,ServiceRunAsPassword,string,\"\",NotAllowed,TEST,Setup\\FabricDeployer\\Constants.cs,\"The account password under which to run fabric host service.\"" . "\n" .
        "Setup,ServiceStartupType,string,\"\",NotAllowed,TEST,Setup\\FabricDeployer\\Constants.cs,\"The startup type of the fabric host service.\"" . "\n" .
        "Setup,SkipFirewallConfiguration,bool,false,NotAllowed,PUBLIC,Setup\\FabricDeployer\\Constants.cs,\"Whether to skip firewall settings.\"" . "\n" .
        "Setup,SkipContainerNetworkResetOnReboot,bool,false,NotAllowed,PUBLIC,Setup\\FabricDeployer\\Constants.cs,\"Whether to skip resetting container network on reboot.\"" . "\n" .
        "Setup,SkipIsolatedNetworkResetOnReboot,bool,false,NotAllowed,PUBLIC,Setup\\FabricDeployer\\Constants.cs,\"Whether to skip resetting isolated network on reboot.\"" . "\n" .
        "Setup,ContainerDnsSetup,string,\"Allow\",Static,PUBLIC,Setup\\FabricDeployer\\Constants.cs,\"Allows controlling Docker/Container DNS related setup. Valid values are: Allow, Require, Disallow. Allow = install but continue on container DNS setup errors, Require = install but fail on container DNS setup errors, Disallow = don't install and try to clean up if container DNS setup was done previously.\"" . "\n" .
        "Setup,IsTestCluster,bool,false,NotAllowed,INTERNAL,Setup\\FabricDeployer\\Constants.cs,\"Set to true when creating test clusters to allow scale-min deployment in multiple machines. (Only for Standalone Deployments)\"" . "\n" .
        "",
        "");
}

sub GetUpgradeOrchestrationServiceConfigurations
{
    return VerifyManagedConfigDefaultValuesMatchesWithTheCode(
        "UpgradeOrchestrationService,AutoupgradeInstallEnabled,bool,false,Static,PUBLIC,StandAloneFabricSettingsGenerator.cs,\"Automatic polling, provisioning and install of code upgrade action based on a goal-state file.\"" . "\n" .
        "UpgradeOrchestrationService,SkipInitialGoalStateCheck,bool,false,Static,PUBLIC,StandaloneGoalStateProvisioner.cs,\"Skips the initial check for goal state while setting up polling. Used in tests that verify auto code upgrades.\"" . "\n" .
        "UpgradeOrchestrationService,GoalStateExpirationReminderInDays,int,30,Static,PUBLIC,StandaloneGoalStateProvisioner.cs,\"Sets the number of remaining days after which goal state reminder should be shown.\"" . "\n" .
        "",
        "");
}

sub GetEventStoreServiceConfigurations
{
    return VerifyManagedConfigDefaultValuesMatchesWithTheCode(
        "EventStoreService,PlacementConstraints,wstring,,Static,PUBLIC,src\\managed\\EventsStore\\EventStore.Service\\Settings.cs,\"The PlacementConstraints for EventStoreService\"" . "\n" .
        "EventStoreService,TargetReplicaSetSize,int,0,Static,PUBLIC,src\\managed\\EventsStore\\EventStore.Service\\Settings.cs,\"The TargetReplicaSetSize for EventStoreService\"" . "\n" .
        "EventStoreService,MinReplicaSetSize,int,0,Static,PUBLIC,src\\managed\\EventsStore\\EventStore.Service\\Settings.cs,\"The MinReplicaSetSize for EventStoreService\"" . "\n" .
        "",
        "");
}

sub Trim
{
    my $text = shift;
    $text =~ s/^\s+//;
    $text =~ s/\s+$//;
    return $text;
}

sub UpdateDescription
{
    my $description = shift;
    my $type = shift;

    if (index($type, "TimeSpan") >= 0)
    {
        $description = "Specify timespan in seconds. $description";
    }
    else
    {
        $description = "$description";
    }

    return $description;
}

sub ParseConfigEntry
{
    my $line = shift;
    my @tokens = ();

    for (@EntryParserFormats)
    {    
        if ($line =~ /$_/)
        {
            my $type = $1;
            my $scopeOperator = rindex($type, "::");
            if ($scopeOperator > 0)
            {
                $type = substr($type, $scopeOperator + 2);
            }

            my $section = $2;
            my $name = $3;
            
            my $default = $4;
            
            $scopeOperator = rindex($5, "::");
            my $upgradePolicy = substr($5, $scopeOperator + 2);
            
            my $configType = substr($_, 0, index($_, "_"));
            $section =~ s/L//;
            $section =~ s/\"//g;

            @tokens = ($section, $name, $type, $default, $upgradePolicy, $configType);
            last;
        }
    }

    return \@tokens;
}


sub ParseReplicationConfigEntry
{
    my $line = shift;
    my $isInternal = shift;

    my @tokens = ();

    if ($line =~ /#define/)
    {
        return \@tokens;
    }
    
    my $lineFormat = ":";
    if ($isInternal == 0)
    {
        $lineFormat = $ReplicationParserFormat;
    }
    else
    {
        $lineFormat = $ReplicationInternalParserFormat;
    }

    if ($line =~ /$lineFormat/)
    {
        my $section = $1;
        my $batchAcknowledgementInterval = $2;
        my $maxPrimaryReplicationQueueSize = $3;
        my $maxPrimaryReplicationQueueMemorySize =$4;
        my $maxSecondaryReplicationQueueSize = $5;
        my $maxSecondaryReplicationQueueMemorySize =$6;
        $section =~ s/L//;
        $section =~ s/\"//g;

        @tokens = ($section, $batchAcknowledgementInterval, $maxPrimaryReplicationQueueSize, $maxPrimaryReplicationQueueMemorySize, $maxSecondaryReplicationQueueSize, $maxSecondaryReplicationQueueMemorySize);
    }

    return \@tokens;
}

sub ParseTransactionalReplicationConfigEntry
{
    my $line = shift;
    my $isInternal = shift;

    my @tokens = ();

    if ($line =~ /#define/)
    {
        return \@tokens;
    }
    
    my $lineFormat = ":";
    if ($isInternal == 0)
    {
        $lineFormat = $TransactionalReplicatorParserFormat;
    }
    else
    {
        $lineFormat = $TransactionalReplicatorInternalParserFormat;
    }

    if ($line =~ /$lineFormat/)
    {
        my $section = $1;

        $section =~ s/L//;
        $section =~ s/\"//g;
        if($section ne "Invalid")
        {
            @tokens = ($section);
        }
    }

    return \@tokens;
}

sub ParseConfigGroup
{
    my $line = shift;
    my @tokens = ();

    for (@GroupParserFormats)
    {
        if ($line =~ /$_/)
        {
            my $type = $1;
            my $scopeOperator = rindex($type, "::");
            if ($scopeOperator > 0)
            {
                $type = substr($type, $scopeOperator + 2);
            }

            my $section = $2;
            my $name = "PropertyGroup";
            my $default = "None";

            $scopeOperator = rindex($4, "::");
            my $upgradePolicy = substr($4, $scopeOperator + 2);
            my $configType = substr($_, 0, index($_, "_"));
            $section =~ s/L//;
            $section =~ s/\"//g;

            @tokens = ($section, $name, $type, $default, $upgradePolicy, $configType);
            last;
        }
    }

    return \@tokens;
}

sub ParseReplicationFile
{
    my $fileToParse = shift;
    my $isInternal = shift;

    my @replicationConfig = ();

    my $foundConfig = 0;
    my $foundInternalConfig = 0;

    my @lines = @{ReadAllLines($fileToParse)};
    foreach my $line (@lines)
    {
        $line = Trim($line);

        if (($foundConfig == 1) || ($foundInternalConfig == 1))
        {
            if (substr($line, 0, 2) eq "//")
            {
                $line = substr($line, 2);
            }

            my @configEntry = @{ParseConfigEntry($line)};
            if (scalar (@configEntry) > 0)
            {
                my $name = $configEntry[1];
                my $description = "TODO: GET DETAILS AND ADD TO GenerateConfigurationsCSV.pl";
                if (exists $REConfigurationDescription{$name})
                {
                    $description = $REConfigurationDescription{$name};
                }
                
                push (@configEntry, $description);
                if ($foundConfig == 1 && $isInternal == 0)
                {
                    push (@replicationConfig, \@configEntry);
                }
                elsif ($foundInternalConfig == 1 && $isInternal ==1)
                {
                    push (@replicationConfig, \@configEntry);
                }
            }

            if (length($line) == 0)
            {
                $foundConfig = 0;
                $foundInternalConfig = 0;
            }
        }
        elsif (index($line, "#define RE_CONFIG_PROPERTIES") == 0)
        {
            $foundConfig = 1;
            $foundInternalConfig = 0;
        }
        elsif (index($line, "#define RE_INTERNAL_CONFIG_PROPERTIES") == 0)
        {
            $foundConfig = 0;
            $foundInternalConfig = 1;
        }
    }

    return \@replicationConfig;
}

sub ParseTransactionalReplicationFile
{
    my $fileToParse = shift;
    my $isInternal = shift;

    my @transactionalReplicatorConfig = ();
    my $foundConfig = 0;
    my $foundInternalConfig = 0;

    my @lines = @{ReadAllLines($fileToParse)};
    foreach my $line (@lines)
    {
        $line = Trim($line);
        if (($foundConfig == 1) || ($foundInternalConfig == 1))
        {
            if (substr($line, 0, 2) eq "//")
            {
                $line = substr($line, 2);
            }

            my @configEntry = @{ParseConfigEntry($line)};
            if (scalar (@configEntry) > 0)
            {
                my $name = $configEntry[1];
                my $description = "TODO: GET DETAILS AND ADD TO GenerateConfigurationsCSV.pl";
                if (exists $TRConfigurationDescription{$name})
                {
                    $description = $TRConfigurationDescription{$name};
                }
                
                push (@configEntry, $description);
                if ($foundConfig == 1 && $isInternal == 0)
                {
                    push (@transactionalReplicatorConfig, \@configEntry);
                }
                elsif ($foundInternalConfig == 1 && $isInternal ==1)
                {
                    push (@transactionalReplicatorConfig, \@configEntry);
                }
            }

            if (length($line) == 0)
            {
                $foundConfig = 0;
                $foundInternalConfig = 0;
            }
        }
        elsif (index($line, "#define TR_CONFIG_PROPERTIES") == 0)
        {
            $foundConfig = 1;
            $foundInternalConfig = 0;
        }
        elsif (index($line, "#define TR_INTERNAL_CONFIG_PROPERTIES") == 0)
        {
            $foundConfig = 0;
            $foundInternalConfig = 1;
        }
    }

    return \@transactionalReplicatorConfig;
}

sub ParseFile
{
    my $fileToParse = shift;
    my $replicationConfigRef = shift;
    my $replicationInternalConfigRef = shift;
    my $transactionalReplicatorConfigRef = shift;
    my $transactionalReplicatorInternalConfigRef = shift;
    my $isLinux = shift;

    my @replicationConfig = @{$replicationConfigRef};
    my @replicationInternalConfig = @{$replicationInternalConfigRef};

    my @transactionalReplicatorConfig = @{$transactionalReplicatorConfigRef};
    my @transactionalReplicatorInternalConfig = @{$transactionalReplicatorInternalConfigRef};

    my $fileFragment = substr($fileToParse, length($ProdRoot) + 1);
    $fileFragment =~ s/\//\\/g;

    my @lines = @{ReadAllLines($fileToParse)};
    
    my $fileConfigText = "";

    my $description = "";
    my $codeIsSkippedForCompile = 0;
    
    foreach my $line (@lines)
    {
        $line = Trim($line);

        if (substr($line, 0, 23) eq "//NOT_PLATFORM_UNIX_END")
        {
            $codeIsSkippedForCompile = 0;
            next;
        }
        if (substr($line, 0, 19) eq "//PLATFORM_UNIX_END")
        {
            $codeIsSkippedForCompile = 0;
            next;
        }
        if (substr($line, 0, 6) eq "#endif")
        {
            $codeIsSkippedForCompile = 0;
            next;
        }

        if (substr($line, 0, 5) eq "#else")
        {
            $codeIsSkippedForCompile = 1 - $codeIsSkippedForCompile;
            next;
        }

        if ($codeIsSkippedForCompile ne 0)
        {
            next;
        }
        
        if ($isLinux ne 0 && substr($line, 0, 25) eq "//NOT_PLATFORM_UNIX_START")# processing linux, hit windows block
        {
            $codeIsSkippedForCompile = 1;
            next;
        }
        if ($isLinux eq 0 && substr($line, 0, 21) eq "//PLATFORM_UNIX_START")# processing windows, hit linux block
        {
            $codeIsSkippedForCompile = 1;
            next;
        }
        if ($isLinux ne 0 && substr($line, 0, 21) eq "#ifndef PLATFORM_UNIX")# processing linux, hit windows block
        {
            $codeIsSkippedForCompile = 1;
            next;
        }
        if ($isLinux eq 0 && substr($line, 0, 20) eq "#ifdef PLATFORM_UNIX")# processing windows, hit linux block
        {
            $codeIsSkippedForCompile = 1;
            next;
        }
        
        # Comments may indicate description text. Concatenate them until told otherwise...
        if (substr($line, 0, 2) eq "//")
        {
            my $commentLine = Trim(substr($line, 2));
            # ',' needs to be replaced as it is delimitor in CSV file
            $commentLine =~ s/,/;/g;

            $description .= $commentLine . " ";
            next;
        }

        for (@InvalidEntryParserFormats)
        {
            if ($line =~ /$_/)
            {
                push @invalidConfigEntries, "$fileToParse : The entire configuration entry should be in a single line - $line\n";
            }
        }

        # Is this a normal config entry?
        my @configEntry = @{ParseConfigEntry($line)};
        if (scalar (@configEntry) > 0 && $configEntry[0] ne "section_name")
        {
            $description = UpdateDescription($description, $configEntry[2]);
            $fileConfigText .= "$configEntry[0],$configEntry[1],$configEntry[2],$configEntry[3],$configEntry[4],$configEntry[5],$fileFragment,$description\n";
        }
        else
        {
            # Is this a group config entry?
            @configEntry = @{ParseConfigGroup($line)};
            if (scalar (@configEntry) > 0)
            {
                $description = UpdateDescription($description, $configEntry[2]);
                $fileConfigText .= "$configEntry[0],$configEntry[1],$configEntry[2],$configEntry[3],$configEntry[4],$configEntry[5],$fileFragment,$description\n";
            }
            else
            {
                # Is this a replication config entry?
                @configEntry = @{ParseReplicationConfigEntry($line, 0)};
                if (scalar (@configEntry) > 0)
                {
                    for (@replicationConfig)
                    {                       
                        my @replicationProperty = @{$_};
                        
                        my $batchAckReplacement = $replicationProperty[3];
                        $batchAckReplacement =~ s/batch_acknowledgement_interval/$configEntry[1]/;

                        my $name = 
                            $replicationProperty[1] eq "MaxPrimaryReplicationQueueSize" ?
                                $configEntry[2] :
                                $replicationProperty[1] eq "BatchAcknowledgementInterval" ?
                                    $batchAckReplacement :
                                    $replicationProperty[1] eq "MaxPrimaryReplicationQueueMemorySize" ?
                                        $configEntry[3] :
                                        $replicationProperty[1] eq "MaxSecondaryReplicationQueueSize" ?
                                            $configEntry[4] :
                                            $replicationProperty[1] eq "MaxSecondaryReplicationQueueMemorySize" ?
                                                $configEntry[5] :
                                                $replicationProperty[3];
                        
                        $description = UpdateDescription($replicationProperty[6], $replicationProperty[2]);
                        $fileConfigText .= "$configEntry[0],$replicationProperty[1],$replicationProperty[2],$name,$replicationProperty[4],$replicationProperty[5],$fileFragment,$description\n";                        
                    }
                }

                else 
                {
                    # Is this a replication internal config entry ?
                    @configEntry = @{ParseReplicationConfigEntry($line, 1)};
                    if (scalar (@configEntry) > 0)
                    {
                        for (@replicationInternalConfig)
                        {                       
                            my @replicationProperty = @{$_};
                            
                            my $batchAckReplacement = $replicationProperty[3];
                            $batchAckReplacement =~ s/batch_acknowledgement_interval/$configEntry[1]/;

                            my $name = 
                                $replicationProperty[1] eq "MaxPrimaryReplicationQueueSize" ?
                                    $configEntry[2] :
                                    $replicationProperty[1] eq "BatchAcknowledgementInterval" ?
                                        $batchAckReplacement :
                                        $replicationProperty[1] eq "MaxPrimaryReplicationQueueMemorySize" ?
                                            $configEntry[3] :
                                            $replicationProperty[1] eq "MaxSecondaryReplicationQueueSize" ?
                                                $configEntry[4] :
                                                $replicationProperty[1] eq "MaxSecondaryReplicationQueueMemorySize" ?
                                                    $configEntry[5] :
                                                        # For internal configs, use the max replication queue memory size to be same as max primary queue memory size
                                                        $replicationProperty[1] eq "MaxReplicationQueueMemorySize" ?
                                                        $configEntry[3] :
                                                        $replicationProperty[3];
                            
                            $description = UpdateDescription($replicationProperty[6], $replicationProperty[2]);
                            $fileConfigText .= "$configEntry[0],$replicationProperty[1],$replicationProperty[2],$name,$replicationProperty[4],$replicationProperty[5],$fileFragment,$description\n";                        
                        }
                    }
                    else
                    {
                        # Is this a native transactional replicator config entry?
                        @configEntry = @{ParseTransactionalReplicationConfigEntry($line, 0)};
                        if (scalar (@configEntry) > 0)
                        {
                            for(@transactionalReplicatorConfig)
                            {
                                my @transactionalReplicatorProperty = @{$_};
                                $fileConfigText .= "$configEntry[0],$transactionalReplicatorProperty[1],$transactionalReplicatorProperty[2],$transactionalReplicatorProperty[3],$transactionalReplicatorProperty[4],$transactionalReplicatorProperty[5],$fileFragment,$transactionalReplicatorProperty[6]\n";
                            }
                        }
                        else
                        {
                            # Is this a native transactional replicator internal config entry?
                            @configEntry = @{ParseTransactionalReplicationConfigEntry($line, 1)};
                            if (scalar (@configEntry) > 0)
                            {
                                for(@transactionalReplicatorInternalConfig)
                                {
                                    my @transactionalReplicatorProperty = @{$_};
                                    $fileConfigText .= "$configEntry[0],$transactionalReplicatorProperty[1],$transactionalReplicatorProperty[2],$transactionalReplicatorProperty[3],$transactionalReplicatorProperty[4],$transactionalReplicatorProperty[5],$fileFragment,$transactionalReplicatorProperty[6]\n";
                                }
                            }
                        }
                    }
                }
            }
        }

        $description = "";
    }

    return $fileConfigText;
}

sub ParseManagedConfigNameEntry
{
    my $line = shift;
    my @tokens = ();
    
    # format of the config should be
    # Tuple<string, TimeSpan> RetryInterval = new Tuple<string, TimeSpan>("RetryInterval", TimeSpan.FromSeconds(5));
    # "(.*?) Tuple<(.*?), (.*?)> (.*?) = new Tuple<(.*?), (.*?)>\\(\"(.*?)\", (.*?)\\);";
    
    if ($line =~ /$ManagedConfigNameParserFormat/)
    {
        my $configName = $7;
        my $default =$8;

        @tokens = ($configName, $default);
    }

    return \@tokens;
}

sub VerifyManagedConfigDefaultValuesMatchesWithTheCode
{
    my $configsInScript = shift;    
    my $fileName = shift;
    
    if (length($fileName) eq 0)
    {
    return $configsInScript;
    }
    
    my $directory = $ProdRoot;
    my @foundfile = ();
    
    find(sub { push @foundfile, $File::Find::name if $_ eq $fileName}, $directory);
    
    if (length(@foundfile) ne 1)
    {
    push @invalidConfigEntries , "Could not find managed configuration name file $fileName";
    return $configsInScript;
    }

    my @lines = @{ReadAllLines($foundfile[0])};
    
    my @configEntriesInScript = split("\n", $configsInScript);
    
    foreach my $line (@lines)
    {
        $line = Trim($line);
        
        if (index($line, "Tuple") ne -1)
        {   
            # we found a config name
            my @configEntry = @{ParseManagedConfigNameEntry($line)};
            my $foundConfigEntry = "";
            
            foreach my $configEntryInScript (@configEntriesInScript)
            {         
            if (index($configEntryInScript, $configEntry[0]) ne -1)
            {
                $foundConfigEntry = $configEntryInScript;
                last;
            }        
            }
            
            if (length($foundConfigEntry) lt 1)
            {
                push @invalidConfigEntries , "Could not find managed config-$configEntry[0] that is present in file-$fileName but not in GenerateConfigurationCSV.pl script";
                return $configsInScript;
            }
                    
            my @decodedScriptConfig = split(',', $foundConfigEntry);

            my $defaultScriptConfig = FixupNativeToManagedPropertyValue($decodedScriptConfig[3]);

            if ($defaultScriptConfig ne $configEntry[1])
            {
                push @invalidConfigEntries , "Default Value for managed config-$configEntry[0] is $configEntry[1] in the code, but $defaultScriptConfig in the GenerateConfigurationCSV.pl script";
                return $configsInScript;
            }

            if ($decodedScriptConfig[6] ne $fileName)
            {
                push @invalidConfigEntries , "File Name for managed config-$configEntry[0] is $fileName in the code, but $decodedScriptConfig[6] in the GenerateConfigurationCSV.pl script";
                return $configsInScript;
            }
        }
    }
    
    return $configsInScript;
}

sub BuildConfigCSV
{
    my $directories = $ProdRoot;
    my @foundfiles = ();
    my @foundDefinitionfiles = ();

    # Find all the *Config.h files...
    find(sub { push @foundfiles, $File::Find::name if /Config\.h$/i }, $directories);

    # Find the macro definition file for replication config
    find(sub { push @foundDefinitionfiles, $File::Find::name if /REConfigMacros\.h/i }, $directories);

    # Find the macro definition file for native transactional replicator config
    find(sub { push @foundDefinitionfiles, $File::Find::name if /TRConfigMacros\.h/i }, $directories);
    
    # Build up the initial CSV text with various hard-coded sections...
    my $configurationsCSVText =
        "#This file contains all possible configurations. The only supported configurations are the public configurations.\n" .
        "#Section Name,Setting Name,Type,Default Value,Upgrade Policy,Setting Type,Config Found In,Description\n";

    # windows
    $configurationsCSVText .= GetDCAConfigurations();
    $configurationsCSVText .= GetTraceConfigurations();
    $configurationsCSVText .= GetCounterConfigurations();
    $configurationsCSVText .= GetCommonConfigurations();
    $configurationsCSVText .= GetSetupConfigurations();
    $configurationsCSVText .= GetTransactionalReplicatorConfigurations();
    $configurationsCSVText .= GetTStoreConfigurations();
    $configurationsCSVText .= GetNativeRunConfigurations();
    $configurationsCSVText .= GetUpgradeOrchestrationServiceConfigurations();
    $configurationsCSVText .= GetEventStoreServiceConfigurations();
    
    my $linuxConfigurationsCSVText =
        "#This file contains all possible configurations. The only supported configurations are the public configurations.\n" .
        "#Section Name,Setting Name,Type,Default Value,Upgrade Policy,Setting Type,Config Found In,Description\n";
    
    # linux
    $linuxConfigurationsCSVText .= GetDCAConfigurations();
    $linuxConfigurationsCSVText .= GetTraceConfigurations();
    $linuxConfigurationsCSVText .= GetCounterConfigurations();
    $linuxConfigurationsCSVText .= GetCommonConfigurations();
    $linuxConfigurationsCSVText .= GetSetupConfigurations();
    $linuxConfigurationsCSVText .= GetTransactionalReplicatorConfigurations();
    $linuxConfigurationsCSVText .= GetTStoreConfigurations();
    $linuxConfigurationsCSVText .= GetNativeRunConfigurations();
    
    # Special-case the Replication section since it is nested in various other places...
    my @replicationConfig = ();
    my @replicationInternalConfig = ();
    foreach my $file (@foundDefinitionfiles)
    {
        next if $file !~ /REConfigMacros\.h/;

        @replicationConfig = @{ParseReplicationFile($file, 0)};
        @replicationInternalConfig = @{ParseReplicationFile($file, 1)};
        last;
    }

    # Special-case the Native Transactional Replicator section since it is also nested
    my @transactionalReplicatorConfig = ();
    my @transactionalReplicatorInternalConfig = ();
    foreach my $file (@foundDefinitionfiles)
    {
        next if $file !~ /TRConfigMacros\.h/;
        @transactionalReplicatorConfig = @{ParseTransactionalReplicationFile($file, 0)};
        @transactionalReplicatorInternalConfig = @{ParseTransactionalReplicationFile($file, 1)};
        last;
    }

    # Process each of the Config files and build up the CSV text...
    foreach my $file (@foundfiles)
    {
        next if $file =~ /ComponentConfig\.h/;

        $configurationsCSVText .= ParseFile($file, \@replicationConfig, \@replicationInternalConfig, \@transactionalReplicatorConfig, \@transactionalReplicatorInternalConfig, 0);
        $linuxConfigurationsCSVText .= ParseFile($file, \@replicationConfig, \@replicationInternalConfig, \@transactionalReplicatorConfig, \@transactionalReplicatorInternalConfig, 1);
    }
    
    $configurationsCSVText .= GetHttpGatewayConfigurations();
    
    $linuxConfigurationsCSVText .= GetHttpGatewayConfigurations();
    
    my $ret_hash_ref = {};
    $ret_hash_ref->{'linux'} = $linuxConfigurationsCSVText;
    $ret_hash_ref->{'windows'} = $configurationsCSVText;
    
    return $ret_hash_ref;
}

sub FilterPublicConfigCSV
{    
    my $configurationsCSVText = shift;
    my $publicConfigurationsCSVText = "#Section Name,Setting Name,Type,Default Value,Upgrade Policy,Setting Type,Config Found In,Description\n";
    my @configs = split/\n/, $configurationsCSVText;
    foreach my $config(@configs)
    {
        my @settings = split/,/,$config;  
        
        # Add all the lines which Setting Type (value of the 6th column) is PUBLIC or DEPRECATED
        if(@settings >= 6 && ($settings[5] =~ /PUBLIC/ || $settings[5] =~ /DEPRECATED/))
        {
            $publicConfigurationsCSVText = $publicConfigurationsCSVText.$config."\n" ;
        }       
    }    

    return $publicConfigurationsCSVText;
}

sub Main
{   
    # Write internal config file 
    my $ret_hash_ref = BuildConfigCSV();
    my $configurationsCSVText = $ret_hash_ref->{'windows'};
    WriteAllText($DestFile, $configurationsCSVText);
    
    my $linuxConfigurationsCSVText = $ret_hash_ref->{'linux'};
    WriteAllText($LinuxDestFile, $linuxConfigurationsCSVText);
    
    # Write public config file
    my $publicConfigurationsCSVText = FilterPublicConfigCSV($configurationsCSVText);
    WriteAllText($PublicDestFile, $publicConfigurationsCSVText);
    
    my $retVal;
    if (@invalidConfigEntries)
    {
        open (my $of_err_log, ">", $ErrorFile) or die "can't write to $ErrorFile: $!";

        foreach my $line1 (@invalidConfigEntries)
        {
            print $of_err_log "$line1";
        }

        close($of_err_log);
        
        print STDERR "Configuration generation failed. Please check $ErrorFile for more details.\n";
        $retVal = 1;
    }
    else
    {
        $retVal = 0;
    }

    return $retVal;
}

my $exitCode = Main();
printf "\n\n\nExitCode ";
printf $exitCode;

if ($exitCode == 0)
{
    exit $exitCode;
}
else
{
    die;
}
