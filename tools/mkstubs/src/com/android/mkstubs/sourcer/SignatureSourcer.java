/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.android.mkstubs.sourcer;

import org.objectweb.asm.Type;
import org.objectweb.asm.signature.SignatureReader;
import org.objectweb.asm.signature.SignatureVisitor;
import org.objectweb.asm.signature.SignatureWriter;

import java.util.ArrayList;

/**
 * Note: most of the implementation is a duplicate of
 * ASM's SignatureWriter with some slight variations.
 * <p/>
 * Note: When processing a method's signature, the signature order is the
 * reverse of the source order, e.g. it is (parameters)return-type where
 * we want to generate "return-type method-name (parameters)".
 * So in this case the return-type and parameters are not output directly
 * but are instead accumulated in internal variables.
 */
class SignatureSourcer implements SignatureVisitor {

    /**
     * Buffer used to construct the signature.
     */
    private final StringBuilder mBuf = new StringBuilder();

    /**
     * Buffer used to construct the formals signature.
     */
    private final StringBuilder mFormalsBuf = new StringBuilder();

    /**
     * Indicates if the signature is currently processing formal type parameters.
     */
    private boolean mWritingFormals;

    /**
     * Stack used to keep track of class types that have arguments. Each element
     * of this stack is a boolean encoded in one bit. The top of the stack is
     * the lowest order bit. Pushing false = *2, pushing true = *2+1, popping =
     * /2.
     */
    private int mArgumentStack;

    private SignatureSourcer mReturnType;

    private SignatureSourcer mSuperClass;

    private ArrayList<SignatureSourcer> mParameters = new ArrayList<SignatureSourcer>();


    
    /**
     * Constructs a new {@link SignatureWriter} object.
     */
    public SignatureSourcer() {
    }
    
    private StringBuilder getBuf() {
        if (mWritingFormals) {
            return mFormalsBuf;
        } else {
            return mBuf;
        }
    }

    /**
     * Contains the whole signature type when called by
     * {@link SignatureReader#acceptType(SignatureVisitor)} or just the formals if
     * called by {@link SignatureReader#accept(SignatureVisitor)}.
     */
    @Override
    public String toString() {
        return mBuf.toString();
    }

    /**
     * Will be non-null if a return type was processed
     * by {@link SignatureReader#accept(SignatureVisitor)}
     */
    public SignatureSourcer getReturnType() {
        return mReturnType;
    }
    
    /**
     * Will be non-empty if a parameters were processed
     * by {@link SignatureReader#accept(SignatureVisitor)}
     */
    public ArrayList<SignatureSourcer> getParameters() {
        return mParameters;
    }
    
    /**
     * True if the signature contains formal type parameters, which are available 
     * via {@link #formalsToString()} after calling {@link SignatureReader#accept(SignatureVisitor)}
     */
    public boolean hasFormalsContent() {
        return mFormalsBuf.length() > 0;
    }
    
    public String formalsToString() {
        return mFormalsBuf.toString();
    }
    
    /**
     * Will be non-null if a super class was processed
     * by {@link SignatureReader#accept(SignatureVisitor)}
     */
    public SignatureSourcer getSuperClass() {
        return mSuperClass;
    }

    // ------------------------------------------------------------------------
    // Implementation of the SignatureVisitor interface
    // ------------------------------------------------------------------------

    public void visitFormalTypeParameter(final String name) {
        if (!mWritingFormals) {
            mWritingFormals = true;
            getBuf().append('<');
        } else {
            getBuf().append(", ");
        }
        getBuf().append(name);
        getBuf().append(" extends ");
    }

    public SignatureVisitor visitClassBound() {
        // we don't differentiate between visiting a sub class or interface type 
        return this;
    }

    public SignatureVisitor visitInterfaceBound() {
        // we don't differentiate between visiting a sub class or interface type 
        return this;
    }

    public SignatureVisitor visitSuperclass() {
        endFormals();
        SignatureSourcer sourcer = new SignatureSourcer();
        assert mSuperClass == null;
        mSuperClass = sourcer;
        return sourcer;
    }

    public SignatureVisitor visitInterface() {
        return this;
    }

    public SignatureVisitor visitParameterType() {
        endFormals();
        SignatureSourcer sourcer = new SignatureSourcer();
        mParameters.add(sourcer);
        return sourcer;
    }

    public SignatureVisitor visitReturnType() {
        endFormals();
        SignatureSourcer sourcer = new SignatureSourcer();
        assert mReturnType == null;
        mReturnType = sourcer;
        return sourcer;
    }

    public SignatureVisitor visitExceptionType() {
        getBuf().append('^');
        return this;
    }

    public void visitBaseType(final char descriptor) {
        getBuf().append(Type.getType(Character.toString(descriptor)).getClassName());
    }

    public void visitTypeVariable(final String name) {
        getBuf().append(name.replace('/', '.'));
    }

    public SignatureVisitor visitArrayType() {
        getBuf().append('[');
        return this;
    }

    public void visitClassType(final String name) {
        getBuf().append(name.replace('/', '.'));
        mArgumentStack *= 2;
    }

    public void visitInnerClassType(final String name) {
        endArguments();
        getBuf().append('.');
        getBuf().append(name.replace('/', '.'));
        mArgumentStack *= 2;
    }

    public void visitTypeArgument() {
        if (mArgumentStack % 2 == 0) {
            ++mArgumentStack;
            getBuf().append('<');
        } else {
            getBuf().append(", ");
        }
        getBuf().append('*');
    }

    public SignatureVisitor visitTypeArgument(final char wildcard) {
        if (mArgumentStack % 2 == 0) {
            ++mArgumentStack;
            getBuf().append('<');
        } else {
            getBuf().append(", ");
        }
        if (wildcard != '=') {
            if (wildcard == '+') {
                getBuf().append("? extends ");
            } else if (wildcard == '-') {
                getBuf().append("? super ");
            } else {
                // can this happen?
                getBuf().append(wildcard);
            }
        }
        return this;
    }

    public void visitEnd() {
        endArguments();
    }

    // ------------------------------------------------------------------------
    // Utility methods
    // ------------------------------------------------------------------------

    /**
     * Ends the formal type parameters section of the signature.
     */
    private void endFormals() {
        if (mWritingFormals) {
            getBuf().append('>');
            mWritingFormals = false;
        }
    }

    /**
     * Ends the type arguments of a class or inner class type.
     */
    private void endArguments() {
        if (mArgumentStack % 2 != 0) {
            getBuf().append('>');
        }
        mArgumentStack /= 2;
    }
}
