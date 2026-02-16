package com.registadb;

import org.zeromq.SocketType;
import org.zeromq.ZContext;
import org.zeromq.ZMQ;
import registadb.Playbook.RegistaObject;
import registadb.Playbook.RegistaRequest;

public class RegistaClient implements AutoCloseable {
    private final ZContext context;
    private final ZMQ.Socket pushSocket;  // Port 5555
    private final ZMQ.Socket reqSocket;   // Port 5556

    public RegistaClient(String host) {
        this.context = new ZContext();
        
        // Fast Lane: Push (No ACK)
        this.pushSocket = context.createSocket(SocketType.PUSH);
        this.pushSocket.connect("tcp://" + host + ":5555");

        // Smart Lane: Req (Bidirectional)
        this.reqSocket = context.createSocket(SocketType.REQ);
        this.reqSocket.connect("tcp://" + host + ":5556");
    }

    // high-Speed Ingest (Uses Port 5555)
    public void pushLog(RegistaObject entry) {
        pushSocket.send(entry.toByteArray());
    }

    // verified Ingest (Uses Port 5556 via oneof envelope)
    public boolean storeLogVerified(RegistaObject entry) {
        RegistaRequest request = RegistaRequest.newBuilder()
                .setStoreRequest(entry)
                .build();
        reqSocket.send(request.toByteArray());
        byte[] reply = reqSocket.recv(0);
        return new String(reply).trim().equals("OK");
    }

    // query (Uses Port 5556 via oneof envelope)
    public RegistaObject fetchById(int id) throws Exception {
        RegistaRequest request = RegistaRequest.newBuilder()
                .setFetchId(id)
                .build();
        reqSocket.send(request.toByteArray());
        
        byte[] reply = reqSocket.recv(0);
        String status = new String(reply).trim();
        
        if (status.equals("NOT_FOUND") || status.equals("UNKNOWN_CMD")) {
            return null;
        }
        return RegistaObject.parseFrom(reply);
    }

    // delete (Uses Port 5556 via oneof envelope)
    public boolean deleteById(int id) throws Exception {
        RegistaRequest request = RegistaRequest.newBuilder()
                .setDeleteId(id)
                .build();
        reqSocket.send(request.toByteArray());
        
        byte[] reply = reqSocket.recv(0);
        String status = new String(reply).trim();
        return status.equals("OK");
    }

    @Override
    public void close() {
        context.close();
    }
}