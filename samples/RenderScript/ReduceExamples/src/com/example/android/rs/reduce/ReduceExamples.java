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

package com.example.android.rs.reduce;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import android.util.Log;
import android.widget.TextView;
import android.widget.TextView.BufferType;
import android.renderscript.*;
import java.util.Random;

public class ReduceExamples extends Activity {
    public static final String TAG = "ReduceExamples";
    private RenderScript mRS;
    private ScriptC_reduce mScript;

    private final static int sSize = 100000;

    /**
     * Creates a randomly initialized input array.
     */
    static float[] createInputArray(int seed) {
        float[] input = new float[sSize];
        Random rand = new Random(seed);
        for (int i = 0; i < input.length; ++i) {
            input[i] = rand.nextFloat();
        }
        return input;
    }

    /**
     * Creates an Allocation of floats of the given size.
     */
    public Allocation createFloatAllocation(int size) {
        return Allocation.createSized(mRS, Element.F32(mRS), size);
    }

    /**
     * Uses a reduction to get the index of the minimum value of the array.
     */
    public int indexOfMin(Allocation alloc) {
        int[] indices = new int[sSize];
        for (int i = 0; i < indices.length; ++i) {
            indices[i] = i;
        }

        mScript.set_inputAlloc(alloc);
        return mScript.reduce_indexOfMin(indices);
    }

    /**
     * Returns the index of the minimum value in the allocation, Java version.
     */
    public int indexOfMinJava(Allocation alloc) {
        float[] values = new float[sSize];
        alloc.copyTo(values);
        int index = 0;
        for (int i = 0; i < sSize; ++i) {
            if (values[i] <= values[index]) {
                index = i;
            }
        }
        return index;
    }

    /**
     * Uses a reduction to get the maximum value in the array. This reduction operation is done
     * using a call to a helper function, rather than by passing an input Allocation.
     */
    public float maxValue(float[] array) {
        return mScript.reduce_maxf(array);
    }

    /**
     * Returns the maximum value in the array, Java version.
     */
    public float maxValueJava(float[] array) {
        int index = 0;
        for (int i = 0; i < sSize; ++i) {
            if (array[i] >= array[index]) {
                index = i;
            }
        }
        return array[index];
    }

    /**
     * Uses a multiply followed by a reduce to perform a dot product.
     */
    public float dotProduct(Allocation first, Allocation second) {
        Allocation mulOutput = createFloatAllocation(sSize);
        Allocation reduceOutput = createFloatAllocation(1);

        mScript.forEach_mulFloat(first, second, mulOutput);
        mScript.reduce_addFloat(mulOutput, reduceOutput);

        float[] outArray = new float[1];
        reduceOutput.copyTo(outArray);
        return outArray[0];
    }

    /**
     * Returns the dot product of the two Allocations, Java version.
     */
    public float dotProductJava(Allocation first, Allocation second) {
        float[] firstValues = new float[sSize];
        float[] secondValues = new float[sSize];

        first.copyTo(firstValues);
        second.copyTo(secondValues);
        float accum = 0;
        for (int i = 0; i < sSize; ++i) {
            accum += firstValues[i] * secondValues[i];
        }
        return accum;
    }

    /**
     * Called with the activity is first created.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mRS = RenderScript.create(this);
        mScript = new ScriptC_reduce(mRS);

        float[] inputArray = createInputArray(0);
        Allocation firstAllocation = createFloatAllocation(sSize);
        Allocation secondAllocation = createFloatAllocation(sSize);

        firstAllocation.copyFrom(inputArray);
        secondAllocation.copyFrom(createInputArray(1));

        String output = String.format(
                "Index of min value: %d (Java: %d)\n" +
                "Max value in array: %e (Java: %e)\n" +
                "Dot product value:  %e (Java: %e)\n",
                indexOfMin(firstAllocation),
                indexOfMinJava(firstAllocation),
                maxValue(inputArray),
                maxValueJava(inputArray),
                dotProduct(firstAllocation, secondAllocation),
                dotProductJava(firstAllocation, secondAllocation));

        TextView display = new TextView(this);
        display.setTextSize(20.f);
        display.setText(output, BufferType.NORMAL);
        setContentView(display);
    }
}
