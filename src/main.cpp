#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/daemon_run.hpp>

#include "POST/registration/registration.hpp"
#include "GET/get_balance/get_balance.hpp"
#include "POST/changing_balance/changing_balance.hpp"
#include "POST/money_order/money_order.hpp"

int main(int argc, char* argv[]) {
  auto component_list = userver::components::MinimalServerComponentList()
                            .Append<userver::server::handlers::Ping>()
                            .Append<userver::components::TestsuiteSupport>()
                            .Append<userver::components::HttpClient>()
                            .Append<userver::server::handlers::TestsControl>()
                            .Append<userver::components::Postgres>("postgres-db-1")
                            .Append<userver::clients::dns::Component>();

  pg_service_template::AppendRegistration(component_list);
  pg_service_template::AppendBalance(component_list);
  pg_service_template::AppendChanging(component_list);
  pg_service_template::AppendOrder(component_list);

  return userver::utils::DaemonMain(argc, argv, component_list);
}
