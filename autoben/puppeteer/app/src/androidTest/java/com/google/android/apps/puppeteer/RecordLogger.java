/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.android.apps.puppeteer;

import android.os.SystemClock;
import android.util.JsonWriter;
import android.util.Log;
import android.util.Pair;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Record logger.
 * It stores records in memory with timestamps attached to records when adding.
 * It writes the records to file when safeToFile is called().
 */
public class RecordLogger {

    // Name of the logger, used as filename for logs to store.
    private final String mLoggerName;

    // List of (timestamp, data) tuple.
    private final List<Record> mRecords;

    // Optional. Base directory to store log file.
    private String mBaseDir;

    /**
     * Constructor.
     * @param loggerName filename of log to save under baseDir.
     */
    public RecordLogger(String loggerName) {
        mLoggerName = loggerName;
        mRecords = new ArrayList<>();
    }

    public void setBaseDir(String baseDir) {
        mBaseDir = baseDir;
    }

    /**
     * Writes measurement result to JSON file.
     */
    public void saveToFile() {
        File file = new File(mBaseDir, mLoggerName + ".json");

        try (FileOutputStream outputStream = new FileOutputStream(file)) {
            Log.i("RecordLogger", "Save log to file: " + file.getCanonicalPath());
            RecordWriter writer = new RecordWriter(new OutputStreamWriter(outputStream));
            writer.write(mRecords);
            writer.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * Adds a record with current timestamp.
     * @param record record object of type Number, String, Array, or Map.
     * @param label label of the record.
     */
    protected final void addRecord(Object record, String label) {
        addRecord(SystemClock.uptimeMillis(), record, label);
    }

    /**
     * Adds a record with timestamp given.
     * @param timestamp timestamp in nanoseconds.
     * @param record record object of type Number, String, Array, or Map.
     * @param label label of the record.
     */
    protected final void addRecord(long timestamp, Object record, String label) {
        mRecords.add(new Record(timestamp, record, label));
    }

    /**
     * Data structure of a timestamped record.
     */
    private static class Record {
        final long timestamp;
        final Object record;
        final String label;

        Record(long timestamp, Object record, String label) {
            this.timestamp = timestamp;
            this.record = record;
            this.label = label;
        }
    }

    /**
     * A JSON writer for object of type Number, String, Pair, Map, List and Record.
     *
     * Usage:
     *   writer = RecordWriter(stringWriter);
     *   writer.write(...);
     *   ...
     *   writer.close();
     */
    private static class RecordWriter {
        JsonWriter jsonWriter;

        RecordWriter(Writer writer) {
            jsonWriter = new JsonWriter(writer);
            jsonWriter.setIndent("  ");
        }

        void close() throws IOException {
            jsonWriter.close();
        }

        @SuppressWarnings("unchecked")
        void write(Object data) throws IOException {
            if (data instanceof Number) {
                jsonWriter.value((Number) data);
            } else if (data instanceof String) {
                jsonWriter.value((String) data);
            } else if (data instanceof Pair) {
                writePair((Pair) data);
            } else if (data instanceof List) {
                writeList((List) data);
            } else if (data instanceof Map) {
                writeMap((Map) data);
            } else if (data instanceof Record) {
                writeRecord((Record) data);
            } else {
                // Fallback unmatched type as string.
                jsonWriter.value(data.toString());
            }
        }

        void writePair(Pair pair) throws IOException {
            jsonWriter.beginArray();
            write(pair.first);
            write(pair.second);
            jsonWriter.endArray();
        }

        void writeMap(Map<String, Object> map) throws IOException {
            jsonWriter.beginObject();
            for (Map.Entry<String, Object> entry : map.entrySet()) {
                jsonWriter.name(entry.getKey());
                write(entry.getValue());
            }
            jsonWriter.endObject();
        }

        void writeList(List array) throws IOException {
            jsonWriter.beginArray();
            for (Object o : array) {
                write(o);
            }
            jsonWriter.endArray();
        }

        void writeRecord(Record record) throws IOException {
            jsonWriter.beginObject();
            jsonWriter.name("timestamp_ns").value(record.timestamp);
            jsonWriter.name(record.label);
            write(record.record);
            jsonWriter.endObject();
        }
    }
}
