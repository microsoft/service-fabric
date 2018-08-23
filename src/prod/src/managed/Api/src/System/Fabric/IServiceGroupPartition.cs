// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Service group members inside a service group can cast the provided <see cref="System.Fabric.IStatefulServicePartition" /> or 
    /// <see cref="System.Fabric.IStatelessServicePartition" /> to a <see cref="System.Fabric.IServiceGroupPartition" /> to access the methods that are specific to members within service groups.</para>
    /// </summary>
    public interface IServiceGroupPartition
    {
        /// <summary>
        /// <para>Enables the member to get direct access to the other members of the service group.</para>
        /// </summary>
        /// <param name="name">
        /// <para>The <c>fabric:/</c> name of the member to resolve.</para>
        /// </param>
        /// <typeparam name="T">
        /// <para>The type of the service member that should be resolved.</para>
        /// </typeparam>
        /// <returns>
        /// <para>Returns the member that is specified by name as an object of the specified type.</para>
        /// </returns>
        /// <remarks>
        /// <para>The <see cref="System.Fabric.IServiceGroupPartition.ResolveMember{T}(System.Uri)" /> method enables a service group member to obtain a direct reference to the other members in the group. The direct object-object communication with the other members does not require communication outside of the machine or virtual machine, either for communication with the Naming service to resolve the member or via some external transport to send the actual commands to the member.</para>
        /// </remarks>
        T ResolveMember<T>(Uri name) where T : class;
    }

    internal sealed class ServiceGroupPartitionHelper
    {
        public static T ResolveMember<T>(NativeRuntime.IFabricServiceGroupPartition serviceGroupPartition, Uri name) where T : class
        {
            Requires.Argument("name", name).NotNull();

            object member = Utility.WrapNativeSyncInvoke(() => ServiceGroupPartitionHelper.InternalResolveMember(serviceGroupPartition, name), "IFabricServiceGroupPartition.ResolveMember");

            return member as T;
        }

        private static object InternalResolveMember(NativeRuntime.IFabricServiceGroupPartition serviceGroupPartition, Uri name)
        {
            using (var pin = PinBlittable.Create(name))
            {
                Guid iidIUnknown = new Guid("00000000-0000-0000-C000-000000000046"); // IID_IUnknown;
                return serviceGroupPartition.ResolveMember(pin.AddrOfPinnedObject(), ref iidIUnknown);
            }
        }
    }
}