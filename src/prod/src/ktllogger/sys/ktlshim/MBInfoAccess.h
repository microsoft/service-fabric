// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

//
// This class provides access the to the metadata block information
// associated with a log container. It abstracts access to the data
// pages within the metadata blocks allowing the physical location to
// change as needed. Underlying the metadata block info are 2 copies of
// the data so that atomic updates of the blocks can be supported. This
// class also abstracts access to these blocks and the atomicity of the
// writes. It also exposes an exclusice lock over the entire
// metadata block to allow callers safe access to the data. The caller
// is expected to provide all locking to ensure that reads, writes and
// cached data are consistent. Typically this would require the
// metadata block user to take a lock before reading a page, updating
// the page and then writing the page.
//
// Applications use this class to read sections of the metadata block.
// A section must be a multiple of the 4KB page size and is the unit of
// read/write atomicity and verification (ie, checksum). The type of
// data held in the section is opaque to the MBInfoAccess class.
// However each section read or written must be preceeded by the MBInfoHeader
// as the MBInfoHeader is a common header used for data verification
// and determination of which copy is the most recent.
//
#pragma once

class MBInfoAccess : public KAsyncContextBase
{
    K_FORCE_SHARED(MBInfoAccess);
    
    public:

        //
        // Create an MBInfoAccess object that can be used to access
        // the metadata block info.
        //
        // Context - returns with the MBInfoAccess object
        //
        // ContainerId - Id for the log container to which the MBInfo
        //     is attached
        //
        // NumberOfBlocks - the number of metadata blocks in which the
        //     MBInfo is broken into. Each block has its own header.
        //
        // BlockSize - the size of each of the metadata blocks.
        //     BlockSize must be a multiple of 4K
        //
        // Allocator - allocator to use
        //
        // AllocationTag - tag used with allocator
        //
        static NTSTATUS Create(
            __out MBInfoAccess::SPtr& Context,
            __in KGuid DiskId,
            __in_opt KString const * const Path,
            __in KtlLogContainerId ContainerId,
            __in ULONG NumberOfBlocks,
            __in ULONG BlockSize,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );


        //
        // This method will initialize a new metadata block by
        // allocating space for it from the underlying physical
        // container. It should only be called when the log container
        // is initially created. If the metadata block has already been
        // initialized then this will return an error.
        //
        VOID StartInitializeMetadataBlock(
            __in_opt KArray<KIoBuffer::SPtr>* const InitializationData,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        );

        //
        // This method will cleanup any resources allocated when the
        // metadata block was initialized.
        //
        VOID StartCleanupMetadataBlock(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        );

        //
        // This method will open access to a previously initialized
        // metadata block. The metadata block will remain open until
        // the metadata block is closed using StartCloseMetadataBlock
        //
        VOID StartOpenMetadataBlock(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        );

        //
        // This method will close access to a previously opened
        // metadata block. 
        //
        VOID StartCloseMetadataBlock(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        );

        //
        // This method will free any memory held for this metadata
        // block in the cache
        //
        VOID DumpCachedReads(
        );

        
// Leave public so unit tests can use MBInfoHeader
        
        //
        // This is the 256 byte header that must preceed any section
        // read or written. The caller should not access the fields in
        // the header as they are for MBInfoAccess class use only
        //
        typedef struct _MBInfoHeader
        {
            static const ULONGLONG Sig = 0xabacadaeeadacaba;
            static const ULONG CurrentVersion = 1;

            // Fixed signature
            ULONGLONG Signature;
            // Offset of this section within the underlying container
            ULONGLONG FileOffset;
            // Generation number of this copy
            ULONGLONG Generation;
            // 64bit checksum for the metadata section
            ULONGLONG BlockChecksum;
            // Size of this section
            ULONG SectionSize;
            // Version
            ULONG Version;
            // Number of bytes for checksum
            ULONG ChecksumSize;
            // Reserved
            ULONG Reserved;
            LONGLONG Timestamp;
            // Padding to 256 bytes
            ULONGLONG Reserved1[25];
        } MBInfoHeader;
        static_assert(sizeof(MBInfoHeader) == 256, "MBInfoHeader must be 256 bytes in size");
        
    private:
        //
        // This struct holds information about each block within the
        // metadata block info
        //
        typedef enum { NotKnown, CopyA, CopyB } CopyTypeEnum;
        
        class BlockInfo : public KObject<BlockInfo>
        {
            KAllocator_Required();

            public:
                BlockInfo(
                    __in KAllocator& Allocator
                );

                ~BlockInfo();

                BlockInfo&
                BlockInfo::operator=(
                    __in const MBInfoAccess::BlockInfo& Source
                    );
                
            public:             
                ULONGLONG LatestGenerationNumber;
                CopyTypeEnum LatestCopy;
                KIoBuffer::SPtr IoBuffer;

                KAllocator* const   _Allocator;             
        };

        static const ULONG GUID_STRING_LENGTH = 40;
        
        NTSTATUS
        GenerateFileName(
            __out KWString& FileName
            );
        
        //
        // This method validates the section data read
        //
        NTSTATUS ValidateSection(
            __in KIoBuffer::SPtr& SectionData,
            __in CopyTypeEnum CopyType,
            __in ULONGLONG FileOffset,
            __out MBInfoHeader*& Header
        );

        //
        // This method initializes the header and sets the checksum
        // within it
        //
        NTSTATUS ChecksumSection(
            __in KIoBuffer::SPtr& SectionData,
            __in CopyTypeEnum CopyType,
            __in ULONGLONG GenerationNumber,
            __in ULONGLONG FileOffset
        );

        //
        // This method determines if the file offset is valid for use
        //
        BOOLEAN IsValidFileOffset(
            __in ULONGLONG FileOffset
        )
        {
            if (((FileOffset % _BlockSize) != 0) ||
                 ((FileOffset + _BlockSize) > _TotalBlockSize))
            {
                return(FALSE);
            }
            
            return(TRUE);
        }

        //
        // This method will compute the actual file offset based on the
        // copy to be used
        //
        ULONGLONG ActualFileOffset(
            __in CopyTypeEnum CopyType,
            __in ULONGLONG FileOffset
        )
        {
            KInvariant((CopyType == CopyA) || (CopyType == CopyB));

            return( (CopyType == CopyA) ? FileOffset : (FileOffset + _TotalBlockSize) );
        }

        //
        // This method flags the whole metadata block as having an
        // error. All subsequent operations fail with this error
        //
        VOID SetError(
            NTSTATUS Status
        )
        {
            _ObjectState = InError;
            _LastError = Status;
        }
        
    protected:
        VOID
        OnStart(
            );

        VOID
        OnReuse(
            );

    private:
        enum { Unassigned,
               InitializeInitial = 0x100, InitializeDeleteFile, InitializeCreateFile, InitializeAllocateSpace,
                                          InitializeWriteCopyA, InitializeWriteCopyB,
                                          InitializeCloseFile,
                                          
               CleanupInitial =    0x200, CleanupFileRemoved,
               
               OpenInitial =       0x300, OpenOpenFile,         OpenReadBlocks,
                                          OpenFreeReadBuffers,
                                          
               CloseInitial =      0x400, CloseFileClosed,
               
               Completed, CompletedWithError } _OperationState;

        VOID DoComplete(
            __in NTSTATUS Status
            );
                
        VOID InitializeMetadataFSM(
            __in NTSTATUS Status
            );

        VOID CleanupMetadataFSM(
            __in NTSTATUS Status
            );

        VOID OpenMetadataFSM(
            __in NTSTATUS Status
            );

        VOID CloseMetadataFSM(
            __in NTSTATUS Status
            );

        VOID DispatchToFSM(
            __in NTSTATUS Status                            
            );
        
        VOID MBInfoAccessCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );
        
    private:
        //
        // General members
        //
        enum { Closed,        // No access available; this is initial state or state after object closed
               Opened,        // Access available; this is state after a successful open
               Initializing,  // No access available; the file is being initialized
               Opening,       // No access available; the file is being opened
               InError        // Error encountered accessing MB info
                         } _ObjectState;

        KGuid _DiskId;
        KString::CSPtr _Path;
        KtlLogContainerId _ContainerId;

        KBlockFile::SPtr _File;
        
        ULONG _NumberOfBlocks;
        ULONG _BlockSize;
        ULONG _TotalBlockSize;
        KArray<BlockInfo> _BlockInfo;
        
        NTSTATUS _LastError;    // If object is InError ObjectState then this is the error that caused it
        
        KAsyncEvent _ExclAccessEvent;

        //
        // Parameters to api
        //
        KArray<KIoBuffer::SPtr>* _InitializationData;

        //
        // Members needed for functionality
        //
        ULONG _CompletionCounter;
        NTSTATUS _FinalStatusOfWrites;
        NTSTATUS _FinalStatus;

    public:
        class AsyncAcquireExclusiveContext;
        
    private:
        AsyncAcquireExclusiveContext* _OwningAsync;


    public:

        //
        // This async is used to read a section from the metadata block
        //
        class AsyncReadSectionContext : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncReadSectionContext);

            friend MBInfoAccess;
            
            public:
                VOID StartReadSection(
                    __in ULONGLONG FileOffset,
                    __in ULONG SectionSize,
                    __in BOOLEAN CacheResults,
                    __out KIoBuffer::SPtr* SectionData,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                );

            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                enum { NotAssigned, Initial, DetermineBlockToRead, GetBlockA, GetBlockB, GetLatestBlock, ReadBlockFromDisk,
                       Completed, CompletedWithError } _State;

                VOID DoComplete(
                    __in NTSTATUS Status
                    );

                VOID ReadSectionFSM(
                    __in NTSTATUS Status
                    );
            
                VOID ReadSectionCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
            private:
                //
                // General members
                //
                MBInfoAccess::SPtr _MBInfo;         
                
                //
                // Parameters to api
                //
                BOOLEAN _CacheResults;
                ULONGLONG _FileOffset;
                ULONG _SectionSize;
                KIoBuffer::SPtr* _SectionData;
                                
                //
                // Members needed for functionality
                //
                BlockInfo* _BlockInfo;
                KIoBuffer::SPtr _CopyAIoBuffer;
                KIoBuffer::SPtr _CopyBIoBuffer;
        };
        
        friend AsyncReadSectionContext;
        
        NTSTATUS CreateAsyncReadSectionContext(__out AsyncReadSectionContext::SPtr& Context);
            
        //
        // This async is used to write a section from the metadata
        // block. 
        //
        class AsyncWriteSectionContext : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncWriteSectionContext);
            
            friend MBInfoAccess;
            
            public:             
                VOID StartWriteSection(
                    __in ULONGLONG FileOffset,
                    __in KIoBuffer::SPtr& SectionData,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                );
            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                enum { NotAssigned, Initial, DetermineBlockToWrite, WriteBlock,
                       Completed, CompletedWithError } _State;

                VOID DoComplete(
                    __in NTSTATUS Status
                    );

                VOID WriteSectionFSM(
                    __in NTSTATUS Status
                    );
            
                VOID WriteSectionCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                

            private:
                //
                // General members
                //
                MBInfoAccess::SPtr _MBInfo;         
                
                //
                // Parameters to api
                //
                ULONGLONG _FileOffset;
                KIoBuffer::SPtr _SectionData;
                                
                //
                // Members needed for functionality
                //
                BlockInfo* _BlockInfo;
                MBInfoAccess::CopyTypeEnum _CopyType;
                ULONGLONG _NextGenerationNumber;
        };

        friend AsyncWriteSectionContext;
        
        NTSTATUS CreateAsyncWriteSectionContext(__out AsyncWriteSectionContext::SPtr& Context);

        //
        // This async is used to acquire an exclusive lock on the
        // metadata info blocks
        //
        class AsyncAcquireExclusiveContext : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncAcquireExclusiveContext);
            
            friend MBInfoAccess;
            
            public:             
                VOID StartAcquireExclusive(
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                );

                NTSTATUS ReleaseExclusive(
                );
                
            protected:
                VOID OnStart();
                VOID OnCancel();
                VOID OnReuse();

            private:
                VOID AcquireExclusiveCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );              
                
            private:
                //
                // General members
                //
                MBInfoAccess::SPtr _MBInfo;         
                
                //
                // Parameters to api
                //
                                
                //
                // Members needed for functionality
                //
                BOOLEAN _OwningExclusive;
                KAsyncEvent::WaitContext::SPtr _ExclWaitContext;
        };

        friend AsyncAcquireExclusiveContext;
        
        NTSTATUS CreateAsyncAcquireExclusiveContext(__out AsyncAcquireExclusiveContext::SPtr& Context);

};

//
// This class accesses the metadata block information that is associated
// with a shared log container.
//
class SharedLCMBInfoAccess : public KAsyncContextBase
{
    K_FORCE_SHARED(SharedLCMBInfoAccess);

    public:
        //
        // This creates an object that can be used to interact with the
        // shared log container metadata block information.
        //
        static NTSTATUS Create(
            __out SharedLCMBInfoAccess::SPtr& Context,
            __in KGuid DiskId,
            __in_opt KString const * const Path,
            __in KtlLogContainerId ContainerId,
            __in ULONG MaxNumberStreams,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );

        //
        // Each entry contains the extra information required for a logical
        // log stream. It is padded to 1024 bytes for future expansion and
        // ease of use. There are 63 entries in each section with a header
        // of 1K which means each section is 64K in size. The entire
        // metadata block can be composed of many different sections.
        //
        // NOTE: SharedLCEntryWithoutPadding and SharedLCEntry along
        // with SharedLCSectionWithoutPadding and SharedLCSection must
        // be kept in sync with each other to ensure correct alignment
        // and padding of entries.
        //
        static const ULONG _EntriesPerSection = 63;
        inline ULONG GetEntriesPerSection()
        {
            return(_EntriesPerSection);
        }

        enum DispositionFlags
        {
            FlagDeleted = 0,            
            FlagCreating = 1,
            FlagCreated = 2,
            FlagDeleting = 3
        };
                
    private:
        typedef struct
        {
            KtlLogStreamId LogStreamId;
            ULONG Index;
            ULONG MaxLLMetadataSize;
            ULONG MaxRecordSize;
            LONGLONG StreamSize;
            DispositionFlags Flags;
            WCHAR AliasName[KtlLogContainer::MaxAliasLength+1];
            WCHAR PathToDedicatedContainer[KtlLogManager::MaxPathnameLengthInChar];
            UCHAR Padding[1];
        } SharedLCEntryWithoutPadding;

        //
        // Ensure that max pathname fits within an entry
        //
        static const ULONG _SpaceAvailableForPathname = 1024 - FIELD_OFFSET(SharedLCEntryWithoutPadding, PathToDedicatedContainer);
        static_assert(KtlLogManager::MaxPathnameLengthInChar <= (_SpaceAvailableForPathname/sizeof(WCHAR)), "KtlLogManager::MaxPathnameLengthInChar is too large");

    public:
        typedef struct
        {
            KtlLogStreamId LogStreamId;
            ULONG Index;
            ULONG MaxLLMetadataSize;
            ULONG MaxRecordSize;
            LONGLONG StreamSize;
            DispositionFlags Flags;
            WCHAR AliasName[KtlLogContainer::MaxAliasLength+1];
            WCHAR PathToDedicatedContainer[KtlLogManager::MaxPathnameLengthInChar];
            UCHAR PaddingTo1024Bytes[1024 - FIELD_OFFSET(SharedLCEntryWithoutPadding, Padding)];
        } SharedLCEntry;

        typedef struct
        {
            MBInfoAccess::MBInfoHeader Header;
            ULONG SectionIndex;
            ULONG TotalNumberSections;
            ULONG TotalMaxNumberEntries;
            UCHAR Padding[1];
        } SharedLCSectionWithoutPadding;
        
        typedef struct
        {
            MBInfoAccess::MBInfoHeader Header;
            ULONG SectionIndex;
            ULONG TotalNumberSections;
            ULONG TotalMaxNumberEntries;
            UCHAR PadTo1024Bytes[1024 - FIELD_OFFSET(SharedLCSectionWithoutPadding, Padding)];
            SharedLCEntry Entries[_EntriesPerSection];
        } SharedLCSection;
        static_assert( (sizeof(SharedLCSection) == (64 * 1024)), "SharedLCSection must be 64K in size" );

    private:
        BOOLEAN IsValidSection(
            __in SharedLCSection* Section,
            __in ULONG EntryIndex
        )
        {
            ULONG sectionIndex = EntryIndex / _EntriesPerSection;

            //
            // There are 2 cases where a section can be considered
            // valid. The first is when, right after initialization,
            // that the fields are all 0. The second is after the first
            // time the section has been written that the fields are
            // set correctly.
            //
            return(( (Section->SectionIndex == 0) &&
                     (Section->TotalNumberSections == 0) &&
                     (Section->TotalMaxNumberEntries == 0) )  ||
                   ( (Section->SectionIndex == sectionIndex) &&
                     (Section->TotalNumberSections == _NumberOfBlocks) &&
                     (Section->TotalMaxNumberEntries == _MaxNumberEntries) ));
        }

        VOID SetSectionHeader(
            __in SharedLCSection* Section,
            __in ULONG EntryIndex
        )
        {
            ULONG sectionIndex = EntryIndex / _EntriesPerSection;
                                
            Section->SectionIndex = sectionIndex;
            Section->TotalNumberSections = _NumberOfBlocks;
            Section->TotalMaxNumberEntries = _MaxNumberEntries;
        }

    public:
        //
        // This method will initialize a new SharedLCMB metadata block by
        // allocating space for it from the underlying physical
        // container. It should only be called when the log container
        // is initially created. If the metadata block has already been
        // initialized then this will return an error
        //
        VOID StartInitializeMetadataBlock(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        );

        //
        // This method will cleanup any resources allocated when the
        // SharedLCMB metadata block was initialized. It should only be
        // called when the container associated with it is deleted.
        //
        VOID StartCleanupMetadataBlock(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        );

        //
        // This method will open access to a previously initialized
        // SharedLCMB metadata block. The metadata block will remain open until
        // the metadata block is closed using StartCloseMetadataBlock.
        //

        //
        // As part of the open process, all sections are processed to
        // ensure validity and then a callback is made for each entry
        // in the table so the caller can do any processing on each
        // stream found.
        //

        class LCEntryEnumAsync;
        
        typedef KDelegate<NTSTATUS(
                LCEntryEnumAsync& Context
            )> LCEntryEnumCallback;
        
        class LCEntryEnumAsync : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(LCEntryEnumAsync);

            friend SharedLCMBInfoAccess;

            public:
                SharedLCEntry& GetEntry()
                {
                    return(_Entry);
                }

                VOID CompleteIt(NTSTATUS Status)
                {
                    Complete(Status);
                }
            
            private:
                VOID StartCallback(
                    __in SharedLCEntry& Entry,
                    __in LCEntryEnumCallback EnumCallback,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                );

            protected:
                VOID OnStart();

            private:

                //
                // Parameters to API
                //
                SharedLCEntry _Entry;
                LCEntryEnumCallback _EnumCallback;
        };
        
        VOID StartOpenMetadataBlock(
            __in_opt LCEntryEnumCallback EnumCallback,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        );

        //
        // This method will close access to a previously opened
        // SharedLCMB metadata block. 
        //
        VOID StartCloseMetadataBlock(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        );

    private:        
        NTSTATUS AllocateAvailableEntry(
            __out ULONG& Index
            );
        
        VOID FreeAvailableEntry(
            __in ULONG Index
        );

        ULONG EntryIndexToFileOffset(
            __in ULONG Index
        )
        {
            return( (Index/_EntriesPerSection) * sizeof(SharedLCSection) );
        }

        ULONG EntryIndexToSectionIndex(
            __in ULONG Index
        )
        {
            return( (Index % _EntriesPerSection) );
        }
                
        VOID
        OnStart(
            )  override ;

        VOID
        OnReuse(
            ) override ;

    private:
        enum { Unassigned,
               InitializeInitial = 0x100, InitializeCreateFile, 
                                          
               CleanupInitial =    0x200, CleanupDeleteFile,
               
               OpenInitial =       0x300, OpenCloseOnError,          OpenOpenFile,       OpenProcessSectionData,  
                                          
               CloseInitial =      0x400, CloseFileClosed,
               
               Completed, CompletedWithError } _OperationState;


        VOID DoComplete(
            __in NTSTATUS Status
            );
                
        VOID InitializeMetadataFSM(
            __in NTSTATUS Status,
            __in KAsyncContextBase* Async
            );

        VOID CleanupMetadataFSM(
            __in NTSTATUS Status,
            __in KAsyncContextBase* Async
            );

        VOID OpenMetadataFSM(
            __in NTSTATUS Status,
            __in KAsyncContextBase* Async
            );

        VOID CloseMetadataFSM(
            __in NTSTATUS Status,
            __in KAsyncContextBase* Async
            );

        VOID DispatchToFSM(
            __in NTSTATUS Status,                           
            __in KAsyncContextBase* Async
            );
        
        VOID SharedLCMBInfoAccessCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );
        

    private:
        class LCSectionAsync : public KAsyncContextBase
        {
            K_FORCE_SHARED(LCSectionAsync);

            public:
                LCSectionAsync(
                    __in SharedLCMBInfoAccess& LCMBInfo
                );
                
                VOID
                StartProcessLCSection(
                    __in ULONG BlockIndex,
                    __in_opt LCEntryEnumCallback EnumCallback,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr                                                
                    );
                
            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;
                
                VOID
                OnCompleted(
                    ) override ;
                
            private:
                enum { Initial, ReadLCSection, ProcessLCSection } _State;
                
                VOID FSMContinue(
                    __in NTSTATUS Status
                    );
                
                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );


                //
                // General members
                //
                SharedLCMBInfoAccess::SPtr _LCMBInfo;
                LCEntryEnumAsync::SPtr _EnumAsync;
                MBInfoAccess::AsyncReadSectionContext::SPtr _ReadAsync;
                
                //
                // Parameters to api
                //
                ULONG _BlockIndex;
                LCEntryEnumCallback _EnumCallback;
                
                //
                // Members needed for functionality (reused)
                //
                ULONG _Index;
                ULONG _EntryIndex;
                KIoBuffer::SPtr _SectionData;
        };
        
        NTSTATUS
        CreateLCSectionAsync(
            __out LCSectionAsync::SPtr& Context
            );
        

    public:
        inline ULONG GetBlockSize()
        {
            return(_BlockSize);
        }
        
        inline VOID MarkAvailableEntry(
            __in ULONG EntryIndex,
            __in BOOLEAN Available
        )
        {
            // CONSIDER: Is this thread safe ?
             _AvailableEntries[EntryIndex] = Available;
        }
                
                
    private:
        //
        // General members
        //
        MBInfoAccess::SPtr _MBInfo;

        ULONG _MaxNumberStreams;
        ULONG _BlockSize;
        ULONG _NumberOfBlocks;
        ULONG _MaxNumberEntries;

        KSpinLock _Lock;
        KArray<BOOLEAN> _AvailableEntries;

        //
        // Parameters to api
        //
        LCEntryEnumCallback _EnumCallback;

        //
        // Members needed for functionality
        //
        NTSTATUS _FinalStatus;
        ULONG _CompletionCounter;
        ULONG _NextBlockIndexToRead;

        static const ULONG _ParallelCount = 16;
        LCSectionAsync::SPtr _SectionAsync[_ParallelCount];
        
        
    public:     
        //
        // This async is used to add a new stream entry
        //
        class AsyncAddEntryContext : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncAddEntryContext);

            friend SharedLCMBInfoAccess;
            
            public:             
                VOID StartAddEntry(
                    __in KtlLogStreamId StreamId,
                    __in ULONG MaxLLMetadataSize,
                    __in ULONG MaxRecordSize,
                    __in LONGLONG StreamSize,
                    __in DispositionFlags Flags,
                    __in KStringView const * DedicatedContainerPath,
                    __in KStringView const * Alias,
                    __out ULONG* EntryIndex,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                );

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

            private:
                enum { NotAssigned, InitialError, Initial, AcquireExclusiveLock, FindFreeEntry, WriteSection, 
                       Completed, CompletedWithError } _State;

                VOID DoComplete(
                    __in NTSTATUS Status
                    );

                VOID AddEntryFSM(
                    __in NTSTATUS Status
                    );
            
                VOID AddEntryCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
            private:
                //
                // General members
                //
                SharedLCMBInfoAccess::SPtr _SLCMB;         
                MBInfoAccess::AsyncAcquireExclusiveContext::SPtr _MBInfoLock;
                MBInfoAccess::AsyncWriteSectionContext::SPtr _WriteContext;             
                MBInfoAccess::AsyncReadSectionContext::SPtr _ReadContext;               
                
                //
                // Parameters to api
                //
                ULONG _MaxLLMetadataSize;
                ULONG _MaxRecordSize;
                LONGLONG _StreamSize;
                DispositionFlags _Flags;
                KtlLogStreamId _StreamId;
                KString::CSPtr _DedicatedContainerPath;
                KString::CSPtr _Alias;
                ULONG* _EntryIndexPtr;
                                
                //
                // Members needed for functionality
                //
                NTSTATUS _FinalStatus;
                ULONG _EntryIndex;
                KIoBuffer::SPtr _EntrySection;
        };
        
        friend AsyncAddEntryContext;
        
        NTSTATUS CreateAsyncAddEntryContext(__out AsyncAddEntryContext::SPtr& Context);

                
        //
        // This async is used to update or remove a new stream entry
        //
        class AsyncUpdateEntryContext : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncUpdateEntryContext);

            friend SharedLCMBInfoAccess;
            
            public:             
                VOID StartUpdateEntry(
                    __in KtlLogStreamId StreamId,
                    __in KStringView const * DedicatedContainerPath,
                    __in KStringView const * Alias,
                    __in DispositionFlags Flags,
                    __in BOOLEAN RemoveEntry,
                    __in ULONG EntryIndex,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                );

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

            private:
                enum { NotAssigned, InitialError, Initial, AcquireExclusiveLock, FindEntry, WriteSection, 
                       Completed, CompletedWithError } _State;

                VOID DoComplete(
                    __in NTSTATUS Status
                    );

                VOID UpdateEntryFSM(
                    __in NTSTATUS Status
                    );
            
                VOID UpdateEntryCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
            private:
                //
                // General members
                //
                SharedLCMBInfoAccess::SPtr _SLCMB;         
                MBInfoAccess::AsyncAcquireExclusiveContext::SPtr _MBInfoLock;
                MBInfoAccess::AsyncWriteSectionContext::SPtr _WriteContext;             
                MBInfoAccess::AsyncReadSectionContext::SPtr _ReadContext;               
                
                //
                // Parameters to api
                //
                KtlLogStreamId _StreamId;
                KString::CSPtr _DedicatedContainerPath;
                KString::CSPtr _Alias;
                DispositionFlags _Flags;
                BOOLEAN _RemoveEntry;
                ULONG _EntryIndex;
                                
                //
                // Members needed for functionality
                //
                NTSTATUS _FinalStatus;
                KIoBuffer::SPtr _EntrySection;
        };
        
        friend AsyncUpdateEntryContext;
        
        NTSTATUS CreateAsyncUpdateEntryContext(__out AsyncUpdateEntryContext::SPtr& Context);
};

//
// This class accesses the metadata block information that is associated
// with a dedicated log container.
//
class DedicatedLCMBInfoAccess : public KAsyncContextBase
{
    K_FORCE_SHARED(DedicatedLCMBInfoAccess);

    public:
        //
        // This creates an object that can be used to interact with the
        // dedicated log container metadata block information.
        //
        static NTSTATUS Create(
            __out DedicatedLCMBInfoAccess::SPtr& Context,
            __in KGuid DiskId,
            __in_opt KString const * const Path,                              
            __in KtlLogContainerId ContainerId,
            __in ULONG MaxMetadataSize,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );

        void CompleteRequest(__in NTSTATUS Status)
        {
            Complete(Status);
        }

    private:
        //
        // The metadata block has only a single section which is written in
        // its entirety. The fixed header is 4K and the data following
        // is the metadata written by the application
        //
        typedef struct
        {
            MBInfoAccess::MBInfoHeader Header;
            ULONG MaxMetadataSize;
            ULONG SecurityDescriptorSize;
            ULONG ActualMetadataSize;
            UCHAR SecurityDescriptor[1];
        } DedicatedLCHeaderNoPadding;

    public:
        static const int _MaxSecurityDescriptorSize = 0x1000 - FIELD_OFFSET(DedicatedLCHeaderNoPadding, SecurityDescriptor);

    private:
        typedef struct
        {
            MBInfoAccess::MBInfoHeader Header;
            ULONG MaxMetadataSize;
            ULONG SecurityDescriptorSize;
            ULONG ActualMetadataSize;
            UCHAR SecurityDescriptor[_MaxSecurityDescriptorSize];
        } DedicatedLCHeader;
        static_assert( (sizeof(DedicatedLCHeader) == 0x1000), "DedicatedLCHeader must be 4K in size" );     

        BOOLEAN IsValidHeader(
            __in DedicatedLCHeader* Header
        )
        {
            return( (Header->MaxMetadataSize == _MaxMetadataSize) &&
                    (Header->SecurityDescriptorSize <= _MaxSecurityDescriptorSize) &&
                    ((Header->ActualMetadataSize <= _MaxMetadataSize) ||
                     (Header->ActualMetadataSize == (ULONG)-1)));
        }
        
    public:
        //
        // This method will initialize a new DedicatedLCMB metadata block by
        // allocating space for it from the underlying physical
        // container. It should only be called when the dedicated log container
        // is initially created. If the metadata block has already been
        // initialized then this will return an error
        //
        VOID StartInitializeMetadataBlock(
            __in KBuffer::SPtr& SecurityDescriptor,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        );

        //
        // This method will cleanup any resources allocated when the
        // DedicatedLCMB metadata block was initialized. It should only be
        // called when the container associated with it is deleted.
        //
        VOID StartCleanupMetadataBlock(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        );

        //
        // This method will open access to a previously initialized
        // DedicatedLCMB metadata block. The metadata block will remain open until
        // the metadata block is closed using StartCloseMetadataBlock.
        //        
        VOID StartOpenMetadataBlock(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        );

        //
        // This method will close access to a previously opened
        // DedicatedLCMB metadata block. 
        //
        VOID StartCloseMetadataBlock(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        );

    protected:
        VOID
        OnStart(
            )  override ;

        VOID
        OnReuse(
            ) override ;

    private:
        enum { Unassigned,
               InitializeInitial = 0x100, InitializeCreateFile,
                                          
               CleanupInitial =    0x200, CleanupDeleteFile,

               OpenInitial =       0x300, OpenCloseOnError,            OpenOpenFile,                OpenReadHeader,
                                          
               CloseInitial =      0x400, CloseFileClosed,
               
               Completed, CompletedWithError } _OperationState;

        VOID DoComplete(
            __in NTSTATUS Status
            );
                
        VOID InitializeMetadataFSM(
            __in NTSTATUS Status,
            __in KAsyncContextBase* Async
            );

        VOID CleanupMetadataFSM(
            __in NTSTATUS Status,
            __in KAsyncContextBase* Async
            );

        VOID OpenMetadataFSM(
            __in NTSTATUS Status,
            __in KAsyncContextBase* Async
            );

        VOID CloseMetadataFSM(
            __in NTSTATUS Status,
            __in KAsyncContextBase* Async
            );

        VOID DispatchToFSM(
            __in NTSTATUS Status,                           
            __in KAsyncContextBase* Async
            );
        
        VOID DedicatedLCMBInfoAccessCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );
        
    private:
        //
        // General members
        //
        MBInfoAccess::SPtr _MBInfo;
        ULONG _MaxMetadataSize;
        ULONG _BlockSize;
        ULONG _NumberOfBlocks;
        KIoBuffer::SPtr _HeaderIoBuffer;
        DedicatedLCHeader* _Header;

        //
        // Parameters to api
        //
        KBuffer::SPtr _SecurityDescriptor;

        //
        // Members needed for functionality
        //
        KArray<KIoBuffer::SPtr> _InitialData;
        NTSTATUS _FinalStatus;
        
    public:     
        //
        // This async is used to write logical log metadata
        //
        class AsyncWriteMetadataContext : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncWriteMetadataContext);

            friend DedicatedLCMBInfoAccess;
            
            public:             
                VOID StartWriteMetadata(
                    __in KIoBuffer::SPtr& LLMetadata,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                );

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

            private:
                enum { NotAssigned, Initial, AcquireExclusiveLock, WriteMetadata, 
                       Completed, CompletedWithError } _State;

                VOID DoComplete(
                    __in NTSTATUS Status
                    );

                VOID WriteMetadataFSM(
                    __in NTSTATUS Status
                    );
            
                VOID WriteMetadataCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
            private:
                //
                // General members
                //
                DedicatedLCMBInfoAccess::SPtr _DLCMB;         
                MBInfoAccess::AsyncAcquireExclusiveContext::SPtr _MBInfoLock;
                MBInfoAccess::AsyncWriteSectionContext::SPtr _WriteContext;             
                
                //
                // Parameters to api
                //
                KIoBuffer::SPtr _LLMetadata;
                                
                //
                // Members needed for functionality
                //
                
        };
        
        friend AsyncWriteMetadataContext;
        
        NTSTATUS CreateAsyncWriteMetadataContext(__out AsyncWriteMetadataContext::SPtr& Context);

                
        //
        // This async is used to read logical log metadata
        //
        class AsyncReadMetadataContext : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncReadMetadataContext);

            friend DedicatedLCMBInfoAccess;
            
            public:             
                VOID StartReadMetadata(
                    __out KIoBuffer::SPtr& LLMetadata,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                );

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

            private:
                enum { NotAssigned, Initial, AcquireExclusiveLock, ReadMetadata, ExtractLLMetadata,
                       Completed, CompletedWithError } _State;

                VOID DoComplete(
                    __in NTSTATUS Status
                    );

                VOID ReadMetadataFSM(
                    __in NTSTATUS Status
                    );
            
                VOID ReadMetadataCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
            private:
                //
                // General members
                //
                DedicatedLCMBInfoAccess::SPtr _DLCMB;         
                MBInfoAccess::AsyncAcquireExclusiveContext::SPtr _MBInfoLock;
                MBInfoAccess::AsyncReadSectionContext::SPtr _ReadContext;               
                
                //
                // Parameters to api
                //
                KIoBuffer::SPtr* _LLMetadata;
                                
                //
                // Members needed for functionality
                //
                KIoBuffer::SPtr _IoBuffer;
        };
        
        friend AsyncReadMetadataContext;
        
        NTSTATUS CreateAsyncReadMetadataContext(__out AsyncReadMetadataContext::SPtr& Context);
};
