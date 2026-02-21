package com.registadb;

import com.registadb.builders.EntryValueBuilder;
import com.registadb.readers.EntryValueReader;

import registadb.Playbook.EntryValue;
import registadb.Playbook.OperationStatus;
import registadb.Playbook.Response;

/**
 * Producer is a simple Java application that demonstrates how to use the RegistaClient to send entries to the RegistaDB server using the no-reply push method.
 * It allows you to specify the number of entries to send as a command-line argument and measures the time taken for the bulk upload.
 * After sending the entries, it also performs a read operation to verify that the first entry was stored correctly.
 */
public class Producer {
    public static void main(String[] args) throws Exception {
        // Determine how many messages to send
        int numEntries = 1; 
        if (args.length > 0) {
            try {
                numEntries = Integer.parseInt(args[0]);
            } catch (NumberFormatException e) {
                System.err.println("Argument must be an integer. Defaulting to 1.");
            }
        }

        try (RegistaClient client = new RegistaClient("localhost")) {
            System.out.println("Starting bulk upload of " + numEntries + " entries..." );
            long startTime = System.currentTimeMillis();

            int final_id = 1;
            for (int i = 1; i <= numEntries; i++) {
                var value = EntryValueBuilder.ofString("Hello from no-reply! Entry #" + i);

                client.createNoReply(i, value);
                final_id = i;
            }

            long endTime = System.currentTimeMillis();
            System.out.println("Finished! Sent " + numEntries + " messages in " + (endTime - startTime) + "ms");

            Response readResp = client.read(final_id);

            if (readResp.getStatus() != OperationStatus.STATUS_OK) {
                throw new RuntimeException("Read failed: " + readResp.getMessage());
            }

            EntryValue storedValue = readResp.getEntry().getData();

            Object extracted = EntryValueReader.read(storedValue);

            System.out.println("Extracted Java value = " + extracted);

        } catch (Exception e) {
            e.printStackTrace();
        }

    }
}