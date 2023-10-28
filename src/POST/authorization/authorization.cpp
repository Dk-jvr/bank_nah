#include "registration.hpp"

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
#include <../../../../../Poco/JWT/Token.h>

namespace pg = userver::storages::postgres;

namespace pg_service_template {

    namespace {

        const std::string kCheckUser = R"~(SELECT)~";

        class Authorization final : public userver::server::handlers::HttpHandlerJsonBase {
            
        public:
            static constexpr std::string_view kName = "handler-authorization";

            Authorization(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& component_context) : HttpHandlerJsonBase(config, component_context), pg_cluster_(component_context
                    .FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()) {}

            userver::formats::json::Value HandleRequestJsonThrow(const userver::server::http::HttpRequest& request, 
                const userver::formats::json::Value &request_json, userver::server::request::RequestContext &context) const override {
                
                userver::server::http::HttpResponse& response = request.GetHttpResponse();
                userver::formats::json::ValueBuilder builder;
                try
                {
                    
                }
                catch(const std::exception& ex)
                {
                    response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
                    LOG_ERROR() << "Exception due to: " << ex.what();
                    return builder.ExtractValue();
                }
            }
            pg::ClusterPtr pg_cluster_;
        };

        
    }
    void AppendAuthorization(userver::components::ComponentList& component_list) {
        component_list.Append<Authorization>();
    }
}