package com.registadb; // Make sure this matches your folder structure!

import registadb.Playbook.LogEntry;
import registadb.Playbook.RegistaRequest;

public class TestRegistaClient {
    public static void main(String[] args) {
        try (RegistaClient client = new RegistaClient("localhost")) {
            
            int testId = 999;
            LogEntry newLog = LogEntry.newBuilder()
                    .setId(testId)
                    .setCategory("DEBUG")
                    .setContent("Testing Dual Port Architecture")
                    .setTimestamp(System.currentTimeMillis())
                    .build();

            // Step 1: Push to the fast lane
            System.out.println("Pushing log to Port 5555...");
            client.pushLog(newLog);

            // Step 2: Small sleep to ensure C++ has processed the pull
            Thread.sleep(100);

            // Step 3: Query from the smart lane
            System.out.println("Querying for ID " + testId + " on Port 5556...");
            LogEntry fetched = client.fetchById(testId);

            if (fetched != null) {
                System.out.println("Success! Found: " + fetched.getContent());
            } else {
                System.out.println("Log not found yet.");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}