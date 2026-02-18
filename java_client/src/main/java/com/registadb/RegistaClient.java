package com.registadb;

import org.zeromq.SocketType;
import org.zeromq.ZContext;
import org.zeromq.ZMQ;
import registadb.Playbook.RegistaObject;
import registadb.Playbook.RegistaRequest;

/**
 * RegistaClient is a Java client for interacting with the RegistaDB server.
 * It provides methods for high-speed ingestion (push) and verified operations (store, fetch, delete).
 * 
 * The client uses ZeroMQ for communication:
 * - Port 5555 for push (fire-and-forget)
 * - Port 5556 for request-reply interactions (store with verification, fetch, delete)
 */
public class RegistaClient implements AutoCloseable {
    private final ZContext context;
    private final ZMQ.Socket pushSocket;  // Port 5555
    private final ZMQ.Socket reqSocket;   // Port 5556

    /**
     * Constructor to initialize the RegistaClient with the server's host address.
     * @param host The hostname or IP address of the RegistaDB server (e.g., "localhost")
     */
    public RegistaClient(String host) {
        this.context = new ZContext();
        
        // Fast Lane: Push (No ACK)
        this.pushSocket = context.createSocket(SocketType.PUSH);
        this.pushSocket.connect("tcp://" + host + ":5555");

        // Smart Lane: Req (Bidirectional)
        this.reqSocket = context.createSocket(SocketType.REQ);
        this.reqSocket.connect("tcp://" + host + ":5556");
    }

    /**
     * Pushes a RegistaObject entry to the server via the fast lane (Port 5555).
     * @param entry The RegistaObject to be sent to the server. This method does not wait for any acknowledgment.
     */
    public void pushEntry(RegistaObject entry) {
        pushSocket.send(entry.toByteArray());
    }

    /**
     * Sends a RegistaObject entry to the server for storage with verification via the smart lane (Port 5556).
     * @param entry The RegistaObject to be stored on the server. The server will verify the entry and respond with a status message.
     * @return A string response from the server indicating the result of the store operation (e.g., "OK", "ERR_TYPE_MISMATCH", "ERR_INTERNAL_ERROR").
     */
    public String storeEntryVerified(RegistaObject entry) {
        RegistaRequest request = RegistaRequest.newBuilder()
                .setStoreRequest(entry)
                .build();
        reqSocket.send(request.toByteArray());
        byte[] reply = reqSocket.recv(0);
        return new String(reply).trim();
    }

    /**
     * Fetches a RegistaObject from the server by its ID via the smart lane (Port 5556).
     * @param id The integer ID of the RegistaObject to be fetched from the server.
     * @return The RegistaObject corresponding to the provided ID if found; otherwise, returns null if the object is not found or if there was an error.
     * @throws Exception if there is an error during communication with the server or if the response cannot be parsed as a RegistaObject.
     */
    public RegistaObject fetchById(int id) throws Exception {
        RegistaRequest request = RegistaRequest.newBuilder()
                .setFetchId(id)
                .build();
        reqSocket.send(request.toByteArray());
        
        byte[] reply = reqSocket.recv(0);
        String status = new String(reply).trim();
        
        if (status.equals("ERR_NOT_FOUND") || status.equals("UNKNOWN_CMD") || status.equals("ERR_INTERNAL_ERROR")) {
            return null;
        }
        return RegistaObject.parseFrom(reply);
    }

    /**
     * Deletes (tombstones) a RegistaObject on the server by its ID via the smart lane (Port 5556).
     * @param id The integer ID of the RegistaObject to be deleted on the server.
     * @return A string response from the server indicating the result of the delete operation (e.g., "SUCCESS", "ERR_NOT_FOUND", "UNKNOWN_CMD", "ERR_INTERNAL_ERROR").
     * @throws Exception if there is an error during communication with the server or if the response cannot be parsed correctly.
     */
    public String deleteById(int id) throws Exception {
        RegistaRequest request = RegistaRequest.newBuilder()
                .setDeleteId(id)
                .build();
        reqSocket.send(request.toByteArray());
        
        byte[] reply = reqSocket.recv(0);
        return new String(reply).trim();
    }

    /**
     * Closes the underlying ZContext and associated sockets.
     */
    @Override
    public void close() {
        context.close();
    }
}