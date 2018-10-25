// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <ktl.h>
#include <ktrace.h>

#if defined(PLATFORM_UNIX)			
#include <palerr.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <errno.h>
#include <linux/unistd.h>
#include <linux/fs.h>
#include <linux/fiemap.h>
#include <linux/unistd.h>
#include <sys/ioctl.h>
#endif

#include "ValidateLinuxLog.h"

NTSTATUS AsyncValidateLinuxLogForDDContext::Create(
	__out AsyncValidateLinuxLogForDDContext::SPtr& Context,
	__in KAllocator& Allocator,
	__in ULONG AllocationTag
	)
{
	NTSTATUS status;
	AsyncValidateLinuxLogForDDContext::SPtr context;

	Context = nullptr;

	context = _new(AllocationTag, Allocator) AsyncValidateLinuxLogForDDContext();
	if (context == nullptr)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		KTraceOutOfMemory( 0,
						   status,
						   nullptr, AllocationTag, sizeof(AsyncValidateLinuxLogForDDContext));
		return(status);
	}

	status = context->Status();
	if (! NT_SUCCESS(status))
	{
		return(status);
	}

	Context = Ktl::Move(context);

	return(STATUS_SUCCESS);             
}

VOID
AsyncValidateLinuxLogForDDContext::StartValidate(
	__in LPCWSTR LogFile,
	__in_opt KAsyncContextBase* const ParentAsyncContext,
	__in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
	_LogFile = LogFile;
	Start(ParentAsyncContext, CallbackPtr);
}

VOID
AsyncValidateLinuxLogForDDContext::OnStart(
	)
{
	GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
}

VOID
AsyncValidateLinuxLogForDDContext::Execute()
{
	NTSTATUS status = STATUS_SUCCESS;

#if defined(PLATFORM_UNIX)
	int ret;
	int fd;
	std::string pathToFile(Utf16To8(_LogFile));
	ULONGLONG fileSize;
	size_t fiemap_size;
	struct fiemap query;
	struct fiemap* fiemap;
	struct fiemap_extent *extents = nullptr;
	struct stat fileStat;
	ULONGLONG numExtents;
	KBuffer::SPtr fiemapKBuffer;

	fd = open(pathToFile.c_str(), O_RDWR);
	if (fd == -1)
	{
		ret = errno;
		status = LinuxError::LinuxErrorToNTStatus(ret);
		KTraceFailedAsyncRequest(status, NULL, ret, 0);
		Complete(status);
		return;
	}

	ret = fstat(fd, &fileStat);
	if (ret == -1)
	{
		ret = errno;
		status = LinuxError::LinuxErrorToNTStatus(ret);
		KTraceFailedAsyncRequest(status, NULL, ret, 0);
		Complete(status);
		return;
	}

	fileSize = fileStat.st_size;

	//
	// Figure out the number of new extents
	//
	query.fm_start = 0;
	query.fm_length = fileSize;
	query.fm_flags = FIEMAP_FLAG_SYNC;
	query.fm_extent_count = 0;

	ret = ioctl(fd, FS_IOC_FIEMAP, &query);

	if (ret != 0) {
		status = LinuxError::LinuxErrorToNTStatus(errno);
		KTraceFailedAsyncRequest(status, NULL, fd, errno);
		Complete(status);
		return;
	}

	//
	// fm_mapped_extents should now tell us how much space we need
	//
	numExtents = query.fm_mapped_extents;
	fiemap_size = sizeof(struct fiemap) + (sizeof(struct fiemap_extent) * numExtents);

	status = KBuffer::Create(fiemap_size, fiemapKBuffer, GetThisAllocator());
	if (! NT_SUCCESS(status))
	{
		KTraceFailedAsyncRequest(status, NULL, fd, fiemap_size);
		Complete(status);
		return;
	}

	fiemap = (struct fiemap *)fiemapKBuffer->GetBuffer();

	memset(fiemap, 0, fiemap_size);
	fiemap->fm_start = 0;
	fiemap->fm_length = fileSize;
	fiemap->fm_flags = FIEMAP_FLAG_SYNC;
	fiemap->fm_extent_count = numExtents;

	//
	// Get the actual extents
	//
	ret = ioctl(fd, FS_IOC_FIEMAP, fiemap);

	if (ret != 0) {
		status = LinuxError::LinuxErrorToNTStatus(errno);
		KTraceFailedAsyncRequest(status, NULL, fd, errno);
		Complete(status);
		return;
	}

	extents = fiemap->fm_extents;

	for (ULONG i = 0; i < fiemap->fm_extent_count; i++)
	{
		if ((extents[i].fe_flags & FIEMAP_EXTENT_UNWRITTEN) != 0)
		{
			status = K_STATUS_LOG_DATA_FILE_FAULT;
			KTraceFailedAsyncRequest(status, NULL, fd, errno);
			Complete(status);
			return;                 
		}
	}

	close(fd);
#else
	status = STATUS_SUCCESS;
#endif
	Complete(STATUS_SUCCESS);
}

AsyncValidateLinuxLogForDDContext::AsyncValidateLinuxLogForDDContext()
{
}

AsyncValidateLinuxLogForDDContext::~AsyncValidateLinuxLogForDDContext()
{
}
