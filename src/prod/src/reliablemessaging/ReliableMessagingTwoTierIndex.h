// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
	// Needed for unknown reasons to get K_FORCE_SHARED to compile properly below
	using ::_delete;

	/*

	The TwoTierKAvlTree is a double index structure for recording the many-to-many session relationships  
	between source and target partitions.  The reason why we allow multiple sessions from the same source to the same target partition 
	is that in the case of Int64 partitions, the target partition is servicing a range of partition numbers but the source is
	requesting a session to s single numbered target partition.  Thus for each source partition, an Int64 target partition needs to 
	keep a map of the target sessions that fall within ints range.  We generalize this for non-Int64 target partitions at the cost of an
	additional data structure layer to reduce code complexity -- this is only a cost at startup and shutdown of sessions so it
	doesn't have much of a performance impact.

	TODO: implement this as a template structure -- runs into some compiler issues

	*/

	class SecondaryIndex : public KObject<SecondaryIndex>, public KShared<SecondaryIndex>
	{
		K_FORCE_SHARED(SecondaryIndex);

	public:

		// Create a blank poolable QueuedInboundMessage
		static Common::ErrorCode Create(
			__out SecondaryIndex::SPtr& result)
		{		
			result = _new (RMSG_ALLOCATION_TAG,Common::GetSFDefaultPagedKAllocator()) 
					SecondaryIndex();
			KTL_CREATE_CHECK_AND_RETURN(result,KtlObjectCreateFailed,SecondaryIndex)
		}

		Common::ErrorCode Insert(
			__in ReliableSessionServiceSPtr item,
			__in ReliableMessagingServicePartitionSPtr key)
		{
			NTSTATUS status = index_.Insert(item,key);

			if (!NT_SUCCESS(status))
			{
				return Common::ErrorCodeValue::OutOfMemory;
			}
			else
			{
				return Common::ErrorCodeValue::Success;
			}
		}

		BOOLEAN IsEmpty()
		{
			return index_.IsEmpty();
		}

		BOOLEAN Remove(
			__in ReliableMessagingServicePartitionSPtr key)
		{
			return index_.Remove(key);
		}

		BOOLEAN Find(
			__in ReliableMessagingServicePartitionSPtr& key,
			__out ReliableSessionServiceSPtr& item)
		{
			return index_.Find(key,item);
		}

		KAvlTree<ReliableSessionServiceSPtr,ReliableMessagingServicePartitionSPtr>::Iterator GetIterator()
		{
			return index_.GetIterator();
		}

	private:

		KAvlTree<ReliableSessionServiceSPtr,ReliableMessagingServicePartitionSPtr> index_;
	};


	class TwoTierKAvlTree
	{
	public:

		TwoTierKAvlTree():
			primaryMap_(&(ReliableMessagingServicePartition::compare),Common::GetSFDefaultPagedKAllocator())
		{
		}

		//
		// Insert
		//
		// Inserts a new element into the tree. If an element with the same key already exists,
		// optionally updates the existing element.
		//
		// Parameters:
		//   primaryKey           The primary key to the item to be removed.
		//   secondaryKey         The key in the second tier tree where the item to be removed is actually stored
		//	 item				  The item to be inserted
		//  Return value:
		//     STATUS_SUCCESS
		//     STATUS_INSUFFICIENT_RESOURCES
		//     STATUS_OBJECT_NAME_COLLISION         If the key exists and ForceUpdate is FALSE
		//
		Common::ErrorCode Insert(
			__in ReliableMessagingServicePartitionSPtr& primaryKey,
			__in ReliableMessagingServicePartitionSPtr& secondaryKey,
			__in ReliableSessionServiceSPtr& item)
		{
			SecondaryIndex::SPtr secondaryMap = nullptr;
			BOOLEAN primaryFound = primaryMap_.Find(primaryKey, secondaryMap);

			if (primaryFound ==  FALSE)
			{
				Common::ErrorCode createStatus = SecondaryIndex::Create(secondaryMap);

				if (!createStatus.IsSuccess())
				{
					RMTRACE->OutOfMemory("TwoTierKAvlTree::Insert");
					return createStatus;
				}

				NTSTATUS insertPrimaryStatus = primaryMap_.Insert(secondaryMap,primaryKey);

				if (!NT_SUCCESS(insertPrimaryStatus))
				{
					return Common::ErrorCodeValue::OutOfMemory;
				}
			}
				
			Common::ErrorCode insertSecondaryStatus = secondaryMap->Insert(item,secondaryKey);

			return insertSecondaryStatus;
		}

		// Removes an entry from the second tier AVL tree.
		//
		// Parameters:
		//   primaryKey           The primary key to the item to be removed.
		//   secondaryKey         The key in the second tier tree where the item to be removed is actually stored
		//
		// Return value:
		//   TRUE if the item was found and removed
		//   FALSE if the specified item was not in the tree
		//
		BOOLEAN Remove(
			__in ReliableMessagingServicePartitionSPtr& primaryKey,
			__in ReliableMessagingServicePartitionSPtr& secondaryKey)
		{
			SecondaryIndex::SPtr secondaryMap = nullptr;
			BOOLEAN primaryFound = primaryMap_.Find(primaryKey, secondaryMap);

			if (primaryFound ==  FALSE)
			{
				return FALSE;
			}

			BOOLEAN secondaryFound = secondaryMap->Remove(secondaryKey);

			if (secondaryFound && secondaryMap->IsEmpty())
			{
				BOOLEAN primaryRemoved = primaryMap_.Remove(primaryKey);
				ASSERT_IF(primaryRemoved ==  FALSE, "unexpected failure to remove empty secondary map in TwoTierKAvlTree");
			}

			return secondaryFound;
		}

		// Removes a top level entry from the two tier AVL tree.
		// Clears the send tier tree if it is found
		//
		// Parameters:
		//   primaryKey           The primary key to the item to be removed.
		//
		// Return value:
		//   TRUE if the entry was found and removed
		//   FALSE if the specified entry was not in the tree
		//
		BOOLEAN RemovePrimary(
			__in ReliableMessagingServicePartitionSPtr& primaryKey)
		{
			// TODO: implement this
			primaryKey = nullptr;
			return FALSE;
		}

		// Find
		//
		// retrieves a single instance, if it is present.
		//
		// Parameters:
		//   primaryKey           The primary key to the item to be removed.
		//   secondaryKey         The key in the second tier tree where the item to be removed is actually stored
		//   item				  Receives the item, if it was found.  item must have asssigment semantics.
		//
		BOOLEAN Find(
			__in ReliableMessagingServicePartitionSPtr& primaryKey,
			__in ReliableMessagingServicePartitionSPtr& secondaryKey,
			__out ReliableSessionServiceSPtr& item)
		{
			SecondaryIndex::SPtr secondaryMap = nullptr;
			BOOLEAN primaryFound = primaryMap_.Find(primaryKey, secondaryMap);

			if (primaryFound ==  FALSE)
			{
				return FALSE;
			}

			BOOLEAN secondaryFound = secondaryMap->Find(secondaryKey,item);
			return secondaryFound;
		}

		KAvlTree<SecondaryIndex::SPtr,ReliableMessagingServicePartitionSPtr>::Iterator GetIterator()
		{
			return primaryMap_.GetIterator();
		}

	private:

		KAvlTree<SecondaryIndex::SPtr,ReliableMessagingServicePartitionSPtr> primaryMap_;
	};
}
