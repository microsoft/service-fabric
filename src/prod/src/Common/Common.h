// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#pragma warning(push)
#pragma warning(disable: 4201) // nameless struct/union
#include <ntdef.h>
#include <nt.h>
#include <ntrtl.h>
#include <bldver.h>
#include <ntrtl_x.h>
#include <nturtl.h>
#include <ntregapi.h>
#pragma warning(pop)

#include <winsock2.h>
#include <windef.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <psapi.h>

#undef max
#undef min

//Bug#169803 - Remove usings from Common.h
namespace Common
{
    struct NOTHROW {};
    struct NOCLEAR {};
}

// Common STL dependencies
#include <wincrypt.h>

#if defined(PLATFORM_UNIX)

#include <sal.h>
#include <aio.h>
#include <pwd.h>
#include <grp.h>
#include <sys/epoll.h>
#include <signal.h>
#include "PAL.h" // from prod/src/inc/clr

#undef __in    // is used in stdlib
#undef __out   // is used in stdlib
#undef __null  // is used in stdlib

#else

#include <KtmW32.h>
typedef int pid_t;

#endif

#include <algorithm>
#include <functional>
#include <exception>
#include <iostream>
#include <list>
#include <map>
#include <stack>
#if defined(PLATFORM_UNIX)
#include <ext/hash_map>
#else
#include <hash_map>
#include <iterator>
#endif
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <random>
#include <set>
#include <queue>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <initializer_list>

#if defined(PLATFORM_UNIX)
#define __in
#define __out
#define __null 0
#endif

#include <strsafe.h>
#include <Unknwn.h>
#include <windows.h>
#if !defined(PLATFORM_UNIX)
#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#endif
#include <evntprov.h>
#include <evntrace.h>
#include <lm.h>
#include <pdh.h>
#include <pla.h>
#include <comutil.h>
#include <profinfo.h>
#include <userenv.h>
#include <sddl.h>
#include <accctrl.h>
#include <aclapi.h>
#include <fwpvi.h>
#include <fwpmu.h>
#include <fwpmtypes.h>
#include <xmllite.h>
#include <Shlwapi.h>
#include <shlobj.h>
#include <system_error>
#include <limits>
#include <tuple>
#include <bitset>
#include <locale>

#include <perflib.h>

#include "FabricTypes.h"

#include "resources/Resources.h"

#include "Common/WinErrorCategory.h"

#include "Common/macro.h"
#include "Common/Types.h"

#include "Common/TextWriter.h"

#include "serialization/inc/Serialization.h"

#include "ServiceModel/federation/Lease.h"

// New header added for LogLevel.
#include "Common/LogLevel.h"

// Tracing headers.
#include "Common/TraceKeywords.h"
#include "Common/TraceChannelType.h"
#include "Common/TraceTaskCodes.h"
#include "Common/TraceFilterDescription.h"
#include "Common/TraceSinkFilter.h"

#include "Common/FabricSerializationHelper.h"

// VariableArgument Needs them.
#include "Common/TimeSpan.h"
#include "Common/DateTime.h"
#include "Common/StopwatchTime.h"

#include "Common/Utility.h"
#include "Common/VariableArgument.h"

#include "Common/Parse.h"                  // For Config.h (TryParseInt64)
#include "Common/StringUtility.h"          // For Config.h

#include "Common/IConfigUpdateSink.h"      // For Trace.h
#include "Common/TraceSinkType.h"          // For Trace.h
#include "Common/Guid.h"                   // For Trace.h
#include "Common/Throw.h"                  // For File.h
#include "Common/File.h"                   // For FileWriter.h
#include "Common/FileWriter.h"             // For TraceManifestGenerator
#include "Common/TraceManifestGenerator.h" // For Trace.h
#include "Common/RwLock.h"                 // For Trace.h
#include "Common/Trace.h"                  // TraceProvider for TextTraceWriter
#include "Common/TraceTextFileSink.h"      // For TraceEvent
#include "Common/TraceEvent.h"             // For TextTraceWriter
#include "Common/TextTraceWriter.h"
#include "Common/ConfigStore.h"

#include "Common/SecureString.h"           // For Config.h
#include "Common/FabricCodeVersion.h"      // For Config.h
#include "Common/FabricConfigVersion.h"    // For Config.h
#include "Common/FabricVersion.h"          // For Config.h
#include "Common/FabricVersionInstance.h"  // For Config.h
#include "Common/TraceEventContext.h"      // For ExtendedEventMetadata
#include "Common/ExtendedEventMetadata.h"  // For TraceEventWriter
#include "Common/TraceEventWriter.h"       // For ErrorCodeValue.h (DECLARE_ENUM_STRUCTURED_TRACE)
#include "Common/ThreadErrorMessages.h"
#include "Common/ErrorCodeValue.h"         // For ErrorCode.h
#include "Common/ErrorCode.h"              // For Config.h
#include "Common/X509StoreLocation.h"      // For Config.h
#include "Common/Config.h"
#include "Common/AssertWF.h"

#if !defined(PLATFORM_UNIX)
#include "Common/CRTAbortBehavior.h"
#endif

#include "Common/ByteBuffer.h"

#include "Common/Formatter.h"              // For FormatOptions
#include "Common/ITextWritable.h"

#include "Common/Atomics.h"

#ifdef PLATFORM_UNIX
#include "ProcessInfo.h"
#include "minizip/zip.h"
#include "minizip/unzip.h"
#include "Common/Zip/Zip_Linux.h"
#else
#include "Common/CertificateManager.h"
#include "Common/CertStore.h"
#include "Common/CryptProvider.h"
#endif

#include "DbgHelper.h"

#include "Common/Directory.h"

#include "Common/CachedFile.h"
#include "Common/FileLock.h"
#include "Common/FileReaderLock.h"
#include "Common/FileWriterLock.h"
#include "Common/ExclusiveFile.h"

#include "Common/TraceSinkType.h"
#include "Common/TraceManifestGenerator.h"

#include "Common/IntrusiveList.h"
#include "Common/IoBuffers.h"
#include "Common/IntrusivePtr.h"

#include "Common/Path.h"
#if defined(PLATFORM_UNIX)
#include "Common/RealConsole.Linux.h"
#else
#include "Common/RealConsole.h"
#endif
#include "Common/Random.h"
#include "Common/SequenceNumber.h"
#include "Common/Serialization.h"
#include "Common/VectorStream.h"
#include "Common/StackTrace.h"

#include "Common/Stopwatch.h"
#include "Common/Bique.h"

#include "Common/ProcessTerminationService.h"

#include "Common/HandleBase.h"
#include "Common/WaitHandle.h"
#include "Common/ReferencePointer.h"
#include "Common/ReferenceArray.h"
#include "Common/ScopedHeap.h"
#include "Common/TextWritableTypes.h"

// Common retail headers
#include "FabricCommon.h"
#include "FabricCommon_.h"

#include "Common/TraceConsoleSink.h"
#include "Common/TraceCorrelatedEvent.h"
#include "Common/flags.h"

#include "Common/ErrorCodeDetails.h"
#include "Common/Validator.h"

// Base Com headers
#include "Common/ComUtility.h"
#include "Common/ComPointer.h"
#include "Common/ComUnknownBase.h"
#include "Common/ComStringResult.h"

// XML reading and writing
#if defined(PLATFORM_UNIX)
#include "Common/XmlLiteReader.Linux.h"
#include "Common/XmlLiteWriter.Linux.h"
#endif
#include "Common/XmlException.h"
#include "Common/ComXmlFileStream.h"
#include "Common/ComProxyXmlLiteReader.h"
#include "Common/ComProxyXmlLiteWriter.h"
#include "Common/XmlReader.h"
#include "Common/XmlWriter.h"

// convienence/windows utility type headers
#include "Common/ActivityId.h"
#include "Common/ActivityType.h"
#include "Common/ActivityDescription.h"
#include "Common/FabricSerializer.h"
#include "Common/EventArgs.h"
#include "Common/Event.h"
#include "Common/MoveUPtr.h"
#include "Common/Threadpool.h"
#ifdef PLATFORM_UNIX
#include "Common/TimerQueue.h"
#include "Common/EventLoop.h"
#endif
#include "Common/Timer.h"
#include "Common/Handle.h"
#include "Common/ProcessWait.h"
#include "Common/ThreadpoolWait.h"
#include "Common/Tree.h"

#include "Common/TimeoutHelper.h"
#include "Common/StringResource.h"
#include "Common/AllocableObject.h"

// async operation headers
#include "Common/AsyncOperationState.h"
#include "Common/AsyncOperation.h"
#include "Common/AsyncOperationWaiter.h"
#include "Common/CompletedAsyncOperation.h"
#include "Common/ComponentRoot.h"
#include "Common/ComComponentRoot.h"
#include "Common/LinkableAsyncOperation.h"
#include "Common/ParallelAsyncOperationBase.h"
#include "Common/ParallelKeyValueAsyncOperation.h"
#include "Common/ParallelVectorAsyncOperation.h"
#include "Common/RootedObject.h"
#include "Common/RootedObjectHolderT.h"
#include "Common/RootedObjectWeakHolderT.h"
#include "Common/RootedObjectPointer.h"
#include "Common/TimedAsyncOperation.h"
#include "Common/AsyncWaitHandle.h"
#include "Common/AllocableAsyncOperation.h"
#include "Common/KtlAwaitableProxyAsyncOperation.h"
#include "Common/KtlProxyAsyncOperation.h"

// configuration headers
#include "Common/ConfigEntryUpgradePolicy.h"
#include "Common/ConfigEntryBase.h"
#include "Common/ConfigEventSource.h"
#include "Common/ConfigEntry.h"
#include "Common/ConfigGroup.h"
#include "Common/ComponentConfig.h"

#include "Common/SingletonConfig.h"
#include "Common/FileConfigStore.h"
#include "Common/CommonConfig.h"
#include "Common/ConfigParameterOverride.h"
#include "Common/ConfigSectionOverride.h"
#include "Common/ConfigSettingsOverride.h"
#include "Common/ConfigOverrideDescription.h"
#include "Common/ConfigParameter.h"
#include "Common/ConfigSection.h"
#include "Common/ConfigSettings.h"
#include "Common/ConfigSettingsConfigStore.h"
#include "Common/MonitoredConfigSettingsConfigStore.h"
#include "Common/DllConfig.h"
#include "Common/ComConfigStore.h"
#include "Common/ComProxyConfigStore.h"

#include "Common/TokenHandle.h"
#include "Common/ProcessHandle.h"
#include "Common/MutexHandle.h"
#include "Common/ResourceHolder.h"
#if defined(PLATFORM_UNIX)
#include "Common/DiagnosticsConfig.h"
#include "Common/LinuxPackageManagerType.h"
#endif

// internal types
#include "Common/BigInteger.h"
#include "Common/LargeInteger.h"

// TODO move nodeid to common.
#include "ServiceModel/federation/NodeId.h"
#include "ServiceModel/federation/NodeInstance.h"

// Size estimator
#include "Common/ISizeEstimator.h"
#include "Common/SizeEstimatorHelper.h"
// state machines and generic components
#include "Common/StateMachine.h"
#include "Common/FabricComponentState.h"
#include "Common/AsyncFabricComponent.h"
#include "Common/FabricComponent.h"

// data structure headers
#include "Common/AsyncExclusiveLock.h"
#include "Common/ExpiringSet.h"
#include "Common/EmbeddedList.h"
#include "Common/LruCacheWaiterList.h"
#include "Common/LruCacheWaiterTable.h"
#include "Common/LruCache.h"
#include "Common/SynchronizedMap.h"
#include "Common/SynchronizedSet.h"
#include "Common/ReaderQueue.h"
#include "Common/Uri.h"
#include "Common/Uri.Parser.h"
#include "Common/UriBuilder.h"
#include "Common/Environment.h"
#include "Common/NamingUri.h"
#include "Common/LruPrefixCache.h"

// failpoint headers
#include "Common/FailPointContext.h"
#include "Common/IFailPointAction.h"
#include "Common/IFailPointCriteria.h"
#include "Common/FailPoint.h"
#include "Common/FailPointList.h"
#include "Common/FailPointManager.h"

// COM headers
#include "Common/ComAsyncOperationContext.h"
#include "Common/ComAsyncOperationWaiter.h"
#include "Common/ComCompletedAsyncOperationContext.h"
#include "Common/ComProxyAsyncOperation.h"
#include "Common/ParameterValidator.h"
#include "Common/MoveCPtr.h"
#include "Common/ComXmlFileStream.h"

// PaasConfig
#include "Common/PaasConfig.h"

// FabricNodeConfig
#include "Common/FabricNodeConfig.h"
#include "Common/FabricNodeGlobalConfig.h"

// Expression Parser
#include "Common/OperationResult.h"
#include "Common/Operators.h"
#include "Common/Expression.h"

#include "Common/FileVersion.h"

#include "Common/ProcessUtility.h"
#include "Common/FileChangeMonitor.h"

#if defined(PLATFORM_UNIX)
#include "Common/CertDirMonitor.Linux.h"
#endif

// LINUXTODO: consider removing registry usage
// Registry Accessor
#include "Common/RegistryKey.h"

// performance counter headers
#include "Common/PerformanceCounter.h"
#include "Common/PerformanceCounterSetInstanceType.h"
#include "Common/PerformanceCounterType.h"
#include "Common/PerformanceCounterData.h"
#include "Common/PerformanceCounterAverageTimerData.h"
#include "Common/PerformanceProvider.h"
#include "Common/PerformanceProviderCollection.h"
#include "Common/PerformanceCounterDefinition.h"
#include "Common/PerformanceCounterSetInstance.h"
#include "Common/PerformanceCounterSet.h"
#include "Common/PerformanceCounterSetDefinition.h"
#include "Common/PerformanceProviderDefinition.h"
#include "Common/PerfMonitorEventSource.h"

// Job Scheduling
#include "Common/JobQueueConfigSettingsHolder.h"
#include "Common/JobQueueDequePolicyHelper.h"
#include "Common/AsyncWorkJobQueuePerfCounters.h"
#include "Common/JobQueuePerfCounters.h"
#include "Common/AsyncWorkJobItemState.h"
#include "Common/AsyncWorkJobItem.h"
#include "Common/AsyncOperationWorkJobItem.h"
#include "Common/AsyncWorkJobQueue.h"
#include "Common/JobQueue.h"
#include "Common/BatchJobQueue.h"

// FabricConstants
#include "Common/FabricConstants.h"

// VersionRange
#include "Common/VersionRange.h"
#include "Common/VersionRangeCollection.h"

// Structured tracing
#include "Common/CommonEventSource.h"

// Health Reports
#include "Common/SystemHealthReportCode.h"

// Utility classes for public structures
#include "Common/ServiceInfoUtility.h"
#include "Common/ServicePlacementPolicyHelper.h"

// Fabric Environment
#include "Common/FabricEnvironment.h"
#include "Common/ContainerEnvironment.h"

// security related headers
#include "Common/Sid.h"
#include "Common/Dacl.h"
#include "Common/X509FindType.h"
#include "Common/X509FindValue.h"
#include "Common/X509Identity.h"
#include "Common/X509PubKey.h"
#include "Common/Thumbprint.h"
#include "Common/ThumbprintSet.h"
#include "Common/SubjectName.h"
#include "Common/CommonName.h"
#include "Common/X509FindSubjectAltName.h"
#include "Common/CryptoBitBlob.h"
#include "Common/CryptoUtility.h"
#include "Common/SecurityUtility.h"
#include "Common/SecurityDescriptor.h"
#include "Common/ImpersonationContext.h"
#include "Common/SecurityPrincipalHelper.h"
#include "Common/SecurityConfig.h"
#include "Common/ProfileHandle.h"
#include "Common/AccessToken.h"

// transport related headers
#include "Common/Endpoint.h"
#include "Common/Socket.h"
#include "Common/IpAddressPrefix.h"
#include "Common/TestPortHelper.h"
#include "Common/IpUtility.h"

// windows resource wrappers
#include "Common/WorkstationHandle.h"
#include "Common/DesktopHandle.h"
#include "Common/JobHandle.h"
#include "Common/SMBShareUtility.h"
#include "Common/AccountHelper.h"
#include "Common/ScopedResourceOwner.h"
#include "Common/Service.h"

// Json serialization
#include "Common/Serializable.h"
#include "Common/EnumJsonSerializer.h"
#include "Common/JsonInterface.h"
#include "Common/JsonReaderWriter.h"
#include "Common/JsonReaderHelper.h"
#include "Common/JsonWriter.h"
#include "Common/JsonReader.h"

// Cab FDI operations
#if !defined(PLATFORM_UNIX)
#include <fcntl.h>
#include <sys/stat.h>
#include <fdi.h>
#endif
#include "Common/CabOperations.h"

#ifndef PLATFORM_UNIX

// Async file operation
#include "Common/OverlappedIO.h"
#include "Common/AsyncFile.h"
#include "Common/AsyncFileOverlappedIO.h"
#include "Common/AsyncFile.ReadFileAsyncOperation.h"
#include "Common/AsyncFile.WriteFileAsyncOperation.h"

//
// Lookaside allocator.
// This implementation of lookaside allocator uses Interlocked SLists which are
// available on windows header files. We prbly need to take that implementation into our own code
// in order to make this OS agnostic(so we remove our dependency on windows header file for this.).
//
#include "Common/InterlockedPoolPerfCounters.h"
#include "Common/IndividualHeapAllocator.h"
#include "Common/InterlockedPool.h"
#include "Common/InterlockedBufferPool.h"
#include "Common/InterlockedBufferPoolAllocator.h"

#endif

#include "Common/ApiMonitoring.Pointers.h"
#include "Common/ApiMonitoring.InterfaceName.h"
#include "Common/ApiMonitoring.ApiName.h"
#include "Common/ApiMonitoring.ApiNameDescription.h"
#include "Common/ApiMonitoring.MonitoringParameters.h"
#include "Common/ApiMonitoring.MonitoringData.h"
#include "Common/ApiMonitoring.MonitoringComponentMetadata.h"
#include "Common/ApiMonitoring.ApiCallDescription.h"
#include "Common/ApiMonitoring.MonitoringComponent.h"


#include "Common/ApiEventSource.h"

#include "Common/Math.h"

#include "Common/crc.h"
