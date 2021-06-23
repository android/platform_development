# Inputs:
#   LOCAL_NAME -- Name of the sdk_repo (e.g. platform-tools)
#   LOCAL_PREFIX_DIR -- The name of the directory to use within the zip file
#   LOCAL_FILES -- src:dest pairs of files to copy into the sdk repo
#   LOCAL_STRIP_FILES -- src:dest pairs of files to copy into the sdk repo, then strip
#   LOCAL_DIRS -- src:dest pairs of directories to copy into the sdk repo

intermediates := $(call intermediates-dir-for,PACKAGING,sdk_repo_$(LOCAL_NAME),$(patsubst %_,%,$(my_prefix)))
my_zipfile := $(intermediates)/sdk-repo-$($(my_prefix)OS)-$(LOCAL_NAME).zip

# Combine LOCAL_FILES and LOCAL_STRIP_FILES, as we need to copy both.
LOCAL_FILES := $(sort $(LOCAL_FILES) $(LOCAL_STRIP_FILES))

.PHONY: sdk_repo_$(LOCAL_NAME)
sdk_repo_$(LOCAL_NAME): $(my_zipfile)
$(call dist-for-goals,sdk_repo sdk_repo_$(LOCAL_NAME),$(my_zipfile):sdk-repo-$($(my_prefix)OS)-$(LOCAL_NAME)-$(FILE_NAME_TAG).zip)

my_deps := \
  $(foreach pair,$(LOCAL_FILES),$(call word-colon,1,$(pair))) \
  $(foreach pair,$(LOCAL_DIRS),$(sort $(patsubst ./%,%,$(shell find -L $(call word-colon,1,$(pair))))))

$(my_zipfile): PRIVATE_TMPDIR := $(intermediates)/tmp
$(my_zipfile): PRIVATE_DESTDIR := $(intermediates)/tmp/$(LOCAL_PREFIX_DIR)
$(my_zipfile): PRIVATE_PARENT_DIRS := \
  $(sort $(foreach pair,$(LOCAL_FILES) $(LOCAL_DIRS),\
    $(dir $(call word-colon,2,$(pair)))))
$(my_zipfile): PRIVATE_FILES := $(LOCAL_FILES)
$(my_zipfile): PRIVATE_STRIP_FILES := $(LOCAL_STRIP_FILES)
$(my_zipfile): PRIVATE_DIRS := $(LOCAL_DIRS)
$(my_zipfile): $(SOONG_ZIP) $(HOST_STRIP) $(my_deps)
	rm -rf $@ $(PRIVATE_TMPDIR)
	mkdir -p $(PRIVATE_DESTDIR)
	$(foreach d,$(PRIVATE_PARENT_DIRS),mkdir -p $(PRIVATE_DESTDIR)/$(d) && ) true
	$(foreach f,$(PRIVATE_FILES),\
	  cp $(call word-colon,1,$(f)) $(PRIVATE_DESTDIR)/$(call word-colon,2,$(f)) && ) true
	$(foreach d,$(PRIVATE_DIRS),\
	  cp -R $(call word-colon,1,$(d)) $(PRIVATE_DESTDIR)/$(call word-colon,2,$(d)) && ) true
	# TODO: Handle sdk excluded files (sdk.exclude.atree)
	$(foreach f,$(PRIVATE_STRIP_FILES),\
	  $(HOST_STRIP) -x $(PRIVATE_DESTDIR)/$(f))
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
