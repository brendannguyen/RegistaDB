#include <gtest/gtest.h>
#include <filesystem>
#include <atomic>
#include <thread>
#include <cpr/cpr.h>
#include <cpr/cpr.h>
#include <json/json.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

namespace fs = std::filesystem;

class RestTest : public ::testing::Test {
protected:
    static inline std::string test_path = "./test_db_sandbox";
    static inline std::string base_url = "http://localhost:8081";
    static inline pid_t server_pid = -1;

    static void SetUpTestSuite() {
        if (fs::exists(test_path)) {
            fs::remove_all(test_path);
        }

        server_pid = fork();
        if (server_pid == 0) {
            int fd = open("server_test.log", O_RDWR | O_CREAT | O_TRUNC, 0644);
            if (fd != -1) {
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                close(fd);
            }

            char* binary = (char*)"./registadb_engine";
            char* flag   = (char*)"--path";
            char* p_val  = (char*)test_path.c_str(); 
            char* args[] = { binary, flag, p_val, NULL };

            execv(args[0], args);
            
            perror("execv failed");
            _exit(1);
        } 
        else if (server_pid > 0) {
            if (!waitForServer(10)) {
                kill(server_pid, SIGKILL);
                FAIL() << "Server failed to start on 8081. Check server_test.log";
            }
        }
    }

    static void TearDownTestSuite() {
        if (server_pid > 0) {
            kill(server_pid, SIGTERM);
        
            int status;
            waitpid(server_pid, &status, 0);
            
            std::system("pkill -9 registadb_engine > /dev/null 2>&1");
            server_pid = -1;
        }
        if (fs::exists(test_path)) {
            fs::remove_all(test_path);
        }
    }

    static bool waitForServer(int timeout_seconds) {
        for (int i = 0; i < timeout_seconds * 2; ++i) {
            auto r = cpr::Get(cpr::Url{base_url + "/entries"});
            if (r.status_code > 0) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        return false;
    }

    Json::Value parseJson(const std::string& str) {
        Json::Value obj;
        Json::CharReaderBuilder builder;
        std::string errs;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        reader->parse(str.c_str(), str.c_str() + str.length(), &obj, &errs);
        return obj;
    }
};

TEST_F(RestTest, CreateEntry) {
    auto r = cpr::Post(cpr::Url{base_url + "/entries"},
                       cpr::Body{R"({"id": 100, "data": {"string_value": "hello!"}})"},
                       cpr::Header{{"Content-Type", "application/json"}});

    ASSERT_EQ(r.status_code, 201);

    Json::Value json = parseJson(r.text);

    ASSERT_TRUE(json.isMember("id")) << "Response missing 'id' field";
    
    EXPECT_TRUE(json.isMember("createdAt")) << "Server returned camelCase 'createdAt'";
    EXPECT_TRUE(json.isMember("updatedAt")) << "Server returned camelCase 'updatedAt'";
    EXPECT_EQ(json["data"]["stringValue"].asString(), "hello!");
}

TEST_F(RestTest, UpdateEntrySuccess) {
    auto r = cpr::Put(cpr::Url{base_url + "/entries/100"},
                      cpr::Body{R"({"data": {"string_value": "updated_text"}})"},
                      cpr::Header{{"Content-Type", "application/json"}});

    ASSERT_EQ(r.status_code, 200);
    Json::Value json = parseJson(r.text);
    
    EXPECT_EQ(json["data"]["stringValue"].asString(), "updated_text");
    EXPECT_TRUE(json.isMember("updatedAt"));
}

TEST_F(RestTest, DeleteEntrySuccess) {
    auto r = cpr::Delete(cpr::Url{base_url + "/entries/100"});
    
    ASSERT_EQ(r.status_code, 204);

    auto check = cpr::Get(cpr::Url{base_url + "/entries/100"});
    EXPECT_EQ(check.status_code, 404);
}

TEST_F(RestTest, ReadNonExistentEntry) {
    auto r = cpr::Get(cpr::Url{base_url + "/entries/999999"});
    
    EXPECT_EQ(r.status_code, 404);
    EXPECT_EQ(r.text, "Entry not found");
}

TEST_F(RestTest, MalformedJsonBody) {
    auto r = cpr::Post(cpr::Url{base_url + "/entries"},
                       cpr::Body{R"({"id": 200, "data": {"stringValue": "oops")"},
                       cpr::Header{{"Content-Type", "application/json"}});

    EXPECT_EQ(r.status_code, 400);
}

TEST_F(RestTest, ProtobufNegotiation) {
    cpr::Post(cpr::Url{base_url + "/entries"},
              cpr::Body{R"({"id": 100, "data": {"string_value": "proto_test"}})"},
              cpr::Header{{"Content-Type", "application/json"}});

    auto r = cpr::Get(cpr::Url{base_url + "/entries/100"},
                      cpr::Header{{"Accept", "application/x-protobuf"}});

    ASSERT_EQ(r.status_code, 200);
    EXPECT_EQ(r.header["Content-Type"], "application/x-protobuf");
}

TEST_F(RestTest, PutIdOverrideLogic) {
    cpr::Post(cpr::Url{base_url + "/entries"},
              cpr::Body{R"({"id": 100, "data": {"string_value": "original"}})"},
              cpr::Header{{"Content-Type", "application/json"}});

    // PUT to ID 100, but pass ID 999 in the body
    auto r = cpr::Put(cpr::Url{base_url + "/entries/100"},
                      cpr::Body{R"({"id": 999, "data": {"string_value": "overridden"}})"},
                      cpr::Header{{"Content-Type", "application/json"}});

    ASSERT_EQ(r.status_code, 200);
    Json::Value json = parseJson(r.text);

    // ID in the response should still be 100
    EXPECT_EQ(json["id"].asString(), "100") 
        << "The URL ID (100) should have overridden the body ID (999)";
    
    EXPECT_EQ(json["data"]["stringValue"].asString(), "overridden");

    // ensure entry 999 was NOT created/affected
    auto check_999 = cpr::Get(cpr::Url{base_url + "/entries/999"});
    EXPECT_EQ(check_999.status_code, 404) 
        << "Entry 999 should not exist; the override failed if it does.";
}