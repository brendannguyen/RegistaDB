package com.registadb;

import java.util.Map;

import org.zeromq.SocketType;
import org.zeromq.ZContext;
import org.zeromq.ZMQ;

import com.registadb.builders.EntryBuilder;

import java.io.IOException;
import registadb.Playbook.Entry;
import registadb.Playbook.EntryValue;
import registadb.Playbook.OperationType;
import registadb.Playbook.Request;
import registadb.Playbook.Response;


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
     * Creates a new entry in RegistaDB with the given value and optional metadata, returning the server's response.
     * @param value The value to store in the entry.
     * @return Response from the server indicating the result of the create operation.
     * @throws IOException if there is an error sending the request or receiving the response.
     */
    public Response create(EntryValue value) throws IOException {
        return create(0, Map.of(), value);
    }
    /**
     * Creates a new entry in RegistaDB with the given ID and value, returning the server's response.
     * @param id The ID to assign to the entry (0 for server-generated).
     * @param value The value to store in the entry.
     * @return Response from the server indicating the result of the create operation.
     * @throws IOException if there is an error sending the request or receiving the response.
     */
    public Response create(long id, EntryValue value) throws IOException {
        return create(id, Map.of(), value);
    }
    /**
     * Creates a new entry in RegistaDB with the given metadata and value, returning the server's response.
     * @param metadata A map of metadata key-value pairs to associate with the entry.
     * @param value The value to store in the entry.
     * @return Response from the server indicating the result of the create operation.
     * @throws IOException if there is an error sending the request or receiving the response.
     */
    public Response create(Map<String,String> metadata, EntryValue value) throws IOException {
        return create(0, metadata, value);
    }
    /**
     * Creates a new entry in RegistaDB with the given ID, metadata, and value, returning the server's response.
     * @param id The ID to assign to the entry (0 for server-generated).
     * @param metadata A map of metadata key-value pairs to associate with the entry.
     * @param value The value to store in the entry.
     * @return Response from the server indicating the result of the create operation.
     * @throws IOException if there is an error sending the request or receiving the response.
     */
    public Response create(long id, Map<String,String> metadata, EntryValue value) throws IOException {
        Entry entry = new EntryBuilder()
                .setId(id)
                .setMetadata(metadata)
                .setValue(value)
                .build();

        Request req = Request.newBuilder()
                .setOp(OperationType.OP_CREATE)
                .setEntry(entry)
                .build();

        return sendWithReply(req);
    }

    /**
     * Sends a Request protobuf to the server and waits for a Response, handling serialization and deserialization.
     * @param req The Request protobuf to send to the server.
     * @return The Response protobuf received from the server.
     * @throws IOException if there is an error sending the request or receiving the response.
     */
    private Response sendWithReply(Request req) throws IOException {
        reqSocket.send(req.toByteArray(), 0);

        byte[] replyBytes = reqSocket.recv(0);
        if (replyBytes == null) {
            throw new IOException("No reply received");
        }

        Response resp = Response.parseFrom(replyBytes);
        return resp;
    }

   /**
    * Creates a new entry in RegistaDB with the given ID, metadata, and value without waiting for a response from the server (fire-and-forget).
    * @param value The value to store in the entry.
    */
    public void createNoReply(EntryValue value) {
        createNoReply(0, Map.of(), value);
    }
    /**
     * Creates a new entry in RegistaDB with the given ID and value without waiting for a response from the server (fire-and-forget).
     * @param id The ID to assign to the entry (0 for server-generated).
     * @param value The value to store in the entry.

     */
    public void createNoReply(long id, EntryValue value) {
        createNoReply(id, Map.of(), value);
    }
    /**
     * Creates a new entry in RegistaDB with the given metadata and value without waiting for a response from the server (fire-and-forget).
     * @param metadata A map of metadata key-value pairs to associate with the entry.
      * @param value The value to store in the entry.

     */
    public void createNoReply(Map<String,String> metadata, EntryValue value) {
        createNoReply(0, metadata, value);
    }

    /**
     * Creates a new entry in RegistaDB with the given ID, metadata, and value without waiting for a response from the server (fire-and-forget).
     * @param id The ID to assign to the entry (0 for server-generated).
     * @param metadata A map of metadata key-value pairs to associate with the entry.
     * @param value The value to store in the entry.

     */
    public void createNoReply(long id, Map<String,String> metadata, EntryValue value) {
        Entry entry = new EntryBuilder()
                .setId(id)
                .setMetadata(metadata)
                .setValue(value)
                .build();

        pushSocket.send(entry.toByteArray(), 0);
    }

    /**
     * Fetches an entry from RegistaDB by its ID, returning the server's response which includes the entry data if found.
     * @param id The ID of the entry to fetch.
     * @return Response from the server indicating the result of the read operation, including the entry data if the read was successful.
     * @throws IOException if there is an error sending the request or receiving the response.
     */
    public Response read(long id) throws IOException {
        Request req = Request.newBuilder()
                .setOp(OperationType.OP_READ)
                .setId(id)
                .build();
        return sendWithReply(req);
    }

    /**
     * Deletes an entry from RegistaDB by its ID, returning the server's response indicating the result of the delete operation.
     * @param id The ID of the entry to delete.
     * @return Response from the server indicating the result of the delete operation.
     * @throws IOException if there is an error sending the request or receiving the response.
     */
    public Response delete(long id) throws IOException {
        Request req = Request.newBuilder()
                .setOp(OperationType.OP_DELETE)
                .setId(id)
                .build();
        return sendWithReply(req);
    }

    /**
     * Closes the underlying ZContext and associated sockets.
     */
    @Override
    public void close() {
        context.close();
    }
}