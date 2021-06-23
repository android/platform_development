# Inputs:
#   my_prefix -- HOST_ / HOST_CROSS_
#   LOCAL_NAME -- Name of the sdk_repo (e.g. platform-tools)
#   LOCAL_PREFIX_DIR -- The name of the directory to use within the zip file
#   LOCAL_FILES -- src:dest pairs of files to copy into the sdk repo.
#   LOCAL_STRIP_FILES -- src:dest pairs of files to copy into the sdk repo, then strip
#   LOCAL_DIRS -- src:dest pairs of directories to copy into the sdk repo
#
# Any of the src:dest pairs above can just be a source file name if the destination should just be
# the basename of the source file.

intermediates := $(call intermediates-dir-for,PACKAGING,sdk_repo_$(LOCAL_NAME),$(patsubst %_,%,$(my_prefix)))
my_zipfile := $(intermediates)/sdk-repo-$($(my_prefix)OS)-$(LOCAL_NAME).zip

# Add any missing destinations
sdk_repo_expand = $(foreach p,$(1),$(if $(findstring :,$(p)),$(p),$(p):$(notdir $(p))))
LOCAL_FILES := $(call sdk_repo_expand,$(LOCAL_FILES))
LOCAL_STRIP_FILES := $(call sdk_repo_expand,$(LOCAL_STRIP_FILES))
LOCAL_DIRS := $(call sdk_repo_expand,$(LOCAL_DIRS))
sdk_repo_expand :=

# Combine LOCAL_FILES and LOCAL_STRIP_FILES, as we need to copy both.
LOCAL_FILES := $(sort $(LOCAL_FILES) $(LOCAL_STRIP_FILES))
LOCAL_STRIP_FILES := $(sort $(foreach p,$(LOCAL_STRIP_FILES),$(call word-colon,2,$(p))))

.PHONY: sdk_repo_$(LOCAL_NAME)
sdk_repo_$(LOCAL_NAME): $(my_zipfile)
$(call dist-for-goals,sdk_repo sdk_repo_$(LOCAL_NAME),$(my_zipfile):sdk-repo-$($(my_prefix)OS)-$(LOCAL_NAME)-$(FILE_NAME_TAG).zip)

my_deps := \
  $(foreach pair,$(LOCAL_FILES),$(call word-colon,1,$(pair))) \
  $(foreach pair,$(LOCAL_DIRS),$(sort $(patsubst ./%,%,$(shell find -L $(call word-colon,1,$(pair)))))) \
  $(SOONG_ZIP) \
  $(HOST_STRIP) \
  $(HOST_OUT_EXECUTABLES)/line_endings \
  development/build/tools/sdk_clean.sh

$(my_zipfile): PRIVATE_TMPDIR := $(intermediates)/tmp
$(my_zipfile): PRIVATE_DESTDIR := $(intermediates)/tmp/$(LOCAL_PREFIX_DIR)
$(my_zipfile): PRIVATE_PARENT_DIRS := \
  $(sort $(foreach pair,$(LOCAL_FILES) $(LOCAL_DIRS),\
    $(dir $(call word-colon,2,$(pair)))))
$(my_zipfile): PRIVATE_FILES := $(LOCAL_FILES)
$(my_zipfile): PRIVATE_STRIP_FILES := $(LOCAL_STRIP_FILES)
$(my_zipfile): PRIVATE_DIRS := $(LOCAL_DIRS)
ifeq ($($(my_prefix)OS),windows)
$(my_zipfile): PRIVATE_STRIP := $(HOST_STRIP)
else
$(my_zipfile): PRIVATE_STRIP := $(HOST_STRIP) -x
endif
$(my_zipfile): $(my_deps)
	rm -rf $@ $(PRIVATE_TMPDIR)
	mkdir -p $(PRIVATE_DESTDIR)
	$(foreach d,$(PRIVATE_PARENT_DIRS),mkdir -p $(PRIVATE_DESTDIR)/$(d) && ) true
	$(foreach f,$(PRIVATE_FILES),\
	  cp $(call word-colon,1,$(f)) $(PRIVATE_DESTDIR)/$(call word-colon,2,$(f)) && ) true
	$(foreach d,$(PRIVATE_DIRS),\
	  cp -R $(call word-colon,1,$(d)) $(PRIVATE_DESTDIR)/$(call word-colon,2,$(d)) && ) true
	# TODO: Handle more sdk excluded files (sdk.exclude.atree)
	find $(PRIVATE_DESTDIR) '(' -name '.*' -o -name '*~' -o -name 'Makefile' -o -name 'Android.mk' ')' | xargs rm -rf
	find $(PRIVATE_DESTDIR) -name '_*' ! -name '__*' | xargs rm -rf
	$(foreach f,$(PRIVATE_STRIP_FILES),\
	  $(PRIVATE_STRIP) $(PRIVATE_DESTDIR)/$(f))
	HOST_OUT_EXECUTABLES=$(HOST_OUT_EXECUTABLES) HOST_OS=$(HOST_OS) \
	  development/build/tools/sdk_clean.sh $(PRIVATE_DESTDIR)
	$(SOONG_ZIP) -C $(PRIVATE_TMPDIR) -D $(PRIVATE_TMPDIR) -o $@

# Clear used variables
my_prefix :=
my_deps :=
my_zipfile :=
LOCAL_NAME :=
LOCAL_PREFIX_DIR :=
LOCAL_FILES :=
LOCAL_STRIP_FILES :=
LOCAL_DIRS :=
