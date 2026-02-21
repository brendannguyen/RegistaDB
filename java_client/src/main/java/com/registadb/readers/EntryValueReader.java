package com.registadb.readers;

import registadb.Playbook.EntryValue;

/**
 * EntryValueReader is a utility class that provides a method to read and convert an EntryValue protobuf into a native Java object.
 * It handles all the different types of values that can be stored in an EntryValue, such as strings, numbers, lists, maps, JSON, and bytes.
 */
public class EntryValueReader {
    private EntryValueReader() {}

    /**
     * Reads an EntryValue protobuf and converts it into the corresponding Java object based on the type of value stored in the EntryValue.
     * @param v The EntryValue protobuf to read and convert.
     * @return The Java object representation of the value stored in the EntryValue.
     */
    public static Object read(EntryValue v) {
        return switch (v.getKindCase()) {

            case STRING_VALUE -> v.getStringValue();

            case DOUBLE_VALUE -> v.getDoubleValue();

            case INT_VALUE -> v.getIntValue();

            case BOOL_VALUE -> v.getBoolValue();

            case STRING_LIST -> v.getStringList().getValueList();

            case DOUBLE_LIST -> v.getDoubleList().getValueList();

            case INT_LIST -> v.getIntList().getValueList();

            case BOOL_LIST -> v.getBoolList().getValueList();

            case STRING_MAP -> v.getStringMap().getValueMap();

            case JSON_VALUE -> v.getJsonValue();   // returns Struct

            case BYTES_VALUE -> v.getBytesValue().toByteArray();

            case KIND_NOT_SET -> null;
        };
    }
}
