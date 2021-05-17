/*
 * Copyright (C) 2011-2014 The Android Open Source Project
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

/*
 * This file is auto-generated. DO NOT MODIFY!
 * The source Renderscript file: ./Levels/src/com/android/rs/levels/levels.rscript
 */

package com.android.rs.levels;

import android.os.Build;
import android.os.Process;
import java.lang.reflect.Field;
import android.renderscript.*;
import com.android.rs.levels.levelsBitCode;

/**
 * @hide
 */
public class ScriptC_levels extends ScriptC {
    private static final String __rs_resource_name = "levels";
    // Constructor
    public  ScriptC_levels(RenderScript rs) {
        super(rs,
              __rs_resource_name,
              levelsBitCode.getBitCode32(),
              levelsBitCode.getBitCode64());
        __F32 = Element.F32(rs);
        __U8_4 = Element.U8_4(rs);
    }

    private Element __F32;
    private Element __U8_4;
    private FieldPacker __rs_fp_F32;
    private final static int mExportVarIdx_inBlack = 0;
    private float mExportVar_inBlack;
    public synchronized void set_inBlack(float v) {
        setVar(mExportVarIdx_inBlack, v);
        mExportVar_inBlack = v;
    }

    public float get_inBlack() {
        return mExportVar_inBlack;
    }

    public Script.FieldID getFieldID_inBlack() {
        return createFieldID(mExportVarIdx_inBlack, null);
    }

    private final static int mExportVarIdx_outBlack = 1;
    private float mExportVar_outBlack;
    public synchronized void set_outBlack(float v) {
        setVar(mExportVarIdx_outBlack, v);
        mExportVar_outBlack = v;
    }

    public float get_outBlack() {
        return mExportVar_outBlack;
    }

    public Script.FieldID getFieldID_outBlack() {
        return createFieldID(mExportVarIdx_outBlack, null);
    }

    private final static int mExportVarIdx_inWMinInB = 2;
    private float mExportVar_inWMinInB;
    public synchronized void set_inWMinInB(float v) {
        setVar(mExportVarIdx_inWMinInB, v);
        mExportVar_inWMinInB = v;
    }

    public float get_inWMinInB() {
        return mExportVar_inWMinInB;
    }

    public Script.FieldID getFieldID_inWMinInB() {
        return createFieldID(mExportVarIdx_inWMinInB, null);
    }

    private final static int mExportVarIdx_outWMinOutB = 3;
    private float mExportVar_outWMinOutB;
    public synchronized void set_outWMinOutB(float v) {
        setVar(mExportVarIdx_outWMinOutB, v);
        mExportVar_outWMinOutB = v;
    }

    public float get_outWMinOutB() {
        return mExportVar_outWMinOutB;
    }

    public Script.FieldID getFieldID_outWMinOutB() {
        return createFieldID(mExportVarIdx_outWMinOutB, null);
    }

    private final static int mExportVarIdx_overInWMinInB = 4;
    private float mExportVar_overInWMinInB;
    public synchronized void set_overInWMinInB(float v) {
        setVar(mExportVarIdx_overInWMinInB, v);
        mExportVar_overInWMinInB = v;
    }

    public float get_overInWMinInB() {
        return mExportVar_overInWMinInB;
    }

    public Script.FieldID getFieldID_overInWMinInB() {
        return createFieldID(mExportVarIdx_overInWMinInB, null);
    }

    private final static int mExportVarIdx_gamma = 5;
    private float mExportVar_gamma;
    public synchronized void set_gamma(float v) {
        setVar(mExportVarIdx_gamma, v);
        mExportVar_gamma = v;
    }

    public float get_gamma() {
        return mExportVar_gamma;
    }

    public Script.FieldID getFieldID_gamma() {
        return createFieldID(mExportVarIdx_gamma, null);
    }

    private final static int mExportVarIdx_colorMat = 6;
    private Matrix3f mExportVar_colorMat;
    public synchronized void set_colorMat(Matrix3f v) {
        mExportVar_colorMat = v;
        FieldPacker fp = new FieldPacker(36);
        fp.addMatrix(v);
        setVar(mExportVarIdx_colorMat, fp);
    }

    public Matrix3f get_colorMat() {
        return mExportVar_colorMat;
    }

    public Script.FieldID getFieldID_colorMat() {
        return createFieldID(mExportVarIdx_colorMat, null);
    }

    private final static int mExportForEachIdx_root = 0;
    public Script.KernelID getKernelID_root() {
        return createKernelID(mExportForEachIdx_root, 27, null, null);
    }

    public void forEach_root(Allocation ain, Allocation aout) {
        forEach_root(ain, aout, null);
    }

    public void forEach_root(Allocation ain, Allocation aout, Script.LaunchOptions sc) {
        // check ain
        if (!ain.getType().getElement().isCompatible(__U8_4)) {
            throw new RSRuntimeException("Type mismatch with U8_4!");
        }
        // check aout
        if (!aout.getType().getElement().isCompatible(__U8_4)) {
            throw new RSRuntimeException("Type mismatch with U8_4!");
        }
        Type t0, t1;        // Verify dimensions
        t0 = ain.getType();
        t1 = aout.getType();
        if ((t0.getCount() != t1.getCount()) ||
            (t0.getX() != t1.getX()) ||
            (t0.getY() != t1.getY()) ||
            (t0.getZ() != t1.getZ()) ||
            (t0.hasFaces()   != t1.hasFaces()) ||
            (t0.hasMipmaps() != t1.hasMipmaps())) {
            throw new RSRuntimeException("Dimension mismatch between parameters ain and aout!");
        }

        forEach(mExportForEachIdx_root, ain, aout, null, sc);
    }

}

