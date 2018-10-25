// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        interface ILogManagerHandle 
            : public Utilities::IDisposable
        {
            K_SHARED_INTERFACE(ILogManagerHandle)

        public:

            __declspec(property(get = get_Mode)) KtlLoggerMode Mode;
            virtual KtlLoggerMode get_Mode() const = 0;

            /// <summary>
            /// Close the log manager asynchronously
            /// 
            ///  NOTE: Any currently opened IPhysicalLog instances that were created/opened through a current ILogManager
            ///        will remain open - they have an independent life-span once created.  
            /// 
            /// </summary>
            /// <param name="cancellationToken">Used to cancel the OpenAsync operation</param>
            virtual ktl::Awaitable<NTSTATUS> CloseAsync(__in ktl::CancellationToken const & cancellationToken) = 0;

            /// <summary>
            /// Abort the log manager handle synchronously.
            /// 
            ///  NOTE: Any currently opened IPhysicalLog instances that were created/opened through a current ILogManager
            ///        will remain open - they have an independent life-span once created.
            ///        
            /// </summary>
            virtual VOID Abort() = 0;

            /// <summary>
            /// Create a new physical log after which the container storage structures exists for the new physical log.
            /// </summary>
            /// <param name="pathToCommonContainer">Supplies fully qualified pathname to the logger file</param>
            /// <param name="physicalLogId">Supplies ID (Guid) of the log container to create</param>
            /// <param name="containerSize">Supplies the desired size of the log container in bytes</param>
            /// <param name="maximumNumberStreams">Supplies the maximum number of streams that can be created in the log container</param>
            /// <param name="maximumLogicalLogBlockSize">Supplies the maximum size of a physical log stream block to write in a shared intermediate 
            /// log containers - stream that write blocks larger than this may not use such a shared container</param>
            /// <param name="creationFlags"></param>
            /// <param name="cancellationToken">Used to cancel the CreatePhysicalLogAsync operation</param>
            /// <returns>An open IPhysicalLog handle used to further manipulate the created physical log</returns>
            virtual ktl::Awaitable<NTSTATUS> CreateAndOpenPhysicalLogAsync(
                __in KString const & pathToCommonContainer,
                __in KGuid const & physicalLogId,
                __in LONGLONG containerSize,
                __in ULONG maximumNumberStreams,
                __in ULONG maximumLogicalLogBlockSize,
                __in LogCreationFlags creationFlags,
                __in ktl::CancellationToken const & cancellationToken,
                __out IPhysicalLogHandle::SPtr& physicalLog) = 0;

            // Create and open the physical log using the settings provided by FabricNode.
            virtual ktl::Awaitable<NTSTATUS> CreateAndOpenPhysicalLogAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out IPhysicalLogHandle::SPtr& physicalLog) = 0;

            /// <summary>
            /// Opens an existing physical log. Once this operation completes, the returned IPhysicalLog represents the 
            /// underlying physical log state that has been recovered and is ready to use.
            /// </summary>
            /// <param name="pathToCommonPhysicalLog">Fully qualified pathname to the log container file</param>
            /// <param name="physicalLogId">Supplies ID (Guid) of the log container</param>
            /// <param name="cancellationToken">Used to cancel the OpenPhysicalLogAsync operation</param>
            /// <returns>An open IPhysicalLog handle used to further manipulate the opened physical log</returns>
            virtual ktl::Awaitable<NTSTATUS> OpenPhysicalLogAsync(
                __in KString const & pathToCommonPhysicalLog,
                __in KGuid const & physicalLogId,
                __in ktl::CancellationToken const & cancellationToken,
                __out IPhysicalLogHandle::SPtr& physicalLog) = 0;

            // Open the physical log using the settings provided by FabricNode.
            virtual ktl::Awaitable<NTSTATUS> OpenPhysicalLogAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out IPhysicalLogHandle::SPtr& physicalLog) = 0;

            /// <summary>
            /// Deletes an existing log container
            /// </summary>
            /// <param name="pathToCommonPhysicalLog">Fully qualified pathname to the log container file</param>
            /// <param name="physicalLogId">Supplies ID (Guid) of the log container</param>
            /// <param name="cancellationToken">Used to cancel the DeletePhysicalLogAsync operation</param>
            virtual ktl::Awaitable<NTSTATUS> DeletePhysicalLogAsync(
                __in KString const & pathToCommonPhysicalLog,
                __in KGuid const & physicalLogId,
                __in ktl::CancellationToken const & cancellationToken) = 0;

            // Delete the physical log using the settings provided by FabricNode.
            virtual ktl::Awaitable<NTSTATUS> DeletePhysicalLogAsync(__in ktl::CancellationToken const & cancellationToken) = 0;
        };
    }
}
