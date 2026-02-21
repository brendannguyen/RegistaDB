package com.registadb.builders;

import registadb.Playbook.BoolList;
import registadb.Playbook.EntryValue;
import registadb.Playbook.StringList;
import registadb.Playbook.StringMap;
import registadb.Playbook.DoubleList;
import registadb.Playbook.IntList;

import com.google.protobuf.ByteString;
import com.google.protobuf.Struct;
import com.google.protobuf.Value;

import java.util.List;
import java.util.Map;

/**
 * EntryValueBuilder is a utility class that provides static methods to create EntryValue protobufs from various Java types.
 * It simplifies the process of constructing EntryValue objects by providing convenient factory methods for different data types.
 */
public class EntryValueBuilder {
    private EntryValueBuilder() {}

    /**
     * Creates an EntryValue protobuf with a string value.
     * @param s The string value to store in the EntryValue.
     * @return An EntryValue protobuf containing the provided string value.
     */
    public static EntryValue ofString(String s) {
        return EntryValue.newBuilder().setStringValue(s).build();
    }

    /**
     * Creates an EntryValue protobuf with a double value.
     * @param d The double value to store in the EntryValue.
     * @return An EntryValue protobuf containing the provided double value.
     */
    public static EntryValue ofDouble(double d) {
        return EntryValue.newBuilder().setDoubleValue(d).build();
    }

    /**
     * Creates an EntryValue protobuf with an integer value.
     * @param v The integer value to store in the EntryValue.
     * @return An EntryValue protobuf containing the provided integer value.
     */
    public static EntryValue ofInt(long v) {
        return EntryValue.newBuilder().setIntValue(v).build();
    }

    /**
     * Creates an EntryValue protobuf with a boolean value.
     * @param b The boolean value to store in the EntryValue.
     * @return An EntryValue protobuf containing the provided boolean value.
     */
    public static EntryValue ofBool(boolean b) {
        return EntryValue.newBuilder().setBoolValue(b).build();
    }

    /**
     * Creates an EntryValue protobuf with a list of strings.
     * @param list The list of strings to store in the EntryValue.
     * @return An EntryValue protobuf containing the provided list of strings.
     */
    public static EntryValue ofStringList(List<String> list) {
        return EntryValue.newBuilder()
                .setStringList(StringList.newBuilder().addAllValue(list))
                .build();
    }

    /**
     * Creates an EntryValue protobuf with a list of doubles.
     * @param list The list of doubles to store in the EntryValue.
     * @return An EntryValue protobuf containing the provided list of doubles.
     */
    public static EntryValue ofDoubleList(List<Double> list) {
        return EntryValue.newBuilder()
                .setDoubleList(DoubleList.newBuilder().addAllValue(list))
                .build();
    }

    /**
     * Creates an EntryValue protobuf with a list of longs.
     * @param list The list of longs to store in the EntryValue.
     * @return An EntryValue protobuf containing the provided list of longs.
     */
    public static EntryValue ofIntList(List<Long> list) {
        return EntryValue.newBuilder()
                .setIntList(IntList.newBuilder().addAllValue(list))
                .build();
    }

    /**
     * Creates an EntryValue protobuf with a list of booleans.
     * @param list The list of booleans to store in the EntryValue.
     * @return An EntryValue protobuf containing the provided list of booleans.
     */
    public static EntryValue ofBoolList(List<Boolean> list) {
        return EntryValue.newBuilder()
                .setBoolList(BoolList.newBuilder().addAllValue(list))
                .build();
    }

    /**
     * Creates an EntryValue protobuf with a map of strings.
     * @param map The map of strings to store in the EntryValue.
     * @return An EntryValue protobuf containing the provided map of strings.
     */
    public static EntryValue ofStringMap(Map<String,String> map) {
        return EntryValue.newBuilder()
                .setStringMap(StringMap.newBuilder().putAllValue(map))
                .build();
    }

    /**
     * Creates an EntryValue protobuf with a byte array value.
     * @param bytes The byte array to store in the EntryValue.
     * @return An EntryValue protobuf containing the provided byte array value.
     */
    public static EntryValue ofBytes(byte[] bytes) {
        return EntryValue.newBuilder()
                .setBytesValue(ByteString.copyFrom(bytes))
                .build();
    }

    /**
     * Creates an EntryValue protobuf with a JSON value represented as a map, converting it to a Struct protobuf.
     * @param json The map representing the JSON object to store in the EntryValue.
     * @return An EntryValue protobuf containing the provided JSON value as a Struct.
     */
    public static EntryValue ofJson(Map<String,Object> json) {
        Struct.Builder sb = Struct.newBuilder();
        for (var e : json.entrySet()) {
            sb.putFields(e.getKey(), toValue(e.getValue()));
        }
        return EntryValue.newBuilder().setJsonValue(sb.build()).build();
    }

    /**
     * Helper method to convert a Java object into a protobuf Value, used for constructing JSON EntryValues.
     * @param o The Java object to convert into a protobuf Value. Supported types include null, Boolean, Number, and String (default).
     * @return A protobuf Value representing the provided Java object, suitable for inclusion in a Struct for JSON EntryValues.
     */
    private static Value toValue(Object o) {
        Value.Builder vb = Value.newBuilder();
        if (o == null) return vb.setNullValueValue(0).build();
        if (o instanceof Boolean b) return vb.setBoolValue(b).build();
        if (o instanceof Number n) return vb.setNumberValue(n.doubleValue()).build();
        return vb.setStringValue(o.toString()).build();
    }
}
