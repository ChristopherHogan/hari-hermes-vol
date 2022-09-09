/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5. The full HDF5 copyright notice, including      *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html. COPYING can be found at the root    *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page. It can also be found at      *
 * http://hdfgroup.org/HDF5/doc/Copyright.html. If you do not have           *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
*
* Created: hermes_vol_private.c
* June 11 2018
* Hariharan Devarajan <hdevarajan@hdfgroup.org>
*
* Purpose:Defines the Hermes VOL Plugin
*
*-------------------------------------------------------------------------
*/

#include <hdf5.h>
#include <libgen.h>
#include "hermes_vol_private.h"
/**
 * This method implements the setting of vol driver inside application's fapl.
 *
 * @param fapl_id
 * @param layers
 * @param count_
 * @return vol_id
 */
#ifndef HERMES_LAYERS_COMPILE_TIME
H5_DLL hid_t H5Pset_fapl_hermes_vol(hid_t fapl_id,LayerInfo* layers, uint16_t count_){
    if(count_< 1){
        perror("At least 1 layer must be defined.");
        exit(-2);
    }
    H5_UpdateLayer(layers,count_);
    HermesVol layer;
    layer.file_name=NULL;
    layer.dataset_name=NULL;
    layer.sync=true;
    layer.native_fapl=H5Pcopy(fapl_id);
    layer.native_driver_id=H5VLget_driver_id("native");
    layer.vol_id = H5VLregister(&H5VL_hermes_g);
    H5Pset_vol(fapl_id, layer.vol_id, &layer);
    return layer.vol_id;
}
#else
H5_DLL hid_t H5Pset_fapl_hermes_vol(hid_t fapl_id){
    HermesVol layer;
    layer.file_name=NULL;
    layer.dataset_name=NULL;
    layer.sync=true;
    layer.native_fapl=H5Pcopy(fapl_id);
    layer.native_driver_id=H5VLget_driver_id("native");
    layer.vol_id = H5VLregister(&H5VL_hermes_g);
    H5Pset_vol(fapl_id, layer.vol_id, &layer);
    return layer.vol_id;
}
#endif

/* Hermes VOL Dataset callbacks */
static void  *hermes_dataset_create(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req){
    HermesVol *dset;
    HermesVol *o = (HermesVol *)obj;

    dset= (HermesVol *)(H5VL_hermes_fapl_copy(o));
    dset->dataset_name=(char*)malloc(strlen(name)+1);
    strcpy(dset->dataset_name,name);
    hid_t dataspace,type_id;
    H5Pget(dcpl_id, H5VL_PROP_DSET_TYPE_ID, &type_id);
    H5Pget(dcpl_id, H5VL_PROP_DSET_SPACE_ID, &dataspace);
    hid_t dataset_id= H5Dcreate1(o->object_id, name, type_id,dataspace, dcpl_id);
    dset->object_id=dataset_id;
    //char* filename=o->file_name;
    char* filename=basename( o->file_name );
    char* dataset_name=dset->dataset_name;
    hsize_t *dims,*max_dims;
    int rank_=H5Sget_simple_extent_dims(dataspace,NULL, NULL);
    dims= (hsize_t*) calloc(rank_, sizeof(hsize_t));
    max_dims= (hsize_t*) calloc(rank_, sizeof(hsize_t));
    H5Sget_simple_extent_dims(dataspace,dims, max_dims);
    H5_BufferInit(filename,dataset_name,rank_,type_id,dims, max_dims,dataset_id);
    free(dims);
    free(max_dims);
    return (void *)dset;
}
static void  *hermes_dataset_open(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t dapl_id, hid_t dxpl_id, void **req){
    HermesVol *dset;
    HermesVol *o = (HermesVol *)obj;
    dset= (HermesVol *)(H5VL_hermes_fapl_copy(o));
    dset->dataset_name=(char*)malloc(strlen(name)+1);
    strcpy(dset->dataset_name,name);
    hid_t dataset_id= H5Dopen2(o->object_id, name, dapl_id);
    dset->object_id=dataset_id;
    char* filename=basename( o->file_name );//char* filename=o->file_name;
    char* dataset_name=o->dataset_name;
    hsize_t *dims,*max_dims;
    hid_t file_space_id=H5Dget_space(dataset_id);
    int rank_=H5Sget_simple_extent_dims(file_space_id,NULL, NULL);
    dims= (hsize_t*) calloc(rank_, sizeof(hsize_t));
    max_dims= (hsize_t*) calloc(rank_, sizeof(hsize_t));
    H5Sget_simple_extent_dims(file_space_id,dims, max_dims);
    int64_t type_=H5Dget_type(dataset_id);
    H5_BufferInit(filename,dataset_name,rank_,type_,dims, max_dims,dataset_id);
    return (void *)dset;
}
static herr_t hermes_dataset_read(void *dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t dxpl_id, void *buf, void **req){
    HermesVol *o = (HermesVol *)(dset);
    hid_t dataset_id=o->object_id;
    char* filename=basename( o->file_name );//char* filename=o->file_name;
    char* dataset_name=o->dataset_name;
    hsize_t *memory_start,*memory_dim,*file_start,*file_end;
    int rank_;
    if(file_space_id==0){
        file_space_id=H5Dget_space(dataset_id);
        rank_=H5Sget_simple_extent_dims(file_space_id,NULL, NULL);
        file_start= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        file_end= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        rank_=H5Sget_simple_extent_dims(file_space_id,file_end,NULL );
        int i;
	for(i=0;i<rank_;i++){
            file_start[i]=0;
            file_end[i]-=1;
        }
    }else{
        rank_=H5Sget_simple_extent_dims(file_space_id,NULL, NULL);
        file_start= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        file_end= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        H5Sget_select_bounds(file_space_id, file_start, file_end);
    }
    if(mem_space_id==0){
        mem_space_id=H5Dget_space(dataset_id);
        rank_=H5Sget_simple_extent_dims(mem_space_id,NULL, NULL);
        memory_start= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        memory_dim= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        rank_=H5Sget_simple_extent_dims(mem_space_id,memory_dim, NULL);
        int i;
	for(i=0;i<rank_;i++){
            memory_start[i]=0;
        }
    }else{
        rank_=H5Sget_simple_extent_dims(mem_space_id,NULL, NULL);
        memory_start= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        memory_dim= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        H5Sget_select_bounds(mem_space_id, memory_start, memory_dim);
        H5Sget_simple_extent_dims(mem_space_id,memory_dim, NULL);
    }
    int64_t type_=H5Dget_type(dataset_id);
    herr_t  output=H5_BufferRead(filename,dataset_name,rank_,type_,file_start,file_end,memory_start, memory_dim,dataset_id,buf);
    free(file_start);
    free(file_end);
    free(memory_start);
    free(memory_dim);
    return output;
}
static herr_t hermes_dataset_write(void *dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t dxpl_id, const void *buf, void **req){
    HermesVol *o = (HermesVol *)(dset);
    hid_t dataset_id=o->object_id;
    char* filename=basename( o->file_name );//char* filename=o->file_name;
    char* dataset_name=o->dataset_name;
    hsize_t *memory_start,*memory_dim,*file_start,*file_end;
    int rank_;
    if(file_space_id==0){
        file_space_id=H5Dget_space(dataset_id);
        rank_=H5Sget_simple_extent_dims(file_space_id,NULL, NULL);
        file_start= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        file_end= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        rank_=H5Sget_simple_extent_dims(file_space_id,file_end,NULL );
        int i;
	for(i=0;i<rank_;i++){
            file_start[i]=0;
            file_end[i]-=1;
        }
    }else{
        rank_=H5Sget_simple_extent_dims(file_space_id,NULL, NULL);
        file_start= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        file_end= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        H5Sget_select_bounds(file_space_id, file_start, file_end);
    }
    if(mem_space_id==0){
        mem_space_id=H5Dget_space(dataset_id);
        rank_=H5Sget_simple_extent_dims(mem_space_id,NULL, NULL);
        memory_start= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        memory_dim= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        rank_=H5Sget_simple_extent_dims(mem_space_id,memory_dim, NULL);
        int i;
	for(i=0;i<rank_;i++){
            memory_start[i]=0;
        }
    }else{
        rank_=H5Sget_simple_extent_dims(mem_space_id,NULL, NULL);
        memory_start= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        memory_dim= (hsize_t*) calloc(rank_, sizeof(hsize_t));
        H5Sget_select_bounds(mem_space_id, memory_start, memory_dim);
        H5Sget_simple_extent_dims(mem_space_id,memory_dim, NULL);
    }
    int64_t type_=H5Dget_type(dataset_id);
    herr_t output=H5_BufferWrite(filename, dataset_name, rank_, type_, file_start, file_end, memory_start, memory_dim, dataset_id,
                                 (void *)(buf));
    free(file_start);
    free(file_end);
    free(memory_start);
    free(memory_dim);
    return output;
}
static herr_t  hermes_dataset_get(void *dset, H5VL_dataset_get_t get_type, hid_t dxpl_id, void **req, va_list arguments){
    HermesVol *o = (HermesVol *)(dset);
    switch (get_type) {
        /* H5Dget_space */
        case H5VL_DATASET_GET_SPACE:
        {
            hid_t	*ret_id = va_arg(arguments, hid_t *);
            *ret_id = H5Dget_space(o->object_id);
            break;
        }
            /* H5Dget_space_statuc */
        case H5VL_DATASET_GET_SPACE_STATUS:
        {
            H5D_space_status_t *allocation = va_arg(arguments, H5D_space_status_t *);
            /* Read data space address and return */
            H5Dget_space_status(o->object_id, allocation);
            break;
        }
            /* H5Dget_type */
        case H5VL_DATASET_GET_TYPE:
        {
            hid_t	*ret_id = va_arg(arguments, hid_t *);
            *ret_id = H5Dget_space(o->object_id);
            break;
        }
            /* H5Dget_create_plist */
        case H5VL_DATASET_GET_DCPL:
        {
            hid_t	*ret_id = va_arg(arguments, hid_t *);
            *ret_id = H5Dget_create_plist(o->object_id);
            break;
        }
            /* H5Dget_access_plist */
        case H5VL_DATASET_GET_DAPL:
        {
            hid_t	*ret_id = va_arg(arguments, hid_t *);
            *ret_id = H5Dget_access_plist(o->object_id);
            break;
        }
            /* H5Dget_storage_size */
        case H5VL_DATASET_GET_STORAGE_SIZE:
        {
            hsize_t *ret = va_arg(arguments, hsize_t *);
            *ret = H5Dget_storage_size(o->object_id);
            break;
        }
            /* H5Dget_offset */
        case H5VL_DATASET_GET_OFFSET:
        {
            haddr_t *ret = va_arg(arguments, haddr_t *);

            /* Set return value */
            *ret = H5Dget_offset(o->object_id);
            break;
        }
        default:{

        }

    }
    return 0;
}
static herr_t hermes_dataset_close(void *dset, hid_t dxpl_id, void **req){
    HermesVol *o = (HermesVol *)(dset);
    hid_t dataset_id= o->object_id;
    hsize_t *dims,*max_dims;
    int rank_;
    hid_t file_space_id=H5Dget_space(dataset_id);
    rank_=H5Sget_simple_extent_dims(file_space_id,NULL, NULL);
    dims= (hsize_t*) calloc(rank_, sizeof(hsize_t));
    max_dims= (hsize_t*) calloc(rank_, sizeof(hsize_t));
    rank_=H5Sget_simple_extent_dims(file_space_id,dims,max_dims );
    char* filename=basename( o->file_name );
    if(o->sync)
        H5_BufferSync(filename,o->dataset_name,rank_,dims,max_dims,dataset_id);
    herr_t output=H5Dclose(dataset_id);
    free(dims);
    free(max_dims);
    free(o->file_name);
    free(o->dataset_name);
    free(o);
    return output;
}

/* Hermes VOL File callbacks */
static void* hermes_file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req){
    HermesVol *file= (HermesVol *)(H5Pget_vol_info(fapl_id));
    file->file_name=(char*)malloc(strlen(name)+1);
    strcpy(file->file_name,name);
    hid_t file_id=H5Fcreate(name,H5F_ACC_TRUNC,fcpl_id,file->native_fapl);
    file->object_id=file_id;
    return (void *)file;

}
static void  *hermes_file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req){
    HermesVol *file= (HermesVol *)(H5Pget_vol_info(fapl_id));
    file->file_name=(char*)malloc(strlen(name)+1);
    strcpy(file->file_name,name);
    hid_t file_id=H5Fopen(name,H5F_ACC_TRUNC,file->native_fapl);
    file->object_id=file_id;
    return (void *)file;
}
static herr_t hermes_file_close(void *file, hid_t dxpl_id, void **req){
    HermesVol *o = (HermesVol *)(file);
    herr_t output= H5Fclose(o->object_id);
    return output;
}

/* Hermes VOL other callbacks*/
static herr_t H5VL_hermes_init(hid_t vipl_id){
    return 0;
}
static herr_t H5VL_hermes_term(hid_t vtpl_id){
    return 0;
}
static void *H5VL_hermes_fapl_copy(const void *info){
    HermesVol *o = (HermesVol *)info;
    HermesVol *ret = (HermesVol *)(calloc(1, sizeof(HermesVol)));
    ret->native_driver_id=o->native_driver_id;
    ret->vol_id=o->vol_id;
    ret->native_fapl=o->native_fapl;
    ret->sync=o->sync;
    if(o->file_name){
        ret->file_name=(char*)malloc(strlen(o->file_name)+1);
        strcpy(ret->file_name,o->file_name);
    }
    if(o->dataset_name) {
        ret->dataset_name = (char *) malloc(strlen(o->dataset_name) + 1);
        strcpy(ret->dataset_name, o->dataset_name);
    }
    return ret;
}
static herr_t H5VL_hermes_fapl_free(void *info){
    HermesVol *o = (HermesVol *)(info);
    H5VLunregister(o->vol_id);
    herr_t output=H5Pclose(o->native_fapl);
    free(o->file_name);
    free(o);
    H5_CleanBuffer();
    return 0;
}