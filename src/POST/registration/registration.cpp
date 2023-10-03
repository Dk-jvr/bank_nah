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

namespace pg = userver::storages::postgres;

namespace pg_service_template {

    namespace {

        class Registration final : public userver::server::handlers::HttpHandlerJsonBase {
            
        public:
            static constexpr std::string_view kName = "handler-registration";

            Registration(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& component_context) : HttpHandlerJsonBase(config, component_context), pg_cluster_(component_context
                    .FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()) {}

            userver::formats::json::Value HandleRequestJsonThrow(const userver::server::http::HttpRequest& request, 
                const userver::formats::json::Value &request_json, userver::server::request::RequestContext &context) const override {
                
                std::string email = request_json["email"].As<std::string>();
                std::string password_str = request_json["password"].As<std::string>();
                std::string password = userver::crypto::hash::Sha256(password_str, userver::crypto::hash::OutputEncoding::kHex);
                auto result = pg_cluster_->Execute(pg::ClusterHostType::kMaster, "INSERT INTO hello_schema.users "
                    "VALUES ($1, bytea($2))", email, password);

                userver::formats::json::ValueBuilder builder;
                builder["email"] = email;
                builder["password"] = password;

                return builder.ExtractValue();
            }
            pg::ClusterPtr pg_cluster_;
        };

        
    }
    void AppendRegistration(userver::components::ComponentList& component_list) {
        component_list.Append<Registration>();
    }
}