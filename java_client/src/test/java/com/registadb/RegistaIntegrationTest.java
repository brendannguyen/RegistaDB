package com.registadb;

import org.junit.jupiter.api.*;

import com.google.protobuf.ByteString;

import registadb.Playbook.RegistaObject;

import static org.junit.jupiter.api.Assertions.*;
import java.io.File;
import java.io.IOException;
import java.util.concurrent.TimeUnit;

@TestInstance(TestInstance.Lifecycle.PER_CLASS)
public class RegistaIntegrationTest {
    private static Process serverProcess;
    private static RegistaClient client;
    private static final String testDbPath = "../test_db";

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

    @BeforeAll
    static void startServer() throws IOException, InterruptedException {

        // path to c++ binary
        String cppPath = "../regista_db/build/registadb_engine";

        // esnure clean slate
        File dbDir = new File(testDbPath);
        if (dbDir.exists()) { clearDirectory(dbDir); }

        // setup process builder
        ProcessBuilder builder = new ProcessBuilder(cppPath, "--path", testDbPath);
        builder.inheritIO(); // to see server logs in console
        serverProcess = builder.start();

        Thread.sleep(1000); // wait for server to start

        client = new RegistaClient("localhost");
    }

    @Nested
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
            System.out.println("Pushing entry to Port 5555...");
            client.pushEntry(newData);

            // small sleep to ensure C++ has processed the pull
            Thread.sleep(100);

             // query from the smart lane
            System.out.println("Querying for ID " + testId + " on Port 5556...");
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
            boolean deleteStatus = client.deleteById(testId);
            assertTrue(deleteStatus, "Delete should return true for existing entry");

            // verify deletion
            RegistaObject postDeleteEntry = client.fetchById(testId);
            assertNull(postDeleteEntry, "Entry should be null after deletion");
        }
    }

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
}
