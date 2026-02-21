package com.registadb.builders;

import java.util.Map;

import registadb.Playbook.Entry;
import registadb.Playbook.EntryValue;

/**
 * EntryBuilder is a utility class that provides a fluent builder pattern for constructing Entry protobufs.
 */
public class EntryBuilder {
    private long id = 0; // 0 = server generates
    private Map<String,String> metadata = Map.of();
    private EntryValue value;

    /**
     * Sets the ID for the entry being built. If the ID is set to 0, the server will generate a unique ID for the entry upon creation.
     * @param id The ID to assign to the entry (0 for server-generated).
     * @return The EntryBuilder instance for method chaining.
     */
    public EntryBuilder setId(long id) {
        this.id = id;
        return this;
    }

    /**
     * Sets the metadata for the entry being built. Metadata is a map of key-value pairs that can be associated with the entry for additional context or information.
     * @param metadata A map of metadata key-value pairs to associate with the entry.
     * @return The EntryBuilder instance for method chaining.
     */
    public EntryBuilder setMetadata(Map<String,String> metadata) {
        this.metadata = metadata;
        return this;
    }

    /**
     * Sets the value for the entry being built. The value is an EntryValue protobuf that can represent various types of data.
     * @param value The EntryValue protobuf to store in the entry.
     * @return The EntryBuilder instance for method chaining.
     */
    public EntryBuilder setValue(EntryValue value) {
        this.value = value;
        return this;
    }

    /**
     * Builds and returns an Entry protobuf based on the ID, metadata, and value that have been set in the builder. If no ID is set, it defaults to 0, indicating that the server should generate an ID for the entry.
     * @return An Entry protobuf constructed with the specified ID, metadata, and value.
     */
    public Entry build() {
        Entry.Builder b = Entry.newBuilder()
                .setId(id)
                .setData(value);

        if (!metadata.isEmpty()) {
            b.putAllMetadata(metadata);
        }

        return b.build();
    }

}
