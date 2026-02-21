package com.registadb;

import org.junit.jupiter.api.*;

import com.google.protobuf.ByteString;
import com.registadb.builders.EntryValueBuilder;
import com.registadb.readers.EntryValueReader;

import registadb.Playbook.EntryValue;
import registadb.Playbook.OperationStatus;
import registadb.Playbook.Response;

import static org.junit.jupiter.api.Assertions.*;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
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
            String content = "Hello RegistaDB!";
            EntryValue value = EntryValueBuilder.ofString(content);

            Response createResp = client.create(testId, value);

            assertEquals(OperationStatus.STATUS_OK, createResp.getStatus(), "Create operation should succeed, instead got: " + createResp.getStatus());

            Thread.sleep(100);
            Response readResp = client.read(testId);

            assertNotNull(readResp, "Read response should not be null");
            assertNotNull(readResp.getEntry(), "Entry in read response should not be null");

            EntryValue storedValue = readResp.getEntry().getData();

            assertEquals(testId, readResp.getEntry().getId(), "IDs should match");
            assertEquals(content, EntryValueReader.read(storedValue), "Content should match");
        }

        @Test
        @Order(2)
        @DisplayName("Test Delete")
        void testDelete() throws Exception {
            int testId = 999;

            // delete from smart line
            Response deleteStatus = client.delete(testId);
            assertEquals(OperationStatus.STATUS_OK, deleteStatus.getStatus(), "Delete should return SUCCESS for existing entry");

            Thread.sleep(100);

            // verify deletion
            Response postDeleteEntry = client.read(testId);
            assertEquals(OperationStatus.STATUS_NOT_FOUND, postDeleteEntry.getStatus(), "Read after delete should return NOT_FOUND");
        }

        @Test
        @Order(3)
        @DisplayName("Test Non Verified Ingest")
        void testNonVerifiedIngest() throws Exception {
            int testId = 1000;
            String content = "This is a non-verified ingest test.";
            var value = EntryValueBuilder.ofString(content);

            client.createNoReply(testId, value);

            Thread.sleep(100);

            // query to verify storage
            // System.out.println("Querying for ID " + testId + " on Port 5556...");
            Response postIngestResp = client.read(testId);

            assertNotNull(postIngestResp.getEntry(), "Entry should be found");

            EntryValue storedValue = postIngestResp.getEntry().getData();

            assertEquals(testId, postIngestResp.getEntry().getId(), "IDs should match");
            assertEquals(content, EntryValueReader.read(storedValue), "Content should match");
        }

        @Test
        @Order(4)
        @DisplayName("Test List Type Ingest and Retrieval")
        void testListTypeIngestAndRetrieval() throws Exception {
            int testId = 1002;
            List<Integer> int_content = List.of(10, 2 , 3);
            List<Long> content = int_content.stream().map(Integer::longValue).toList();
            EntryValue value = EntryValueBuilder.ofIntList(content);

            Response createResp = client.create(testId, value);

            assertEquals(OperationStatus.STATUS_OK, createResp.getStatus(), "Create operation should succeed, instead got: " + createResp.getStatus());

            Thread.sleep(100);
            Response readResp = client.read(testId);

            assertNotNull(readResp, "Read response should not be null");
            assertNotNull(readResp.getEntry(), "Entry in read response should not be null");

            EntryValue storedValue = readResp.getEntry().getData();

            assertEquals(testId, readResp.getEntry().getId(), "IDs should match");
            assertEquals(content, EntryValueReader.read(storedValue), "Content should match");
        }

        @Test
        @Order(5)
        @DisplayName("Test Hash Type Ingest and Retrieval")
        void testHashTypeIngestAndRetrieval() throws Exception {
            int testId = 1003;
            Map<String, String> content = Map.of(
                "type", "event",
                "source", "java",
                "status", "active"
            );
            EntryValue value = EntryValueBuilder.ofStringMap(content);

            Response createResp = client.create(testId, value);

            assertEquals(OperationStatus.STATUS_OK, createResp.getStatus(), "Create operation should succeed, instead got: " + createResp.getStatus());

            Thread.sleep(100);
            Response readResp = client.read(testId);

            assertNotNull(readResp, "Read response should not be null");
            assertNotNull(readResp.getEntry(), "Entry in read response should not be null");

            EntryValue storedValue = readResp.getEntry().getData();

            assertEquals(testId, readResp.getEntry().getId(), "IDs should match");
            assertEquals(content, EntryValueReader.read(storedValue), "Content should match");
        }

        @Test
        @Order(6)
        @DisplayName("Test JSON Type Ingest and Retrieval")
        void testJsonTypeIngestAndRetrieval() throws Exception {
            int testId = 1004;
            Map<String, Object> content = Map.of(
                "name", "Bob",
                "city", "Melbourne",
                "active", true,
                "score", 99
            );

            EntryValue value = EntryValueBuilder.ofJson(content);

            Response createResp = client.create(testId, value);

            assertEquals(OperationStatus.STATUS_OK, createResp.getStatus(), "Create operation should succeed, instead got: " + createResp.getStatus());

            Thread.sleep(100);
            Response readResp = client.read(testId);

            assertNotNull(readResp, "Read response should not be null");
            assertNotNull(readResp.getEntry(), "Entry in read response should not be null");

            EntryValue storedValue = readResp.getEntry().getData();
            var struct = (com.google.protobuf.Struct) EntryValueReader.read(storedValue);

            assertEquals(testId, readResp.getEntry().getId(), "IDs should match");
            assertEquals("Bob", struct.getFieldsOrThrow("name").getStringValue());
            assertEquals("Melbourne", struct.getFieldsOrThrow("city").getStringValue());
            assertTrue(struct.getFieldsOrThrow("active").getBoolValue());
            assertEquals(99.0, struct.getFieldsOrThrow("score").getNumberValue(), 0.001);
        }

        @Test
        @Order(7)
        @DisplayName("Test Vector Type Ingest and Retrieval")
        void testVectorTypeIngestAndRetrieval() throws Exception {
            int testId = 1005;
            List<Double> content = List.of(11.1, 2.22234567, 4765.11096);
            EntryValue value = EntryValueBuilder.ofDoubleList(content);

            Response createResp = client.create(testId, value);

            assertEquals(OperationStatus.STATUS_OK, createResp.getStatus(), "Create operation should succeed, instead got: " + createResp.getStatus());

            Thread.sleep(100);
            Response readResp = client.read(testId);

            assertNotNull(readResp, "Read response should not be null");
            assertNotNull(readResp.getEntry(), "Entry in read response should not be null");

            EntryValue storedValue = readResp.getEntry().getData();

            assertEquals(testId, readResp.getEntry().getId(), "IDs should match");
            assertEquals(content, EntryValueReader.read(storedValue), "Content should match");
        }

        @Test
        @Order(8)
        @DisplayName("Test server ID generation")
        void testServerIdGeneration() throws Exception {
            List<String> generatedIdContent = new ArrayList<>();
            for (int i = 1; i <= 5; i++) {
                generatedIdContent.add("Entry with server-generated ID " + i);
            }

            for (String idContent : generatedIdContent) {
                EntryValue value = EntryValueBuilder.ofString(idContent);

                // ingest from the smart lane
                // System.out.println("Storing entry with server-generated ID via Port 5556...");
                Response createResp = client.create(value);
                assertEquals(OperationStatus.STATUS_OK, createResp.getStatus(), "Create operation should succeed, instead got: " + createResp.getStatus());
            }

            Thread.sleep(100);

            // query to verify storage
            for (int i = 1; i <= generatedIdContent.size(); i++) {
                String content = generatedIdContent.get(i-1);
                // System.out.println("Querying for generated ID " + i + " on Port 5556...");
                Response readResp = client.read(i);

                assertNotNull(readResp, "Read response should not be null");
                assertNotNull(readResp.getEntry(), "Entry in read response should not be null");
                assertEquals(i, readResp.getEntry().getId(), "IDs should match");
                EntryValue storedValue = readResp.getEntry().getData();
                assertEquals(content, EntryValueReader.read(storedValue), "Content should match");
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
            String content = "This is a non-verified ingest test.";
            var value = EntryValueBuilder.ofString(content);

            client.createNoReply(testId, value);

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

            Response readResp = client.read(testId);

            assertNotNull(readResp, "Read response should not be null");
            assertNotNull(readResp.getEntry(), "Entry in read response should not be null");

            EntryValue storedValue = readResp.getEntry().getData();

            assertEquals(testId, readResp.getEntry().getId(), "IDs should match");
            assertEquals(content, EntryValueReader.read(storedValue), "Content should match");
        }
    }
}
