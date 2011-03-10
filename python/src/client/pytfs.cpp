/*
 * (C) 2011-2012 chuantong.huang@gmail.com Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: pytfs.cpp 95 2011-03-03 09:06:24Z chuantong.huang@gmail.com $
 *
 * Authors:
 *   chuantong <chuantong.huang@gmail.com>
 *      - initial release
 *
 */

#include "tfs_file.h"
#include "tfs_session.h"
#include "tfs_session_pool.h"
#include <Python.h>

using namespace tfs::client;
using namespace tfs::common;
using namespace std;

typedef struct {
    PyObject_HEAD
    PyObject *dict;                 /* Python attributes dictionary */
    TfsFile *tfs_handle;
} TfsClientObject;

static PyObject *ErrorObject = NULL;
static PyTypeObject *p_TfsClient_Type = NULL;

static char module_doc [] =
"This module implements an interface to the tfs client library.\n"
"\n"
"Types:\n"
"\n"
"TfsClient() -> New object.  Create a new TfsClient object.\n"
"Functions:\n"
"\n"
"version() -> tuple.  Return version information.\n"
;
static char tfsclient_doc [] =
"import pytfs\n"
"tfs = pytfs.TfsClient()\n tfs.initialize('127.0.0.1:10000')\n"
"tfs.newfile(open('/tmp/tmp.txt')) -> T1XXXXXXX\n"
"tfs.open('T1XXXXXX')\ntfs.read(size)\ntfs.close()\n\n"
"or you can use the easy function:\n"
"\ttfs.put(stream) # put a new file to tfs.\n"
"\ttfs.get('T1xxxxxxx') # get a file from tfs.\n"
;

const char * _check_str_obj(PyObject *obj)
{
    char *str = NULL;
	if (Py_None == obj)
		return str;

	if (PyString_Check(obj))
	{
		if (0 == PyString_Size(obj) || 0 != PyString_AsStringAndSize(obj, &str, NULL))
			return NULL;
		return str;
	}
	return NULL;
}

static char pytfs_version_doc [] = "pytfs.version() -> version info";
static PyObject *
do_pytfs_version(PyObject *self, PyObject *args)
{
	PyObject* pTuple = PyTuple_New(2);
	PyTuple_SetItem(pTuple, 0, Py_BuildValue("s", "0.1"));
	PyTuple_SetItem(pTuple, 1, Py_BuildValue("s", "author: chuantong.huang@gmail.com"));
	Py_INCREF(pTuple);
	return pTuple;
}

static char pytfs_setloglevel_doc [] = "setloglevel(DEBUG|WARN|INFO|ERROR) default = 'ERROR' -> None";
static PyObject *
do_pytfs_setloglevel(PyObject *self, PyObject *args)
{
	const char *level = NULL;
	Py_INCREF(Py_None);

	if (!PyArg_ParseTuple(args, "s", &level))
		return Py_None;

	TBSYS_LOGGER.setLogLevel(level);
	return Py_None;
}

// 构造函数
static char tfsclient_new_doc [] = "Create new instances of Class TfsClient. you must call initialize to initialize it.";
TfsClientObject*
do_tfsclient_new(PyObject *dummy)
{
	UNUSED(dummy);
	TfsClientObject *self = NULL;

	self = (TfsClientObject *) PyObject_GC_New(TfsClientObject, p_TfsClient_Type);
	if (self == NULL)
		goto error;

	self->tfs_handle = new TfsFile();
	if (self == NULL)
		goto error;

	self->dict = PyDict_New();
	if(self->dict == NULL)
		goto error;

	PyObject_GC_Track(self);

	return self;
error:
	PyErr_SetString(ErrorObject, "PyObject_GC_New(TfsClientObject) error!");
	Py_DECREF(self);
	return NULL;
};

// 析构函数，后先调用clear
static void
_tfsclient_dealloc(PyObject *self)
{
    PyObject_GC_UnTrack(self);
    Py_TRASHCAN_SAFE_BEGIN(self)

    PyObject_GC_Del(self);

    Py_TRASHCAN_SAFE_END(self)
}

static int
_tfsclient_clear(TfsClientObject *self)
{
	delete self->tfs_handle;
	self->tfs_handle = NULL;
    return 0;
}

static int
_tfsclient_traverse(TfsClientObject *self, visitproc visit, void *arg)
{
    return 0;
}

static char tfsclient_initialize_doc [] =
		"tfsclient.initialize('127.0.0.1:10000', cacheTimeBySeconds = 5, cacheItems = 100) -> True or False";
static PyObject *
do_tfsclient_initialize(TfsClientObject *self, PyObject *args)
{
	const char *ns_ip_port = NULL;
	int cache_time = 5;
	int cache_items = 300;
	TfsSession* session = NULL;

	if (!PyArg_ParseTuple(args, "s|ii", &ns_ip_port, &cache_time, &cache_items))
		goto error;

	if (NULL == ns_ip_port)
		goto error;

	session = TfsSessionPool::get_instance().get(ns_ip_port, cache_time, cache_items);
	if (session == NULL) {
		cerr << "connect to name_server failed."<< endl;
		Py_INCREF(Py_False);
		return Py_False;
	}
	self->tfs_handle->set_session(session);
	Py_INCREF(Py_True);
	return Py_True;

error:
	PyErr_SetString(PyExc_TypeError, "invalid arguments to initialize.");
    Py_INCREF(Py_False);
    return Py_False;
}

static char tfsclient_tfs_open_doc [] =
		"tfs_open(file_name, suffix, pytfs.WRITE_MODE|pytfs.READ_MODE)"
		"file_name TFS文件名, 新建时传空None \n"
		"suffix    文件后缀，如果没有空None \n"
		"mode      打开文件的模式\n"
		"\tREAD_MODE 读\n"
		"\tWRITE_MODE 写\n"
		" -> True or False\n";
static PyObject *
do_tfsclient_tfs_open(TfsClientObject *self, PyObject *args)
{
    const char *file_name = NULL;
    const char  *suffix = NULL;
    PyObject *ofname = NULL;
    PyObject *osuffix = NULL;
    int mode ;

    if (!PyArg_ParseTuple(args, "Oi|O:tfs_open", &ofname, &mode, &osuffix)){
        PyErr_SetString(PyExc_TypeError, "invalid arguments to tfs_open...");
        goto error;
    }

    file_name = _check_str_obj(ofname);
    suffix = _check_str_obj(osuffix);

    if (TFS_SUCCESS == self->tfs_handle->tfs_open(file_name, suffix, mode))
    {
        Py_INCREF(Py_True);
        return Py_True;
    }
    PyErr_SetString(ErrorObject, "error to tfs_open.");
error:
    Py_INCREF(Py_False);
    return Py_False;
}

static Py_ssize_t WROTE_PRE_ONE = 2 * 1024 * 1024;


static int _tfs_write(TfsClientObject *self, PyObject* stream)
{
    char *buff = NULL;
    Py_ssize_t ret = 0;
    Py_ssize_t len = 0;
    Py_ssize_t wrote = 0;
    Py_ssize_t left ;
    int wrote_size ;

    if (0 != PyString_AsStringAndSize(stream, &buff, &len)){
        cerr << "string is empty to tfs_write." << endl;
        return -1;
    }

    left = len;

    while (left > 0) {
        wrote_size = left > WROTE_PRE_ONE ? WROTE_PRE_ONE : left;
        // 将buffer中的数据写入tfs
        ret = self->tfs_handle->tfs_write((char*)(buff + wrote), wrote_size);
        if (ret >= left) {
            break;
        }
        else if (ret < 0) {
            cerr << "tfs_write file error." << endl;
            return -2;
        } else {
            // 若ret>0，则ret为实际写入的数据量
            wrote += ret;
            left -= ret;
        }
    }
    return 0;
}

static char tfsclient_tfs_write_doc [] =
    "tfs_write(str)\n"
    "must call tfs_open first. len(str) <= (2 * 1024 * 1024) well be better.\n";
static PyObject *
do_tfsclient_tfs_write(TfsClientObject *self, PyObject *args)
{
    PyObject *obj = NULL;

    if (!PyArg_ParseTuple(args, "O", &obj) && !PyString_Check(obj)){
        PyErr_SetString(PyExc_TypeError, "invalid arguments to tfs_write.");
        goto error;
    }

    if (0 == _tfs_write(self, obj)){
        Py_INCREF(Py_True);
        return Py_True;
    }
error:
    Py_INCREF(Py_False);
    return Py_False;
}

static char tfsclient_tfs_close_doc [] =
    "tfs_close()\n Return (True/False)";
static PyObject *
do_tfsclient_tfs_close(TfsClientObject *self, PyObject *args)
{
    int ret = self->tfs_handle->tfs_close();
    if (TFS_SUCCESS != ret) {
        PyErr_SetString(ErrorObject, "tfs_close: upload file error." );
        cout << ret << endl;
        Py_INCREF(Py_False);
        return Py_False;
    }
    Py_INCREF(Py_True);
    return Py_True;
}

static char tfsclient_tfs_getname_doc [] =
    "tfs_getname()\n Return (tfsname/None)";
static PyObject *
do_tfsclient_tfs_getname(TfsClientObject *self, PyObject *args)
{
    const char *str = self->tfs_handle->get_file_name();
    if (NULL == str) {
        PyErr_SetString(ErrorObject, "tfs_getname: get file name error." );

        Py_INCREF(Py_None);
        return Py_None;
    }

    PyObject* pString = Py_BuildValue("s", str);
    return pString;
}

static char tfsclient_tfs_read_doc [] =
    "tfs_read()\n Return (str)";
static PyObject *
do_tfsclient_tfs_read(TfsClientObject *self, PyObject *args)
{
    FileInfo  finfo;
    int ret;
    char* buffer = NULL;
    int read = 0;
    int read_size;
    Py_ssize_t crc = 0;
    Py_ssize_t left = 0;
    PyObject* pString;
    ret = self->tfs_handle->tfs_stat(&finfo);
    if (ret != TFS_SUCCESS || finfo.size_ <= 0)
    {
        PyErr_SetString(ErrorObject, "get remote file info error\n");
        goto error;
    }

    buffer = new char[finfo.size_];
    left = finfo.size_;

    while (read < finfo.size_) {
        read_size = left > WROTE_PRE_ONE ? WROTE_PRE_ONE : left;
        ret = self->tfs_handle->tfs_read((char*)buffer + read, read_size);
        if (ret < 0) {
            break;
        } else {
            crc = Func::crc(crc, (const char*)(buffer + read), ret); // 对读取的文件计算crc值
            read += ret;
            left -= ret;
        }
    }

    if (ret < 0 || crc != finfo.crc_){
        printf("read remote file error!\n");
        delete []buffer;
        goto error ;
    }

    ret = self->tfs_handle->tfs_close();
    if (ret < 0)
    {
        printf("close remote file error!");
        delete []buffer;
        goto error ;
    }
    pString = (PyObject*)PyString_FromStringAndSize(buffer, finfo.size_);
    delete []buffer;
    buffer = NULL;
    return pString;
error:
    Py_INCREF(Py_None);
    return Py_None;
}

static char tfsclient_tfs_put_doc [] =
    "tfs_put(file_bin_stream_as_str)\n create a new file and save to tfs.\n "
    "Return success -> tfsname; error->False";
static PyObject *
do_tfsclient_tfs_put(TfsClientObject *self, PyObject *args)
{
    const char *buff = NULL;
    PyObject *obj = NULL;
    PyObject* pString = NULL;

    if (!PyArg_ParseTuple(args, "O:tfs_put", &obj) && !PyString_Check(obj)){
        cerr << "invalid arguments to tfs_put..." << endl;
        goto error;
    }

    if(TFS_SUCCESS != self->tfs_handle->tfs_open(buff, buff, WRITE_MODE)){
        cerr << "error to open tfs file to tfs_put..." << endl;
        goto error;
    }

    if (0 != _tfs_write(self, obj)){
        cerr << "write file error to tfs_put..." << endl;
        goto error;
    }

    if (TFS_SUCCESS != self->tfs_handle->tfs_close()) {
        cerr << "close file error to tfs_put..." << endl;
        goto error;
    }

    buff = self->tfs_handle->get_file_name();
    if (NULL == buff) {
        cerr << "get file name error to tfs_put..." << endl;
        goto error;
    }

    pString = Py_BuildValue("s", buff);
    return pString;
error:
    Py_INCREF(Py_False);
    return Py_False;
}

static char tfsclient_tfs_get_doc [] =
    "tfs_get(tfsname)\n create a new file and save to tfs.\n Return str";
static PyObject *
do_tfsclient_tfs_get(TfsClientObject *self, PyObject *args)
{
    char *tfsname = NULL;
    PyObject *obj = NULL;
    Py_ssize_t len;

    if (!PyArg_ParseTuple(args, "O:tfs_get", &obj) && !PyString_Check(obj)){
        cerr << "invalid arguments to tfs_get..." << endl;
        goto error;
    }

    if (0 != PyString_AsStringAndSize(obj, &tfsname, &len) && TFS_FILE_LEN != len){
        cerr << "error tfsname to tfs_get." << endl;
        goto error;
    }

    if(TFS_SUCCESS != self->tfs_handle->tfs_open(tfsname, NULL, READ_MODE)){
        cerr << "error to open tfs file to tfs_get..." << endl;
        goto error;
    }

    return do_tfsclient_tfs_read(self, NULL);

error:
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef pytfsobject_methods[] = {
    {"init", (PyCFunction)do_tfsclient_initialize, METH_VARARGS, tfsclient_initialize_doc},
    {"tfs_open", (PyCFunction)do_tfsclient_tfs_open, METH_VARARGS, tfsclient_tfs_open_doc},
    {"tfs_write", (PyCFunction)do_tfsclient_tfs_write, METH_VARARGS, tfsclient_tfs_write_doc},
    {"tfs_close", (PyCFunction)do_tfsclient_tfs_close, METH_NOARGS, tfsclient_tfs_close_doc},
    {"tfs_getname", (PyCFunction)do_tfsclient_tfs_getname, METH_NOARGS, tfsclient_tfs_getname_doc},
    {"tfs_read", (PyCFunction)do_tfsclient_tfs_read, METH_NOARGS, tfsclient_tfs_read_doc},
    {"tfs_put", (PyCFunction)do_tfsclient_tfs_put, METH_VARARGS, tfsclient_tfs_put_doc},
    {"tfs_get", (PyCFunction)do_tfsclient_tfs_get, METH_VARARGS, tfsclient_tfs_get_doc},
    {NULL, NULL, 0, NULL}
};

// 2）方法列表
static PyMethodDef pytfsMethods[] =
{
   {"TfsClient", (PyCFunction)do_tfsclient_new, METH_NOARGS, tfsclient_new_doc},
   {"version", (PyCFunction)do_pytfs_version, METH_VARARGS, pytfs_version_doc},
   {"setloglevel", (PyCFunction)do_pytfs_setloglevel, METH_VARARGS, pytfs_setloglevel_doc},
   {NULL, NULL}
};
static int
_tfsclient_setattr(TfsClientObject *self, char *name, PyObject *v)
{
    if (v == NULL) {
        int rv = -1;
        if (self->dict != NULL)
            rv = PyDict_DelItemString(self->dict, name);
        if (rv < 0)
            PyErr_SetString(PyExc_AttributeError, "delete non-existing attribute");
        return rv;
    }
    if (self->dict == NULL) {
        self->dict = PyDict_New();
        if (self->dict == NULL)
            return -1;
    }
    return PyDict_SetItemString(self->dict, name, v);
}

static PyObject *
_tfsclient_getattr(TfsClientObject *self, char *name)
{
    PyObject *v = NULL;
    if (v == NULL && self->dict != NULL)
        v = PyDict_GetItemString(self->dict, name);

    if (v != NULL) {
        Py_INCREF(v);
        return v;
    }
    return Py_FindMethod(pytfsobject_methods, (PyObject *)self, name);
}
// 类定义
static PyTypeObject Pytfs_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                          /* ob_size */
    "pytfs.TfsClient",	            /* tp_name */
    sizeof(PyTypeObject),    	/* tp_basicsize */
    0,                          /* tp_itemsize */
    /* Methods */
    (destructor)_tfsclient_dealloc,   /* tp_dealloc */
    0,                          /* tp_print */
    (getattrfunc)_tfsclient_getattr,    /* tp_getattr */
    (setattrfunc)_tfsclient_setattr,    /* tp_setattr */
    0,                          /* tp_compare */
    0,                          /* tp_repr */
    0,                          /* tp_as_number */
    0,                          /* tp_as_sequence */
    0,                          /* tp_as_mapping */
    0,                          /* tp_hash */
    0,                          /* tp_call */
    0,                          /* tp_str */
    0,                          /* tp_getattro */
    0,                          /* tp_setattro */
    0,                          /* tp_as_buffer */
    Py_TPFLAGS_HAVE_GC,         /* tp_flags */
    tfsclient_doc,              /* tp_doc */
    (traverseproc)_tfsclient_traverse, /* tp_traverse */
    (inquiry)_tfsclient_clear      /* tp_clear */
    /* More fields follow here, depending on your Python version. You can
     * safely ignore any compiler warnings about missing initializers.
     */
};

// 3) 初始化函数, 为C的dll导出函数
extern "C"
{
    DL_EXPORT(void) initpytfs()
    {	// default log level is ERROR
    	TBSYS_LOGGER.setLogLevel("ERROR");

    	Pytfs_Type.ob_type = &PyType_Type;
    	p_TfsClient_Type = &Pytfs_Type;
    	Pytfs_Type.tp_methods = pytfsobject_methods;

    	PyObject *module, *mods_dict;

    	module = Py_InitModule3("pytfs", pytfsMethods, module_doc);
        assert(module != NULL && PyModule_Check(module));

		/* Add error object to the module */
        mods_dict = PyModule_GetDict(module);
	    assert(mods_dict != NULL);
	    ErrorObject = PyErr_NewException("pytfs.TfsError", NULL, NULL);
	    assert(ErrorObject != NULL);
	    PyDict_SetItemString(mods_dict, "TfsError", ErrorObject);

	    //for tfs_open
	    PyDict_SetItemString(mods_dict, "READ_MODE", PyInt_FromLong(READ_MODE));
	    PyDict_SetItemString(mods_dict, "WRITE_MODE", PyInt_FromLong(WRITE_MODE));
//	    PyDict_SetItemString(mods_dict, "APPEND_MODE", PyInt_FromLong(APPEND_MODE));
//	    PyDict_SetItemString(mods_dict, "UNLINK_MODE", PyInt_FromLong(UNLINK_MODE));
//	    PyDict_SetItemString(mods_dict, "NEWBLK_MODE", PyInt_FromLong(NEWBLK_MODE));
	    //for link unlink
//	    PyDict_SetItemString(mods_dict, "DELETE", PyInt_FromLong(DELETE));
//	    PyDict_SetItemString(mods_dict, "UNDELETE", PyInt_FromLong(UNDELETE));
//	    PyDict_SetItemString(mods_dict, "CONCEAL", PyInt_FromLong(CONCEAL));
//	    PyDict_SetItemString(mods_dict, "REVEAL", PyInt_FromLong(REVEAL));

//	    PyDict_SetItemString(mods_dict, "T_SEEK_SET", PyInt_FromLong(T_SEEK_SET));
//	    PyDict_SetItemString(mods_dict, "T_SEEK_CUR", PyInt_FromLong(T_SEEK_CUR));
//	    PyDict_SetItemString(mods_dict, "T_SEEK_END", PyInt_FromLong(T_SEEK_END));


    }
}