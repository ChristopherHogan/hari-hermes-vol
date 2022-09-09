/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights resehermesed.                                                      *
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
* Created: hermes_vol.h
* June2018
* Hariharan Devarajan <hdevarajan@hdfgroup.org>
*
* Purpose:Defines the Hermes VOL Plugin Private Header file
*
*-------------------------------------------------------------------------
*/

#ifndef HERMES_PROJECT_HERMES_VOL_PRIVATE_H
#define HERMES_PROJECT_HERMES_VOL_PRIVATE_H
#include <hdf5.h>
#include <H5VLpublic.h>
#include <assert.h>
#include <stdlib.h>
#include "../include/hermes_vol.h"
#include "../include/hermes.h"
#include <memory.h>

typedef struct H5VL_t {
    const H5VL_class_t *vol_cls;        /* constant driver class info                           */
    /* XXX: Is an integer big enough? */
    int                 nrefs;          /* number of references by objects using this struct    */
    hid_t               vol_id;         /* identifier for the VOL class                         */
} H5VL_t;
typedef struct H5VL_object_t {
    void               *vol_obj;        /* pointer to object created by driver                  */
    H5VL_t             *vol_info;       /* pointer to VOL info struct                           */
} H5VL_object_t;
/**
 * This is the Data structure used within Hermes VOl for maintaining basic data.
 */
typedef struct HermesVol {
    hid_t object_id;
    hid_t native_driver_id;
    hid_t vol_id;
    hid_t native_fapl;
    char* file_name;
    char* dataset_name;
    bool sync;
} HermesVol;

/* Hermes VOL Dataset callbacks */
static void  *hermes_dataset_create(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req);
static void  *hermes_dataset_open(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t dapl_id, hid_t dxpl_id, void **req);
static herr_t hermes_dataset_read(void *dset, hid_t mem_type_id, hid_t mem_space_id,
                              hid_t file_space_id, hid_t dxpl_id, void *buf, void **req);
static herr_t hermes_dataset_write(void *dset, hid_t mem_type_id, hid_t mem_space_id,
                               hid_t file_space_id, hid_t dxpl_id, const void *buf, void **req);
static herr_t  hermes_dataset_get(void *dset, H5VL_dataset_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t hermes_dataset_close(void *dset, hid_t dxpl_id, void **req);

/* Hermes VOL File callbacks */
static void  *hermes_file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req);
static void  *hermes_file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req);
static herr_t hermes_file_close(void *file, hid_t dxpl_id, void **req);

/* Hermes VOL other callbacks */
static herr_t H5VL_hermes_init(hid_t vipl_id);

static herr_t H5VL_hermes_term(hid_t vtpl_id);

static void *H5VL_hermes_fapl_copy(const void *info);

static herr_t H5VL_hermes_fapl_free(void *info);

/* definition of Hermes VOL plugin. */
static const H5VL_class_t H5VL_hermes_g = {
        1,
        (H5VL_class_value_t)(HERMES),
        "hermes",               /* name */
        H5VL_hermes_init,       /* initialize */
        H5VL_hermes_term,       /* terminate */
        sizeof(HermesVol),
        H5VL_hermes_fapl_copy,  /* fapl copy */
        H5VL_hermes_fapl_free,  /* fapl free */
        {
                NULL,           /* Attribute create function      */
                NULL,           /* Attribute open function        */
                NULL,           /* Attribute read function        */
                NULL,           /* Attribute write function       */
                NULL,           /* Attribute get function         */
                NULL,           /* Attribute specific function    */
                NULL,           /* Attribute optional function    */
                NULL            /* Attribute close function       */
        },
        {
                hermes_dataset_create,     /* Dataset create function        */
                hermes_dataset_open,       /* Dataset open function          */
                hermes_dataset_read,       /* Dataset read function          */
                hermes_dataset_write,      /* Dataset write function         */
                hermes_dataset_get,        /* Dataset get function           */
                NULL,                      /* Dataset specific function      */
                NULL,                      /* Dataset optional function      */
                hermes_dataset_close       /* Dataset close function         */
        },
        {
                NULL,                      /* Datatype commit function       */
                NULL,                      /* Datatype open function         */
                NULL,                      /* Datatype get function          */
                NULL,                      /* Datatype specific function     */
                NULL,                      /* Datatype optional function     */
                NULL                       /* Datatype close function        */
        },
        {
                hermes_file_create,             /* File create function           */
                hermes_file_open,               /* File open function             */
                NULL,                           /* File get function              */
                NULL,                           /* File specific function         */
                NULL,                           /* File optional function         */
                hermes_file_close               /* File close function            */
        },
        {
                NULL,                           /* Group create function          */
                NULL,                           /* Group open function            */
                NULL,                           /* Group get function             */
                NULL,                           /* Group specific function        */
                NULL,                           /* Group optional function        */
                NULL                            /* Group close function           */
        },
        {
                NULL,                           /* Link create function           */
                NULL,                           /* Link copy function             */
                NULL,                           /* Link move function             */
                NULL,                           /* Link get function              */
                NULL,                           /* Link specific function         */
                NULL                            /* Link optional function         */
        },
        {
                NULL,                           /* Object open function           */
                NULL,                           /* Object copy function           */
                NULL,                           /* Object get function            */
                NULL,                           /* Object specific function       */
                NULL                            /* Object optional function       */
        },
        {
                NULL,
                NULL,
                NULL
        },
        NULL
};
#endif //HERMES_PROJECT_HERMES_VOL_H
