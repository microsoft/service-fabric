set( LINUX_SOURCES
  ../AccountHelper.cpp
  ../ActivityId.cpp
  ../ActivityType.cpp
  ../ActivityDescription.cpp
  ../ApiEventSource.cpp
  ../ApiMonitoring.ApiName.cpp
  ../ApiMonitoring.ApiNameDescription.cpp
  ../ApiMonitoring.InterfaceName.cpp
  ../ApiMonitoring.MonitoringComponent.cpp
  ../AssertWF.cpp
  ../AsyncExclusiveLock.cpp
  ../AsyncFabricComponent.cpp
  ../AsyncWorkJobItem.cpp
  ../AsyncOperationWorkJobItem.cpp
  ../AsyncWorkJobItemState.cpp
  ../AsyncWorkJobQueuePerfCounters.cpp
  ../AsyncOperation.cpp
  ../AsyncOperationState.cpp
  ../AsyncWaitHandle.Linux.cpp
  ../BigInteger.cpp
  ../CachedFile.cpp
  ../CabOperations.cpp
  ../CertDirMonitor.Linux.cpp
  ../ComAsyncOperationContext.cpp
  ../ComAsyncOperationWaiter.cpp
  ../ComCompletedAsyncOperationContext.cpp
  ../ComConfigStore.cpp
  ../CommonConfig.cpp
  ../CommonEventSource.cpp
  ../CommonName.cpp
  ../CompletedAsyncOperation.cpp
  ../ComponentConfig.cpp
  ../componentroot.cpp
  ../ComProxyAsyncOperation.cpp
  ../ComProxyConfigStore.cpp
  ../ComProxyXmlLiteReader.cpp
  ../ComProxyXmlLiteWriter.cpp
  ../ComStringResult.cpp
  ../ComUnknownBase.cpp
  ../ComUtility.cpp
  ../ComXmlFileStream.cpp
  ../Config.cpp
  ../ConfigEntryBase.cpp
  ../ConfigEntryUpgradePolicy.cpp
  ../ConfigEventSource.cpp
  ../ConfigOverrideDescription.cpp
  ../ConfigParameter.cpp
  ../ConfigParameterOverride.cpp
  ../ConfigSection.cpp
  ../ConfigSectionOverride.cpp
  ../ConfigSettings.cpp
  ../ConfigSettingsConfigStore.cpp
  ../ConfigSettingsOverride.cpp
  ../ConfigStore.cpp
  ../ContainerEnvironment.cpp
  ../crc.cpp
  ../CryptoBitBlob.cpp
  ../CryptoUtility.cpp
  ../DateTime.cpp
  ../DbgHelper.cpp
  ../Directory.cpp
  ../DllConfig.cpp
  ../Endpoint.cpp
  ../Environment.cpp
  ../ErrorCode.cpp
  ../ErrorCodeDetails.cpp
  ../ErrorCodeValue.cpp
  ../EventLoop.cpp
  ../Expression.cpp
  ../ExtendedEventMetadata.cpp
  ../FabricCodeVersion.cpp
  ../FabricComponent.cpp
  ../FabricComponentState.cpp
  ../FabricConfigVersion.cpp
  ../FabricConstants.cpp
  ../FabricEnvironment.cpp  #Partially migrated, logic related to registry needs to be changed
  ../FabricNodeConfig.cpp
  ../FabricNodeGlobalConfig.cpp
  ../FabricSerializer.cpp
  ../FabricSignal.cpp
  ../FabricVersion.cpp
  ../FabricVersionConstants.cpp
  ../FabricVersionInstance.cpp
  ../FailPointList.cpp
  ../FailPointManager.cpp
  ../File.cpp
  ../FileChangeMonitor.Linux.cpp
  ../FileConfigStore.cpp
  ../FileLock.Linux.cpp
  ../FileReaderWriterLock.Linux.cpp
  ../FileWriter.cpp
  ../Formatter.cpp
  ../Guid.cpp
  ../IpAddressPrefix.cpp
  ../IpUtility.cpp
  ../JobQueuePerfCounters.cpp
  ../JobQueueConfigSettingsHolder.cpp
  ../JobQueueDequePolicyHelper.cpp
  ../KtlSF.Common.cpp
  ../KtlAwaitableProxyAsyncOperation.cpp
  ../KtlProxyAsyncOperation.cpp
  ../LargeInteger.cpp
  ../LinkableAsyncOperation.cpp
  ../CryptoUtility.Linux.cpp
  ../LinuxPackageManagerType.cpp
  ../LogLevel.cpp
  ../ManagedPerformanceCounterSetWrapper.cpp
  ../Math.cpp
  ../MonitoredConfigSettingsConfigStore.cpp
  ../MutexHandle.cpp
  ../NamingUri.cpp
  ../Operators.cpp
  ../PaasConfig.cpp
  ../PAL.cpp
  ../ParameterValidator.cpp
  ../Path.cpp
  ../Parse.cpp
  ../PerformanceCounterAverageTimerData.cpp
  ../PerformanceCounterDefinition.cpp
  ../PerformanceCounterSet.cpp
  ../PerformanceCounterSetDefinition.cpp
  ../PerformanceCounterSetInstance.cpp
  ../PerformanceCounterSetInstanceType.cpp
  ../PerformanceCounterType.cpp
  ../PerformanceProvider.cpp
  ../PerformanceProviderCollection.cpp
  ../PerformanceProviderDefinition.cpp
  ../PerformanceCounterApi.cpp  
  ../processterminationservice.cpp  
  ../Random.cpp
  ../RealConsole.Linux.cpp
  ../RegistryKeyLinux.cpp
  ../SubjectName.cpp
  ../ProcessHandle.cpp
  ../ProcessInfo.cpp
  ../ProcessUtility.cpp #Job/Security related functions not ported
  ../RwLock.cpp
  ../ScopedHeap.cpp  #Not supported
  ../SecureString.cpp
  ../SecurityConfig.cpp
  ../SequenceNumber.cpp
  ../ServiceInfoUtility.cpp
  ../ServicePlacementPolicyHelper.cpp
  ../Socket.cpp
  ../StackTrace.Linux.cpp
  ../StateMachine.cpp
  ../Stopwatch.cpp
  ../StopwatchTime.cpp
  ../StringResource.cpp
  ../StringUtility.cpp
  ../SystemHealthReportCode.cpp
  ../TestPortHelper.cpp
  ../TextTraceWriter.cpp
  ../TextWriter.cpp
  ../ThreadErrorMessages.cpp
  ../Threadpool.cpp
  #../Threadpool.Linux.cpp
  ../ProcessWait.Linux.cpp
  ../Throw.cpp
  ../Thumbprint.cpp
  ../ThumbprintSet.cpp
  ../TimedAsyncOperation.cpp
  ../TimeoutHelper.cpp
  ../TimerQueue.cpp
  ../Timer.Linux.cpp  #Needs improvement
  ../TimeSpan.cpp
  ../TokenHandle.cpp
  ../Trace.cpp
  ../TraceChannelType.cpp
  ../DiagnosticsConfig.cpp
  ../TraceConsoleSink.cpp
  ../TraceCorrelatedEvent.cpp
  ../TraceEvent.cpp
  ../TraceEventContext.cpp
  ../TraceFilterDescription.cpp
  ../TraceKeywords.cpp
  ../TraceManifestGenerator.cpp
  ../TraceSession.cpp
  ../TraceSinkFilter.cpp
  ../TraceTextFileSink.cpp
  ../Types.cpp
  ../Uri.cpp
  ../Uri.Parser.cpp
  ../UriBuilder.cpp
  ../VariableArgument.cpp
  ../VectorStream.cpp
  ../VersionRange.cpp
  ../VersionRangeCollection.cpp
  ../WaitHandle.cpp
  ../WinErrorCategory.cpp  #Remove or rename
  ../X509FindSubjectAltName.cpp
  ../X509FindType.cpp
  ../X509FindValue.cpp
  ../X509Identity.cpp
  ../X509PubKey.cpp
  ../X509StoreLocation.cpp
  ../XmlLiteReader.Linux.cpp
  ../XmlLiteWriter.Linux.cpp
  ../XmlReader.cpp
  ../XmlWriter.cpp
  ../Zip/Zip_Linux.cpp
  
#Not Migrated
 ../AccessToken.cpp
# ../Dacl.cpp
 ../DesktopHandle.cpp
# ../FileVersion.cpp
 ../ImpersonationContext.cpp
 ../JobHandle.cpp
# ../ProfileHandle.cpp
# ../SecurityDescriptor.cpp
  ../SecurityPrincipalHelper.cpp
# ../asyncfile.cpp
# ../AsyncFileOverlappedIO.cpp
# ../AsyncFile.ReadFileAsyncOperation.cpp
# ../OverlappedIO.cpp
  ../SecurityUtility.cpp
#  ../Service.cpp
  ../Sid.Linux.cpp
# ../SMBShareUtility.cpp
)

set (LINUX_SOURCES_THREADPOOL
  ../Threadpool/Win32ThreadpoolWrapper.cpp
  ../Threadpool/Win32ThreadpoolConfig.cpp
  ../Threadpool/Threadpool.cpp
  ../Threadpool/ThreadpoolRequest.cpp
  ../Threadpool/HillClimbing.cpp
  ../Threadpool/MinPal.cpp
  ../Threadpool/UnfairSemaphore.cpp
)

add_library(objects_Common_Threadpool OBJECT ${LINUX_SOURCES_THREADPOOL})

include_directories("..")

add_library(objects_Common OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_Common ../stdafx.h)

#add_library(${lib_Common} STATIC $<TARGET_OBJECTS:objects_Common>)
add_library(${lib_Common} STATIC $<TARGET_OBJECTS:objects_Common> $<TARGET_OBJECTS:objects_Common_Threadpool>)
target_link_libraries(${lib_Common}
    ${Ktlfull}
    ${lib_Pal}
    minizip
    z
)

set_target_properties(${lib_Common} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})
