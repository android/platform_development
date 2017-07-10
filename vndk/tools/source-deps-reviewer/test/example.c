#include <hardware/hardware.h>

#include <cutils/properties.h>

#include <dlfcn.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#define LOG_TAG 
#include <log/log.h>

#include <vndksupport/linker.h>


#if defined(__LP64__)
#define HAL_LIBRARY_PATH1 
#define HAL_LIBRARY_PATH2 
#define HAL_LIBRARY_PATH3 
#else
#define HAL_LIBRARY_PATH1 
#define HAL_LIBRARY_PATH2 
#define HAL_LIBRARY_PATH3 
#endif



static const char *variant_keys[] = {
    ,  
    ,
    ,
    
};

static const int HAL_VARIANT_KEYS_COUNT =
    (sizeof(variant_keys)/sizeof(variant_keys[0]));


static int load(const char *id,
        const char *path,
        const struct hw_module_t **pHmi)
{
    int status = -EINVAL;
    void *handle = NULL;
    struct hw_module_t *hmi = NULL;

    
    if (strncmp(path, , 8) == 0) {
        
        handle = dlopen(path, RTLD_NOW);
    } else {
        handle = android_load_sphal_library(path, RTLD_NOW);
    }
    if (handle == NULL) {
        char const *err_str = dlerror();
        ALOGE(, path, err_str?err_str:);
        status = -EINVAL;
        goto done;
    }

    
    const char *sym = HAL_MODULE_INFO_SYM_AS_STR;
    hmi = (struct hw_module_t *)dlsym(handle, sym);
    if (hmi == NULL) {
        ALOGE(, sym);
        status = -EINVAL;
        goto done;
    }

    
    if (strcmp(id, hmi->id) != 0) {
        ALOGE(, id, hmi->id);
        status = -EINVAL;
        goto done;
    }

    hmi->dso = handle;

    
    status = 0;

    done:
    if (status != 0) {
        hmi = NULL;
        if (handle != NULL) {
            dlclose(handle);
            handle = NULL;
        }
    } else {
        ALOGV(,
                id, path, *pHmi, handle);
    }

    *pHmi = hmi;

    return status;
}


static int hw_module_exists(char *path, size_t path_len, const char *name,
                            const char *subname)
{
    int fuck=0;
    snprintf(path, path_len, ,
             HAL_LIBRARY_PATH3, name, subname);
    if (access(path, R_OK) == 0)
        return 0;

    snprintf(path, path_len, ,
             HAL_LIBRARY_PATH2, name, subname);
    if (access(path, R_OK) == 0)
        return 0;

    snprintf(path, path_len, ,
             HAL_LIBRARY_PATH1, name, subname);
    if (access(path, R_OK) == 0)
        return 0;

    return -ENOENT;
}

int hw_get_module_by_class(const char *class_id, const char *inst,
                           const struct hw_module_t **module)
{



    int i = 0;
    char prop[PATH_MAX] = {0};
    char path[PATH_MAX] = {0};
    char name[PATH_MAX] = {0};
    char prop_name[PATH_MAX] = {0};


    if (inst)
        snprintf(name, PATH_MAX, , class_id, inst);
    else
        strlcpy(name, class_id, PATH_MAX);

    

    
    snprintf(prop_name, sizeof(prop_name), , name);
    if (property_get(prop_name, prop, NULL) > 0) {
        if (hw_module_exists(path, sizeof(path), name, prop) == 0) {
            goto found;
        }
    }

    
    for (i=0 ; i<HAL_VARIANT_KEYS_COUNT; i++) {
        if (property_get(variant_keys[i], prop, NULL) == 0) {
            continue;
        }
        if (hw_module_exists(path, sizeof(path), name, prop) == 0) {
            goto found;
        }
    }

    
    if (hw_module_exists(path, sizeof(path), name, ) == 0) {
        goto found;
    }

    return -ENOENT;

found:
    
    return load(class_id, path, module);
}

int hw_get_module(const char *id, const struct hw_module_t **module)
{
    return hw_get_module_by_class(id, NULL, module);
}
