package com.registadb;

import registadb.Playbook.RegistaObject;

import java.io.FileOutputStream;
import org.zeromq.ZMQ;

import com.google.protobuf.ByteString;

import org.zeromq.ZContext;

/**
 * Producer is a simple Java application that demonstrates how to use the RegistaClient to send log entries to the RegistaDB server.
 * It can be used for bulk uploading logs via the fast lane (Port 5555) or for testing verified storage and retrieval via the smart lane (Port 5556).
 * 
 * Usage:
 * - To send a single log entry: `java Producer`
 * - To send multiple log entries: `java Producer <number_of_entries>`
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
            System.out.println("Starting bulk upload of " + numEntries + " logs..." );
            long startTime = System.currentTimeMillis();

            for (int i = 1; i <= numEntries; i++) {
                RegistaObject entry = RegistaObject.newBuilder()
                    .setType(RegistaObject.Type.STRING)
                    .setId(i)
                    .setBlob(ByteString.copyFromUtf8("Bulk message number " + i))
                    .build();

                client.pushEntry(entry);
            }

            long endTime = System.currentTimeMillis();
            System.out.println("Finished! Sent " + numEntries + " messages in " + (endTime - startTime) + "ms");
        } catch (Exception e) {
            e.printStackTrace();
        }

    }
}