#ifndef __PyCamera_H__
#define __PyCamera_H__

#include <Python.h>
#include <structmember.h>

#include "Camera.h"

typedef struct
{
    PyObject_HEAD Camera *handler;

} PyCamera;

static int PyCamera_clear(PyCamera *self)
{
    if (self->handler)
    {
        delete self->handler;
    }
    return 0;
}

static void PyCamera_dealloc(PyCamera *self)
{
    PyCamera_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *PyCamera_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyCamera *self;

    self = (PyCamera *)type->tp_alloc(type, 0);
    if (self != NULL)
    {
        self->handler = new Camera();
    }

    return (PyObject *)self;
}

static PyObject *open(PyObject *obj, PyObject *args)
{
    PyCamera *self = (PyCamera *)obj;

    int index = 0;
    if (!PyArg_ParseTuple(args, "i", &index))
    {
        return NULL;
    }

    bool ret = self->handler->Open(index);
    return Py_BuildValue("i", ret);
}

static PyObject *listMediaTypes(PyObject *obj, PyObject *args)
{
    PyCamera *self = (PyCamera *)obj;

    std::vector<MediaTypeInfo> mediaTypes = self->handler->ListSupportedMediaTypes();

    PyObject *pyList = PyList_New(0);

    for (size_t i = 0; i < mediaTypes.size(); i++)
    {
        MediaTypeInfo &mediaType = mediaTypes[i];

#ifdef _WIN32
        // For Windows: Convert wide-character string to UTF-8
        PyObject *subtypeName = PyUnicode_FromWideChar(mediaType.subtypeName, wcslen(mediaType.subtypeName));
        PyObject *pyMediaType = Py_BuildValue("{s:i,s:i,s:O}",
                                              "width", mediaType.width,
                                              "height", mediaType.height,
                                              "subtypeName", subtypeName);
        if (subtypeName != NULL)
        {
            Py_DECREF(subtypeName);
        }

#else
        // For Linux and macOS: Use char* directly
        PyObject *pyMediaType = Py_BuildValue("{s:i,s:i,s:s}",
                                              "width", mediaType.width,
                                              "height", mediaType.height,
                                              "subtypeName", mediaType.subtypeName);
#endif

        if (pyMediaType != NULL)
        {
            PyList_Append(pyList, pyMediaType);
            Py_DECREF(pyMediaType); // Fix memory leak: decrement reference after adding to list
        }
    }

    return pyList;
}

static PyObject *release(PyObject *obj, PyObject *args)
{
    PyCamera *self = (PyCamera *)obj;

    self->handler->Release();
    Py_RETURN_NONE;
}

static PyObject *setResolution(PyObject *obj, PyObject *args)
{
    PyCamera *self = (PyCamera *)obj;
    int width = 0, height = 0;
    if (!PyArg_ParseTuple(args, "ii", &width, &height))
    {
        return NULL;
    }
    int ret = self->handler->SetResolution(width, height);
    return Py_BuildValue("i", ret);
}

static PyObject *captureFrameInfo(PyObject *obj, PyObject *args)
{
    PyCamera *self = (PyCamera *)obj;

    FrameData frame = self->handler->CaptureFrame();
    if (frame.rgbData)
    {
        // Store frame dimensions
        int frameWidth = frame.width;
        int frameHeight = frame.height;
        size_t frameSize = frame.size;

        // Release the C++ frame data immediately without creating Python objects
        ReleaseFrame(frame);

        // Return just the frame info without the data
        return Py_BuildValue("iii", frameWidth, frameHeight, frameSize);
    }
    else
    {
        Py_RETURN_NONE;
    }
}

static PyObject *captureFrame(PyObject *obj, PyObject *args)
{
    PyCamera *self = (PyCamera *)obj;

    FrameData frame = self->handler->CaptureFrame();
    if (frame.rgbData)
    {
        // Store frame dimensions before releasing
        int frameWidth = frame.width;
        int frameHeight = frame.height;
        size_t frameSize = frame.size;

        // Create bytearray with a copy of the data
        PyObject *rgbData = PyByteArray_FromStringAndSize((const char *)frame.rgbData, frameSize);

        // Release the C++ frame data immediately after copying
        ReleaseFrame(frame);

        if (rgbData == NULL)
        {
            // Handle memory allocation failure
            return NULL;
        }

        // Build the return tuple with width, height, size, and data
        PyObject *pyFrame = PyTuple_New(4);
        if (pyFrame == NULL)
        {
            Py_DECREF(rgbData);
            return NULL;
        }

        PyTuple_SET_ITEM(pyFrame, 0, PyLong_FromLong(frameWidth));
        PyTuple_SET_ITEM(pyFrame, 1, PyLong_FromLong(frameHeight));
        PyTuple_SET_ITEM(pyFrame, 2, PyLong_FromLong(frameSize));
        PyTuple_SET_ITEM(pyFrame, 3, rgbData); // rgbData reference is stolen by tuple

        return pyFrame;
    }
    else
    {
        Py_RETURN_NONE;
    }
}

static PyObject *getWidth(PyObject *obj, PyObject *args)
{
    PyCamera *self = (PyCamera *)obj;

    int width = self->handler->frameWidth;
    return Py_BuildValue("i", width);
}

static PyObject *getHeight(PyObject *obj, PyObject *args)
{
    PyCamera *self = (PyCamera *)obj;

    int height = self->handler->frameHeight;
    return Py_BuildValue("i", height);
}

static PyMethodDef instance_methods[] = {
    {"open", open, METH_VARARGS, NULL},
    {"listMediaTypes", listMediaTypes, METH_VARARGS, NULL},
    {"release", release, METH_VARARGS, NULL},
    {"setResolution", setResolution, METH_VARARGS, NULL},
    {"captureFrame", captureFrame, METH_VARARGS, NULL},
    {"captureFrameInfo", captureFrameInfo, METH_VARARGS, NULL},
    {"getWidth", getWidth, METH_VARARGS, NULL},
    {"getHeight", getHeight, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}};

static PyTypeObject PyCameraType = {
    PyVarObject_HEAD_INIT(NULL, 0) "litecam.PyCamera", /* tp_name */
    sizeof(PyCamera),                                  /* tp_basicsize */
    0,                                                 /* tp_itemsize */
    (destructor)PyCamera_dealloc,                      /* tp_dealloc */
    0,                                                 /* tp_print */
    0,                                                 /* tp_getattr */
    0,                                                 /* tp_setattr */
    0,                                                 /* tp_reserved */
    0,                                                 /* tp_repr */
    0,                                                 /* tp_as_number */
    0,                                                 /* tp_as_sequence */
    0,                                                 /* tp_as_mapping */
    0,                                                 /* tp_hash  */
    0,                                                 /* tp_call */
    0,                                                 /* tp_str */
    PyObject_GenericGetAttr,                           /* tp_getattro */
    PyObject_GenericSetAttr,                           /* tp_setattro */
    0,                                                 /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,          /*tp_flags*/
    "PyCamera",                                        /* tp_doc */
    0,                                                 /* tp_traverse */
    0,                                                 /* tp_clear */
    0,                                                 /* tp_richcompare */
    0,                                                 /* tp_weaklistoffset */
    0,                                                 /* tp_iter */
    0,                                                 /* tp_iternext */
    instance_methods,                                  /* tp_methods */
    0,                                                 /* tp_members */
    0,                                                 /* tp_getset */
    0,                                                 /* tp_base */
    0,                                                 /* tp_dict */
    0,                                                 /* tp_descr_get */
    0,                                                 /* tp_descr_set */
    0,                                                 /* tp_dictoffset */
    0,                                                 /* tp_init */
    0,                                                 /* tp_alloc */
    PyCamera_new,                                      /* tp_new */
};

#endif