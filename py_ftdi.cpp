/**
 * Copyright (C) 2023 Daniel Turecek
 *
 * @file      py_ftdi.cpp
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2023-02-03
 *
 */
#include "Python.h"
#include "structmember.h"
#include "ftdidev.h"
#include "buffer.h"

typedef struct {
    PyObject_HEAD
    FtdiDev* dev;
} Device;

static int device_init(Device *self, PyObject *args, PyObject *kwds)
{
    self->dev = NULL;
    return 0;
}

static void device_dealloc(Device *self)
{
    if (self->dev){
        delete self->dev;
        self->dev = NULL;
    }

    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* device_listDevices(PyObject* self, PyObject *args)
{
    (void)self;
    std::vector<FtdiDevInfo> devs;
    FtdiDev::listDevicesByNameFast(NULL, 0, devs, false);
    PyObject* obj = PyList_New(devs.size());
    for (size_t i = 0; i < devs.size(); i++){
        PyList_SetItem(obj, i, PyUnicode_FromString(devs[i].name.c_str()));
    }
    return obj;
}

static PyObject* device_open(Device* self, PyObject *args)
{
    (void)self;
    const char* devName;
    int baud;
    int interface;
    if (!PyArg_ParseTuple(args, "sii", &devName, &baud, &interface))
        return NULL;

    if (self->dev)
        self->dev->closeDevice();

    self->dev = new FtdiDev(devName, false);
    int rc = self->dev->openDevice(false, 0, interface);
    if (baud != 0)
        self->dev->setBaudRate(baud);

    return Py_BuildValue("i", rc);
}

static PyObject* device_setSyncMode(Device* self, PyObject *args)
{
    int sync;
    if (!PyArg_ParseTuple(args, "i", &sync))
        return NULL;

    if (!self->dev)
        return Py_BuildValue("i", -1000);

    int rc = self->dev->setBitMode(sync ? FtdiDev::BIT_SYNC : FtdiDev::BIT_ASYNC);
    return Py_BuildValue("i", rc);
}

static PyObject* device_isConnected(Device* self, PyObject *args)
{
    (void)self;
    if (!self->dev)
        return Py_BuildValue("i", 0);

    int rc = self->dev->isConnected();
    return Py_BuildValue("i", rc);
}

static PyObject* device_clearBuffers(Device* self, PyObject *args)
{
    (void)self;
    if (!self->dev)
        return Py_BuildValue("i", -1000);

    int rc = self->dev->clearBuffers();
    return Py_BuildValue("i", rc);
}


static PyObject* device_close(Device* self, PyObject *args)
{
    (void)args;
    int rc = 0;
    if (self->dev){
        rc = self->dev->closeDevice();
        delete self->dev;
    }
    self->dev = NULL;
    return Py_BuildValue("i", rc);
}

static PyObject* device_send(Device* self, PyObject *args)
{
    PyObject* data;
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &data))
        return NULL;

    if (!self->dev)
        return Py_BuildValue("i", -1000);

    Py_ssize_t count = PyList_Size(data);
    if (count < 0){
        PyErr_SetString(PyExc_IOError, "Invalid data.");
        return NULL;
    }

    Buffer<unsigned char> buff(count);
    for (int i = 0; i < count; i++)
        buff[i] = static_cast<unsigned char>(PyLong_AsLong(PyList_GetItem(data, i)));

    int rc = self->dev->send((char*)buff.data(), (size_t)count);
    return Py_BuildValue("i", rc);
}

static PyObject* device_read(Device* self, PyObject *args)
{
    (void)self;
    int size;
    double timeout;
    if (!PyArg_ParseTuple(args, "id", &size, &timeout))
        return NULL;
    if (!self->dev)
        return Py_BuildValue("i", -1000);

    Buffer<unsigned char> buff(size + 1);
    buff.zero();
    int rc = self->dev->receive((char*)buff.data(), size, size, timeout);
    PyObject* list = PyList_New(2);

    PyObject* dataout = PyList_New(rc > 0 ? rc : 0);
    if (rc > 0)
        for (int i = 0; i < rc; i++)
            PyList_SetItem(dataout, i, PyLong_FromLong(buff[i]));

    PyList_SET_ITEM(list, 0, Py_BuildValue("i", rc));
    PyList_SET_ITEM(list, 1, dataout);
    return list;
}


static PyMemberDef device_members[] =
{
   { NULL }
};

static PyMethodDef device_methods[] =
{
    {"list_devices", (PyCFunction)device_listDevices, METH_VARARGS, "list_devices()"},
    {"open", (PyCFunction)device_open, METH_VARARGS, "open(dev_name,baud,interface_index)"},
    {"close", (PyCFunction)device_close, METH_VARARGS, "close()"},
    {"set_sync_mode", (PyCFunction)device_setSyncMode, METH_VARARGS, "set_sync_mode(is_sync_mode)"},
    {"is_connected", (PyCFunction)device_isConnected, METH_VARARGS, "is_connected()"},
    {"clear_buffers", (PyCFunction)device_clearBuffers, METH_VARARGS, "clear_buffers()"},
    {"send", (PyCFunction)device_send, METH_VARARGS, "send(data)"},
    {"read", (PyCFunction)device_read, METH_VARARGS, "read(size, timeout)"},
    { NULL }
};

PyTypeObject DeviceType =
{
   PyVarObject_HEAD_INIT(NULL, 0)
   //PyObject_HEAD_INIT(NULL)
   //0,                         /* ob_size */
   "Device",               /* tp_name */
   sizeof(Device),         /* tp_basicsize */
   0,                         /* tp_itemsize */
   (destructor)device_dealloc, /* tp_dealloc */
   0,                         /* tp_print */
   0,                         /* tp_getattr */
   0,                         /* tp_setattr */
   0,                         /* tp_compare */
   0,                         /* tp_repr */
   0,                         /* tp_as_number */
   0,                         /* tp_as_sequence */
   0,                         /* tp_as_mapping */
   0,                         /* tp_hash */
   0,                         /* tp_call */
   0,                         /* tp_str */
   0,                         /* tp_getattro */
   0,                         /* tp_setattro */
   0,                         /* tp_as_buffer */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags*/
   "Device object",        /* tp_doc */
   0,                         /* tp_traverse */
   0,                         /* tp_clear */
   0,                         /* tp_richcompare */
   0,                         /* tp_weaklistoffset */
   0,                         /* tp_iter */
   0,                         /* tp_iternext */
   device_methods,         /* tp_methods */
   device_members,         /* tp_members */
   0,                         /* tp_getset */
   0,                         /* tp_base */
   0,                         /* tp_dict */
   0,                         /* tp_descr_get */
   0,                         /* tp_descr_set */
   0,                         /* tp_dictoffset */
   (initproc)device_init,  /* tp_init */
   0,                         /* tp_alloc */
   0,                         /* tp_new */
};


//################################################################################
//                      INIT MODULE
//################################################################################

static PyMethodDef module_methods[] = {
    {"list_devices", (PyCFunction)device_listDevices, METH_VARARGS, "list_devices()"},
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "py_ftdi",           /* m_name */
    "Interface to ftdi",    /* m_doc */
    -1,                  /* m_size */
    module_methods,    /* m_methods */
    NULL,                /* m_reload */
    NULL,                /* m_traverse */
    NULL,                /* m_clear */
    NULL,                /* m_free */
};

PyMODINIT_FUNC PyInit_py_ftdi(void)
{
    PyObject* m = PyModule_Create(&moduledef);
    if (m == NULL)
        return NULL;

    DeviceType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&DeviceType) < 0)
        return m;

    Py_INCREF(&DeviceType);
    PyModule_AddObject(m, "Device", (PyObject*)&DeviceType);

    return m;
}

PyMODINIT_FUNC PyInit_py_ftdi_linux(void)
{
    moduledef.m_name = "py_ftdi_linux";
    return PyInit_py_ftdi();
}
PyMODINIT_FUNC PyInit_py_ftdi_mac(void)
{
    moduledef.m_name = "py_ftdi_mac";
    return PyInit_py_ftdi();
}






