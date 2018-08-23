// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Indicates the type of fault that a service reports: invalid, transient or permanent. </para>
    /// </summary>
    /// <remarks>
    /// <para>Services can report faults during runtime by using the <see cref="System.Fabric.IServicePartition.ReportFault(System.Fabric.FaultType)" /> method to indicate the type of fault.</para>
    /// </remarks>
    public enum FaultType
    {
        /// <summary>
        /// <para>The type is invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_FAULT_TYPE.FABRIC_FAULT_TYPE_INVALID,
        /// <summary>
        /// <para>A permanent fault is a fault that the replica cannot recover from. This type of fault indicates that the replica can make no further progress and should be removed and replaced. </para>
        /// </summary>
        /// <remarks>
        /// <para>An example of a permanent fault would be a persistent stateful service that tries to write information to disk and determines that the disk had been removed or was unusable. Calling <see cref="System.Fabric.IServicePartition.ReportFault(System.Fabric.FaultType)" /> and reporting a permanent fault result in the service to be Aborted via <languageKeyword>IStatefulServiceReplica.</languageKeyword><see cref="System.Fabric.IStatefulServiceReplica.Abort" /> or <languageKeyword>IStatelessServiceInstance.</languageKeyword><see cref="System.Fabric.IStatelessServiceInstance.Abort" /> without a chance to gracefully clean up state or complete operations. Therefore, if any cleanup or other long-running work is necessary, it should be performed before <see cref="System.Fabric.IServicePartition.ReportFault(System.Fabric.FaultType)" /> is called. Note that the distinction between permanent and transient faults is useful mainly for persistent stateful services. Other than the call sequence, the effects on other service types are the same: the replica or instance is removed, all state at that replica or instance is lost, and the replica or instance is recreated, potentially in a different location.</para>
        /// </remarks>
        Permanent = NativeTypes.FABRIC_FAULT_TYPE.FABRIC_FAULT_TYPE_PERMANENT,
        /// <summary>
        /// <para>A transient fault indicates that there is some temporary condition which prevents the replica from making further progress or from processing further user requests. </para>
        /// </summary>
        /// <remarks>
        /// <para>An example of a transient fault is a service that determines that a portion of its state or some reference file is corrupted, but can be repaired if the service were to be re-initialized. In this case, the service uses the <see cref="System.Fabric.IServicePartition.ReportFault(System.Fabric.FaultType)" /> method to report a transient fault. Reporting a transient fault closes the service via <languageKeyword>IStatefulServiceReplica.</languageKeyword><see cref="System.Fabric.IStatefulServiceReplica.CloseAsync(System.Threading.CancellationToken)" /> or <languageKeyword>IStatelessServiceInstance.</languageKeyword><see cref="System.Fabric.IStatelessServiceInstance.CloseAsync(System.Threading.CancellationToken)" />. Note that for stateless and stateful services, volatile transient faults are not very useful because state is not preserved across the failure. For these services, whether to use transient or permanent faults is dependent on whether the service should be gracefully closed asynchronously with cleanup or ungracefully closed with a synchronous <languageKeyword>IStatefulServiceReplica.</languageKeyword><see cref="System.Fabric.IStatefulServiceReplica.Abort" /> or <languageKeyword>IStatelessServiceInstance.</languageKeyword><see cref="System.Fabric.IStatelessServiceInstance.Abort" /> method.</para>
        /// </remarks>
        Transient = NativeTypes.FABRIC_FAULT_TYPE.FABRIC_FAULT_TYPE_TRANSIENT,
    }
}