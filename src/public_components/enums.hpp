#pragma once

#include <userver/storages/postgres/io/enum_types.hpp>

enum class OperationType { kWithdrawal, kRefill, kTransfer };
enum class OperationStatus { kSuccesful, kFailed };

namespace userver::storages::postgres::io {
    template <>
    struct CppToUserPg<OperationType> : EnumMappingBase<OperationType>
    {
        static constexpr DBTypeName postgres_name = "OperationType";
        static constexpr Enumerator enumerators[] {
            {EnumType::kWithdrawal, "Withdrawal"},
            {EnumType::kRefill, "Refill"},
            {EnumType::kTransfer, "Transfer"}};
    };
    template <>
    struct CppToUserPg<OperationStatus> : EnumMappingBase<OperationStatus>
    {
        static constexpr DBTypeName postgres_name = "OperationStatus";
        static constexpr Enumerator enumerators[] {
            {EnumType::kSuccesful, "succesful"},
            {EnumType::kFailed, "failed"}};
    };
    
}