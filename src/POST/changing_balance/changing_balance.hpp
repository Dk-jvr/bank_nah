#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_list.hpp>

enum class OperationType { kWithdrawal, kRefill, kTransfer };
enum class OperationStatus { kSuccesful, kFailed };

namespace pg_service_template {
    void AppendChanging(userver::components::ComponentList& component_list);
} 