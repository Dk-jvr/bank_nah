#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_list.hpp>

namespace pg_service_template {
    void AppendBalance(userver::components::ComponentList& component_list);
} 