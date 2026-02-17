package com.registadb; // Make sure this matches your folder structure!

import registadb.Playbook.RegistaObject;
import registadb.Playbook.RegistaRequest;
import registadb.Playbook.StatusCode;

import com.google.protobuf.ByteString;


public class TestRegistaClient {
    public static void main(String[] args) {
        try (RegistaClient client = new RegistaClient("localhost")) {
            
            int testId = 999;
            RegistaObject newData = RegistaObject.newBuilder()
                    .setType(RegistaObject.Type.STRING)
                    .setId(testId)
                    .setBlob(ByteString.copyFromUtf8("This is a test."))
                    .build();

            // push to the fast lane
            System.out.println("Pushing log to Port 5555...");
            client.pushLog(newData);

            // small sleep to ensure C++ has processed the pull
            Thread.sleep(100);

            // // ingest from the smart lane
            // System.out.println("Storing log via Port 5556 with verification...");
            // String response = client.storeLogVerified(newData);

            // if (response.equals("OK")) {
            //     System.out.println("Success! Log stored with verification.");
            // } else {
            //     System.out.println("Failed to store log: " + response);
            // }

            // query from the smart lane
            System.out.println("Querying for ID " + testId + " on Port 5556...");
            RegistaObject fetched = client.fetchById(testId);

            if (fetched != null) {
                System.out.println("Success! Found: " + fetched.getBlob().toStringUtf8());
            } else {
                System.out.println("Log not found yet.");
            }

            // delete from smart line
            boolean status = client.deleteById(testId);
            if (status) {
                System.out.println(status);
                System.out.println("Success! Deleted (tombstoned): " + testId);
            } else {
                System.out.println(status);
                System.out.println("Log not found/deleted.");
            }

        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}