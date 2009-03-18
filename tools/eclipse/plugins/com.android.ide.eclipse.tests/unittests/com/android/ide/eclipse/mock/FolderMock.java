/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.eclipse.org/org/documents/epl-v10.php
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.ide.eclipse.mock;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IMarker;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IProjectDescription;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.resources.IResourceProxy;
import org.eclipse.core.resources.IResourceProxyVisitor;
import org.eclipse.core.resources.IResourceVisitor;
import org.eclipse.core.resources.IWorkspace;
import org.eclipse.core.resources.ResourceAttributes;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.QualifiedName;
import org.eclipse.core.runtime.jobs.ISchedulingRule;

import sun.reflect.generics.reflectiveObjects.NotImplementedException;

import java.net.URI;
import java.util.Map;

/**
 * Mock implementation of {@link IFolder}.
 * <p/>Supported methods:
 * <ul>
 * <li>{@link #getName()}</li>
 * <li>{@link #members()}</li>
 * </ul>
 */
public final class FolderMock implements IFolder {
    
    private String mName;
    private IResource[] mMembers;
    
    public FolderMock(String name) {
        mName = name;
        mMembers = new IResource[0];
    }
    
    public FolderMock(String name, IResource[] members) {
        mName = name;
        mMembers = members;
    }
    
    // -------- MOCKED METHODS ----------------
    
    public String getName() {
        return mName;
    }
    
    public IResource[] members() throws CoreException {
        return mMembers;
    }

    // -------- UNIMPLEMENTED METHODS ----------------

    public void create(boolean force, boolean local, IProgressMonitor monitor) throws CoreException {
        throw new NotImplementedException();
    }

    public void create(int updateFlags, boolean local, IProgressMonitor monitor)
            throws CoreException {
        throw new NotImplementedException();
    }

    public void createLink(IPath localLocation, int updateFlags, IProgressMonitor monitor)
            throws CoreException {
        throw new NotImplementedException();
    }

    public void createLink(URI location, int updateFlags, IProgressMonitor monitor)
            throws CoreException {
        throw new NotImplementedException();
    }

    public void delete(boolean force, boolean keepHistory, IProgressMonitor monitor)
            throws CoreException {
        throw new NotImplementedException();
    }

    public IFile getFile(String name) {
        throw new NotImplementedException();
    }

    public IFolder getFolder(String name) {
        throw new NotImplementedException();
    }

    public void move(IPath destination, boolean force, boolean keepHistory, IProgressMonitor monitor)
            throws CoreException {
        throw new NotImplementedException();
    }

    public boolean exists(IPath path) {
        throw new NotImplementedException();
    }

    public IFile[] findDeletedMembersWithHistory(int depth, IProgressMonitor monitor)
            throws CoreException {
        throw new NotImplementedException();
    }

    public IResource findMember(String name) {
        throw new NotImplementedException();
    }

    public IResource findMember(IPath path) {
        throw new NotImplementedException();
    }

    public IResource findMember(String name, boolean includePhantoms) {
        throw new NotImplementedException();
    }

    public IResource findMember(IPath path, boolean includePhantoms) {
        throw new NotImplementedException();
    }

    public String getDefaultCharset() throws CoreException {
        throw new NotImplementedException();
    }

    public String getDefaultCharset(boolean checkImplicit) throws CoreException {
        throw new NotImplementedException();
    }

    public IFile getFile(IPath path) {
        throw new NotImplementedException();
    }

    public IFolder getFolder(IPath path) {
        throw new NotImplementedException();
    }

    public IResource[] members(boolean includePhantoms) throws CoreException {
        throw new NotImplementedException();
    }

    public IResource[] members(int memberFlags) throws CoreException {
        throw new NotImplementedException();
    }

    public void setDefaultCharset(String charset) throws CoreException {
        throw new NotImplementedException();
    }

    public void setDefaultCharset(String charset, IProgressMonitor monitor) throws CoreException {
        throw new NotImplementedException();
    }

    public void accept(IResourceVisitor visitor) throws CoreException {
        throw new NotImplementedException();
    }

    public void accept(IResourceProxyVisitor visitor, int memberFlags) throws CoreException {
        throw new NotImplementedException();
    }

    public void accept(IResourceVisitor visitor, int depth, boolean includePhantoms)
            throws CoreException {
        throw new NotImplementedException();
    }

    public void accept(IResourceVisitor visitor, int depth, int memberFlags) throws CoreException {
        throw new NotImplementedException();
    }

    public void clearHistory(IProgressMonitor monitor) throws CoreException {
        throw new NotImplementedException();
    }

    public void copy(IPath destination, boolean force, IProgressMonitor monitor)
            throws CoreException {
        throw new NotImplementedException();
    }

    public void copy(IPath destination, int updateFlags, IProgressMonitor monitor)
            throws CoreException {
        throw new NotImplementedException();
    }

    public void copy(IProjectDescription description, boolean force, IProgressMonitor monitor)
            throws CoreException {
        throw new NotImplementedException();
    }

    public void copy(IProjectDescription description, int updateFlags, IProgressMonitor monitor)
            throws CoreException {
        throw new NotImplementedException();
    }

    public IMarker createMarker(String type) throws CoreException {
        throw new NotImplementedException();
    }

    public IResourceProxy createProxy() {
        throw new NotImplementedException();
    }

    public void delete(boolean force, IProgressMonitor monitor) throws CoreException {
        throw new NotImplementedException();
    }

    public void delete(int updateFlags, IProgressMonitor monitor) throws CoreException {
        throw new NotImplementedException();
    }

    public void deleteMarkers(String type, boolean includeSubtypes, int depth) throws CoreException {
        throw new NotImplementedException();
    }

    public boolean exists() {
        throw new NotImplementedException();
    }

    public IMarker findMarker(long id) throws CoreException {
        throw new NotImplementedException();
    }

    public IMarker[] findMarkers(String type, boolean includeSubtypes, int depth)
            throws CoreException {
        throw new NotImplementedException();
    }

    public int findMaxProblemSeverity(String type, boolean includeSubtypes, int depth)
            throws CoreException {
        throw new NotImplementedException();
    }

    public String getFileExtension() {
        throw new NotImplementedException();
    }

    public IPath getFullPath() {
        throw new NotImplementedException();
    }

    public long getLocalTimeStamp() {
        throw new NotImplementedException();
    }

    public IPath getLocation() {
        throw new NotImplementedException();
    }

    public URI getLocationURI() {
        throw new NotImplementedException();
    }

    public IMarker getMarker(long id) {
        throw new NotImplementedException();
    }

    public long getModificationStamp() {
        throw new NotImplementedException();
    }

    public IContainer getParent() {
        throw new NotImplementedException();
    }

    public String getPersistentProperty(QualifiedName key) throws CoreException {
        throw new NotImplementedException();
    }

    public IProject getProject() {
        throw new NotImplementedException();
    }

    public IPath getProjectRelativePath() {
        throw new NotImplementedException();
    }

    public IPath getRawLocation() {
        throw new NotImplementedException();
    }

    public URI getRawLocationURI() {
        throw new NotImplementedException();
    }

    public ResourceAttributes getResourceAttributes() {
        throw new NotImplementedException();
    }

    public Object getSessionProperty(QualifiedName key) throws CoreException {
        throw new NotImplementedException();
    }

    public int getType() {
        throw new NotImplementedException();
    }

    public IWorkspace getWorkspace() {
        throw new NotImplementedException();
    }

    public boolean isAccessible() {
        throw new NotImplementedException();
    }

    public boolean isDerived() {
        throw new NotImplementedException();
    }

    public boolean isLinked() {
        throw new NotImplementedException();
    }

    public boolean isLinked(int options) {
        throw new NotImplementedException();
    }

    public boolean isLocal(int depth) {
        throw new NotImplementedException();
    }

    public boolean isPhantom() {
        throw new NotImplementedException();
    }

    public boolean isReadOnly() {
        throw new NotImplementedException();
    }

    public boolean isSynchronized(int depth) {
        throw new NotImplementedException();
    }

    public boolean isTeamPrivateMember() {
        throw new NotImplementedException();
    }

    public void move(IPath destination, boolean force, IProgressMonitor monitor)
            throws CoreException {
        throw new NotImplementedException();
    }

    public void move(IPath destination, int updateFlags, IProgressMonitor monitor)
            throws CoreException {
        throw new NotImplementedException();
    }

    public void move(IProjectDescription description, int updateFlags, IProgressMonitor monitor)
            throws CoreException {
        throw new NotImplementedException();
    }

    public void move(IProjectDescription description, boolean force, boolean keepHistory,
            IProgressMonitor monitor) throws CoreException {
        throw new NotImplementedException();
    }

    public void refreshLocal(int depth, IProgressMonitor monitor) throws CoreException {
        throw new NotImplementedException();
    }

    public void revertModificationStamp(long value) throws CoreException {
        throw new NotImplementedException();
    }

    public void setDerived(boolean isDerived) throws CoreException {
        throw new NotImplementedException();
    }

    public void setLocal(boolean flag, int depth, IProgressMonitor monitor) throws CoreException {
        throw new NotImplementedException();
    }

    public long setLocalTimeStamp(long value) throws CoreException {
        throw new NotImplementedException();
    }

    public void setPersistentProperty(QualifiedName key, String value) throws CoreException {
        throw new NotImplementedException();
    }

    public void setReadOnly(boolean readOnly) {
        throw new NotImplementedException();
    }

    public void setResourceAttributes(ResourceAttributes attributes) throws CoreException {
        throw new NotImplementedException();
    }

    public void setSessionProperty(QualifiedName key, Object value) throws CoreException {
        throw new NotImplementedException();
    }

    public void setTeamPrivateMember(boolean isTeamPrivate) throws CoreException {
        throw new NotImplementedException();
    }

    public void touch(IProgressMonitor monitor) throws CoreException {
        throw new NotImplementedException();
    }

    @SuppressWarnings("unchecked")
    public Object getAdapter(Class adapter) {
        throw new NotImplementedException();
    }

    public boolean contains(ISchedulingRule rule) {
        throw new NotImplementedException();
    }

    public boolean isConflicting(ISchedulingRule rule) {
        throw new NotImplementedException();
    }

	public Map<?,?> getPersistentProperties() throws CoreException {
        throw new NotImplementedException();
	}

	public Map<?,?> getSessionProperties() throws CoreException {
        throw new NotImplementedException();
	}

	public boolean isDerived(int options) {
        throw new NotImplementedException();
	}

	public boolean isHidden() {
        throw new NotImplementedException();
	}

	public void setHidden(boolean isHidden) throws CoreException {
        throw new NotImplementedException();
	}

}
