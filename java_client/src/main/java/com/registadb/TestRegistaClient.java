package com.registadb;

import registadb.Playbook.EntryValue;
import registadb.Playbook.OperationStatus;
import registadb.Playbook.Response;

import com.registadb.builders.EntryValueBuilder;
import com.registadb.readers.EntryValueReader;

/**
 * TestRegistaClient is a simple test class to demonstrate the usage of RegistaClient for basic CRUD operations.
 * It creates an entry, reads it back, and then deletes it, printing the results at each step.
 */
public class TestRegistaClient {
    public static void main(String[] args) {
        try (RegistaClient client = new RegistaClient("localhost")) {
            
            int testId = 999;
            EntryValue value = EntryValueBuilder.ofString("Hello RegistaDB!");

            Response createResp = client.create(testId, value);

            if (createResp.getStatus() != OperationStatus.STATUS_OK) {
                throw new RuntimeException("Create failed: " + createResp.getMessage());
            }
            System.out.println("Created entry with ID = " + testId);

            Response readResp = client.read(testId);

            if (readResp.getStatus() != OperationStatus.STATUS_OK) {
                throw new RuntimeException("Read failed: " + readResp.getMessage());
            }

            EntryValue storedValue = readResp.getEntry().getData();

            Object extracted = EntryValueReader.read(storedValue);

            System.out.println("Extracted Java value = " + extracted);

            Response deleteResp = client.delete(testId);

            if (deleteResp.getStatus() != OperationStatus.STATUS_OK) {
                throw new RuntimeException("Delete failed: " + deleteResp.getMessage());
            }

            System.out.println("Deleted entry with ID = " + testId);

            Response readAfterDelete = client.read(testId);

            if (readAfterDelete.getStatus() == OperationStatus.STATUS_NOT_FOUND) {
                System.out.println("Confirmed deletion: entry " + testId + " no longer exists");
            } else {
                System.out.println("Unexpected read result: " + readAfterDelete.getStatus());
            }

        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}