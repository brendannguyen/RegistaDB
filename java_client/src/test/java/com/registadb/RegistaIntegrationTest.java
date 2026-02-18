package com.registadb;

import org.junit.jupiter.api.*;

import com.google.protobuf.ByteString;

import registadb.Playbook.ListValue;
import registadb.Playbook.MapValue;
import registadb.Playbook.RegistaObject;
import registadb.Playbook.StatusCode;
import registadb.Playbook.VectorValue;

import static org.junit.jupiter.api.Assertions.*;
import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

@TestInstance(TestInstance.Lifecycle.PER_CLASS)
public class RegistaIntegrationTest {
    private static Process serverProcess;
    private static RegistaClient client;
    private static final String testDbPath = "../test_db";

    /**
     * Recursively deletes all files and subdirectories in the specified directory. This method is used to ensure a clean state for the test database before and after tests are run.
     * @param directory The File object representing the directory to be cleared. The method will delete all files and subdirectories within this directory, but will not delete the directory itself.
     */
    private static void clearDirectory(File directory) {
    File[] allContents = directory.listFiles();
        if (allContents != null) {
            for (File file : allContents) {
                if (file.isDirectory()) {
                    // recursive call to handle subdirectories
                    clearDirectory(file); 
                }
                file.delete();
            }
        }
    }

    /**
     * Starts the RegistaDB server as a subprocess with the specified database path. The server's output is redirected to a log file for debugging purposes.
     * @throws IOException if there is an error starting the server process or creating the log file.
     * @throws InterruptedException if the thread is interrupted while waiting for the server to start.
     */
    private static void startServer() throws IOException, InterruptedException {
        // path to c++ binary
        String cppPath = "../regista_db/build/registadb_engine";

        // setup process builder
        ProcessBuilder builder = new ProcessBuilder(cppPath, "--path", testDbPath);
        builder.redirectErrorStream(true);
        File logFile = new File("target/engine.log");
        builder.redirectOutput(ProcessBuilder.Redirect.to(logFile));

        serverProcess = builder.start();

        Thread.sleep(1000); // wait for server to start
    }

    /**
     * Sets up the test environment by ensuring a clean state for the test database, starting the RegistaDB server, and initializing the RegistaClient. This method is executed once before all tests in this class are run.
     * @throws IOException if there is an error starting the server process or creating the log file.
     * @throws InterruptedException if the thread is interrupted while waiting for the server to start.
     */
    @BeforeAll
    static void start() throws IOException, InterruptedException {
        // ensure clean slate
        File dbDir = new File(testDbPath);
        if (dbDir.exists()) { clearDirectory(dbDir); }

        // start server
        startServer();

        // create client
        client = new RegistaClient("localhost");
    }

    /**
     * Tears down the test environment by closing the RegistaClient, shutting down the RegistaDB server, and cleaning up the test database directory. This method is executed once after all tests in this class have been run.
     * @throws InterruptedException if the thread is interrupted while waiting for the server process to terminate.
     */
    @AfterAll
    static void tearDown() throws InterruptedException {
        if (client != null) {
            client.close();
        }
        if (serverProcess != null) {
            serverProcess.destroy();

            boolean exited = serverProcess.waitFor(5, TimeUnit.SECONDS);
            if (!exited) {
                serverProcess.destroyForcibly();
            }
        }

        File dbDir = new File(testDbPath);
        if (dbDir.exists()) {
            clearDirectory(dbDir);
            dbDir.delete();
        }
    }

    @Nested
    @Order(1)
    @DisplayName("CRUD Operation Tests")
    class CrudTests {
        @Test
        @Order(1)
        @DisplayName("Test Create and Read")
        void testCreateAndRead() throws Exception {
            int testId = 999;
            String content = "This is a test.";
            RegistaObject newData = RegistaObject.newBuilder()
                    .setType(RegistaObject.Type.STRING)
                    .setId(testId)
                    .setBlob(ByteString.copyFromUtf8(content))
                    .build();

            // push to the fast lane
            // System.out.println("Pushing entry to Port 5555...");
            client.pushEntry(newData);

            // small sleep to ensure C++ has processed the pull
            Thread.sleep(100);

             // query from the smart lane
            // System.out.println("Querying for ID " + testId + " on Port 5556...");
            RegistaObject retrieved_entry = client.fetchById(testId);

            assertNotNull(retrieved_entry, "Entry should be found");
            assertEquals(testId, retrieved_entry.getId(), "IDs should match");
            assertEquals(RegistaObject.Type.STRING, retrieved_entry.getType(), "Types should match");
            assertEquals(content, retrieved_entry.getBlob().toStringUtf8(), "Content should match");
        }

        @Test
        @Order(2)
        @DisplayName("Test Delete")
        void testDelete() throws Exception {
            int testId = 999;

            // delete from smart line
            String deleteStatus = client.deleteById(testId);
            assertEquals("SUCCESS", deleteStatus, "Delete should return SUCCESS for existing entry");

            Thread.sleep(100);

            // verify deletion
            RegistaObject postDeleteEntry = client.fetchById(testId);
            assertNull(postDeleteEntry, "Entry should be null after deletion");
        }

        @Test
        @Order(3)
        @DisplayName("Test Verified Ingest")
        void testVerifiedIngest() throws Exception {
            int testId = 1000;
            String content = "This is a verified ingest test.";
            RegistaObject newData = RegistaObject.newBuilder()
                    .setType(RegistaObject.Type.STRING)
                    .setId(testId)
                    .setBlob(ByteString.copyFromUtf8(content))
                    .build();

            // ingest from the smart lane
            // System.out.println("Storing entry via Port 5556 with verification...");
            String response = client.storeEntryVerified(newData);
            assertEquals("SUCCESS", response, "Response should be SUCCESS for successful verified ingest, instead got: " + response);

            Thread.sleep(100);

            // query to verify storage
            // System.out.println("Querying for ID " + testId + " on Port 5556...");
            RegistaObject retrieved_entry = client.fetchById(testId);

            assertNotNull(retrieved_entry, "Entry should be found");
            assertEquals(testId, retrieved_entry.getId(), "IDs should match");
            assertEquals(RegistaObject.Type.STRING, retrieved_entry.getType(), "Types should match");
            assertEquals(content, retrieved_entry.getBlob().toStringUtf8(), "Content should match");
        }

        @Test
        @Order(4)
        @DisplayName("Test Malformed Type & Data Ingest")
        void testMalformedTypeAndDataIngest() throws Exception {
            int testId = 1001;
            String content = "This entry has a mismatched type.";
            RegistaObject malformedData = RegistaObject.newBuilder()
                    .setType(RegistaObject.Type.LIST) // Intentionally wrong type
                    .setId(testId)
                    .setBlob(ByteString.copyFromUtf8(content))
                    .build();

            // ingest from the smart lane
            // System.out.println("Attempting to store malformed entry via Port 5556...");
            String response = client.storeEntryVerified(malformedData);
            assertEquals("ERR_TYPE_MISMATCH", response, "Response should indicate type mismatch, instead got: " + response);

            Thread.sleep(100);

            // query to verify it was not stored
            // System.out.println("Querying for ID " + testId + " on Port 5556...");
            RegistaObject retrieved_entry = client.fetchById(testId);
            assertNull(retrieved_entry, "Malformed entry should not be stored");
        }

        @Test
        @Order(5)
        @DisplayName("Test List Type Ingest and Retrieval")
        void testListTypeIngestAndRetrieval() throws Exception {
            int testId = 1002;
            ListValue listContent = ListValue.newBuilder()
                    .addElements(ByteString.copyFromUtf8("First item"))
                    .addElements(ByteString.copyFromUtf8("Second item"))
                    .build();
            RegistaObject listData = RegistaObject.newBuilder()
                    .setType(RegistaObject.Type.LIST)
                    .setId(testId)
                    .setList(listContent)
                    .build();

            // ingest from the smart lane
            // System.out.println("Storing list entry via Port 5556...");
            String response = client.storeEntryVerified(listData);
            assertEquals("SUCCESS", response, "Response should be SUCCESS for successful list ingest, instead got: " + response);

            Thread.sleep(100);

            // query to verify storage
            // System.out.println("Querying for ID " + testId + " on Port 5556...");
            RegistaObject retrieved_entry = client.fetchById(testId);

            assertNotNull(retrieved_entry, "Entry should be found");
            assertEquals(testId, retrieved_entry.getId(), "IDs should match");
            assertEquals(RegistaObject.Type.LIST, retrieved_entry.getType(), "Types should match");
            assertEquals(listContent, retrieved_entry.getList(), "List content should match");
        }

        @Test
        @Order(6)
        @DisplayName("Test Hash Type Ingest and Retrieval")
        void testHashTypeIngestAndRetrieval() throws Exception {
            int testId = 1003;
            MapValue mapContent = MapValue.newBuilder()
                    .putFields("key1", ByteString.copyFromUtf8("value1"))
                    .putFields("key2", ByteString.copyFromUtf8("value2"))
                    .build();
            RegistaObject hashData = RegistaObject.newBuilder()
                    .setType(RegistaObject.Type.HASH)
                    .setId(testId)
                    .setHash(mapContent)
                    .build();

            // ingest from the smart lane
            // System.out.println("Storing hash entry via Port 5556...");
            String response = client.storeEntryVerified(hashData);
            assertEquals("SUCCESS", response, "Response should be SUCCESS for successful hash ingest, instead got: " + response);

            Thread.sleep(100);

            // query to verify storage
            // System.out.println("Querying for ID " + testId + " on Port 5556...");
            RegistaObject retrieved_entry = client.fetchById(testId);

            assertNotNull(retrieved_entry, "Entry should be found");
            assertEquals(testId, retrieved_entry.getId(), "IDs should match");
            assertEquals(RegistaObject.Type.HASH, retrieved_entry.getType(), "Types should match");
            assertEquals(mapContent, retrieved_entry.getHash(), "Hash content should match");
        }

        @Test
        @Order(7)
        @DisplayName("Test JSON Type Ingest and Retrieval")
        void testJsonTypeIngestAndRetrieval() throws Exception {
            int testId = 1004;
            String jsonContent = "{\"name\": \"Alice\", \"age\": 30}";
            RegistaObject jsonData = RegistaObject.newBuilder()
                    .setType(RegistaObject.Type.JSON)
                    .setId(testId)
                    .setBlob(ByteString.copyFromUtf8(jsonContent))
                    .build();

            // ingest from the smart lane
            // System.out.println("Storing JSON entry via Port 5556...");
            String response = client.storeEntryVerified(jsonData);
            assertEquals("SUCCESS", response, "Response should be SUCCESS for successful JSON ingest, instead got: " + response);

            Thread.sleep(100);

            // query to verify storage
            // System.out.println("Querying for ID " + testId + " on Port 5556...");
            RegistaObject retrieved_entry = client.fetchById(testId);

            assertNotNull(retrieved_entry, "Entry should be found");
            assertEquals(testId, retrieved_entry.getId(), "IDs should match");
            assertEquals(RegistaObject.Type.JSON, retrieved_entry.getType(), "Types should match");
            assertEquals(jsonContent, retrieved_entry.getBlob().toStringUtf8(), "JSON content should match");
        }

        @Test
        @Order(8)
        @DisplayName("Test Vector Type Ingest and Retrieval")
        void testVectorTypeIngestAndRetrieval() throws Exception {
            int testId = 1005;
            VectorValue vectorValue = VectorValue.newBuilder()
                    .addElements(1.0f)
                    .addElements(2.0f)
                    .addElements(3.0f)
                    .build();

            RegistaObject vectorData = RegistaObject.newBuilder()
                    .setType(RegistaObject.Type.VECTOR)
                    .setId(testId)
                    .setVector(vectorValue)
                    .build();

            // ingest from the smart lane
            // System.out.println("Storing vector entry via Port 5556...");
            String response = client.storeEntryVerified(vectorData);
            assertEquals("SUCCESS", response, "Response should be SUCCESS for successful vector ingest, instead got: " + response);

            Thread.sleep(100);

            // query to verify storage
            // System.out.println("Querying for ID " + testId + " on Port 5556...");
            RegistaObject retrieved_entry = client.fetchById(testId);

            assertNotNull(retrieved_entry, "Entry should be found");
            assertEquals(testId, retrieved_entry.getId(), "IDs should match");
            assertEquals(RegistaObject.Type.VECTOR, retrieved_entry.getType(), "Types should match");
            assertEquals(vectorValue, retrieved_entry.getVector(), "Vector content should match");
        }

        @Test
        @Order(9)
        @DisplayName("Test server ID generation")
        void testServerIdGeneration() throws Exception {
            List<String> generatedIdContent = new ArrayList<>();
            for (int i = 1; i <= 5; i++) {
                generatedIdContent.add("Entry with server-generated ID " + i);
            }

            for (String idContent : generatedIdContent) {
                RegistaObject newData = RegistaObject.newBuilder()
                        .setType(RegistaObject.Type.STRING)
                        .setBlob(ByteString.copyFromUtf8(idContent))
                        .build();

                // ingest from the smart lane
                // System.out.println("Storing entry with server-generated ID via Port 5556...");
                String response = client.storeEntryVerified(newData);
                assertEquals("SUCCESS", response, "Response should be SUCCESS for successful ingest, instead got: " + response);
            }

            Thread.sleep(100);

            // query to verify storage
            for (int i = 1; i <= generatedIdContent.size(); i++) {
                String content = generatedIdContent.get(i-1);
                // System.out.println("Querying for generated ID " + i + " on Port 5556...");
                RegistaObject retrieved_entry = client.fetchById(i);

                assertNotNull(retrieved_entry, "Entry should be found");
                assertEquals(i, retrieved_entry.getId(), "IDs should match");
                assertEquals(RegistaObject.Type.STRING, retrieved_entry.getType(), "Types should match");
                assertEquals(content, retrieved_entry.getBlob().toStringUtf8(), "Content should match");
            }
        }
    }

    @Nested
    @Order(2)
    @DisplayName("Test Persistence Across Restarts")
    class PersistenceTests {
        @Test
        @DisplayName("Test that stored entries persist across server restarts")
        void testPersistenceAcrossRestarts() throws Exception {
            int testId = 2000;
            String content = "This entry should persist across restarts.";
            RegistaObject newData = RegistaObject.newBuilder()
                    .setType(RegistaObject.Type.STRING)
                    .setId(testId)
                    .setBlob(ByteString.copyFromUtf8(content))
                    .build();

            // ingest from the smart lane
            // System.out.println("Storing entry for persistence test via Port 5556...");
            String response = client.storeEntryVerified(newData);
            assertEquals("SUCCESS", response, "Response should be SUCCESS for successful ingest, instead got: " + response);

            Thread.sleep(100);

            // shutdown server
            serverProcess.destroy();
            boolean exited = serverProcess.waitFor(5, TimeUnit.SECONDS);
            if (!exited) {
                serverProcess.destroyForcibly();
            }

            // restart server
            startServer();

            Thread.sleep(1000); // wait for server to start

            // query to verify persistence
            // System.out.println("Querying for ID " + testId + " after restart on Port 5556...");
            RegistaObject retrieved_entry = client.fetchById(testId);

            assertNotNull(retrieved_entry, "Entry should be found after restart");
            assertEquals(testId, retrieved_entry.getId(), "IDs should match");
            assertEquals(RegistaObject.Type.STRING, retrieved_entry.getType(), "Types should match");
            assertEquals(content, retrieved_entry.getBlob().toStringUtf8(), "Content should match");
        }
    }
    

}
