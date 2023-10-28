#include "get_balance.hpp"

#include <fmt/format.h>
#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/assert.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/crypto/hash.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/uuid4.hpp>

namespace pg = userver::storages::postgres;
namespace JValue = userver::formats::json;

namespace pg_service_template {

    namespace{

        const std::string kGetBalance = R"~(SELECT balance FROM hello_schema.user_balance WHERE user_id = $1)~";

        class Balance final : public userver::server::handlers::HttpHandlerJsonBase {

        public:

            static constexpr std::string_view kName = "handler-get_balance";

            Balance(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& component_context) : HttpHandlerJsonBase(config, component_context), pg_cluster_(component_context
                    .FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()) {}

            JValue::Value HandleRequestJsonThrow(const userver::server::http::HttpRequest& request, 
                const userver::formats::json::Value &request_json, userver::server::request::RequestContext &context) const override {
                
                userver::server::http::HttpResponse& response = request.GetHttpResponse();
                JValue::ValueBuilder builder;
                try {
                    std::string user_id = request_json["user_id"].As<std::string>();

                    auto result = pg_cluster_->Execute(pg::ClusterHostType::kMaster, kGetBalance, user_id);
                    if(result.IsEmpty()){
                        response.SetStatus(userver::server::http::HttpStatus::kNotFound);
                        LOG_ERROR() << "User not found";
                        return builder.ExtractValue();
                    }
                    builder["balance"] = result.AsSingleRow<int>();
                    return builder.ExtractValue(); 
                } catch (std::exception& ex){
                    response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
                    LOG_ERROR() << "Exception due to: " << ex.what();
                    return builder.ExtractValue();
                }
            }


            pg::ClusterPtr pg_cluster_;
        };
    }
    void AppendBalance(userver::components::ComponentList& component_list) {
        component_list.Append<Balance>();
    }
}