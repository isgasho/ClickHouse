#pragma once

#if !defined(ARCADIA_BUILD)
#include <Common/config.h>
#endif

#if USE_CASSANDRA
#include <cassandra.h>
#include <utility>

namespace DB
{

namespace Cassandra
{

template<typename CassT>
CassT * defaultCtor() { return nullptr; }

/// RAII wrapper for raw pointers to objects from cassandra driver library
template<typename CassT, auto Dtor, auto Ctor = defaultCtor<CassT>>
class ObjectHolder
{
    CassT * ptr = nullptr;
public:
    template<typename... Args>
    ObjectHolder(Args &&... args) : ptr(Ctor(std::forward<Args>(args)...)) {}
    ObjectHolder(CassT * ptr_) : ptr(ptr_) {}

    ObjectHolder(const ObjectHolder &) = delete;
    ObjectHolder & operator = (const ObjectHolder &) = delete;

    ObjectHolder(ObjectHolder && rhs) noexcept : ptr(rhs.ptr) { rhs.ptr = nullptr; }
    ObjectHolder & operator = (ObjectHolder && rhs) noexcept
    {
        if (ptr)
            Dtor(ptr);
        ptr = rhs.ptr;
        rhs.ptr = nullptr;
    }

    ~ObjectHolder()
    {
        if (ptr)
            Dtor(ptr);
    }

    /// For implicit conversion when passing object to driver library functions
    operator CassT * () { return ptr; }
    operator const CassT * () const { return ptr; }
};

}

/// These object are created on pointer construction
using CassClusterPtr = Cassandra::ObjectHolder<CassCluster, cass_cluster_free, cass_cluster_new>;
using CassSessionPtr = Cassandra::ObjectHolder<CassSession, cass_session_free, cass_session_new>;
using CassStatementPtr = Cassandra::ObjectHolder<CassStatement, cass_statement_free, cass_statement_new>;

/// The following objects are created inside Cassandra driver library,
/// but must be freed by user code
using CassFuturePtr = Cassandra::ObjectHolder<CassFuture, cass_future_free>;
using CassResultPtr = Cassandra::ObjectHolder<const CassResult, cass_result_free>;
using CassIteratorPtr = Cassandra::ObjectHolder<CassIterator, cass_iterator_free>;

/// Checks return code, throws exception on error
void cassandraCheck(CassError code);
void cassandraWaitAndCheck(CassFuturePtr && future);

}

#endif
