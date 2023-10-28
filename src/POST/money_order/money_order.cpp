#include "money_order.hpp"
#include "../../public_components/enums.hpp"

#include <fmt/format.h>
#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/utils/assert.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/io/enum_types.hpp>
#include <userver/utils/uuid4.hpp>

namespace pg = userver::storages::postgres;
namespace JValue = userver::formats::json;


namespace pg_service_template {

    namespace{

        const std::string kSendMessage = R"~(WITH user_info AS (
            SELECT user_id FROM hello_schema.users 
            WHERE user_id = $1
        ),
            recipient_info AS (
            SELECT user_id FROM hello_schema.users
            WHERE login = $3
        ),
            balance_info AS (
            UPDATE hello_schema.user_balance
            SET balance = balance - $2
            WHERE user_id = (SELECT user_id FROM user_info) AND balance >= $2 AND (SELECT COUNT(*) FROM recipient_info) = 1
            RETURNING balance
        )
        SELECT (SELECT user_id FROM user_info), (SELECT balance FROM balance_info), (SELECT user_id FROM recipient_info) AS recipient;)~";

        const std::string kSetOperation = R"~(WITH send_messages AS (
            UPDATE hello_schema.user_balance
            SET balance = balance + $4
            WHERE user_id = $6
        )
        INSERT INTO hello_schema.operations VALUES ($1, $2, $3, $4, $5, NOW(), $6);)~";

        class Order final : public userver::server::handlers::HttpHandlerJsonBase {
            
        public:
            static constexpr std::string_view kName = "handler-money_order";

            Order(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& component_context) : HttpHandlerJsonBase(config, component_context), 
                    pg_cluster_(component_context
                        .FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()) {}
            
            JValue::Value HandleRequestJsonThrow(const userver::server::http::HttpRequest& request, 
                const userver::formats::json::Value &request_json, userver::server::request::RequestContext &) const override {
                
                JValue::ValueBuilder builder;
                userver::server::http::HttpResponse& response = request.GetHttpResponse();

                try
                {
                    auto user_id = request_json["user_id"].As<std::string>();
                    auto summary = request_json["summary"].As<int>();
                    auto recipient_login = request_json["recipient"].As<std::string>();

                    auto operation_id = userver::utils::generators::GenerateUuid();

                    auto ordering = pg_cluster_->Execute(pg::ClusterHostType::kMaster, kSendMessage, user_id, summary, recipient_login);
                    
                    for (auto& row : ordering)
                    {
                        auto user = row["user_id"].As<std::optional<std::string>>(),
                            recipient_id = row["recipient"].As<std::optional<std::string>>();
                        auto sum = row["balance"].As<std::optional<int>>();
                        LOG_DEBUG() << sum;
                        if(user != std::nullopt && sum != std::nullopt && recipient_id != std::nullopt) {
                            auto set_operation = pg_cluster_->Execute(pg::ClusterHostType::kMaster, kSetOperation, operation_id,
                                user, OperationType::kTransfer, summary, OperationStatus::kSuccesful, recipient_id);
                            builder["balance"] = summary, builder["Status"] = "Succesful";
                        } else if (sum == std::nullopt && recipient_id != std::nullopt && user != std::nullopt) {
                            auto set_operation = pg_cluster_->Execute(pg::ClusterHostType::kMaster, kSetOperation, operation_id,
                                user_id, OperationType::kTransfer, summary, OperationStatus::kFailed, recipient_id);
                            response.SetStatus(userver::server::http::HttpStatus::kForbidden);
                            LOG_ERROR() << "Insufficient funds";
                            return builder.ExtractValue();
                        } else {
                            response.SetStatus(userver::server::http::HttpStatus::kNotFound);
                            LOG_ERROR() << "User not found...";
                            return builder.ExtractValue();
                        }
                    }
                    
                }
                catch(const std::exception& ex)
                {
                    response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
                    LOG_ERROR() << "Exception due to: " << ex.what();
                    return builder.ExtractValue();
                }
                return builder.ExtractValue();
                
            }

            pg::ClusterPtr pg_cluster_;
        };

        
    }
    void AppendOrder(userver::components::ComponentList& component_list) {
        component_list.Append<Order>();
    }
}