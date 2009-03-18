/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.android.ide.eclipse.adt.build;

import com.android.ide.eclipse.adt.AdtConstants;
import com.android.ide.eclipse.adt.AdtPlugin;
import com.android.ide.eclipse.adt.build.BaseBuilder.BaseDeltaVisitor;
import com.android.ide.eclipse.adt.build.PreCompilerBuilder.AidlData;
import com.android.ide.eclipse.common.AndroidConstants;
import com.android.ide.eclipse.common.project.AndroidManifestParser;
import com.android.ide.eclipse.common.project.BaseProjectHelper;
import com.android.sdklib.SdkConstants;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.resources.IResourceDelta;
import org.eclipse.core.resources.IResourceDeltaVisitor;
import org.eclipse.core.resources.IWorkspaceRoot;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IPath;

import java.util.ArrayList;

/**
 * Resource Delta visitor for the pre-compiler.
 * <p/>This delta visitor only cares about files that are the source or the result of actions of the
 * {@link PreCompilerBuilder}:
 * <ul><li>R.java/Manifest.java generated by compiling the resources</li>
 * <li>Any Java files generated by <code>aidl</code></li></ul>.
 * 
 * Therefore it looks for the following:
 * <ul><li>Any modification in the resource folder</li>
 * <li>Removed files from the source folder receiving generated Java files</li>
 * <li>Any modification to aidl files.</li>
 * 
 */
class PreCompilerDeltaVisitor extends BaseDeltaVisitor implements
        IResourceDeltaVisitor {

    // Result fields.
    /**
     * Compile flag. This is set to true if one of the changed/added/removed
     * file is a resource file. Upon visiting all the delta resources, if
     * this flag is true, then we know we'll have to compile the resources
     * into R.java
     */
    private boolean mCompileResources = false;

    /** List of .aidl files found that are modified or new. */
    private final ArrayList<AidlData> mAidlToCompile = new ArrayList<AidlData>();

    /** List of .aidl files that have been removed. */
    private final ArrayList<AidlData> mAidlToRemove = new ArrayList<AidlData>();
    
    /** Manifest check/parsing flag. */
    private boolean mCheckedManifestXml = false;

    /** Application Pacakge, gathered from the parsing of the manifest */
    private String mJavaPackage = null;

    // Internal usage fields.
    /**
     * In Resource folder flag. This allows us to know if we're in the
     * resource folder.
     */
    private boolean mInRes = false;

    /**
     * Current Source folder. This allows us to know if we're in a source
     * folder, and which folder.
     */
    private IFolder mSourceFolder = null;

    /** List of source folders. */
    private ArrayList<IPath> mSourceFolders;
    private boolean mIsGenSourceFolder = false;

    private IWorkspaceRoot mRoot;


    public PreCompilerDeltaVisitor(BaseBuilder builder, ArrayList<IPath> sourceFolders) {
        super(builder);
        mSourceFolders = sourceFolders;
        mRoot = ResourcesPlugin.getWorkspace().getRoot();
    }

    public boolean getCompileResources() {
        return mCompileResources;
    }

    public ArrayList<AidlData> getAidlToCompile() {
        return mAidlToCompile;
    }

    public ArrayList<AidlData> getAidlToRemove() {
        return mAidlToRemove;
    }
    
    /**
     * Returns whether the manifest file was parsed/checked for error during the resource delta
     * visiting.
     */
    public boolean getCheckedManifestXml() {
        return mCheckedManifestXml;
    }
    
    /**
     * Returns the manifest package if the manifest was checked/parsed.
     * <p/>
     * This can return null in two cases:
     * <ul>
     * <li>The manifest was not part of the resource change delta, and the manifest was
     * not checked/parsed ({@link #getCheckedManifestXml()} returns <code>false</code>)</li>
     * <li>The manifest was parsed ({@link #getCheckedManifestXml()} returns <code>true</code>),
     * but the package declaration is missing</li>
     * </ul>
     * @return the manifest package or null.
     */
    public String getManifestPackage() {
        return mJavaPackage;
    }

    /*
     * (non-Javadoc)
     *
     * @see org.eclipse.core.resources.IResourceDeltaVisitor
     *      #visit(org.eclipse.core.resources.IResourceDelta)
     */
    public boolean visit(IResourceDelta delta) throws CoreException {
        // we are only going to look for changes in res/, source folders and in
        // AndroidManifest.xml since the delta visitor goes through the main
        // folder before its children we can check when the path segment
        // count is 2 (format will be /$Project/folder) and make sure we are
        // processing res/, source folders or AndroidManifest.xml

        IResource resource = delta.getResource();
        IPath path = resource.getFullPath();
        String[] segments = path.segments();

        // since the delta visitor also visits the root we return true if
        // segments.length = 1
        if (segments.length == 1) {
            // FIXME: check this is an Android project.
            return true;
        } else if (segments.length == 2) {
            // if we are at an item directly under the root directory,
            // then we are not yet in a source or resource folder
            mInRes = false;
            mSourceFolder = null;

            if (SdkConstants.FD_RESOURCES.equalsIgnoreCase(segments[1])) {
                // this is the resource folder that was modified. we want to
                // see its content.

                // since we're going to visit its children next, we set the
                // flag
                mInRes = true;
                mSourceFolder = null;
                return true;
            } else if (AndroidConstants.FN_ANDROID_MANIFEST.equalsIgnoreCase(segments[1])) {
                // any change in the manifest could trigger a new R.java
                // class, so we don't need to check the delta kind
                if (delta.getKind() != IResourceDelta.REMOVED) {
                    // parse the manifest for errors
                    AndroidManifestParser parser = BaseProjectHelper.parseManifestForError(
                            (IFile)resource, this);
                    
                    if (parser != null) {
                        mJavaPackage = parser.getPackage();
                    }

                    mCheckedManifestXml = true;
                }
                mCompileResources = true;

                // we don't want to go to the children, not like they are
                // any for this resource anyway.
                return false;
            }
        }

        // at this point we can either be in the source folder or in the
        // resource folder or in a different folder that contains a source
        // folder.
        // This is due to not all source folder being src/. Some could be
        // something/somethingelse/src/

        // so first we test if we already know we are in a source or
        // resource folder.

        if (mSourceFolder != null) {
            // if we are in the res folder, we are looking for the following changes:
            // - added/removed/modified aidl files.
            // - missing R.java file

            // if the resource is a folder, we just go straight to the children
            if (resource.getType() == IResource.FOLDER) {
                return true;
            }

            if (resource.getType() != IResource.FILE) {
                return false;
            }
            IFile file = (IFile)resource;

            // get the modification kind
            int kind = delta.getKind();

            // we process normal source folder and the 'gen' source folder differently.
            if (mIsGenSourceFolder) {
                // this is the generated java file source folder.
                // - if R.java/Manifest.java are removed/modified, we recompile the resources
                // - if aidl files are removed/modified, we recompile them.

                boolean outputWarning = false;

                String fileName = resource.getName();

                // Special case of R.java/Manifest.java.
                if (AndroidConstants.FN_RESOURCE_CLASS.equals(fileName) ||
                        AndroidConstants.FN_MANIFEST_CLASS.equals(fileName)) {
                    // if it was removed, there's a possibility that it was removed due to a
                    // package change, or an aidl that was removed, but the only thing
                    // that will happen is that we'll have an extra build. Not much of a problem.
                    mCompileResources = true;

                    // we want a warning
                    outputWarning = true;
                } else {
                    // this has to be a Java file created from an aidl file.
                    // Look for the source aidl file in all the source folders.
                    String aidlFileName = fileName.replaceAll(AndroidConstants.RE_JAVA_EXT,
                            AndroidConstants.DOT_AIDL);
                    
                    for (IPath sourceFolderPath : mSourceFolders) {
                        // do not search in the current source folder as it is the 'gen' folder.
                        if (sourceFolderPath.equals(mSourceFolder.getFullPath())) {
                            continue;
                        }
                        
                        IFolder sourceFolder = getFolder(sourceFolderPath);
                        if (sourceFolder != null) {
                            // go recursively, segment by segment.
                            // index starts at 2 (0 is project, 1 is 'gen' 
                            IFile sourceFile = findFile(sourceFolder, segments, 2, aidlFileName);
                            
                            if (sourceFile != null) {
                                // found the source. add it to the list of files to compile
                                mAidlToCompile.add(new AidlData(sourceFolder, sourceFile));
                                outputWarning = true;
                                break;
                            }
                        }
                    }
                }

                if (outputWarning) {
                    if (kind == IResourceDelta.REMOVED) {
                        // We pring an error just so that it's red, but it's just a warning really.
                        String msg = String.format(Messages.s_Removed_Recreating_s, fileName);
                        AdtPlugin.printErrorToConsole(mBuilder.getProject(), msg);
                    } else if (kind == IResourceDelta.CHANGED) {
                        // the file was modified manually! we can't allow it.
                        String msg = String.format(Messages.s_Modified_Manually_Recreating_s,
                                fileName);
                        AdtPlugin.printErrorToConsole(mBuilder.getProject(), msg);
                    }
                }

            } else {
                // this is another source folder.
                // We only care about aidl files being added/modified/removed.

                // get the extension of the resource
                String ext = resource.getFileExtension();
                if (AndroidConstants.EXT_AIDL.equalsIgnoreCase(ext)) {
                    if (kind == IResourceDelta.REMOVED) {
                        // we'll have to remove the generated file.
                        mAidlToRemove.add(new AidlData(mSourceFolder, file));
                    } else {
                        // add the aidl file to the list of file to (re)compile
                        mAidlToCompile.add(new AidlData(mSourceFolder, file));
                    }
                }
            }

            // no children.
            return false;
        } else if (mInRes) {
            // if we are in the res folder, we are looking for the following
            // changes:
            // - added/removed/modified xml files.
            // - added/removed files of any other type

            // if the resource is a folder, we just go straight to the
            // children
            if (resource.getType() == IResource.FOLDER) {
                return true;
            }

            // get the extension of the resource
            String ext = resource.getFileExtension();
            int kind = delta.getKind();

            String p = resource.getProjectRelativePath().toString();
            String message = null;
            switch (kind) {
                case IResourceDelta.CHANGED:
                    // display verbose message
                    message = String.format(Messages.s_Modified_Recreating_s, p,
                            AndroidConstants.FN_RESOURCE_CLASS);
                    break;
                case IResourceDelta.ADDED:
                    // display verbose message
                    message = String.format(Messages.Added_s_s_Needs_Updating, p,
                            AndroidConstants.FN_RESOURCE_CLASS);
                    break;
                case IResourceDelta.REMOVED:
                    // display verbose message
                    message = String.format(Messages.s_Removed_s_Needs_Updating, p,
                            AndroidConstants.FN_RESOURCE_CLASS);
                    break;
            }
            if (message != null) {
                AdtPlugin.printBuildToConsole(AdtConstants.BUILD_VERBOSE,
                        mBuilder.getProject(), message);
            }

            if (AndroidConstants.EXT_XML.equalsIgnoreCase(ext)) {
                if (kind != IResourceDelta.REMOVED) {
                    // check xml Validity
                    mBuilder.checkXML(resource, this);
                }

                // if we are going through this resource, it was modified
                // somehow.
                // we don't care if it was an added/removed/changed event
                mCompileResources = true;
                return false;
            } else {
                // this is a non xml resource.
                if (kind == IResourceDelta.ADDED
                        || kind == IResourceDelta.REMOVED) {
                    mCompileResources = true;
                    return false;
                }
            }
        } else if (resource instanceof IFolder) {
            // in this case we may be inside a folder that contains a source
            // folder, go through the list of known source folders

            for (IPath sourceFolderPath : mSourceFolders) {
                // first check if they match exactly.
                if (sourceFolderPath.equals(path)) {
                    // this is a source folder!
                    mInRes = false;
                    mSourceFolder = getFolder(sourceFolderPath); // all non null due to test above
                    mIsGenSourceFolder = path.segmentCount() == 2 &&
                            path.segment(1).equals(SdkConstants.FD_GEN_SOURCES);
                    return true;
                }
                
                // check if we are on the way to a source folder.
                int count = sourceFolderPath.matchingFirstSegments(path);
                if (count == path.segmentCount()) {
                    mInRes = false;
                    return true;
                }
            }

            // if we're here, we are visiting another folder
            // like /$Project/bin/ for instance (we get notified for changes
            // in .class!)
            // This could also be another source folder and we have found
            // R.java in a previous source folder
            // We don't want to visit its children
            return false;
        }

        return false;
    }
    
    /**
     * Searches for and return a file in a folder. The file is defined by its segments, and a new
     * name (replacing the last segment).
     * @param folder the folder we are searching
     * @param segments the segments of the file to search.
     * @param index the index of the current segment we are looking for
     * @param filename the new name to replace the last segment.
     * @return the {@link IFile} representing the searched file, or null if not found
     */
    private IFile findFile(IFolder folder, String[] segments, int index, String filename) {
        boolean lastSegment = index == segments.length - 1;
        IResource resource = folder.findMember(lastSegment ? filename : segments[index]);
        if (resource != null && resource.exists()) {
            if (lastSegment) {
                if (resource.getType() == IResource.FILE) {
                    return (IFile)resource;
                }
            } else {
                if (resource.getType() == IResource.FOLDER) {
                    return findFile((IFolder)resource, segments, index+1, filename);
                }
            }
        }
        return null;
    }

    /**
     * Returns a handle to the folder identified by the given path in this container.
     * <p/>The different with {@link IContainer#getFolder(IPath)} is that this returns a non
     * null object only if the resource actually exists and is a folder (and not a file)
     * @param path the path of the folder to return.
     * @return a handle to the folder if it exists, or null otherwise.
     */
    private IFolder getFolder(IPath path) {
        IResource resource = mRoot.findMember(path);
        if (resource != null && resource.exists() && resource.getType() == IResource.FOLDER) {
            return (IFolder)resource;
        }
        
        return null;
    }
}
