#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace api {

    /**
     * @brief Handles HTTP requests for CRUD operations on entries, translating them into internal requests to RegistaServer and formatting responses accordingly. Supports both JSON and Protobuf response formats based on the client's "Accept" header.
     * 
     */
    class EntryController : public drogon::HttpController<EntryController> {
    public:
        METHOD_LIST_BEGIN
            ADD_METHOD_TO(EntryController::handleCreate, "/entries", Post);
            ADD_METHOD_TO(EntryController::handleRead, "/entries/{id}", Get);
            ADD_METHOD_TO(EntryController::handleUpdate, "/entries/{id}", Put);
            ADD_METHOD_TO(EntryController::handleDelete, "/entries/{id}", Delete); 
        METHOD_LIST_END

        void handleCreate(const HttpRequestPtr& req, 
                        std::function<void(const HttpResponsePtr&)>&& callback);

        void handleRead(const HttpRequestPtr& req, 
                        std::function<void(const HttpResponsePtr&)>&& callback, 
                        uint64_t id);
        
        void handleUpdate(const HttpRequestPtr& req, 
                        std::function<void(const HttpResponsePtr&)>&& callback, 
                        uint64_t id);

        void handleDelete(const HttpRequestPtr& req, 
                        std::function<void(const HttpResponsePtr&)>&& callback, 
                        uint64_t id);
    };

}