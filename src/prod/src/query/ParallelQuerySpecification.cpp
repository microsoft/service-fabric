// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;

ParallelQuerySpecification::ParallelQuerySpecification(
    Query::QueryNames::Enum queryName,
    Query::QueryAddressGenerator const & queryAddress)
    : QuerySpecification(queryName, queryAddress, QueryType::Parallel)
    , parallelQuerySpecifications_()
{
}

ParallelQuerySpecification::ParallelQuerySpecification(
    Query::QueryNames::Enum queryName,
    Query::QueryAddressGenerator const & queryAddress,
    QueryArgument const & argument0)
    : QuerySpecification(queryName, queryAddress, argument0, QueryType::Parallel)
    , parallelQuerySpecifications_()
{
}

ParallelQuerySpecification::ParallelQuerySpecification(
    Query::QueryNames::Enum queryName,
    Query::QueryAddressGenerator const & queryAddress,
    QueryArgument const & argument0,
    QueryArgument const & argument1)
    : QuerySpecification(queryName, queryAddress, argument0, argument1, QueryType::Parallel)
    , parallelQuerySpecifications_()
{
}

ParallelQuerySpecification::ParallelQuerySpecification(
    Query::QueryNames::Enum queryName,
    Query::QueryAddressGenerator const & queryAddress,
    QueryArgument const & argument0,
    QueryArgument const & argument1,
    QueryArgument const & argument2)
    : QuerySpecification(queryName, queryAddress, argument0, argument1, argument2, QueryType::Parallel)
    , parallelQuerySpecifications_()
{
}

ParallelQuerySpecification::ParallelQuerySpecification(
    Query::QueryNames::Enum queryName,
    Query::QueryAddressGenerator const & queryAddress,
    QueryArgument const & argument0,
    QueryArgument const & argument1,
    QueryArgument const & argument2,
    QueryArgument const & argument3)
    : QuerySpecification(queryName, queryAddress, argument0, argument1, argument2, argument3, QueryType::Parallel)
    , parallelQuerySpecifications_()
{
}

ParallelQuerySpecification::ParallelQuerySpecification(
    Query::QueryNames::Enum queryName,
    Query::QueryAddressGenerator const & queryAddress,
    QueryArgument const & argument0,
    QueryArgument const & argument1,
    QueryArgument const & argument2,
    QueryArgument const & argument3,
    QueryArgument const & argument4)
    : QuerySpecification(queryName, queryAddress, argument0, argument1, argument2, argument3, argument4, QueryType::Parallel)
    , parallelQuerySpecifications_()
{
}

ParallelQuerySpecification::ParallelQuerySpecification(
    Query::QueryNames::Enum queryName,
    Query::QueryAddressGenerator const & queryAddress,
    QueryArgument const & argument0,
    QueryArgument const & argument1,
    QueryArgument const & argument2,
    QueryArgument const & argument3,
    QueryArgument const & argument4,
    QueryArgument const & argument5)
    : QuerySpecification(queryName, queryAddress, argument0, argument1, argument2, argument3, argument4, argument5, QueryType::Parallel)
    , parallelQuerySpecifications_()
{
}

QueryArgumentMap ParallelQuerySpecification::GetQueryArgumentMap(
    size_t index,
    QueryArgumentMap const & baseArg)
{
    UNREFERENCED_PARAMETER(index);
    return baseArg;
}
