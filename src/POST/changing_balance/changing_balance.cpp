#include "changing_balance.hpp"

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

namespace pg_service_template {

    namespace{

        const std::string kSetBalance = R"~(WITH user_info AS (
            SELECT user_id FROM hello_schema.users 
            WHERE user_id = uuid($1)
        ),
            balance_info AS (
            UPDATE hello_schema.user_balance
            SET balance = balance + $2
            WHERE user_id = (SELECT user_id FROM user_info) AND ($2 >= 0 OR balance > ABS($2))
            RETURNING balance
        )
        SELECT (SELECT user_id FROM user_info)::TEXT, (SELECT balance FROM balance_info);)~";

        const std::string kSetOperation = R"~(INSERT INTO hello_schema.operations VALUES (uuid($1), uuid($2), $3, $4, $5))~";

        class Changing final : public userver::server::handlers::HttpHandlerJsonBase {

        public:

            static constexpr std::string_view kName = "handler-changing_balance";

            Changing(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& component_context) : HttpHandlerJsonBase(config, component_context), pg_cluster_(component_context
                    .FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()) {}

            JValue::Value HandleRequestJsonThrow(const userver::server::http::HttpRequest& request, 
                const userver::formats::json::Value &request_json, userver::server::request::RequestContext &) const override {
                
                JValue::ValueBuilder builder;
                userver::server::http::HttpResponse& response = request.GetHttpResponse();

                try {
                    auto user_id = request_json["user_id"].As<std::string>();
                    auto summary = request_json["summary"].As<int>();

                    std::optional<int> check_val(int val) {
                        if(!val)
                            return std::nullopt;
                    }

                    auto operation_id = userver::utils::generators::GenerateUuid();
                    OperationType type;
                    if(summary >= 0)
                        type = OperationType::kRefill;
                    else 
                        type = OperationType::kWithdrawal;

                    LOG_DEBUG() << summary;

                    auto set_balance = pg_cluster_->Execute(pg::ClusterHostType::kMaster, kSetBalance, user_id, summary);
                    if(set_balance.RowsAffected() == 1){
                        for(const auto& row : set_balance){
                            user_id = row["user_id"].As<std::string>();
                            if(row["balance"].As<std::optional<int>>() != std::nullopt){
                                auto set_operation = pg_cluster_->Execute(pg::ClusterHostType::kMaster, kSetOperation, operation_id, user_id, type, summary, OperationStatus::kSuccesful);
                                builder["balance"] = row["balance"].As<int>();
                                builder["status"] = "Succesful";
                                return builder.ExtractValue();
                            } else {
                                auto set_operation = pg_cluster_->Execute(pg::ClusterHostType::kMaster, kSetOperation, operation_id, user_id, type, summary, OperationStatus::kFailed);
                                response.SetStatus(userver::server::http::HttpStatus::kForbidden);
                                LOG_ERROR() << "Insufficient funds";
                                return builder.ExtractValue();
                            }
                        }
                    } else {
                        response.SetStatus(userver::server::http::HttpStatus::kNotFound);
                        LOG_ERROR() << "User not found...";
                        return builder.ExtractValue();
                    }
                } catch (std::exception& ex) {
                    response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
                    LOG_ERROR() << "Exception due to: " << ex.what();
                    return builder.ExtractValue();
                }
            }
            pg::ClusterPtr pg_cluster_;
        };
    }
    void AppendChanging(userver::components::ComponentList& component_list) {
        component_list.Append<Changing>();
    }
}