// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
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
#if !defined(PLATFORM_UNIX)
#include <KtmW32.h>
#endif


#if !defined(_LEASLAYR_H_)
#define _LEASLAYR_H_

#define RTL_USE_AVL_TABLES 0 // switches from splay trees to AVL trees for improved performance
#include <ntosp.h>
#include <limits.h>

//#include "Microsoft-WindowsFabric-LeaseEvents.h"

#define NTSTRSAFE_LIB

#include "leaselayerio.h"
//#include "leaselayertrace.h"
#include "leaselayerinc.h"

//#ifdef KTL_TRANSPORT
//#include "../ktransport/leasetransport.h"
//#else
#include "Transport/Transport.h"
//#endif

//
// Lease Layer Trace Provider.
//
DEFINE_GUID(GUID_TRACE_PROVIDER_LEASELAYER, 
        0x3f68b79e, 0xa1cf, 0x4b10, 0x8c, 0xfd, 0x3d, 0xfe, 0x32, 0x2f, 0x07, 0xcb);
// {3F68B79E-A1CF-4B10-8CFD-3DFE322F07CB}

//
// Lease Layer WMI Provider.
//
DEFINE_GUID (GUID_WMI_PROVIDER_LEASELAYER,
        0xB85B7C50, 0x6A01, 0x11d2, 0xB8, 0x41, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);
//{B85B7C50-6A01-11d2-B841-00C04FAD5171}

//
// Lease Layer WMI event identifier.
//
DEFINE_GUID (LEASELAYER_WMI_NOTIFICATION_EVENT,
        0x1cdaff1, 0xc901, 0x45b4, 0xb3, 0x59, 0xb5, 0x54, 0x27, 0x25, 0xe2, 0x9c);
// {01CDAFF1-C901-45b4-B359-B5542725E29C}

//
// Tag used to mark all memory allocations.
//
#define LEASELAYER_POOL_TAG (ULONG) 'YLSL'
#define LEASE_RELATHIONSHIP_CTXT_TAG (ULONG) 'CSSL'
#define LEASE_RELATHIONSHIP_ID_TAG (ULONG) 'ISSL'
#define LEASE_TABLE_TAG (ULONG) 'BTSL'
#define LEASE_REMOTE_LEASE_AGENT_CTXT_TAG (ULONG) 'CRSL'
#define LEASE_REMOTE_LEASE_AGENT_ID_TAG (ULONG) 'IRSL'
#define LEASE_EVENT_TAG (ULONG) 'NESL'
#define LEASE_APPLICATION_CTXT_TAG (ULONG) 'CASL'
#define LEASE_APPLICATION_ID_TAG (ULONG) 'IASL'
#define LEASE_MESSAGE_TAG (ULONG) 'SMSL'

#define LEASELAYER_REMOTE_CERT_CONTEXT (ULONG) 'CCSL'
#define LEASE_TAG_REMOTE_CERT 'CRtl'
#define LEASE_TAG_LOCAL_CERT_STORENAME 'CLSL'
#define LEASE_LISTEN_ENDPOINT (ULONG) 'PESL'

//#if DBG
//#define LAssert(cond) ((!(cond)) ? (KdBreakPoint(), FALSE) : TRUE)
//#else
//#define LAssert(cond) ( 0 )
//#endif

#define LAssert(cond) Invariant(cond)

//
// MOF
//
#define MOFRESOURCENAME L"WmiLeaseLayer"

//
// Device root name.
//
#define LEASELAYER_DEVICE_NAME_STRING L"\\Device\\LeaseLayer"
#define LEASELAYER_SYMBOLIC_NAME_STRING L"\\DosDevices\\LeaseLayer"

//
// Common LARGE_INTEGER operations.
//
#define LARGE_INTEGER_ADD_LARGE_INTEGER_AND_ASSIGN(a,b,c) (a.QuadPart = b.QuadPart + c.QuadPart)
#define LARGE_INTEGER_ADD_INTEGER_AND_ASSIGN(a,b,c) (a.QuadPart = b.QuadPart + (LONGLONG)c)
#define LARGE_INTEGER_SUBTRACT_INTEGER_AND_ASSIGN(a,b,c) (a.QuadPart = b.QuadPart - (LONGLONG)c)
#define LARGE_INTEGER_SUBTRACT_LARGE_INTEGER_AND_ASSIGN(a,b,c) (a.QuadPart = b.QuadPart - c.QuadPart)
#define LARGE_INTEGER_ADD_INTEGER(a,b) (a.QuadPart = a.QuadPart + (LONGLONG)b)
#define LARGE_INTEGER_SUBTRACT_INTEGER(a,b) (a.QuadPart = a.QuadPart - (LONGLONG)b)
#define LARGE_INTEGER_ASSIGN_INTEGER(a,b) (a.QuadPart = (LONGLONG)b)
#define IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_INTEGER(a,b) (a.QuadPart >= (LONGLONG)b)
#define IS_LARGE_INTEGER_GREATER_THAN_INTEGER(a,b) (a.QuadPart > (LONGLONG)b)
#define IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_LARGE_INTEGER(a,b) (a.QuadPart >= b.QuadPart)
#define IS_LARGE_INTEGER_GREATER_THAN_LARGE_INTEGER(a,b) (a.QuadPart > b.QuadPart)
#define IS_LARGE_INTEGER_LESS_THAN_LARGE_INTEGER(a,b) (a.QuadPart < b.QuadPart)
#define IS_LARGE_INTEGER_LESS_THAN_OR_EQUAL_INTEGER(a,b) (a.QuadPart <= (LONGLONG)b)
#define IS_LARGE_INTEGER_LESS_THAN_INTEGER(a,b) (a.QuadPart < (LONGLONG)b)
#define IS_LARGE_INTEGER_EQUAL_TO_LARGE_INTEGER(a,b) (a.QuadPart == b.QuadPart)
#define IS_LARGE_INTEGER_EQUAL_TO_INTEGER(a,b) (a.QuadPart == (LONGLONG)b)

//
// All default time constants are in milliseconds.
//
#define LEASE_PROTOCOL_MAJOR_VERSION 2
#define LEASE_PROTOCOL_MINOR_VERSION_V1 0
#define LEASE_PROTOCOL_MINOR_VERSION 1
#define MILLISECOND_TO_NANOSECOND_FACTOR 10000 //This is wrong, should be named as something like "TickPerMillisecond", LINUXTODO, replace all of them with TICK_PER_MILLISECOND
#define TICK_PER_MILLISECOND 10000
#define DURATION_MAX_VALUE (MAXLONG - 1)
#define ARBITRATION_TIMEOUT_PERCENTAGE 1
#define RENEW_RETRY_INTERVAL_PERCENTAGE 5
#define RENEW_RETRY_INTERVAL_PERCENTAGE_LOW 3
#define RENEW_RETRY_INTERVAL_PERCENTAGE_HIGH 2
#define MAINTENANCE_INTERVAL 15
#define PRE_ARBITRATION_TIME 2000 * MILLISECOND_TO_NANOSECOND_FACTOR //2 seconds before arbitration
#define PING_RETRY_INTERVAL 10000 * MILLISECOND_TO_NANOSECOND_FACTOR //10 seconds

//
// Cleanup thread (garbage collector).
//
//KSTART_ROUTINE MaintenanceWorkerThread;

// Mock
#define KEVENT Common::ManualResetEvent
#define PKEVENT Common::ManualResetEvent *
#define KeClearEvent(a) ((Common::ManualResetEvent *)a)->Reset();
#define KeInitializeEvent(a,b,c) if(c) ((Common::ManualResetEvent *)a)->Set();
#define KeSetEvent(a,b,c) ((Common::ManualResetEvent *)a)->Set();

inline NTSTATUS KeWaitForSingleObject(PVOID Object, INT WaitReason, INT WaitMode, BOOLEAN Alertable, LARGE_INTEGER * Timeout)
{
    if (Timeout == NULL)
        return ((Common::ManualResetEvent *)Object)->WaitOne() ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    else
        return ((Common::ManualResetEvent *)Object)->WaitOne(Common::TimeSpan::FromTicks(Timeout->QuadPart)) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


#define ExAcquireFastMutex(a) ((Common::RwLock *)a)->AcquireExclusive();
#define ExReleaseFastMutex(a) ((Common::RwLock *)a)->ReleaseExclusive();
#define KeAcquireSpinLock(a,b) ((Common::RwLock *)a)->AcquireExclusive();
#define KeAcquireSpinLockAtDpcLevel(a) ((Common::RwLock *)a)->AcquireExclusive();
#define KeReleaseSpinLock(a,b) ((Common::RwLock *)a)->ReleaseExclusive();
#define KeReleaseSpinLockFromDpcLevel(a) ((Common::RwLock *)a)->ReleaseExclusive();
#define KeInitializeSpinLock(a)

#define KDPC int
#define PKDPC KDPC*

//TODO ignore for now, no detection
#define PEPROCESS INT

#define KTIMER Common::TimerQueue::TimerSPtr
#define PKTIMER KTIMER*

Common::TimerQueue & GetLeaseTimerQueue();
#define CreateTimer(timer, callback, context) \
    timer = GetLeaseTimerQueue().CreateTimer(COMMON_STRINGIFY(timer), [=] { callback(0, context, nullptr, nullptr); })
#define KeQuerySystemTime(a) QueryPerformanceCounter(a)

#define ExAllocatePoolWithTag(a,b,c) new BYTE[b]
#define ExFreePool(a) delete a
#define PDRIVER_OBJECT INT
#define PDRIVER_TRANSPORT INT

#define WDFWMIINSTANCE INT


#define WdfRequestSetInformation(a,b)
#define DRIVER_DISPATCH INT

#define PsGetCurrentProcess(a) NULL
#define PsTerminateSystemThread(a)
#define ObDereferenceObject(a)
#define PsGetCurrentProcessId(a) 0
#define WdfRequestMarkCancelable(a,b)
#define WdfRequestUnmarkCancelable(a) 0
//#define KDEFERRED_ROUTINE VOID*

//devioctl.h
#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

#define RTL_AVL_COMPARE_ROUTINE INT
#define RTL_AVL_FREE_ROUTINE INT
#define RTL_AVL_ALLOCATE_ROUTINE INT
#define DRIVER_INITIALIZE INT
#define SHA1_LENGTH 20
#define Executive 1
#define KernelMode 1
#define PUNICODE_STRING INT
#define PWDFDEVICE_INIT INT
#define WDF_PNPPOWER_EVENT_CALLBACKS INT
#define DECLARE_CONST_UNICODE_STRING(a,b)
#define WdfRequestCompleteWithPriorityBoost(a,b,c) STATUS_SUCCESS

//etl mock
#define EventWriteLeaseAgentCreated(a,b,c,d)  \
LeaseTrace::WriteInfo("LeaseAgentCreated", "Lease agent {0}/{1} created in state {2}",b,c,d)
#define EventWriteAllocateFail1(a)  \
LeaseTrace::WriteInfo("AllocateFail1", "Could not allocate channel state")
#define EventWriteAllocateFail2(a)  \
LeaseTrace::WriteInfo("AllocateFail2", "Could not allocate monitor failed accepted list")
#define EventWriteAllocateFail3(a)  \
LeaseTrace::WriteInfo("AllocateFail3", "Could not allocate leasing application event")
#define EventWriteAllocateFail4(a)  \
LeaseTrace::WriteInfo("AllocateFail4", "Could not allocate leasing application context")
#define EventWriteAllocateFail5(a)  \
LeaseTrace::WriteInfo("AllocateFail5", "Could not allocate leasing application identifier")
#define EventWriteAllocateFail6(a)  \
LeaseTrace::WriteInfo("AllocateFail6", "Could not allocate lease context")
#define EventWriteAllocateFail7(a)  \
LeaseTrace::WriteInfo("AllocateFail7", "Could not allocate lease relationship identifier")
#define EventWriteAllocateFail8(a)  \
LeaseTrace::WriteInfo("AllocateFail8", "Could not allocate lease relationship identifier (remote)")
#define EventWriteAllocateFail9(a)  \
LeaseTrace::WriteInfo("AllocateFail9", "Could not allocate lease request message")
#define EventWriteAllocateFail10(a)  \
LeaseTrace::WriteInfo("AllocateFail10", "Could not allocate send Irp")
#define EventWriteAllocateFail11(a)  \
LeaseTrace::WriteInfo("AllocateFail11", "Could not allocate receive Irp")
#define EventWriteAllocateFail12(a)  \
LeaseTrace::WriteInfo("AllocateFail12", "Could not allocate channel")
#define EventWriteAllocateFail13(a)  \
LeaseTrace::WriteInfo("AllocateFail13", "Could not allocate listen Irp")
#define EventWriteAllocateFail14(a)  \
LeaseTrace::WriteInfo("AllocateFail14", "Could not allocate connect Irp")
#define EventWriteAllocateFail15(a)  \
LeaseTrace::WriteInfo("AllocateFail15", "Could not allocate socket")
#define EventWriteAllocateFail16(a)  \
LeaseTrace::WriteInfo("AllocateFail16", "Could not allocate close Irp")
#define EventWriteAllocateFail17(a)  \
LeaseTrace::WriteInfo("AllocateFail17", "Could not allocate lease agent")
#define EventWriteLeaseAgentInStateForRemote(a,b,c,d)  \
LeaseTrace::WriteInfo("LeaseAgentInStateForRemote", "Lease agent {0}/{1} is now in state {2}",b,c,d)
#define EventWriteLeaseAgentDelete(a,b,c)  \
LeaseTrace::WriteInfo("LeaseAgentDelete", "Lease agent {0}/{1} is being deleted",b,c)
#define EventWriteRemoteLeaseAgentCreated(a,b,c,d)  \
LeaseTrace::WriteInfo("RemoteLeaseAgentCreated", "Remote lease agent {0}/{1} created in state {2}",b,c,d)
#define EventWriteTerminateSubLeaseMonitorFailed(a,b,c,d,e)  \
LeaseTrace::WriteInfo("TerminateSubLeaseMonitorFailed", "Terminating subject lease relationship {0}/{1} on remote lease agent {2}/{3} reason monitor Failed",b,c,d,e)
#define EventWriteSubLeaseFailureAcked(a,b,c,d,e)  \
LeaseTrace::WriteInfo("SubLeaseFailureAcked", "Subject lease relationship {0}/{1} failure acked on remote lease agent {2}/{3}",b,c,d,e)
#define EventWriteSubLeaseTerminationAcked(a,b,c,d,e)  \
LeaseTrace::WriteInfo("SubLeaseTerminationAcked", "Subject lease relationship {0}/{1} termination acked on remote lease agent {2}/{3}",b,c,d,e)
#define EventWriteEstabMonitorLease(a,b,c,d,e)  \
LeaseTrace::WriteInfo("EstabMonitorLease", "Establishing monitor lease relationship {0}/{1} on remote lease agent {2}/{3}",b,c,d,e)
#define EventWriteLeaseRelationRcvMsg(a,b,c,d,e,f,g,h,i,j)  \
LeaseTrace::WriteInfo("LeaseRelationRcvMsg", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) received lease {7} message {8}",b,c,d,e,f,g,h,i,j)
#define EventWriteMonitorLeaseFailureAcked(a,b,c,d,e)  \
LeaseTrace::WriteInfo("MonitorLeaseFailureAcked", "Monitor lease relationship {0}/{1} failure acked on remote lease agent {2}/{3}",b,c,d,e)
#define EventWriteLeaseRelationRcvTerminationMsg(a,b,c,d,e,f,g,h,i,j)  \
LeaseTrace::WriteInfo("LeaseRelationRcvTerminationMsg", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) received lease termination {7} message {8}",b,c,d,e,f,g,h,i,j)
#define EventWriteRemoteLeaseAgentChangeInstance(a,b,c,d,e)  \
LeaseTrace::WriteInfo("RemoteLeaseAgentChangeInstance", "Remote lease agent {0}/{1} changed the instance of lease agent remote {2}-&gt;{3}",b,c,d,e)
#define EventWriteLeaseRelationSendingTermination(a,b,c,d,e,f,g,h,i)  \
LeaseTrace::WriteInfo("LeaseRelationSendingTermination", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) is sending lease termination {6} message {7}",b,c,d,e,f,g,h,i)
#define EventWriteLeaseRelationSendingLease(a,b,c,d,e,f,g,h,i)  \
LeaseTrace::WriteInfo("LeaseRelationSendingLease", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) is sending lease {6} message {7}",b,c,d,e,f,g,h,i)
#define EventWriteLeaseRelationSendingLeaseRenewal(a,b,c,d,e,f,g,h,i)  \
LeaseTrace::WriteInfo("LeaseRelationSendingLeaseRenewal", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) is sending lease {6} renewal message {7}",b,c,d,e,f,g,h,i)
#define EventWriteLeaseRelationInStateAfterLeaseMsg(a,b,c,d,e,f,g,h,i,j)  \
LeaseTrace::WriteInfo("LeaseRelationInStateAfterLeaseMsg", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) after processing lease {7} message {8}",b,c,d,e,f,g,h,i,j)
#define EventWriteLeaseRelationInStateAfterLeaseTermMsg(a,b,c,d,e,f,g,h,i,j)  \
LeaseTrace::WriteInfo("LeaseRelationInStateAfterLeaseTermMsg", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) after processing lease termination {7} message {8}",b,c,d,e,f,g,h,i,j)
#define EventWriteDropingLeaseMsgLAFailed(a,b,c,d)  \
LeaseTrace::WriteInfo("DropingLeaseMsgLAFailed", "Dropping lease {0} message reason lease agent {1}/{2} already failed",b,c,d)
#define EventWriteIncomingChnlWithRemoteLA(a,b,c,d)  \
LeaseTrace::WriteInfo("IncomingChnlWithRemoteLA", "Incoming channel {0} associated with remote lease agent {1}/{2}",b,c,d)
#define EventWriteRegisterRemoteLAFail(a,b,c,d,e)  \
LeaseTrace::WriteInfo("RegisterRemoteLAFail", "Could not register remote lease agent {0}/{1} with lease agent {2}/{3}",b,c,d,e)
#define EventWriteRemoteLAExist(a,b,c,d,e)  \
LeaseTrace::WriteInfo("RemoteLAExist", "Remote lease agent {0}/{1} already exists on lease agent {2}/{3}",b,c,d,e)
#define EventWriteRegisterRemoteLA(a,b,c,d,e)  \
LeaseTrace::WriteInfo("RegisterRemoteLA", "Remote lease agent {0}/{1} registered with lease agent {2}/{3}",b,c,d,e)
#define EventWriteCreateRemoteLAFail(a)  \
LeaseTrace::WriteInfo("CreateRemoteLAFail", "Could not create remote lease agent")
#define EventWriteTerminateSubLeaseSubTerminated(a,b,c,d,e)  \
LeaseTrace::WriteInfo("TerminateSubLeaseSubTerminated", "Terminating subject lease relationship {0}/{1} on remote lease agent {2}/{3} reason subject Terminated",b,c,d,e)
#define EventWriteTerminateSubLeaseSubFailed(a,b,c,d,e)  \
LeaseTrace::WriteInfo("TerminateSubLeaseSubFailed", "Terminating subject lease relationship {0}/{1} on remote lease agent {2}/{3} reason subject Failed",b,c,d,e)
#define EventWriteCannotFailLALeaseAppRegistered(a,b,c,d)  \
LeaseTrace::WriteInfo("CannotFailLALeaseAppRegistered", "Lease agent {0}/{1} cannot be failed reason leasing application {2} still registered",b,c,d)
#define EventWriteCannotFailLARemoteLAInState(a,b,c,d,e,f)  \
LeaseTrace::WriteInfo("CannotFailLARemoteLAInState", "Lease agent {0}/{1} cannot be failed reason remote lease agent {2}/{3} is in state {4}",b,c,d,e,f)
#define EventWriteCanFailLA(a,b,c)  \
LeaseTrace::WriteInfo("CanFailLA", "Lease agent {0}/{1} can now be failed",b,c)
#define EventWriteCanNotDelLASocNotClosed(a,b,c)  \
LeaseTrace::WriteInfo("CanNotDelLASocNotClosed", "Lease agent {0}/{1} cannot be deleted reason listening socket is not closed",b,c)
#define EventWriteCanNotDelLARemoteLAState(a,b,c,d,e,f)  \
LeaseTrace::WriteInfo("CanNotDelLARemoteLAState", "Lease agent {0}/{1} cannot be deleted reason remote lease agent {2}/{3} is in state {4}",b,c,d,e,f)
#define EventWriteCanDelLA(a,b,c)  \
LeaseTrace::WriteInfo("CanDelLA", "Lease agent {0}/{1} can now be deleted",b,c)
#define EventWriteRemoteLAStateTransition(a,b,c,d,e)  \
LeaseTrace::WriteInfo("RemoteLAStateTransition", "Remote lease agent {0}/{1} state transition {2}-&gt;{3}",b,c,d,e)
#define EventWriteLeaseAppFailed(a,b,c,d)  \
LeaseTrace::WriteInfo("LeaseAppFailed", "Leasing application {0} on lease agent {1}/{2} has now failed/expired",b,c,d)
#define EventWriteLeaseAgentInState(a,b,c,d)  \
LeaseTrace::WriteInfo("LeaseAgentInState", "Lease agent {0}/{1} is already in state {2}",b,c,d)
#define EventWriteLeaseRelationInState1(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("LeaseRelationInState1", "OnSubjectExpired: Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5})",b,c,d,e,f,g)
#define EventWriteLeaseRelationInState2(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("LeaseRelationInState2", "OnMonitorExpired: Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5})",b,c,d,e,f,g)
#define EventWriteLeaseRelationInState3(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("LeaseRelationInState3", "Starting arbitration for: Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5})",b,c,d,e,f,g)
#define EventWriteLeaseRelationInState4(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("LeaseRelationInState4", " no leasing application found to arbitrate : Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5})",b,c,d,e,f,g)
#define EventWriteArbAbandonedLeaseRelationInState4(a,b,c,d,e,f,g,h)  \
LeaseTrace::WriteInfo("ArbAbandonedLeaseRelationInState4", "Arbitration abandoned for lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) reason  no leasing application found to arbitrate  {6}",b,c,d,e,f,g,h)
#define EventWriteLeaseAppChosenLeaseRelationInState(a,b,c,d,e,f,g,h,i,j)  \
LeaseTrace::WriteInfo("LeaseAppChosenLeaseRelationInState", "Leasing application {0} has been chosen to arbitrate Lease relationship ({1}/{2}, subject = {3}, monitor = {4}) in state ({5}-{6}) with Monitor TTL = {7}, Subject TTL = {8}",b,c,d,e,f,g,h,i,j)
#define EventWriteLeaseRelationTransition1(a,b,c,d,e,f,g,h,i)  \
LeaseTrace::WriteInfo("LeaseRelationTransition1", "Subject Expired  Timer: Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) transition ({4}-{5})-&gt;({6}-{7})",b,c,d,e,f,g,h,i)
#define EventWriteLeaseRelationTransition2(a,b,c,d,e,f,g,h,i)  \
LeaseTrace::WriteInfo("LeaseRelationTransition2", "Monitor Expired  Timer: Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) transition ({4}-{5})-&gt;({6}-{7})",b,c,d,e,f,g,h,i)
#define EventWriteLeaseRelationTransition3(a,b,c,d,e,f,g,h,i)  \
LeaseTrace::WriteInfo("LeaseRelationTransition3", "Renew/Arbitrate  Timer: Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) transition ({4}-{5})-&gt;({6}-{7})",b,c,d,e,f,g,h,i)
#define EventWriteLeaseRelationTransition4(a,b,c,d,e,f,g,h,i)  \
LeaseTrace::WriteInfo("LeaseRelationTransition4", "Post Arbitration  Timer: Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) transition ({4}-{5})-&gt;({6}-{7})",b,c,d,e,f,g,h,i)
#define EventWriteLeaseRelationEnqueue1(a,b,c,d,e,f,g,h)  \
LeaseTrace::WriteInfo("LeaseRelationEnqueue1", "{0} Timer: Lease relationship ({1}/{2}, subject = {3}, monitor = {4}) was enqueued, ttl = {5}, count = {6}",b,c,d,e,f,g,h)
#define EventWriteLeaseRelationEnqueue2(a,b,c,d,e,f)  \
LeaseTrace::WriteInfo("LeaseRelationEnqueue2", "{0} Timer: Lease relationship ({1}/{2}, subject = {3}, monitor = {4}) was dequeued",b,c,d,e,f)
#define EventWriteLeaseRelationTerminateFail1(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseRelationTerminateFail1", "Monitor lease relationship {0}/{1} could not be terminated on remote lease agent {2}/{3}",b,c,d,e)
#define EventWriteLeaseRelationTerminateFail2(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseRelationTerminateFail2", "Subject lease relationship {0}/{1} could not be terminated on remote lease agent {2}/{3}",b,c,d,e)
#define EventWriteLeaseRelationRcvArbitrationTTL(a,b,c,d,e,f,g,h,i,j)  \
LeaseTrace::WriteInfo("LeaseRelationRcvArbitrationTTL", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) Received arbitration LocalTTL = {6}, RemoteTTL = {7}, IsDelayed = {8}",b,c,d,e,f,g,h,i,j)
#define EventWriteDeviceFailedWith4(a,b)  \
LeaseTrace::WriteInfo("DeviceFailedWith4", "ObReferenceObjectByHandle failed with 0x{0}",b)
#define EventWriteLeaseAgentBindingFailedWith(a,b,c,d)  \
LeaseTrace::WriteInfo("LeaseAgentBindingFailedWith", "Lease agent {0}/{1} binding failed with 0x{2}",b,c,d)
#define EventWriteInvalidReq1(a)  \
LeaseTrace::WriteInfo("InvalidReq1", "Invalid request - unexpected input buffer size")
#define EventWriteInvalidReq2(a)  \
LeaseTrace::WriteInfo("InvalidReq2", "Invalid request - unexpected certificate setting")
#define EventWriteInvalidReq3(a)  \
LeaseTrace::WriteInfo("InvalidReq3", "Invalid request - unexpected lease agent instance")
#define EventWriteInvalidReq4(a)  \
LeaseTrace::WriteInfo("InvalidReq4", "Invalid request - unexpected remote lease agent instance")
#define EventWriteInvalidReq5(a)  \
LeaseTrace::WriteInfo("InvalidReq5", "Invalid request - unexpected output buffer size")
#define EventWriteInvalidReq6(a)  \
LeaseTrace::WriteInfo("InvalidReq6", "Invalid request - unexpected socket address")
#define EventWriteInvalidReq7(a)  \
LeaseTrace::WriteInfo("InvalidReq7", "Invalid request - unexpected lease agent handle")
#define EventWriteInvalidReq8(a)  \
LeaseTrace::WriteInfo("InvalidReq8", "Invalid request - unexpected leasing application identifier")
#define EventWriteInvalidReq9(a)  \
LeaseTrace::WriteInfo("InvalidReq9", "Invalid request - unexpected leasing application handle")
#define EventWriteInvalidReq10(a)  \
LeaseTrace::WriteInfo("InvalidReq10", "Invalid request - unexpected lease duration")
#define EventWriteInvalidReq11(a)  \
LeaseTrace::WriteInfo("InvalidReq11", "Invalid request - unexpected remote leasing application identifier")
#define EventWriteInvalidReq12(a)  \
LeaseTrace::WriteInfo("InvalidReq12", "Invalid request - unexpected time to live")
#define EventWriteRetrieveFail1(a)  \
LeaseTrace::WriteInfo("RetrieveFail1", "Could not retrieve request Input buffer")
#define EventWriteRetrieveFail2(a)  \
LeaseTrace::WriteInfo("RetrieveFail2", "Could not retrieve request Output buffer")
#define EventWriteWorkerThread1(a)  \
LeaseTrace::WriteInfo("WorkerThread1", "Worker thread started")
#define EventWriteWorkerThread2(a)  \
LeaseTrace::WriteInfo("WorkerThread2", "Worker thread is exiting")
#define EventWriteLeaseAppOnLeaseAgent1(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseAppOnLeaseAgent1", "Leasing application {0} could not be created on lease agent {1}/{2}",b,c,d,e)
#define EventWriteLeaseAppOnLeaseAgent2(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseAppOnLeaseAgent2", "Leasing application {0} already exists on lease agent {1}/{2}",b,c,d,e)
#define EventWriteLeaseAppOnLeaseAgent3(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseAppOnLeaseAgent3", "Leasing application {0} created on lease agent {1}/{2}",b,c,d,e)
#define EventWriteLeaseAppOnLeaseAgent4(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseAppOnLeaseAgent4", "Leasing application {0} registered on lease agent {1}/{2}",b,c,d,e)
#define EventWriteLeaseAppOnLeaseAgent5(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseAppOnLeaseAgent5", "Leasing application {0} already registered on lease agent {1}/{2}",b,c,d,e)
#define EventWriteLeaseAppCreatedOnLeaseAgent3(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseAppCreatedOnLeaseAgent3", "Leasing application {0} created on lease agent {1}/{2}",b,c,d,e)
#define EventWriteLeaseAppOnLeaseAgentInState1(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseAppOnLeaseAgentInState1", "Leasing application {0} could not be created on lease agent {1}/{2} in state {3}",b,c,d,e)
#define EventWriteLeaseAppOnLeaseAgentInState2(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseAppOnLeaseAgentInState2", "Leasing application {0} already exists on lease agent {1}/{2} in state {3}",b,c,d,e)
#define EventWriteLeaseAppOnLeaseAgentInState3(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseAppOnLeaseAgentInState3", "Leasing application {0} created on lease agent {1}/{2} in state {3}",b,c,d,e)
#define EventWriteLeaseAppOnLeaseAgentInState4(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseAppOnLeaseAgentInState4", "Leasing application {0} registered on lease agent {1}/{2} in state {3}",b,c,d,e)
#define EventWriteLeaseAppOnLeaseAgentInState5(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseAppOnLeaseAgentInState5", "Leasing application {0} already registered on lease agent {1}/{2} in state {3}",b,c,d,e)
#define EventWriteLeaseAppOnLeaseAgentInState6(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseAppOnLeaseAgentInState6", "Leasing application {0} could not be registered on lease agent {1}/{2} in state {3}",b,c,d,e)
#define EventWriteCleanUpRemoteLeaseAgent(a,b,c)  \
LeaseTrace::WriteInfo("CleanUpRemoteLeaseAgent", "Cleaning up remote lease agent {0}/{1}",b,c)
#define EventWriteLeaseAgentActionStatus1(a,b,c)  \
LeaseTrace::WriteInfo("LeaseAgentActionStatus1", "Lease agent {0}/{1} could not be created",b,c)
#define EventWriteLeaseAgentActionStatus2(a,b,c)  \
LeaseTrace::WriteInfo("LeaseAgentActionStatus2", "Lease agent {0}/{1} already exists",b,c)
#define EventWriteLeaseAgentActionStatus3(a,b,c)  \
LeaseTrace::WriteInfo("LeaseAgentActionStatus3", "Lease agent {0}/{1} created",b,c)
#define EventWriteLeaseAgentActionStatus4(a,b,c)  \
LeaseTrace::WriteInfo("LeaseAgentActionStatus4", "Lease agent {0}/{1} registered",b,c)
#define EventWriteLeaseAgentActionError1(a,b,c)  \
LeaseTrace::WriteInfo("LeaseAgentActionError1", "Lease agent {0}/{1} could not be created",b,c)
#define EventWriteLeaseAgentActionError2(a,b,c)  \
LeaseTrace::WriteInfo("LeaseAgentActionError2", "Lease agent {0}/{1} already exists",b,c)
#define EventWriteLeaseAgentActionError3(a,b,c)  \
LeaseTrace::WriteInfo("LeaseAgentActionError3", "Lease agent {0}/{1} created",b,c)
#define EventWriteLeaseAgentActionError4(a,b,c)  \
LeaseTrace::WriteInfo("LeaseAgentActionError4", "Lease agent {0}/{1} registered",b,c)
#define EventWriteLeaseAgentActionError5(a,b,c)  \
LeaseTrace::WriteInfo("LeaseAgentActionError5", "Lease agent {0}/{1} already registered",b,c)
#define EventWriteLeaseAgentActionError6(a,b,c)  \
LeaseTrace::WriteInfo("LeaseAgentActionError6", "Lease agent {0}/{1} could not be registered",b,c)
#define EventWriteLeaseAgentNowInState(a,b,c,d)  \
LeaseTrace::WriteInfo("LeaseAgentNowInState", "Lease agent {0}/{1} is now in state {2}",b,c,d)
#define EventWriteEstLeaseRelationFail(a,b,c,d,e,f)  \
LeaseTrace::WriteInfo("EstLeaseRelationFail", "Could not establish lease relationship {0}/{1} on remote lease agent {2}/{3} in state {4}",b,c,d,e,f)
#define EventWriteRemoteLeaseAgentConnectionFail(a,b,c,d)  \
LeaseTrace::WriteInfo("RemoteLeaseAgentConnectionFail", "Remote lease agent {0}/{1} connection failed with 0x{2}",b,c,d)
#define EventWriteLeaseRelationExistRemoteLA(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseRelationExistRemoteLA", "Lease relationship identifier {0}/{1} already exists on remote lease agent {2}/{3}",b,c,d,e)
#define EventWriteLeaseRelationExistInReverseLeaseEstablish(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseRelationExistInReverseLeaseEstablish", "Lease relationship identifier {0}/{1} already exists on remote lease agent {2}/{3} when try to establish reverse lease",b,c,d,e)
#define EventWriteReverseLeaseEstablishRLASuspended(a,b,c,d,e)  \
LeaseTrace::WriteInfo("ReverseLeaseEstablishRLASuspended", "Lease relationship identifier {0}/{1} could not establish reverse side lease on remote lease agent {2}/{3}, because remote lease agent is suspended",b,c,d,e)
#define EventWriteRegisterLRIDSubListFail(a,b,c,d,e)  \
LeaseTrace::WriteInfo("RegisterLRIDSubListFail", "Could not register lease relationship identifier {0}/{1} in subject list on remote lease agent {2}/{3}",b,c,d,e)
#define EventWriteRegisterLRIDSubPendingListFail(a,b,c,d,e)  \
LeaseTrace::WriteInfo("RegisterLRIDSubPendingListFail", "Could not register lease relationship identifier {0}/{1} in subject pending list on remote lease agent {2}/{3}",b,c,d,e)
#define EventWriteInvalidIOCTL(a,b)  \
LeaseTrace::WriteInfo("InvalidIOCTL", "Invalid IOCTL code {0}",b)
#define EventWriteTerminateAllLeaseRelation(a,b,c,d)  \
LeaseTrace::WriteInfo("TerminateAllLeaseRelation", "Terminating all lease relationships for leasing application {0} registered with lease agent {1}/{2}",b,c,d)
#define EventWriteTerminateLeaseRelationSkipped(a,b,c,d)  \
LeaseTrace::WriteInfo("TerminateLeaseRelationSkipped", "Terminate lease relationship on remote lease agent {0}/{1} in state {2} was skipped",b,c,d)
#define EventWriteLeaseAppUnregister(a,b,c,d)  \
LeaseTrace::WriteInfo("LeaseAppUnregister", "Leasing application {0} unregistered from lease agent {1}/{2}",b,c,d)
#define EventWriteLeaseAppUnregisterFailed(a,b,c,d)  \
LeaseTrace::WriteInfo("LeaseAppUnregisterFailed", "Could not unregister leasing application {0} from lease agent {1}/{2}",b,c,d)
#define EventWriteLeaseRelationRegisteredRemoteLA(a,b,c,d,e)  \
LeaseTrace::WriteInfo("LeaseRelationRegisteredRemoteLA", "Lease relationship identifier {0}/{1} registered in on remote lease agent {2}/{3}",b,c,d,e)
#define EventWriteEstSubLeaseRelationOnRemote(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("EstSubLeaseRelationOnRemote", "Establishing subject lease relationship {0}/{1} with TTL = {2} on remote lease agent {3}/{4}, lease agent remote instance {5}",b,c,d,e,f,g)
#define EventWriteEstReverseLeaseRelationOnRemote(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("EstReverseLeaseRelationOnRemote", "Establishing reverse side subject lease relationship {0}/{1} with TTL = {2} on remote lease agent {3}/{4}, lease agent remote instance {5}",b,c,d,e,f,g)
#define EventWriteTermReverseLease(a,b,c)  \
LeaseTrace::WriteInfo("TermReverseLease", "Terminating reverse side lease relationship on remote lease agent {0}/{1}",b,c)
#define EventWriteSubLeaseEstablishingOnRemote(a,b,c,d,e,f)  \
LeaseTrace::WriteInfo("SubLeaseEstablishingOnRemote", "Subject lease relationship {0}/{1} is already establishing on remote lease agent {2}{3}, lease agent remote instance {4}",b,c,d,e,f)
#define EventWriteSubLeaseEstablishedOnRemote(a,b,c,d,e,f)  \
LeaseTrace::WriteInfo("SubLeaseEstablishedOnRemote", "Subject lease relationship {0}/{1} is already established on remote lease agent {2}{3}, lease agent remote instance {4}",b,c,d,e,f)
#define EventWriteLeasingAppNotRegisteredInReverseLease(a,b,c,d)  \
LeaseTrace::WriteInfo("LeasingAppNotRegisteredInReverseLease", "Establish reverse lease: Leasing application {0} has not yet registered on lease agent {1}/{2}",b,c,d)
#define EventWriteRaisingQueuingEventLeaseApp1(a,b,c,d)  \
LeaseTrace::WriteInfo("RaisingQueuingEventLeaseApp1", "Raising event {0} for leasing application {1}",b,c,d)
#define EventWriteRaisingQueuingEventLeaseApp2(a,b,c,d)  \
LeaseTrace::WriteInfo("RaisingQueuingEventLeaseApp2", "Queuing event {0} for leasing application {1}",b,c,d)
#define EventWriteRaisingQueuingEventLeaseAppFailed(a,b,c)  \
LeaseTrace::WriteInfo("RaisingQueuingEventLeaseAppFailed", "Queuing event {0} for leasing application {1} failed, the event will be lost",b,c)
#define EventWriteRaisingQueuingEventLeaseAppRemote1(a,b,c,d)  \
LeaseTrace::WriteInfo("RaisingQueuingEventLeaseAppRemote1", "Raising event {0} for leasing application {1} about remote leasing application {2}",b,c,d)
#define EventWriteRaisingQueuingEventLeaseAppRemote2(a,b,c,d)  \
LeaseTrace::WriteInfo("RaisingQueuingEventLeaseAppRemote2", "Queuing event {0} for leasing application {1} about remote leasing application {2}",b,c,d)
#define EventWriteRaisingQueuingEventLeaseAppRemote3(a,b,c,d)  \
LeaseTrace::WriteInfo("RaisingQueuingEventLeaseAppRemote3", "Raising queued event {0} for leasing application {1} about remote leasing application {2}",b,c,d)
#define EventWriteTransportListenStatus(a,b,c,d)  \
LeaseTrace::WriteInfo("TransportListenStatus", "Transport listen status {0} on {1}/{2}",b,c,d)
#define EventWriteTransportCreateFailed(a,b)  \
LeaseTrace::WriteInfo("TransportCreateFailed", "Transport create failed with {0}",b)
#define EventWriteInvalidMessage(a,b)  \
LeaseTrace::WriteInfo("InvalidMessage", "Invalid message, portion {0}",b)
#define EventWriteInvalidMessageHeader(a,b)  \
LeaseTrace::WriteInfo("InvalidMessageHeader", "Invalid message header with wrong {0} ",b)
#define EventWriteDeserializationError(a,b,c)  \
LeaseTrace::WriteInfo("DeserializationError", "Error deserializing message {0} id {1}",b,c)
#define EventWriteSetSubjectExpiration(a,b,c,d,e)  \
LeaseTrace::WriteInfo("SetSubjectExpiration", "Subject {0} in state {1} advancing expiration by {2}ms to {3}",b,c,d,e)
#define EventWriteClearSubjectExpiration(a,b,c)  \
LeaseTrace::WriteInfo("ClearSubjectExpiration", "Subject {0} in state {1} clearing expiration",b,c)
#define EventWriteSubjectExpired(a,b,c,d,e,f,g,h,i)  \
LeaseTrace::WriteInfo("SubjectExpired", "Subject Expired: {0}/{1} subject = {2}, monitor = {3} in state ({4}-{5}) Current time = {6} Expiration = {7}",b,c,d,e,f,g,h,i)
#define EventWriteMonitorExpired(a,b,c,d,e,f,g,h,i)  \
LeaseTrace::WriteInfo("MonitorExpired", "Monitor Expired: {0}/{1} subject = {2}, monitor = {3} in state ({4}-{5}) Current time = {6} Expiration = {7}",b,c,d,e,f,g,h,i)
#define EventWriteLeaseAgentBlocked(a,b,c)  \
LeaseTrace::WriteInfo("LeaseAgentBlocked", "Lease agent blocked {0}/{1}",b,c)
#define EventWritePerformMaintenance(a)  \
LeaseTrace::WriteInfo("PerformMaintenance", "Worker thread is performing maintenance")
#define EventWriteLeaseAgentCleanup(a,b,c)  \
LeaseTrace::WriteInfo("LeaseAgentCleanup", "Cleaning up lease agent {0}/{1}",b,c)
#define EventWriteCleanupApplication(a,b,c,d)  \
LeaseTrace::WriteInfo("CleanupApplication", "Cleanup application for process PID={0} LeaseAgent={1} LeasingApplication={2}",b,c,d)
#define EventWriteProcessCleanup(a,b)  \
LeaseTrace::WriteInfo("ProcessCleanup", "Cleaning up process PID={0}",b)
#define EventWriteUnregisterTimerEnqueue(a,b,c,d,e)  \
LeaseTrace::WriteInfo("UnregisterTimerEnqueue", "Enqueue unregister timer for leasing application {0} with TTL milli seconds {1}, timer already in queue ? &lt;{2}&gt;,  TTL ticks {3}",b,c,d,e)
#define EventWriteCancelUnregisterTimer(a,b)  \
LeaseTrace::WriteInfo("CancelUnregisterTimer", "Unregister timer is cancelled for leasing application {0} .",b)
#define EventWriteUnregisterTimerDPC(a,b,c)  \
LeaseTrace::WriteInfo("UnregisterTimerDPC", "Unregister timer is fired to clean leasing application {0}, status {1} .",b,c)
#define EventWriteInsertUnregisterListFail(a,b,c)  \
LeaseTrace::WriteInfo("InsertUnregisterListFail", "Fail to insert leasing application {0} to the unregister list",b,c)
#define EventWriteLeasingAppIsBeingUnregistered(a,b,c)  \
LeaseTrace::WriteInfo("LeasingAppIsBeingUnregistered", "Leasing application {0} is being unregistered, timer won&apos;t be reset",b,c)
#define EventWriteGetLeasingAPPTTL(a,b,c,d,e,f)  \
LeaseTrace::WriteInfo("GetLeasingAPPTTL", "Get Expiration Timer for leasing application {0} with TTL {1} milliseconds, Kernel current time {3}, Request TTL {4}",b,c,d,e,f)
#define EventWriteCanNotDelLALeasingAppIsBeingUnregistered(a,b,c)  \
LeaseTrace::WriteInfo("CanNotDelLALeasingAppIsBeingUnregistered", "Cannot delete lease agent because Leasing application {0} is being unregistered",b,c)
#define EventWriteDriverUnloading(a,b)  \
LeaseTrace::WriteInfo("DriverUnloading", "Driver unloading, driver context = {0}",b)
#define EventWriteMaintenanceThreadStopped(a)  \
LeaseTrace::WriteInfo("MaintenanceThreadStopped", "Maintenance thread stopped")
#define EventWriteDeallocateLeaseAgents(a)  \
LeaseTrace::WriteInfo("DeallocateLeaseAgents", "Deallocate lease agents")
#define EventWriteUnloadComplete(a)  \
LeaseTrace::WriteInfo("UnloadComplete", "Driver unload complete")
#define EventWriteRemoteLeaseAgentInstanceNewer(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("RemoteLeaseAgentInstanceNewer", "Remote lease agent {0}/{1}, remote instance={2} is newer than {3}/{4}, remote instance={5}",b,c,d,e,f,g)
#define EventWriteProcessStaleRequest(a,b,c,d,e,f,g,h,i,j)  \
LeaseTrace::WriteInfo("ProcessStaleRequest", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) received stale request with message ID {6} and lease instance {7}, monitor ID is {8}",b,c,d,e,f,g,h,i,j)
#define EventWriteProcessStaleResponse(a,b,c,d,e,f,g,h,i,j)  \
LeaseTrace::WriteInfo("ProcessStaleResponse", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) received stale response with message ID {6} and lease instance {7}, subject ID is {8}",b,c,d,e,f,g,h,i,j)
#define EventWriteDisconnectFailedRemoteLA(a,b,c,d,e,f,g,h)  \
LeaseTrace::WriteInfo("DisconnectFailedRemoteLA", "Failed remote lease agent ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) is disconnected, lease agent remote instance {6}",b,c,d,e,f,g,h)
#define EventWriteFindActiveRLAInTable(a,b,c,d,e,f,g,h,i)  \
LeaseTrace::WriteInfo("FindActiveRLAInTable", "Active RLA:({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) is found in the table, lease agent remote instance {6}, in state {7}",b,c,d,e,f,g,h,i)
#define EventWriteInitiateTwoWayTermination(a,b,c)  \
LeaseTrace::WriteInfo("InitiateTwoWayTermination", "Remote lease agent {0}/{1} initiate two way lease termination; reason: there is no subject/monitor lease relationship left",b,c)
#define EventWriteReceivedTwoWayTermination(a,b,c)  \
LeaseTrace::WriteInfo("ReceivedTwoWayTermination", "Remote lease agent {0}/{1} received two way lease termination;",b,c)
#define EventWriteKernelUserVersionMismatch(a,b,c)  \
LeaseTrace::WriteInfo("KernelUserVersionMismatch", "User mode (V: {0}) and Kernel model (V: {1}) version number do not match.",b,c)
#define EventWriteRLAIgnoreRenewRequestInTwoWayTermination(a,b,c,d)  \
LeaseTrace::WriteInfo("RLAIgnoreRenewRequestInTwoWayTermination", "Remote lease agent {0}/{1} ignore renew request {2} because it is in two way termination",b,c,d)
#define EventWriteLeaseRelationshipCreateTrace(a,b,c,d)  \
LeaseTrace::WriteInfo("LeaseRelationshipCreateTrace", "Lease relationship {0}/{1} is created with address {2}",b,c,d)
#define EventWriteSubExpiredTimerInWrongState(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("SubExpiredTimerInWrongState", "Subject Expired Timer: Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}), subject should be in Active due to reverse lease establish.",b,c,d,e,f,g)
#define EventWriteSendingTerminationSkippedInPing(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("SendingTerminationSkippedInPing", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) skip sending termination message since still in ping process",b,c,d,e,f,g)
#define EventWriteRemainingLeaseTimeoutLessThanConfigExpiryTimeout(a,b,c,d,e)  \
LeaseTrace::WriteInfo("RemainingLeaseTimeoutLessThanConfigExpiryTimeout", "Leasing application {0} return remaining lease TTL {1} since it is less than the config expiry {2}; Request TTL is {3}",b,c,d,e)
#define EventWritePingProcessAlreadyStarted(a,b,c,d,e)  \
LeaseTrace::WriteInfo("PingProcessAlreadyStarted", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) already started the ping process",b,c,d,e)
#define EventWriteSendingTerminationSkippedSubjectInactive(a,b,c,d,e,f,g,h)  \
LeaseTrace::WriteInfo("SendingTerminationSkippedSubjectInactive", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) skip sending termination message since the subject is Inactive; is in ping? &lt;{6}&gt;",b,c,d,e,f,g,h)
#define EventWriteProcessStalePingResponse(a,b,c,d,e,f,g,h)  \
LeaseTrace::WriteInfo("ProcessStalePingResponse", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) received stale ping response with message ID {6}",b,c,d,e,f,g,h)
#define EventWriteUpdateSecuritySettingsFailed(a,b,c,d)  \
LeaseTrace::WriteInfo("UpdateSecuritySettingsFailed", "Failed to update security settings for leasing application {0} registered with lease agent {1}/{2}",b,c,d)
#define EventWriteTransportSetCredentialsFailed(a,b,c,d)  \
LeaseTrace::WriteInfo("TransportSetCredentialsFailed", "Failed to set the credentials for leasing application {0} registered with lease agent {1}/{2}",b,c,d)
#define EventWriteTransportSetCredentialsSucceeded(a,b,c,d)  \
LeaseTrace::WriteInfo("TransportSetCredentialsSucceeded", "Succeeded to set the credentials for leasing application {0} registered with lease agent {1}/{2}",b,c,d)
#define EventWriteLeaseSecuritySettingMatch(a,b,c,d)  \
LeaseTrace::WriteInfo("LeaseSecuritySettingMatch", "Same security is found when update certificte for leasing application {0} registered with lease agent {1}/{2}",b,c,d)
#define EventWriteRemoteCertVerifyReq(a,b,c,d)  \
LeaseTrace::WriteInfo("RemoteCertVerifyReq", "Create the remote cert verify request for leasing app {0}, cbCert {1}, operation handle {2}",b,c,d)
#define EventWriteRemoteCertVerifyReqFailNoLeasingApp(a,b,c,d)  \
LeaseTrace::WriteInfo("RemoteCertVerifyReqFailNoLeasingApp", "Fail to create the remote cert verify request for Lease agent {0}/{1}, no leasing app left, cbCert {2}.",b,c,d)
#define EventWriteFailedRemoteCertVerifyResult(a,b,c,d)  \
LeaseTrace::WriteInfo("FailedRemoteCertVerifyResult", "Process remote cert verify result from user mode failed with error {0} -- operation handle {1}, verify result {2}.",b,c,d)
#define EventWriteFailedVerifyOperationTableLookup(a,b,c)  \
LeaseTrace::WriteInfo("FailedVerifyOperationTableLookup", "Could not find cert verify operation handle {0} in the table, verify result {1}.",b,c)
#define EventWriteAllocateFailLeaseRemoteCert(a)  \
LeaseTrace::WriteInfo("AllocateFailLeaseRemoteCert", "Could not allocate lease remote certificate")
#define EventWriteFailedTrustedCertListInsertion(a,b,c)  \
LeaseTrace::WriteInfo("FailedTrustedCertListInsertion", "Could not insert certificate to the trusted list: operation handle {0}, verify result {1}.",b,c)
#define EventWriteCertExistInTrustedList(a,b,c)  \
LeaseTrace::WriteInfo("CertExistInTrustedList", "Certificate is already in the trusted list: operation handle {0}, verify result {1}.",b,c)
#define EventWriteInsertCertInTrustedList(a,b,c)  \
LeaseTrace::WriteInfo("InsertCertInTrustedList", "Successfully insert Certificate in the trusted list: operation handle {0}, verify result {1}.",b,c)
#define EventWriteFoundCertInTrustedList(a,b,c)  \
LeaseTrace::WriteInfo("FoundCertInTrustedList", "Found Certificate in the trusted list: operation handle {0}, verify result {1}.",b,c)
#define EventWriteInsertCertInPendingList(a,b,c)  \
LeaseTrace::WriteInfo("InsertCertInPendingList", "Successfully insert Certificate in the pending list: leasing app {0}, operation handle {1}",b,c)
#define EventWriteInsertCertInPendingListAlreadyExistent(a,b,c)  \
LeaseTrace::WriteInfo("InsertCertInPendingListAlreadyExistent", "certificate operation already exists when insert Certificate in the pending list: leasing app {0}, operation handle {1}",b,c)
#define EventWriteLeaseSecuritySettingMismatch(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("LeaseSecuritySettingMismatch", "Security setting mismatch when try to reuse the kernel lease agent; Existing Lease Agent {0}/{1} security type {2}; Newly created Lease Agent {3}/{4} security type {5};",b,c,d,e,f,g)
#define EventWriteLeaseDriverTextTraceInfo(a,b)  \
LeaseTrace::WriteInfo("LeaseDriverTextTraceInfo", "{0}",b)
#define EventWriteLeaseDriverTextTraceError(a,b,c)  \
LeaseTrace::WriteInfo("LeaseDriverTextTraceError", "{0} due to {1}",b,c)
#define EventWriteLeaseRenewRetry(a,b,c,d,e,f,g,h,i,j)  \
LeaseTrace::WriteInfo("LeaseRenewRetry", "Lease renew is being retried: lease agent {0}/{1}; Remote LA: {2}/{3} subject = {4}, monitor = {5} in state ({6}-{7}); Is termination? &lt;{8}&gt;",b,c,d,e,f,g,h,i,j)
#define EventWriteBlockingConnection(a,b,c,d,e,f,g,h,i,j)  \
LeaseTrace::WriteInfo("BlockingConnection", "Block lease connection for KLA {0}/{1} to RLA: {2}/{3}; Lease status: S = {4}, M = {5} in state ({6}-{7}); is blocking? &lt;{8}&gt;",b,c,d,e,f,g,h,i,j)
#define EventWriteSendForwardMsgForSameRLASkipped(a,b,c,d)  \
LeaseTrace::WriteInfo("SendForwardMsgForSameRLASkipped", "Send forward request skipped: RLA {0}/{1} in state {2}",b,c,d)
#define EventWriteFindRLARelayForwardMsg(a,b,c,d)  \
LeaseTrace::WriteInfo("FindRLARelayForwardMsg", "GetCurrentActiveRLA found RLA {0}/{1} in state {2} to relay the forward message",b,c,d)
#define EventWriteSerializeAndSendRelayMsg(a,b,c,d,e)  \
LeaseTrace::WriteInfo("SerializeAndSendRelayMsg", "RLA {0}/{1} process {2} message with ID {3}",b,c,d,e)
#define EventWriteProcessForwardMsgNoExistingRLA(a,b,c,d,e)  \
LeaseTrace::WriteInfo("ProcessForwardMsgNoExistingRLA", "Lease agent {0}/{1} do not find existing RLA to relay when process {2} message with ID {3}",b,c,d,e)
#define EventWriteDeserializeForwardMessageError(a,b,c)  \
LeaseTrace::WriteInfo("DeserializeForwardMessageError", "DeserializeForwardMessage {0} address failed because of {1}",b,c)
#define EventWriteCheckLeaseDurationList(a,b,c)  \
LeaseTrace::WriteInfo("CheckLeaseDurationList", "Lease duration {0} : {1}",b,c)
#define EventWriteMonitorGrantGreaterLeaseDuration(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("MonitorGrantGreaterLeaseDuration", "Grant greater local config durations: {0}/{1}, incoming lease duration: {2}; local config: {3}; local config establish index: {4}; incoming expiration: {5}",b,c,d,e,f,g)
#define EventWriteUpdateLeaseAgentDuration(a,b,c,d)  \
LeaseTrace::WriteInfo("UpdateLeaseAgentDuration", "Updating LeaseAgentContext {0} duration. Current duration: {1}; Updated duration: {2}",b,c,d)
#define EventWriteUpdateRemoteLeaseAgentDurationFlag(a,b,c,d,e)  \
LeaseTrace::WriteInfo("UpdateRemoteLeaseAgentDurationFlag", "Updating RemoteLeaseAgentContext IsUpdate flag. {0}/{1} current lease duration type {2}; current lease relationship duration in use: {3}",b,c,d,e)
#define EventWriteUpdateRemoteLeaseDuration(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("UpdateRemoteLeaseDuration", "Incoming requested durations are different from lease relationship records: {0}/{1}, Incoming: ({2}); Records: ({3}); Local config: ({4}); Lease relation duration: {5}",b,c,d,e,f,g)
#define EventWriteSubjectRequestedLeaseDuration(a,b,c,d,e,f,g,h)  \
LeaseTrace::WriteInfo("SubjectRequestedLeaseDuration", "Get requested duration is different from exising lease relationship: {0}/{1}. Duration would be returned: {2}; Local config: {3} Remote duration: {4}; Existing lease relationship: {5}; Update existing lease? &lt;{6}&gt;",b,c,d,e,f,g,h)
#define EventWriteSubjectRequestedLeaseExpiration(a,b,c,d,e,f,g,h)  \
LeaseTrace::WriteInfo("SubjectRequestedLeaseExpiration", "Get requested expiration is less than subject expire time: {0}/{1}. Requested duration: {2}; Now: {3}; Subject expire time: {4}; New expiration: {5}; Duration update flag? &lt;{6}&gt;",b,c,d,e,f,g,h)
#define EventWriteReceivedStaleMessage(a,b,c,d,e)  \
LeaseTrace::WriteInfo("ReceivedStaleMessage", "Lease agent {0}/{1} received stale message {2}; Message lease agent instance {3}",b,c,d,e)
#define EventWriteGetExpireTimeWithRequestTTL(a,b,c,d)  \
LeaseTrace::WriteInfo("GetExpireTimeWithRequestTTL", "Leasing application {0} has actual TTL {1}, while request TTL is {2}",b,c,d)
#define EventWriteGetExpireTimeForFailureReport(a,b,c,d)  \
LeaseTrace::WriteInfo("GetExpireTimeForFailureReport", "Leasing application {0} has actual TTL {1} calculated from last grant time; request TTL is {2}",b,c,d)
#define EventWriteAssertCrashLeasingApplication(a)  \
LeaseTrace::WriteInfo("AssertCrashLeasingApplication", "Crashing leasing application due to application assert.")
#define EventWriteProcessExit(a)  \
LeaseTrace::WriteInfo("ProcessExit", "Process exited normally or abnormally.")
#define EventWriteUnresponsiveDurationCheck(a,b,c,d,e,f)  \
LeaseTrace::WriteInfo("UnresponsiveDurationCheck", "Active Remote Lease agent {0}/{1} check for last ioctl update. Time passed since last update: {2}, last update ioctl code: {3}; Unresponsive duration: {4}",b,c,d,e,f)
#define EventWriteHeartbeatTooLong(a,b)  \
LeaseTrace::WriteInfo("HeartbeatTooLong", "Heartbeat takes too long. {0} milliseconds passed since last update.",b)
#define EventWriteHeartbeatFailed(a,b)  \
LeaseTrace::WriteInfo("HeartbeatFailed", "Heartbeat failed, error code: {0}",b)
#define EventWriteHeartbeatSucceeded(a)  \
LeaseTrace::WriteInfo("HeartbeatSucceeded", "Heartbeat succeeded.")
#define EventWriteDelayTimerEnqueue(a,b,c,d,e,f)  \
LeaseTrace::WriteInfo("DelayTimerEnqueue", "Enqueue delay timer for lease agent {0}/{1} with TTL milli seconds {2}, timer already in queue ? &lt;{3}&gt;, TTL ticks {4}",b,c,d,e,f)
#define EventWriteDelayTimer(a,b,c,d)  \
LeaseTrace::WriteInfo("DelayTimer", "Delay timer is {0} for lease agent {1}/{2} .",b,c,d)
#define EventWriteRemoteLeaseAgentVersion(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("RemoteLeaseAgentVersion", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}), local lease agent version {4}, remote lease agent version {5}.",b,c,d,e,f,g)
#define EventWriteAddLeaseBehavior(a,b,c,d,e,f,g,h,i)  \
LeaseTrace::WriteInfo("AddLeaseBehavior", "Add lease behavior (fromAny {0}) {1}:{2} -> (toAny {3}) {4}:{5} type {6} alias {7}.",b,c,d,e,f,g,h,i)
#define EventWriteRemoveLeaseBehavior(a,b)  \
LeaseTrace::WriteInfo("RemoveLeaseBehavior", "Remove lease behavior alias {0}.",b)
#define EventWriteTransportMessageBlocked(a,b,c,d,e,f)  \
LeaseTrace::WriteInfo("TransportMessageBlocked", "Transport message blocked {0}:{1} -> {2}:{3} type {4}.",b,c,d,e,f)
// TODO hit max parameters, workaround by combine the strings
#define EventWriteRemoteLeaseExpirationTime(a,b,c,d,e,f,g,h,i)  \
LeaseTrace::WriteInfo("RemoteLeaseExpirationTime", "Get remote lease expiration time, remote lease agent ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}), remote leasing application id {5}, subject ttl: {6}, monitor ttl: {7}.",b,c,d,e,f,g,h,i)
#define EventWriteRemoteLeaseExpirationTimeNotFound(a,b,c,d)  \
LeaseTrace::WriteInfo("RemoteLeaseExpirationTimeNotFound", "Get remote lease expiration time not found, remote leasing application id {0}, subject ttl: {1}, monitor ttl {2}.",b,c,d)
#define EventWriteArbitrationResultTimeout(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("ArbitrationResultTimeout", "Arbitration result timeout, failing lease, remote lease agent {0}/{1}, subject = {2}, monitor = {3} in state ({4}-{5})",b,c,d,e,f,g)
#define EventWriteLeaseAgentRemoteInstanceGreater(a,b,c,d,e,f,g,h,i)  \
LeaseTrace::WriteInfo("LeaseAgentRemoteInstanceGreater", "Remote lease agent instance is greater, remote lease agent {0}/{1}, subject = {2}, monitor = {3} in state ({4}-{5}), lease agent remote instance {6} -> {7}",b,c,d,e,f,g,h,i)
#define EventWriteRemoteLeaseAgentSuspendedDropMessage(a,b,c,d,e,f,g,h)  \
LeaseTrace::WriteInfo("RemoteLeaseAgentSuspendedDropMessage", "Remote lease agent suspended drop message, remote lease agent {0}/{1}, subject = {2}, monitor = {3} in state ({4}-{5}), drop message {6}",b,c,d,e,f,g,h)
#define EventWriteApiStart(a,b)  \
LeaseTrace::WriteInfo("ApiStart", "Lease api start {0}",b)
#define EventWriteApiEnd(a,b)  \
LeaseTrace::WriteInfo("ApiEnd", "Lease api end {0}",b)
#define EventWriteRemoteLeaseExpirationTimeFound(a,b,c,d,e,f,g,h,i,j,k,l)  \
LeaseTrace::WriteInfo("RemoteLeaseExpirationTimeFound", "Get remote lease expiration time, lease agent {0}/{1}, lease relationship ({2}/{3}, subject = {4}, monitor = {5}) in state ({6}-{7}), remote leasing application id {8}, subject ttl: {9}, monitor ttl {10}, found.",b,c,d,e,f,g,h,i,j,k,l)
#define EventWriteRemoteLeaseExpirationTimeNotFound(a,b,c,d,e,f,g,h,i,j,k,l)  \
LeaseTrace::WriteInfo("RemoteLeaseExpirationTimeNotFound", "Get remote lease expiration time, lease agent {0}/{1}, lease relationship ({2}/{3}, subject = {4}, monitor = {5}) in state ({6}-{7}), remote leasing application id {8}, subject ttl: {9}, monitor ttl {10}, not found.",b,c,d,e,f,g,h,i,j,k,l)
#define EventWriteRemoteLeaseExpirationTimeFinal(a,b,c,d,e,f,g,h,i,j,k,l)  \
LeaseTrace::WriteInfo("RemoteLeaseExpirationTimeFinal", "Get remote lease expiration time, lease agent {0}/{1}, lease relationship ({2}/{3}, subject = {4}, monitor = {5}) in state ({6}-{7}), remote leasing application id {8}, subject ttl: {9}, monitor ttl {10}, final.",b,c,d,e,f,g,h,i,j,k,l)
#define EventWriteRemoteLeaseAgentSetNotDestruct(a,b,c,d,e,f,g,h,i,j,k)  \
LeaseTrace::WriteInfo("RemoteLeaseAgentSetNotDestruct", "Remote lease agent ({0}/{1}, subject = {2}, monitor = {3}), state = {4} ({5}-{6}), IsActive = {7}, TimeToBeFailed = {8}, IsInArbitrationNeutral = {9}, set not destructing.",b,c,d,e,f,g,h,i,j,k)
#define EventWriteRemoteLeaseAgentSetDestructSearch(a,b,c,d,e,f,g,h,i,j,k)  \
LeaseTrace::WriteInfo("RemoteLeaseAgentSetDestructSearch", "Remote lease agent ({0}/{1}, subject = {2}, monitor = {3}), state = {4} ({5}-{6}), IsActive = {7}, TimeToBeFailed = {8}, IsInArbitrationNeutral = {9}, set destructing search.",b,c,d,e,f,g,h,i,j,k)
#define EventWriteRemoteLeaseAgentSetDestructFound(a,b,c,d,e,f,g,h,i,j,k)  \
LeaseTrace::WriteInfo("RemoteLeaseAgentSetDestructFound", "Remote lease agent ({0}/{1}, subject = {2}, monitor = {3}), state = {4} ({5}-{6}), IsActive = {7}, TimeToBeFailed = {8}, IsInArbitrationNeutral = {9}, set destructing found.",b,c,d,e,f,g,h,i,j,k)
#define EventWriteRemoteLeaseAgentSetDestructNotFound(a,b,c,d,e,f,g,h,i,j,k)  \
LeaseTrace::WriteInfo("RemoteLeaseAgentSetDestructNotFound", "Remote lease agent ({0}/{1}, subject = {2}, monitor = {3}), state = {4} ({5}-{6}), IsActive = {7}, TimeToBeFailed = {8}, IsInArbitrationNeutral = {9}, set destructing not found.",b,c,d,e,f,g,h,i,j,k)
#define EventWriteCreateRemoteLeaseAgentArbitration(a,b,c,d,e,f,g,h,i,j,k,l)  \
LeaseTrace::WriteInfo("CreateRemoteLeaseAgentArbitration", "Create remote lease agent arbitration, lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}), renewed before = {6}, now = {7}, subject expire time = {8}, ping send time = {9}, arbitration duration upper bound = {10}.",b,c,d,e,f,g,h,i,j,k,l)
#define EventWriteFirePreArbitration(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("FirePreArbitration", "Fire pre arbitration, lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}).",b,c,d,e,f,g)
#define EventWriteLeaseDurationConfigs(a,b,c)  \
LeaseTrace::WriteInfo("LeaseDurationConfigs", "{0} : {1}",b,c)
#define EventWriteIndirectLeaseReachLimit(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("IndirectLeaseReachLimit", "Remote Lease agent {0}/{1} has been renewed indirectly {2} in a row, the limit is {3}; Configed ConsecutiveIndirectLeaseTimeout is {4} and lease duration is {5}.",b,c,d,e,f,g)
#define EventWriteRemoteLeaseAgentOrphan(a,b,c,d,e,f,g,h,i,j,k,l,m)  \
LeaseTrace::WriteInfo("RemoteLeaseAgentOrphan", "Remote Lease agent orphan ({0}/{1}, subject = {2}, monitor = {3}), state = {4} ({5}-{6}), IsActive = {7}, InPing = {8}, TimeToBeFailed = {9}, Now = {10}, Result = {11}.",b,c,d,e,f,g,h,i,j,k,l,m)
#define EventWriteDisconnectOrphanedRemoteLA(a,b,c,d,e,f,g,h)  \
LeaseTrace::WriteInfo("DisconnectOrphanedRemoteLA", "Orphaned remote lease agent ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) is disconnected, lease agent remote instance {6}",b,c,d,e,f,g,h)
#define EventWriteGetLeasingApplicationTTLMax(a,b,c,d)  \
LeaseTrace::WriteInfo("GetLeasingApplicationTTLMax", "Get Expiration Timer for leasing application {0} with TTL Max milliseconds, time = {1}, Request TTL {2}",b,c,d)
#define EventWriteGetLeasingApplicationTTLZero(a,b,c,d)  \
LeaseTrace::WriteInfo("GetLeasingApplicationTTLZero", "Get Expiration Timer for leasing application {0} with TTL Zero milliseconds, time = {1}, Request TTL {2}",b,c,d)
#define EventWriteSubjectExpiredOneway(a,b,c,d,e,f,g,h,i)  \
LeaseTrace::WriteInfo("SubjectExpiredOneway", "Subject Expired: {0}/{1} subject = {2}, monitor = {3} in state ({4}-{5}) Current time = {6} Expiration = {7}",b,c,d,e,f,g,h,i)
#define EventWriteArbitrationResultReceiveTimeout(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("ArbitrationResultReceiveTimeout", "Arbitration result receive timeout, failing lease, remote lease agent {0}/{1}, subject = {2}, monitor = {3} in state ({4}-{5})",b,c,d,e,f,g)
#define EventWriteArbitrationSendTimeout(a,b,c,d,e,f,g)  \
LeaseTrace::WriteInfo("ArbitrationSendTimeout", "Arbitration send timeout, failing lease, remote lease agent {0}/{1}, subject = {2}, monitor = {3} in state ({4}-{5})",b,c,d,e,f,g)
#define EventWriteTerminateLeaseRemoteLeaseAgentNotFound(a,b,c,d)  \
LeaseTrace::WriteInfo("TerminateLeaseRemoteLeaseAgentNotFound", "Terminate lease, remote lease agent not found, leasing application handle = {0}, lease handle = {1}, remote leasing application identifier = {2}",b,c,d)
#define EventWriteLeaseRelationshipIdentifierDelete(a,b,c,d)  \
LeaseTrace::WriteInfo("LeaseRelationshipIdentifierDelete", "Lease relationship identifier {0}/{1} is deleted with address {2}",b,c,d)
#define EventWriteProcessStalePing(a,b,c,d,e,f,g,h,i,j)  \
LeaseTrace::WriteInfo("ProcessStalePing", "Lease relationship ({0}/{1}, subject = {2}, monitor = {3}) in state ({4}-{5}) received stale ping with message type {6}, message ID {7} and lease instance {8}",b,c,d,e,f,g,h,i,j)
#define EventWriteArbitrationNeutralRemoteLeaseAgentFound(a,b,c,d,e,f,g,h,i,j)  \
LeaseTrace::WriteInfo("ArbitrationNeutralRemoteLeaseAgentFound", "Arbitration neutral remote lease agent ({0}/{1}, subject = {2}, monitor = {3}), state = {4} ({5}-{6}), IsActive = {7}, RemoteLeaseAgentInstance = {8}, found",b,c,d,e,f,g,h,i,j)
#define EventWriteProcessArbitrationNeutralStaleMessage(a,b,c,d,e,f,g,h,i,j,k,l,m)  \
LeaseTrace::WriteInfo("ProcessArbitrationNeutralStaleMessage", "Process arbitration neutral stale message ({0}/{1}, subject = {2}, monitor = {3}), state = {4} ({5}-{6}), IsActive = {7}, RemoteLeaseAgentInstance = {8}, MessageId = {9}, MessageType = {10}, LeaseInstance = {11}",b,c,d,e,f,g,h,i,j,k,l,m)
#define EventWriteProcessNonArbitrationNeutralStaleMessage(a,b,c,d,e,f,g,h,i,j,k,l,m)  \
LeaseTrace::WriteInfo("ProcessNonArbitrationNeutralStaleMessage", "Process non arbitration neutral stale message ({0}/{1}, subject = {2}, monitor = {3}), state = {4} ({5}-{6}), IsActive = {7}, RemoteLeaseAgentInstance = {8}, MessageId = {9}, MessageType = {10}, LeaseInstance = {11}",b,c,d,e,f,g,h,i,j,k,l,m)


//rtl 
#define RTL_GENERIC_TABLE INT
#define PRTL_GENERIC_TABLE VOID*
//#define RtlNumberGenericTableElements(a) 0
//#define RtlNumberGenericTableElementsAvl(a) 0
//#define RtlGetElementGenericTable(a,b) NULL
//#define RtlEnumerateGenericTable(a,b) NULL
//#define RtlLookupElementGenericTable(a,b) NULL
//#define RtlLookupElementGenericTableAvl(a,b) NULL
#define RtlULongLongAdd(a,b,c) 0,*c=a+b
#define RtlULongLongMult(a,b,c) 0,*c=a*b;
#define RtlStringCchCopyW(a,b,c) 0,wcsncpy(a,c,b);
#define RtlULongAdd(a,b,c) 0,*c=a+b
//#define RtlDeleteElementGenericTable(a,b) TRUE
//#define RtlInsertElementGenericTable(a,b,c,d) NULL
#define RtlULongSub(a,b,c) 0,*c=a-b
#define RtlStringCbCopyW(a,b,c) 0,wcsncpy(a,c,b);

#define RtlStringCchLengthW(a,b,c)0,*c=wcslen(a);
typedef UCHAR KIRQL;

typedef KIRQL *PKIRQL;
typedef void * PTRANSPORT_SECURITY_DESCRIPTOR;
typedef void * PDEVICE_OBJECT;
//typedef void * PTRANSPORT_LISTEN_ENDPOINT;
//typedef void * PLEASE_CONFIG_DURATIONS;
#define IsSecurityProviderSsl(a) FALSE
//LeaseTransport.cpp
#define TransportCreateSecurityDescriptor(a,b,c) 0
#define TransportSecurityDescriptorRelease(a)
#define TransportSetSecurityDescriptor(a,b)
#define TransportProcessRemoteCertVerifyResult(a,b)
#define TransportSetCredentials(a,b,c,d)

#define EventWriteRemoteLeaseAgentDelete(a,b,c)  \
LeaseTrace::WriteInfo("RemoteLeaseAgentDelete",  "{0}, {1}", b,c)

#define EventWriteWaitingOnRemoteLeaseAgent(a,b,c,d,e) \
LeaseTrace::WriteInfo("WaitingOnRemoteLeaseAgent", "{0}, {1}, {2}, {3}", b,c,d,e)

#define EventWriteRemoteLeaseAgentDeallocReady(a,b,c) \
LeaseTrace::WriteInfo("RemoteLeaseAgentDeallocReady", "{0}, {1}", b,c)

#define EventWriteRemoteLeaseAgentDeallocNotReady(a,b,c,d) \
LeaseTrace::WriteInfo("RemoteLeaseAgentDeallocNotReady", "{0}, {1}, {2}", b,c,d)

#define EventWriteTerminateMonitorLeaseSubFailed(a,b,c,d,e) \
LeaseTrace::WriteInfo("TerminateMonitorLeaseSubFailed", "{0}, {1}, {2}, {3}", b,c,d,e)

#define EventWriteTerminateMonitorLeaseSubTerminated(a,b,c,d,e) \
LeaseTrace::WriteInfo("TerminateMonitorLeaseSubTerminated", "{0}, {1}, {2}, {3}", b,c,d,e)


//status

#define STATUS_INVALID_DEVICE_REQUEST _HRESULT_TYPEDEF_(0xC0000010L)
#define STATUS_REVISION_MISMATCH _HRESULT_TYPEDEF_(0xC0000059L)
#define STATUS_RETRY _HRESULT_TYPEDEF_(0xC000022DL)
#define STATUS_DATA_ERROR _HRESULT_TYPEDEF_(0x80093005L)
#define STATUS_SHARING_VIOLATION _HRESULT_TYPEDEF_(0xC0000043L)
#define STATUS_OBJECTID_EXISTS _HRESULT_TYPEDEF_(0xC000022BL)
#define STATUS_CANCELLED _HRESULT_TYPEDEF_(0xC0000120L)

//
// Maintenance thread context.
//
typedef struct _WORKER_THREAD_CONTEXT {

    //
    // Worker thread handle.
    //
    HANDLE WorkerThreadHandle;
    //
    // Shutdown event. Signaled when the thread has to exit.
    //
    KEVENT Shutdown;
    
} WORKER_THREAD_CONTEXT, *PWORKER_THREAD_CONTEXT;


VOID DriverUnloadAddref();
VOID DriverUnloadRelease();

//
// The device object context space. Extra, nonpageable, memory space 
// that a driver can allocate to a device object.
//
typedef struct _DEVICE_CONTEXT {

    //
    // WMI provider.
    //
    WDFWMIINSTANCE WmiDeviceInstance;

}  DEVICE_CONTEXT, *PDEVICE_CONTEXT;


//
// Message types supported by the lease layer.
//
typedef enum _LEASE_MESSAGE_TYPE {

    LEASE_REQUEST,
    LEASE_RESPONSE,
    PING_REQUEST,
    PING_RESPONSE,
    FORWARD_REQUEST,
    FORWARD_RESPONSE,
    RELAY_REQUEST,
    RELAY_RESPONSE

} LEASE_MESSAGE_TYPE, *PLEASE_MESSAGE_TYPE;

//
// Types of receive operations. Message header is received first.
//
typedef enum _RECEIVE_FRAGMENT {

    RECEIVE_HEADER,
    RECEIVE_BODY

} RECEIVE_FRAGMENT, *PRECEIVE_FRAGMENT;

//
// Message body list descriptor.
//
typedef struct _LEASE_MESSAGE_BODY_LIST_DESCRIPTOR {

    //
    // Number of elements in the list.
    //
    DWORD Count;
    //
    // Offset to the first element in the list.
    //
    DWORD StartOffset;
    //
    // The whole size in bytes.
    //
    ULONG Size;

} LEASE_MESSAGE_BODY_LIST_DESCRIPTOR, *PLEASE_MESSAGE_BODY_LIST_DESCRIPTOR;

#define LEASE_MESSAGE_BODY_LIST_DESCRIPTOR_COUNT 7

//
// Lease message header.
//
typedef struct _LEASE_MESSAGE_HEADER_V1 {

    //
    // Protocol versioning information (major version).
    // This represents a breaking change.
    //
    BYTE MajorVersion;
    //
    // Protocol versioning information (minor version).
    // This represents an upgradable change.
    //
    BYTE MinorVersion;
    //
    // Message header size.
    //
    ULONG MessageHeaderSize;
    //
    // Message body size.
    //
    ULONG MessageSize;
    //
    // Identifier of the message.
    //
    LARGE_INTEGER MessageIdentifier;
    //
    // Message type.
    //
    LEASE_MESSAGE_TYPE MessageType;
    //
    // Lease identifier.
    //
    LARGE_INTEGER LeaseInstance;
    //
    // Remote lease agent instance. Required by arbitration.
    //
    LARGE_INTEGER RemoteLeaseAgentInstance;
    //
    // Lease TTL.
    //
    LONG Duration;
    //
    // Requested lease expiration.
    //
    LARGE_INTEGER Expiration;
    //
    // Lease short arbitration timeout.
    //
    LONG LeaseSuspendDuration;
    //
    // Lease long arbitration timeout.
    //
    LONG ArbitrationDuration;
    //
    // Lease long arbitration timeout.
    //
    BOOLEAN IsTwoWayTermination;
    //
    // List of lease relationships in subject role that are not yet established.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectPendingListDescriptor;
    //
    // List of lease relationships in subject role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectFailedPendingListDescriptor;
    //
    // List of lease relationships in monitor role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR MonitorFailedPendingListDescriptor;
    //
    // List of lease relationships in subject role that are now accepted.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectPendingAcceptedListDescriptor;
    //
    // List of lease relationships in subject role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectFailedAcceptedListDescriptor;
    //
    // List of lease relationships in monitor role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR MonitorFailedAcceptedListDescriptor;
    //
    // List of lease relationships in subject role that are now rejected.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectPendingRejectedListDescriptor;
    //
    // List of lease relationships in subject role that are now terminated.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectTerminatePendingListDescriptor;
    //
    // List of lease relationships in subject role that are now accepected as being terminated.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectTerminateAcceptedListDescriptor;
    //
    // Listen end point for sender in message body
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR MessageListenEndPointDescriptor;

} LEASE_MESSAGE_HEADER_V1, *PLEASE_MESSAGE_HEADER_V1;

//
// Lease message header.
//
typedef struct _LEASE_MESSAGE_HEADER {

    //
    // Protocol versioning information (major version).
    // This represents a breaking change.
    //
    BYTE MajorVersion;
    //
    // Protocol versioning information (minor version).
    // This represents an upgradable change.
    //
    BYTE MinorVersion;
    //
    // Message header size.
    //
    ULONG MessageHeaderSize;
    //
    // Message body size.
    //
    ULONG MessageSize;
    //
    // Identifier of the message.
    //
    LARGE_INTEGER MessageIdentifier;
    //
    // Message type.
    //
    LEASE_MESSAGE_TYPE MessageType;
    //
    // Lease identifier.
    //
    LARGE_INTEGER LeaseInstance;
    //
    // Remote lease agent instance. Required by arbitration.
    //
    LARGE_INTEGER RemoteLeaseAgentInstance;
    //
    // Lease TTL.
    //
    LONG Duration;
    //
    // Requested lease expiration.
    //
    LARGE_INTEGER Expiration;
    //
    // Lease short arbitration timeout.
    //
    LONG LeaseSuspendDuration;
    //
    // Lease long arbitration timeout.
    //
    LONG ArbitrationDuration;
    //
    // Lease long arbitration timeout.
    //
    BOOLEAN IsTwoWayTermination;
    //
    // List of lease relationships in subject role that are not yet established.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectPendingListDescriptor;
    //
    // List of lease relationships in subject role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectFailedPendingListDescriptor;
    //
    // List of lease relationships in monitor role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR MonitorFailedPendingListDescriptor;
    //
    // List of lease relationships in subject role that are now accepted.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectPendingAcceptedListDescriptor;
    //
    // List of lease relationships in subject role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectFailedAcceptedListDescriptor;
    //
    // List of lease relationships in monitor role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR MonitorFailedAcceptedListDescriptor;
    //
    // List of lease relationships in subject role that are now rejected.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectPendingRejectedListDescriptor;
    //
    // List of lease relationships in subject role that are now terminated.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectTerminatePendingListDescriptor;
    //
    // List of lease relationships in subject role that are now accepected as being terminated.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectTerminateAcceptedListDescriptor;
    //
    // Listen end point for sender in message body
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR MessageListenEndPointDescriptor;
    //
    // Listen end point for remote destination used in forward/relay messages
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR LeaseListenEndPointDescriptor;

} LEASE_MESSAGE_HEADER, *PLEASE_MESSAGE_HEADER;

typedef struct _LEASE_MESSAGE_EXT
{
    _LEASE_MESSAGE_EXT()
    {
        MsgLeaseAgentInstance.QuadPart = 0;
    }
    //
    // Lease agent instance known by the remote partner node
    // It would be compared with local lease agent instance to check for stale message
    //
    LARGE_INTEGER MsgLeaseAgentInstance;

} LEASE_MESSAGE_EXT, *PLEASE_MESSAGE_EXT;

//
// Lease states.
//
typedef enum _ONE_WAY_LEASE_STATE {

    LEASE_STATE_INACTIVE,
    LEASE_STATE_ACTIVE,
    LEASE_STATE_EXPIRED,
    LEASE_STATE_FAILED,

} ONE_WAY_LEASE_STATE, *PONE_WAY_LEASE_STATE;

//
// Lease agent state.
//
typedef enum _LEASE_AGENT_STATE {

    OPEN,
    SUSPENDED,
    FAILED

} LEASE_AGENT_STATE, *PLEASE_AGENT_STATE;

struct _REMOTE_LEASE_AGENT_CONTEXT;
struct _LEASING_APPLICATION_CONTEXT;
struct _LEASE_RELATIONSHIP_IDENTIFIER;
struct _LEASE_AGENT_CONTEXT;
struct _TRANSPORT_BLOCKING_TEST_DESCRIPTOR;

struct REMOTE_LEASE_AGENT_CONTEXT_COMPARATOR{
    bool operator()(_REMOTE_LEASE_AGENT_CONTEXT* left, _REMOTE_LEASE_AGENT_CONTEXT* right);
};

struct LEASING_APPLICATION_CONTEXT_COMPARATOR{
    bool operator()(_LEASING_APPLICATION_CONTEXT* left, _LEASING_APPLICATION_CONTEXT* right);
};

struct LEASE_RELATIONSHIP_IDENTIFIER_COMPARATOR{
    bool operator()(_LEASE_RELATIONSHIP_IDENTIFIER* left, _LEASE_RELATIONSHIP_IDENTIFIER* right);
};

struct LEASE_AGENT_CONTEXT_COMPARATOR{
    bool operator()(_LEASE_AGENT_CONTEXT* left, _LEASE_AGENT_CONTEXT* right);
};

struct TRANSPORT_BLOCKING_TEST_DESCRIPTOR_COMPARATOR{
    bool operator()(_TRANSPORT_BLOCKING_TEST_DESCRIPTOR* left, _TRANSPORT_BLOCKING_TEST_DESCRIPTOR* right);
};

typedef std::set<_LEASE_RELATIONSHIP_IDENTIFIER*, LEASE_RELATIONSHIP_IDENTIFIER_COMPARATOR> LEASE_RELATIONSHIP_IDENTIFIER_SET;
typedef std::set<_LEASING_APPLICATION_CONTEXT*, LEASING_APPLICATION_CONTEXT_COMPARATOR> LEASING_APPLICATION_CONTEXT_SET;
typedef std::set<_REMOTE_LEASE_AGENT_CONTEXT*, REMOTE_LEASE_AGENT_CONTEXT_COMPARATOR> REMOTE_LEASE_AGENT_CONTEXT_SET;
typedef std::set<_LEASE_AGENT_CONTEXT*, LEASE_AGENT_CONTEXT_COMPARATOR> LEASE_AGENT_CONTEXT_SET;
typedef std::set<_TRANSPORT_BLOCKING_TEST_DESCRIPTOR*, TRANSPORT_BLOCKING_TEST_DESCRIPTOR_COMPARATOR> TRANSPORT_BEHAVIOR_SET;
//
// Lease agent data structure. 
//
typedef struct _LEASE_AGENT_CONTEXT {
    _LEASE_AGENT_CONTEXT() :
        Transport(NULL),
        TransportClosed(false),
        State(OPEN),
        LeaseSuspendDuration(0),
        ArbitrationDuration(0),
        SessionExpirationTimeout(0),
        LeaseRetryCount(0),
        LeaseRenewBeginRatio(0)
    {
        Instance.QuadPart = 0;
        LeaseConfigDurations.LeaseDuration = 0;
        LeaseConfigDurations.LeaseDurationAcrossFD = 0;
        LeaseConfigDurations.UnresponsiveDuration = 0;
        TimeToBeFailed.QuadPart = 0;
    }
    //
    // Instance of this lease agent.
    //
    LARGE_INTEGER Instance;
    //
    // Transport used by this lease agent
    //
    PLEASE_TRANSPORT Transport;
    bool TransportClosed;
    //
    // Address this agent listens on
    //
    TRANSPORT_LISTEN_ENDPOINT ListenAddress;
    //
    // Hash table concurrency control.
    //
    KSPIN_LOCK Lock;
    //
    // Remote lease agent context hash table.
    //
    REMOTE_LEASE_AGENT_CONTEXT_SET RemoteLeaseAgentContextHashTable;
    //
    // Leasing application context hash table.
    //
    LEASING_APPLICATION_CONTEXT_SET LeasingApplicationContextHashTable;
    //
    // Leasing application context unregister list.
    //
    LEASING_APPLICATION_CONTEXT_SET LeasingApplicationContextUnregisterList;
    //
    // State of this lease agent.
    //
    LEASE_AGENT_STATE State;
    //
    // Indicates whether the maintenance worker will clean up this lease agent.
    //
    LARGE_INTEGER TimeToBeFailed;
    //
    // Lease Agent Configuration
    //
    LEASE_CONFIG_DURATIONS LeaseConfigDurations;
    //
    LONG LeaseSuspendDuration;
    LONG ArbitrationDuration;
    LONG SessionExpirationTimeout;
    //
    // Number of renew retries
    //
    LONG LeaseRetryCount;
    LONG LeaseRenewBeginRatio;
    //
    // Delayed termination timer routine
    // For lease failure, before sending out terminate all lease message
    // We need to delay for TTL to make sure the lease is actually expired from user process perspective
    //
    KTIMER DelayedLeaseFailureTimer;
    KDPC DelayedLeaseFailureTimerDpc;
    //
    // Set to true when set the OnLeaseFailed delay timer.
    // The flag can prevent the timer begin set twice,
    // that can happen when there are multiple arbitration rejects from user mode
    //
    BOOLEAN IsInDelayTimer;

    // leases renewed indirectly
    LONG ConsecutiveIndirectLeaseLimit;

} LEASE_AGENT_CONTEXT, * PLEASE_AGENT_CONTEXT;

//
// The driver object context space. Extra, nonpageable, memory space
// that a driver can allocate to a device object. Globally allocated
// structures typically go in this context.
//
typedef struct _DRIVER_CONTEXT {
    //
    // Singleton state for KTL transport
    //
    PDRIVER_TRANSPORT DriverTransport;
    //
    // Lease agent hash table concurrency control. For performance
    // we coudl consider changing this later to a reader/writer lock.
    //
    FAST_MUTEX LeaseAgentContextHashTableAccessMutex;
    //
    // Map of lease agents.
    //
    LEASE_AGENT_CONTEXT_SET LeaseAgentContextHashTable;
    //
    // Worker thread context.
    //
    WORKER_THREAD_CONTEXT MaintenanceWorkerThreadContext;
    //
    // Flag indicating we're unloading and can skip some work
    //
    BOOLEAN Unloading;
    //
    // Last instance requested
    //
    LARGE_INTEGER LastInstance;
    //
    // Instance concurrency control.
    //
    KSPIN_LOCK InstanceLock;
    //
    // Test only. Table for blocking behaviors
    //
    TRANSPORT_BEHAVIOR_SET TransportBehaviorTable;
    //
    // Test only. SpinLock for accessing TransportBehaviorTable
    //
    KSPIN_LOCK TransportBehaviorTableAccessSpinLock;


} DRIVER_CONTEXT, *PDRIVER_CONTEXT;
//
// Lease Agent Constructor/Destructor.
//
NTSTATUS LeaseAgentConstructor(
    __in PTRANSPORT_LISTEN_ENDPOINT SocketAddress,
    __in PLEASE_CONFIG_DURATIONS LeaseDurations,
    __in LONG LeaseSuspendDuration,
    __in LONG ArbitrationDuration,
    __in LONG NumOfRetry,
    __in LONG StartOfRetry,
    Transport::SecuritySettings const* securitySettings, 
    __out PLEASE_AGENT_CONTEXT * result
     );

VOID 
LeaseAgentUninitialize(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    );

VOID LeaseAgentDestructor(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    );

//
// Lease agent routines.
//
NTSTATUS LeaseAgentCreateListeningTransport(__in PLEASE_AGENT_CONTEXT LeaseAgentContext);

//
// Lease context information.
//
typedef struct _LEASE_RELATIONSHIP_CONTEXT {
    //
    // Subject information.
    //
    ONE_WAY_LEASE_STATE SubjectState;
    LARGE_INTEGER SubjectIdentifier;
    LARGE_INTEGER SubjectExpireTime;
    LARGE_INTEGER SubjectSuspendTime;
    LARGE_INTEGER SubjectFailTime;
    KTIMER SubjectTimer;
    KDPC SubjectTimerDpc;
    BOOL LeaseMessageSent;

    //
    // Monitor information.
    //
    ONE_WAY_LEASE_STATE MonitorState;
    LARGE_INTEGER MonitorIdentifier;
    LARGE_INTEGER MonitorExpireTime;
    KTIMER MonitorTimer;
    KDPC MonitorTimerDpc;

    //
    // Arbitration information.
    //
    KTIMER PreArbitrationSubjectTimer;
    KDPC PreArbitrationSubjectTimerDpc;
    KTIMER PreArbitrationMonitorTimer;
    KDPC PreArbitrationMonitorTimerDpc;
    KTIMER PostArbitrationTimer;
    KDPC PostArbitrationTimerDpc;

    //
    // Common subject and monitor information.
    //
    KTIMER RenewOrArbitrateTimer;
    KDPC RenewOrArbitrateTimerDpc;
    LONG Duration;
    LEASE_DURATION_TYPE EstablishDurationType;
    //
    // Set to true by UpdateDuration IOCTL call
    // Would be reset to false with a new renew request sent out
    //
    BOOLEAN IsDurationUpdated;
    LONG LeaseSuspendDuration;
    LONG ArbitrationDuration;

    // Durations requested by the lease partner; initialized to MAX
    LONG RemoteLeaseDuration;
    LONG RemoteSuspendDuration;
    LONG RemoteArbitrationDuration;

    BOOLEAN IsRenewRetry;
    LONG RenewRetryCount;
    LONG IndirectLeaseCount;

    //
    // Timer for ping retry.
    //
    KTIMER PingRetryTimer;
    KDPC PingRetryTimerDpc;
} LEASE_RELATIONSHIP_CONTEXT, *PLEASE_RELATIONSHIP_CONTEXT;


//
// Leasing application data structure.
//
typedef struct _LEASING_APPLICATION_CONTEXT {
    _LEASING_APPLICATION_CONTEXT() :
    LeasingApplicationIdentifier(NULL),
    Instance(NULL),
    LeasingApplicationIdentifierByteCount(0),
    IsArbitrationEnabled(0),
    UnregisterTimerDpc(0),
    LeaseAgentContext(NULL),
    IsBeingUnregistered(FALSE),
    LeasingApplicationExpiryTimeout(0)
    {
        GlobalLeaseExpireTime.QuadPart = 0;
    }
    //
    // Identifier of this application for this lease agent.
    //
    LPWSTR LeasingApplicationIdentifier;
    //
    // Instance of this lease application.
    //
    HANDLE Instance;
    //
    // Count in bytes of the leasing application identifier.
    // Used in message serialization.
    //
    ULONG LeasingApplicationIdentifierByteCount;
    //
    // The process handle registering this application.
    // Used to detect application failure.
    //
    PEPROCESS ProcessHandle;
    //
    // Delayed request used for leasing application events.
    //
    WDFREQUEST DelayedRequest;
    //
    // Leasing application events.
    //
    std::deque<PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER> LeasingApplicationEventQueue;
    //
    // Specifies if the leasing application is enabled for arbitration.
    //
    BOOLEAN IsArbitrationEnabled;
    //
    // Gloabl Lease Expire Time.
    //
    LARGE_INTEGER GlobalLeaseExpireTime;
    //
    // Delay unregister timer routine.
    //
    KTIMER UnregisterTimer;
    KDPC UnregisterTimerDpc;
    PLEASE_AGENT_CONTEXT LeaseAgentContext;
    //
    // Set to true when set the unregister timer; GetLeasingAPPTTL returns 0 when this flag is true.
    //
    BOOLEAN IsBeingUnregistered;
    //
    // leasing application expiry timeout defined in fed config
    //
    LONG LeasingApplicationExpiryTimeout;

    OVERLAPPED_LEASE_LAYER_EXTENSION Callbacks;
    //
    // 
    LARGE_INTEGER LastGrantExpireTime;

} LEASING_APPLICATION_CONTEXT, *PLEASING_APPLICATION_CONTEXT;



//
// Lease relationship identification.
//
typedef struct _LEASE_RELATIONSHIP_IDENTIFIER {

    //
    // Local leasing application part of this lease.
    //
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext;
    //
    // The remote leasing application used by this lease.
    //
    LPWSTR RemoteLeasingApplicationIdentifier;
    //
    // Count in bytes of the remote leasing application identifier.
    //
    ULONG RemoteLeasingApplicationIdentifierByteCount;

} LEASE_RELATIONSHIP_IDENTIFIER, *PLEASE_RELATIONSHIP_IDENTIFIER;
//
// Remote lease agent data structure. The lease layer will contain one of 
// these structures for each remote machine it maintains a lease with.
//
typedef struct _REMOTE_LEASE_AGENT_CONTEXT {

    //
    // Lease agent context holding this remote lease agent.
    //
    PLEASE_AGENT_CONTEXT LeaseAgentContext;
    //
    // Instance of this remote lease agent.
    //
    LARGE_INTEGER Instance;
    //
    // Instance of this lease agent at the other end of the lease.
    // Required by arbitration.
    //
    LARGE_INTEGER RemoteLeaseAgentInstance;
    // 
    // Remote lease agent identifier (tracing).
    //
    WCHAR RemoteLeaseAgentIdentifier[LEASE_AGENT_ID_CCH_MAX];
    //
    // The remote socket address of the remote lease agent.
    //
    TRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress;
    //
    // Subject list (contains PLEASE_RELATIONSHIP_IDENTIFIER).
    //
    LEASE_RELATIONSHIP_IDENTIFIER_SET SubjectHashTable;
    //
    // Subject failed pending list (contains PLEASE_RELATIONSHIP_IDENTIFIER).
    //
    LEASE_RELATIONSHIP_IDENTIFIER_SET SubjectFailedPendingHashTable;
    //
    // Subject terminate pending list (contains PLEASE_RELATIONSHIP_IDENTIFIER).
    //
    LEASE_RELATIONSHIP_IDENTIFIER_SET SubjectTerminatePendingHashTable;
    //
    // Subject establish pending list (contains PLEASE_RELATIONSHIP_IDENTIFIER).
    //
    LEASE_RELATIONSHIP_IDENTIFIER_SET SubjectEstablishPendingHashTable;
    //
    // Monitor list (contains PLEASE_RELATIONSHIP_IDENTIFIER).
    //
    LEASE_RELATIONSHIP_IDENTIFIER_SET MonitorHashTable;
    //
    // Monitor failed list (contains PLEASE_RELATIONSHIP_IDENTIFIER).
    //
    LEASE_RELATIONSHIP_IDENTIFIER_SET MonitorFailedPendingHashTable;
    //
    // Lease with remote lease agent.
    //
    PLEASE_RELATIONSHIP_CONTEXT LeaseRelationshipContext;
    //
    // Partner Channel context
    //
    PTRANSPORT_SENDTARGET PartnerTarget;
    //
    // State of this remote lease agent.
    //
    LEASE_AGENT_STATE State;
    //
    // Reference count used to safely deallocate this struct.
    //
    LONG RefCount;
    //
    // Event used for waiting for zero reference count.
    //
    KEVENT ZeroRefCountEvent;
    //
    // Indicates when the maintenance worker will clean up this lease agent.
    //
    LARGE_INTEGER TimeToBeFailed;
    //
    // If the arbitration result is neutral, set this flag to keep the remote lease agent from being destructing.
    //
    BOOLEAN IsInArbitrationNeutral;
    //
    // It is possible that there are multiple RLA in the hash table for one remote side lease agent
    // There should only be one active, the rest are obsolete and would be cleaned up during maintenance
    //
    BOOLEAN IsActive;
    //
    // Mark the flag when RLA sent out two way lease termination request
    //
    BOOLEAN IsInTwoWayTermination;
    //
    // In ping process
    //
    BOOLEAN InPing;
    //
    // Has renew ever happened before?
    //
    BOOLEAN RenewedBefore;
    //
    // The time ping request or ping response was sent
    //
    LARGE_INTEGER PingSendTime;
    //
    // The leasing application used for rasing arbitration request to user mode
    //
    PLEASING_APPLICATION_CONTEXT LeasingApplicationForArbitration;
    //
    // Lease protocol version of the remote side
    //
    USHORT RemoteVersion;

} REMOTE_LEASE_AGENT_CONTEXT, *PREMOTE_LEASE_AGENT_CONTEXT;

//
// Lease relationship Constructor/Destructor.
//
NTSTATUS
LeaseRelationshipConstructor(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __out PLEASE_RELATIONSHIP_CONTEXT * result
    );

VOID
CancelRemoteLeaseAgentTimers(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
);

VOID 
RemoteLeaseAgentUninitialize(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

VOID 
LeaseRelationshipUninitialize(
    PLEASE_RELATIONSHIP_CONTEXT LeaseRelationshipContext
    );

VOID 
LeaseRelationshipDestructor(
    PLEASE_RELATIONSHIP_CONTEXT Lease
    );

//
// Remote Lease Agent Constructor/Destructor.
//
NTSTATUS RemoteLeaseAgentConstructor(
    __in_opt PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __out PREMOTE_LEASE_AGENT_CONTEXT * result
    );

VOID
RemoteLeaseAgentDisconnect(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in BOOLEAN AbortConnections
    );

VOID 
RemoteLeaseAgentDestructor(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

//
// Deallocation check routines for lease agent and remote lease agent.
//
BOOLEAN 
IsRemoteLeaseAgentReadyForDeallocation(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in BOOLEAN WaitForZeroRefCount
    );

BOOLEAN
CanLeaseAgentBeFailed(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    );

BOOLEAN
IsLeaseAgentReadyForDeallocation(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    );

//
// Remote lease agent ref couting routines.
//
LONG 
AddRef(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

LONG
Release(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

//
// Remote lease agent timer routines.
//
VOID
EnqueueTimer(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PKTIMER Timer,
    __in LARGE_INTEGER ExpireTime,
    __in PKDPC TimerDpc
    );

VOID
DequeueTimer(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PKTIMER Timer
    );

NTSTATUS
ProcessLeaseMessageBuffer(
    __in PLEASE_TRANSPORT Listener,
    __in PTRANSPORT_SENDTARGET Target,
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PVOID Buffer,
    __in ULONG BufferSize
    );

//
// Leasing application Constructor/Destructor.
//
PLEASING_APPLICATION_CONTEXT 
LeasingApplicationConstructor(
    __in LPCWSTR LeasingApplicationIdentifier,
    __in_opt PEPROCESS ProcessHandle
    );

VOID 
LeasingApplicationDestructor(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

//
// Lease Relationship Constructor/Destructor.
//
PLEASE_RELATIONSHIP_IDENTIFIER 
LeaseRelationshipIdentifierConstructor(
    __in LPCWSTR LeasingApplicationIdentifier,
    __in LPCWSTR RemoteLeasingApplicationIdentifier
    );

VOID LeaseRelationshipIdentifierDestructor(
    __in PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier
    );

VOID
SwitchLeaseRelationshipLeasingApplicationIdentifiers(
    __in PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier
    );

//
// Validation routines.
//
BOOLEAN
IsValidString(
    PWCHAR String,
    __in ULONG StringWcharLengthIncludingNullTerminator
    );

BOOLEAN
IsValidListenEndpoint(
    __in PTRANSPORT_LISTEN_ENDPOINT listenEndpoint
    );

NTSTATUS
IsValidMessageHeader(
    __in PVOID Buffer,
    __in ULONG BufferSize
    );

BOOLEAN
IsValidProcessHandle(
    __in HANDLE UserModeProcessHandle
    );

BOOLEAN
IsValidDuration(
    __in LONG Duration
    );

//
// Routines used to mark lease agents and remote lease agents as failed.
//
VOID
SetLeaseAgentFailed(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    );

VOID
OnLeaseFailure(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    );

VOID
SetRemoteLeaseAgentState(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in LEASE_AGENT_STATE State
    );

BOOLEAN
IsRemoteLeaseAgentFailed(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

BOOLEAN
IsRemoteLeaseAgentSuspended(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

BOOLEAN
IsRemoteLeaseAgentOrphaned(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

BOOLEAN
IsRemoteLeaseAgentInPing(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

BOOLEAN
IsRemoteLeaseAgentOpen(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

BOOLEAN
IsRemoteLeaseAgentOpenTwoWay(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

BOOLEAN
IsLeaseAgentFailed(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    );

VOID
SetRemoteLeaseAgentFailed(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

//
// Message serialization/deserialization routines.
//
NTSTATUS
SerializeLeaseMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in LEASE_MESSAGE_TYPE LeaseMessageType,
    __in_opt LEASE_RELATIONSHIP_IDENTIFIER_SET & SubjectEstablishPendingList,
    __in_opt LEASE_RELATIONSHIP_IDENTIFIER_SET & SubjectFailedPendingList,
    __in_opt LEASE_RELATIONSHIP_IDENTIFIER_SET & MonitorFailedPendingList,
    __in_opt LEASE_RELATIONSHIP_IDENTIFIER_SET & SubjectPendingAcceptedList,
    __in_opt LEASE_RELATIONSHIP_IDENTIFIER_SET & SubjectPendingRejectedList,
    __in_opt LEASE_RELATIONSHIP_IDENTIFIER_SET & SubjectFailedAcceptedList,
    __in_opt LEASE_RELATIONSHIP_IDENTIFIER_SET & MonitorFailedAcceptedList,
    __in_opt LEASE_RELATIONSHIP_IDENTIFIER_SET & SubjectTerminatePendingList,
    __in_opt LEASE_RELATIONSHIP_IDENTIFIER_SET & SubjectTerminateAcceptedList,
    __in LONG Duration,
    __in LARGE_INTEGER Expiration,
    __in LONG LeaseSuspendDuration,
    __in LONG ArbitrationDuration,
    __in BOOLEAN IsTwoWayTermination,
    __in PVOID* Buffer,
    __in PULONG BufferSize
    );

NTSTATUS
DeserializeLeaseMessage(
    __in PLEASE_MESSAGE_HEADER LeaseMessageHeader,
    __in PVOID Buffer,
    __in LEASE_RELATIONSHIP_IDENTIFIER_SET & SubjectPendingList,
    __in LEASE_RELATIONSHIP_IDENTIFIER_SET & SubjectFailedPendingList,
    __in LEASE_RELATIONSHIP_IDENTIFIER_SET & MonitorFailedPendingList,
    __in LEASE_RELATIONSHIP_IDENTIFIER_SET & SubjectPendingAcceptedList,
    __in LEASE_RELATIONSHIP_IDENTIFIER_SET & SubjectPendingRejectedList,
    __in LEASE_RELATIONSHIP_IDENTIFIER_SET & SubjectFailedAcceptedList,
    __in LEASE_RELATIONSHIP_IDENTIFIER_SET & MonitorFailedAcceptedList,
    __in LEASE_RELATIONSHIP_IDENTIFIER_SET & SubjectTerminatePendingList,
    __in LEASE_RELATIONSHIP_IDENTIFIER_SET & SubjectTerminateAcceptedList,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteMessageSocketAddress,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteLeaseSocketAddress,
    __in PLEASE_MESSAGE_EXT MessageExtension
    );

VOID
ClearRelationshipIdentifierList(
    __in LEASE_RELATIONSHIP_IDENTIFIER_SET & HashTable
    );

VOID
ClearLeasingApplicationList(
    __in LEASING_APPLICATION_CONTEXT_SET & HashTable
    );

PLEASE_RELATIONSHIP_IDENTIFIER 
AddToLeaseRelationshipIdentifierList(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LPCWSTR RemoteLeasingApplicationIdentifier,
    __in LEASE_RELATIONSHIP_IDENTIFIER_SET & HashTable
    );

//
// Lease related routines.
//

PREMOTE_LEASE_AGENT_CONTEXT
FindMostRecentRemoteLeaseAgent(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextMatch
    );
    
VOID 
EstablishLease(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

VOID 
ArbitrateLease(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in LONG LocalTimeToLive,
    __in LONG RemoteTimeToLive,
    __in BOOLEAN IsFinal
    );

NTSTATUS
TerminateSubjectLease(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier,
    __in BOOLEAN IsSubjectFailed
    );

NTSTATUS 
TerminateAllLeaseRelationships(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in BOOLEAN isSubjectFailed
    );

//
// Timer related routines.
//
VOID
ArmTimer(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PKTIMER Timer,
    __in LARGE_INTEGER ExpireTime,
    __in PKDPC TimerDpc
    );

VOID
SetSubjectExpireTime(
    __in PLEASE_RELATIONSHIP_CONTEXT LeaseRelationshipContext,
    __in LARGE_INTEGER Expiration,
    __in LONG LeaseSuspendDuration,
    __in LONG ArbitrationDuration
    );

VOID
SetRenewTimer(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in BOOLEAN ByRenewTimerDPC,
    __in LONG RenewDuration
    );

//
// Lease event buffer routines.
//
VOID
DelayedRequestCompletion(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LEASE_LAYER_EVENT_TYPE EventType,
    __in_opt HANDLE LeaseHandle,
    __in HANDLE LeasingApplicationHandle,
    __in_opt LPCWSTR RemoteLeasingApplicationIdentifier,
    __in LONG MonitorTTL,
    __in LONG SubjectTTL,
    __in_opt PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __in LONGLONG LeaseAgentInstance,
    __in LONGLONG RemoteLeaseAgentInstance,
    __in NTSTATUS CompletionStatus,
    __in BOOLEAN IsHighPriority,
    __in LONG cbCertificate,
    PBYTE pbCertificate,
    __in PVOID remoteCertVerifyCompleteOp,
    __in USHORT remoteVersion,
    __in LONGLONG monitorLeaseInstance,
    __in LONGLONG subjectLeaseInstance,
    __in LONG remoteArbitrationDurationUpperBound
    );

VOID
TryDelayedRequestCompletion(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LEASE_LAYER_EVENT_TYPE EventType,
    __in_opt HANDLE LeaseHandle,
    __in HANDLE LeasingApplicationHandle,
    __in_opt LPCWSTR RemoteLeasingApplicationIdentifier,
    __in LONG MonitorTTL,
    __in LONG SubjectTTL,
    __in_opt PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __in LONGLONG LeaseAgentInstance,
    __in LONGLONG RemoteLeaseAgentInstance,
    __in NTSTATUS CompletionStatus,
    __in BOOLEAN IsHighPriority,
    __in LONG cbCertificate,
    PBYTE pbCertificate,
    __in PVOID remoteCertVerifyCompleteOp,
    __in USHORT remoteVersion,
    __in LONGLONG monitorLeaseInstance,
    __in LONGLONG subjectLeaseInstance,
    __in LONG remoteArbitrationDurationUpperBound
    );

PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER
LeaseEventConstructor(
    );

VOID
LeaseEventDestructor(
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER LeaseEvent
    );

BOOLEAN
EnqueueLeasingApplicationLeaseEvent(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

BOOLEAN
EnqueueLeasingApplicationLocalExpirationAndArbitrationEvents(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

BOOLEAN
EnqueueLeasingApplicationRemoteExpirationEvent(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

BOOLEAN
EnqueueLeasingApplicationLeaseEstablishEvents(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

BOOLEAN
EnqueueLeasingApplicationAllLeaseEvents(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

VOID
DequeueLeasingApplicationLeaseEstablishEvent(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

VOID 
TryDequeueLeasingApplicationLeaseEvent(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

//
// Tracing/Supportability.
//
const LPWSTR GetLeaseState(
    __in ONE_WAY_LEASE_STATE State
    );

const LPWSTR GetLeaseAgentState(
    __in LEASE_AGENT_STATE State
    );

const LPWSTR GetMessageType(
    __in LEASE_MESSAGE_TYPE MessageType
    );

const LPWSTR GetBlockMessageType(
    __in LEASE_BLOCKING_ACTION_TYPE BlockMessageType
    );

VOID GetCurrentTime(
    __out PLARGE_INTEGER CurrentTime
    );

NTSTATUS
UnregisterApplication(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in BOOLEAN isSubjectFailed
    );

VOID
MoveToUnregisterList(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

VOID
StartUnregisterApplication(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LONG TTLInMilliseconds
    );

//
// Leasing application unregister timer routine
//
VOID
EnqueueUnregisterTimer(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LONG TTLInMilliseconds
    );

VOID
DequeueUnregisterTimer(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

VOID
ArmUnregisterTimer(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LONG TTLInMilliseconds
    );

//
// Lease agent delay timer routine
//
VOID
EnqueueDelayedLeaseFailureTimer(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in LONG TTLInMilliseconds
    );

VOID
ArmDelayedLeaseFailureTimer(
__in PLEASE_AGENT_CONTEXT LeaseAgentContext,
__in LONG TTLInMilliseconds
);

VOID
DequeueDelayedLeaseFailureTimer(
__in PLEASE_AGENT_CONTEXT LeaseAgentContext
);

VOID
SetLeaseRelationshipDuration(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in LONG Duration,
    __in LONG LeaseSuspendDuration,
    __in LONG ArbitrationDuration
    );

//
// Get a monotonically increasing unique value for instance
//
LARGE_INTEGER GetInstance( );

// 
// update subject and renew timer when receive a termination message
//
VOID
UpdateSubjectTimerForTermination(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

// 
// update monitor timer when receive a termination message
//
VOID
UpdateMonitorTimerForTermination(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

// 
// Check if a remote lease agent is sending out failed termination request
//
BOOLEAN
SendingFailedTerminationRequest(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

//
// Remote certificate struct.
//
typedef struct _LEASE_REMOTE_CERTIFICATE
{
    //
    // The whole size in bytes.
    //
    ULONG cbCertificate;
    //
    // The whole size in bytes.
    //
    BYTE * pbCertificate;
    //
    // Verify operation handle
    //
    PVOID verifyOperation;
    //
    // Leasing application handle
    //
    PLEASING_APPLICATION_CONTEXT certLeasingAppHandle;

} LEASE_REMOTE_CERTIFICATE, *PLEASE_REMOTE_CERTIFICATE;

//
// CTor of remote certificate structure.
//
PLEASE_REMOTE_CERTIFICATE 
LeaseRemoteCertificateConstructor(
    __in ULONG cbCert,
    __in PBYTE pbCert,
    __in PVOID verifyOp,
    __in PLEASING_APPLICATION_CONTEXT leasingAppHandle
    );

VOID 
LeaseRemoteCertificateDestructor(
    __in PLEASE_REMOTE_CERTIFICATE remoteCert
    );

//
// CTor/DTor of listen endpoint structure.
//
PTRANSPORT_LISTEN_ENDPOINT 
LeaseListenEndpointConstructor( );

VOID 
LeaseListenEndpointDestructor( __in PTRANSPORT_LISTEN_ENDPOINT ListenEndPoint );

//
// Construcotr and destructor of lease message extension
PLEASE_MESSAGE_EXT
MessageExtensionConstructor( );

VOID 
MessageExtensionDestructor( __in PLEASE_MESSAGE_EXT MsgExt );

VOID
TerminateSubjectLeaseSendMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in BOOLEAN IsTwoWayTermination
    );

VOID 
EstablishReverseLease(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier
    );

NTSTATUS
TerminateReverseLease(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in LEASE_RELATIONSHIP_IDENTIFIER_SET & SubjectTerminatePendingList
    );

VOID 
EstablishLeaseSendPingRequestMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

VOID
SendPingRequestMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

VOID
AbortPingProcess(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

VOID
SendPingResponseMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

NTSTATUS
DeserializeMessageListenEndPoint(
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __in PVOID Buffer,
    __in ULONG Size
    );


NTSTATUS
SendIndirectLeaseForwardMessages(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

PREMOTE_LEASE_AGENT_CONTEXT
GetCurrentActiveRemoteLeaseAgent(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress
    );

NTSTATUS
SerializeMessageListenEndPoint(
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    PBYTE StartBuffer,
    __in ULONG BufferSize
    );

NTSTATUS
ProcessForwardMessage(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PVOID Buffer,
    __in ULONG BufferSize
    );

NTSTATUS
GetListenEndPointSize(
    __in PTRANSPORT_LISTEN_ENDPOINT ListenEndPoint,
    __in PULONGLONG ListenEndpointAddressSize,
    __in PULONG ListenEndpointSize
    );

NTSTATUS
DeserializeForwardMessage(
    __in PLEASE_MESSAGE_HEADER LeaseMessageHeader,
    __in PVOID Buffer,
    __in PTRANSPORT_LISTEN_ENDPOINT SourceSocketAddress,
    __in PTRANSPORT_LISTEN_ENDPOINT DestSocketAddress
    );

NTSTATUS
SerializeAndSendRelayMessages(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PLEASE_MESSAGE_HEADER LeaseMessageHeader,
    __in PTRANSPORT_LISTEN_ENDPOINT MsgSourceSocketAddress,
    __in PVOID ForwardMsgBuf,
    __in ULONG ForwardMsgSize
    );

BOOLEAN
IsReceivedResponse(__in LEASE_MESSAGE_TYPE LeaseMessageType);

BOOLEAN
IsReceivedRequest(__in LEASE_MESSAGE_TYPE LeaseMessageType);

BOOLEAN
IsSendingResponse(__in LEASE_MESSAGE_TYPE LeaseMessageType);

BOOLEAN
IsSendingRequest(__in LEASE_MESSAGE_TYPE LeaseMessageType);

BOOLEAN
IsIndirectLeaseMsg(__in LEASE_MESSAGE_TYPE LeaseMessageType);

LONG
GetConfigLeaseDuration(__in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext);

VOID
UpdateIndirectLeaseLimit(__in PLEASE_AGENT_CONTEXT LeaseAgentContext);

VOID
GetDurationsForRequest(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __inout PLONG PRequestLeaseDuration,
    __inout PLONG PRequestLeaseSuspendDuration,
    __inout PLONG PRequestArbitrationDuration,
    __in BOOLEAN UpdateLeaseRelationship
    );

BOOLEAN IsBlockTransport(
    PTRANSPORT_LISTEN_ENDPOINT from,
    USHORT senderListenPort,
    PTRANSPORT_LISTEN_ENDPOINT to, 
    LEASE_BLOCKING_ACTION_TYPE blockingType);

USHORT GetLeaseAgentVersion(USHORT majorVersion, USHORT minorVersion);

//
// Transportation blocking descriptions (test used only)
//
typedef struct _TRANSPORT_BLOCKING_TEST_DESCRIPTOR {
    TRANSPORT_LISTEN_ENDPOINT LocalSocketAddress;
    BOOLEAN FromAny;
    TRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress;
    BOOLEAN ToAny;
    LEASE_BLOCKING_ACTION_TYPE BlockingType;
    LARGE_INTEGER Instance;
    WCHAR Alias[TEST_TRANSPORT_ALIAS_LENGTH];
} TRANSPORT_BLOCKING_TEST_DESCRIPTOR, *PTRANSPORT_BLOCKING_TEST_DESCRIPTOR;

VOID
GetLeasingApplicationTTL(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LONG RequestTTL,
    __out PLONG TimeToLive
    );

NTSTATUS
LeaseLayerEvtIoDeviceControl (
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength,
    __in ULONG IoControlCode
    );

PDRIVER_CONTEXT LeaseLayerDriverGetContext();
BOOL WINAPI
LeasingApplicationChangeRefCount(
        POVERLAPPED_LEASE_LAYER_EXTENSION Overlapped,
HANDLE LeasingApplicationHandle,
        BOOL IsIncrement
);

void RemoveAllLeasingApplication();

NTSTATUS
CrashLeasingApplication(
    __in WDFREQUEST Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    );

const LPWSTR GetLeaseIoCode(
    __in ULONG IoControlCode
    );

VOID DestructArbitrationNeutralRemoteLeaseAgents(__in PLEASE_AGENT_CONTEXT LeaseAgentContext);
VOID DoMaintenance(__in PDRIVER_CONTEXT DriverContext);

#endif  // _LEASLAYR_H_
