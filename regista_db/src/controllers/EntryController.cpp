#include "api/EntryController.h"
#include "RegistaServer.h"
#include <google/protobuf/util/json_util.h>

extern RegistaServer* g_regista_server;

namespace api {

    /**
     * @brief Maps a registadb::OperationStatus to an appropriate HTTP status code for the response.
     * 
     * @param status The internal operation status to map.
     * @return drogon::HttpStatusCode The corresponding HTTP status code.
     */
    drogon::HttpStatusCode mapStatus(registadb::OperationStatus status) {
        switch (status) {
            case registadb::STATUS_OK:               
                return k200OK;
            case registadb::STATUS_NOT_FOUND:        
                return k404NotFound;
            case registadb::STATUS_INVALID_ARGUMENT: 
                return k400BadRequest;
            case registadb::STATUS_INTERNAL_ERROR:   
                return k500InternalServerError;
            default:                                 
                return k500InternalServerError;
        }
    }
    
    /**
     * @brief Handles HTTP GET requests to read any entry by ID.
     * 
     * @param req The incoming HTTP request containing the entry ID in the URL path and optional "Accept" header for response format.
     * @param callback The callback function to send the HTTP response asynchronously.
     * @param id The ID of the entry to read, extracted from the URL path.
     */
    void EntryController::handleRead(const HttpRequestPtr& req, 
                                    std::function<void(const HttpResponsePtr&)>&& callback, 
                                    uint64_t id) {
        if (!g_regista_server) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Engine not initialized");
            callback(resp);
            return;
        }

        registadb::Request protoReq;
        protoReq.set_op(registadb::OP_READ);
        protoReq.set_id(id);

        registadb::Response protoResp = g_regista_server->ExecuteRequest(protoReq);

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(mapStatus(protoResp.status()));

        if (protoResp.status() == registadb::STATUS_OK) {

            auto acceptHeader = req->getHeader("Accept");

            if (acceptHeader == "application/x-protobuf") {
                resp->setContentTypeCode(CT_CUSTOM);
                resp->addHeader("Content-Type", "application/x-protobuf");
                resp->setBody(protoResp.entry().SerializeAsString());
            } else {
                std::string jsonStr;
                google::protobuf::util::MessageToJsonString(protoResp.entry(), &jsonStr);
                jsonStr += "\n";
                resp->setContentTypeCode(CT_APPLICATION_JSON);
                resp->setBody(std::move(jsonStr));
            }
        } else {
            resp->setBody("Entry not found");
        }
        callback(resp);
    }

    /**
     * @brief Handles HTTP POST requests to create a new entry, accepting either JSON or Protobuf request bodies.
     * 
     * @param req The incoming HTTP request containing the entry data in the body and optional "Content-Type" header to indicate format.
     * @param callback The callback function to send the HTTP response asynchronously.
     */
    void EntryController::handleCreate(const HttpRequestPtr& req, 
                                    std::function<void(const HttpResponsePtr&)>&& callback) {
        if (!g_regista_server) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }

        registadb::Request protoReq;
        protoReq.set_op(registadb::OP_CREATE);

        auto contentType = req->getHeader("Content-Type");

        if (contentType == "application/x-protobuf") {
            if (!protoReq.mutable_entry()->ParseFromString(std::string(req->body()))) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Invalid binary protobuf body\n");
                callback(resp);
                return;
            }
        } else {
            auto jsonPtr = req->getJsonObject();
            if (!jsonPtr) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Invalid JSON body\n");
                callback(resp);
                return;
            }
            std::string jsonStr = jsonPtr->toStyledString();
            google::protobuf::util::JsonStringToMessage(jsonStr, protoReq.mutable_entry());
        }
        
        registadb::Response protoResp = g_regista_server->ExecuteRequest(protoReq);

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(mapStatus(protoResp.status()));

        if (protoResp.status() == registadb::STATUS_OK) {
            resp->setStatusCode(k201Created);

            auto acceptHeader = req->getHeader("Accept");
            if (acceptHeader == "application/x-protobuf") {
                resp->setContentTypeCode(CT_CUSTOM);
                resp->addHeader("Content-Type", "application/x-protobuf");
                resp->setBody(protoResp.entry().SerializeAsString());
            } else {
                std::string outJson;
                google::protobuf::util::MessageToJsonString(protoResp.entry(), &outJson);
                outJson += "\n";
                resp->setContentTypeCode(CT_APPLICATION_JSON);
                resp->setBody(std::move(outJson));
            }
        } else {
            resp->setBody(protoResp.message() + '\n');
        }
        callback(resp);
    }

    /**
     * @brief Handles HTTP PUT requests to update an existing entry by ID, accepting either JSON or Protobuf request bodies.
     * 
     * @param req The incoming HTTP request containing the updated entry data in the body and optional "Content-Type" header to indicate format.
     * @param callback The callback function to send the HTTP response asynchronously.
     * @param id The ID of the entry to update, extracted from the URL path.
     */
    void EntryController::handleUpdate(const HttpRequestPtr& req, 
                                    std::function<void(const HttpResponsePtr&)>&& callback, 
                                    uint64_t id) {
        if (!g_regista_server) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }

        registadb::Request protoReq;
        protoReq.set_op(registadb::OP_UPDATE);
        protoReq.set_id(id);

        auto contentType = req->getHeader("Content-Type");
        if (contentType == "application/x-protobuf") {
            if (!protoReq.mutable_entry()->ParseFromString(std::string(req->body()))) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Invalid binary protobuf body\n");
                callback(resp);
                return;
            }
        } else {
            auto jsonPtr = req->getJsonObject();
            if (!jsonPtr) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Missing JSON body for update\n");
                callback(resp);
                return;
            }
            std::string jsonStr = jsonPtr->toStyledString();
            google::protobuf::util::JsonStringToMessage(jsonStr, protoReq.mutable_entry());
        }

        protoReq.mutable_entry()->set_id(id);

        registadb::Response protoResp = g_regista_server->ExecuteRequest(protoReq);

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(mapStatus(protoResp.status()));

        if (protoResp.status() == registadb::STATUS_OK) {
            auto acceptHeader = req->getHeader("Accept");
            if (acceptHeader == "application/x-protobuf") {
                resp->setContentTypeCode(CT_CUSTOM);
                resp->addHeader("Content-Type", "application/x-protobuf");
                resp->setBody(protoResp.entry().SerializeAsString());
            } else {
                std::string outJson;
                google::protobuf::util::MessageToJsonString(protoResp.entry(), &outJson);
                outJson += "\n"; 
                resp->setContentTypeCode(CT_APPLICATION_JSON);
                resp->setBody(std::move(outJson));
            }
        } else {
            resp->setBody(protoResp.message() + "\n");
        }
        callback(resp);
    }

    /**
     * @brief Handles HTTP DELETE requests to delete an existing entry by ID.
     * 
     * @param req The incoming HTTP request containing the entry ID in the URL path.
     * @param callback The callback function to send the HTTP response asynchronously.
     * @param id The ID of the entry to delete, extracted from the URL path.
     */
    void EntryController::handleDelete(const HttpRequestPtr& req, 
                                    std::function<void(const HttpResponsePtr&)>&& callback, 
                                    uint64_t id) {
        if (!g_regista_server) {
            callback(HttpResponse::newHttpResponse());
            return;
        }

        registadb::Request protoReq;
        protoReq.set_op(registadb::OP_DELETE);
        protoReq.set_id(id);

        registadb::Response protoResp = g_regista_server->ExecuteRequest(protoReq);

        auto resp = HttpResponse::newHttpResponse();
        
        if (protoResp.status() == registadb::STATUS_OK) {
            resp->setStatusCode(k204NoContent);
        } else {
            resp->setStatusCode(mapStatus(protoResp.status()));
            resp->setBody(protoResp.message() + "\n");
        }
        callback(resp);
    }

}