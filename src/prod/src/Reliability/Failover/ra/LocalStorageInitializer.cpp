// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;

using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;

namespace
{
    GlobalWString LfumLoadActivityId = make_global<wstring>(L"LfumLoad");
}

LocalStorageInitializer::LocalStorageInitializer(
    ReconfigurationAgent & ra,
    Reliability::NodeUpOperationFactory & factory_)
    : ra_(ra),
    factory_(factory_)
{
}


ErrorCode LocalStorageInitializer::Open()
{
    return LoadLFUM();
}

ErrorCode LocalStorageInitializer::LoadLFUM()
{
    auto & raStore = *ra_.LfumStore;
    auto & lfum = ra_.LocalFailoverUnitMapObj;

    // Load the LFUM from the disk
    vector<Storage::Api::Row> rows;
    auto error = raStore.Enumerate(Storage::Api::RowType::FailoverUnit, rows);
    if (!error.IsSuccess())
    {
        return error;
    }

    LocalFailoverUnitMap::InMemoryState lfumState;
    EntityEntryBaseList entries;
    error = DeserializeLfum(rows, lfumState, entries);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Open the LFUM
    lfum.Open(move(lfumState));

    error = UpdateStateOnLFUMLoad(entries);
    if (!error.IsSuccess())
    {
        return error;
    }

    return error;
}

ErrorCode LocalStorageInitializer::DeserializeLfum(
    vector<Storage::Api::Row> & rows,
    __out Infrastructure::LocalFailoverUnitMap::InMemoryState & lfumState,
    __out Infrastructure::EntityEntryBaseList & entries)
{
    // TODO: Can we parallelize this to use all cores
    LocalFailoverUnitMap::InMemoryState innerState;
    EntityEntryBaseList innerEntries;
    for (auto & it : rows)
    {
        auto ft = std::make_shared<FailoverUnit>();
        auto error = FabricSerializer::Deserialize(*ft, it.Data);
        if (!error.IsSuccess())
        {
            ReconfigurationAgent::WriteError("Lifecycle", "Failed to deserialize {0} from LFUM. Size: {1}", it.Id, it.Data.size());
            return error;
        }

        auto ftId = ft->FailoverUnitId;
        auto entry = EntityTraits<FailoverUnit>::Create(ftId, move(ft), ra_);
        innerEntries.push_back(entry);
        innerState.insert(make_pair(ftId, entry));
    }

    lfumState = move(innerState);
    entries = move(innerEntries);
    return ErrorCode::Success();
}

ErrorCode LocalStorageInitializer::UpdateStateOnLFUMLoad(EntityEntryBaseList const & fts)
{
    // Patch up the LFUM    
    auto handler = [this](HandlerParameters & handlerParameters, JobItemContextBase & context)
    {
        return UpdateStateOnLFUMLoadProcessor(handlerParameters, context);
    };

    auto jobItemFactory = [handler](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
    {
        UpdateStateOnLFUMLoadJobItem::Parameters jobItemParameters(
            entry,
            work,
            handler,
            JobItemCheck::None,
            *JobItemDescription::UpdateStateOnLFUMLoad,
            JobItemContextBase());

        return make_shared<UpdateStateOnLFUMLoadJobItem>(move(jobItemParameters));
    };

    return UpdateStateOnLoad(fts, jobItemFactory);
}

bool LocalStorageInitializer::UpdateStateOnLFUMLoadProcessor(
    Infrastructure::HandlerParameters & handlerParameters, 
    JobItemContextBase & context) const
{
    UNREFERENCED_PARAMETER(context);
    
    auto & ft = handlerParameters.FailoverUnit;
    handlerParameters.AssertHasWork();
    
    ft.EnableUpdate();
    ft->UpdateStateOnLFUMLoad(ra_.NodeInstance, handlerParameters.ExecutionContext);

    auto & nodeUpOperationFactory = handlerParameters.Work->GetInput<UpdateEntryOnStorageLoadInput>().NodeUpOperationFactory;
    if (ft->IsClosed)
    {
        nodeUpOperationFactory.AddClosedFailoverUnitFromLfumLoad(ft->FailoverUnitId);
    }
    else
    {
        nodeUpOperationFactory.AddOpenFailoverUnitFromLfumLoad(ft->FailoverUnitId, ft->ServiceDescription.Type.ServicePackageId, ft->LocalReplica.PackageVersionInstance);
    }

    return true;
}

Common::ErrorCode LocalStorageInitializer::UpdateStateOnLoad(
    EntityEntryBaseList const & entities,
    Infrastructure::MultipleEntityWork::FactoryFunctionPtr factory)
{
    ManualResetEvent ev;
    UpdateEntryOnStorageLoadInput input(factory_);

    auto work = make_shared<MultipleFailoverUnitWorkWithInput<UpdateEntryOnStorageLoadInput>>(
        LfumLoadActivityId,
        [&ev](MultipleEntityWork &, ReconfigurationAgent &)
        {
            ev.Set();
        },
        move(input));

    ra_.JobQueueManager.CreateJobItemsAndStartMultipleFailoverUnitWork(work, entities, factory);

    ev.WaitOne();

    return work->GetFirstError();
}
