#include <Python.h>

#include <stdio.h>
#include <vector>

#include "pycamera.h"
#include "pywindow.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define INITERROR return NULL

static PyObject *getDeviceList(PyObject *obj, PyObject *args)
{
	PyObject *pyList = PyList_New(0);

	std::vector<CaptureDeviceInfo> devices = ListCaptureDevices();

	for (size_t i = 0; i < devices.size(); i++)
	{
		CaptureDeviceInfo &device = devices[i];

#ifdef _WIN32
		PyObject *pyName = PyUnicode_FromWideChar(device.friendlyName, wcslen(device.friendlyName));
		if (pyName != NULL)
		{
			PyList_Append(pyList, pyName);
			Py_DECREF(pyName);
		}
#else
		// For Linux and macOS: Use char* directly
		PyObject *pyName = PyUnicode_FromString(device.friendlyName);
		if (pyName != NULL)
		{
			PyList_Append(pyList, pyName);
			Py_DECREF(pyName);
		}
#endif
	}

	return pyList;
}

static PyObject *saveJpeg(PyObject *obj, PyObject *args)
{
	const char *filename = NULL;
	int width = 0, height = 0;
	PyObject *byteArray = NULL;

	if (!PyArg_ParseTuple(args, "siiO", &filename, &width, &height, &byteArray))
	{
		PyErr_SetString(PyExc_TypeError, "Expected arguments: str, int, int, PyByteArray");
		return NULL;
	}

	unsigned char *data = (unsigned char *)PyByteArray_AsString(byteArray);
	Py_ssize_t size = PyByteArray_Size(byteArray);

	if (size != (Py_ssize_t)(width * height * 3))
	{
		PyErr_SetString(PyExc_ValueError, "Invalid byte array size for the given width and height.");
		return NULL;
	}

	if (stbi_write_jpg(filename, width, height, 3, data, 90))
	{
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString(PyExc_RuntimeError, "Failed to save JPEG file.");
		return NULL;
	}
}

void write_to_memory(void *context, void *data, int size)
{
	std::vector<unsigned char> *buffer = (std::vector<unsigned char> *)context;
	unsigned char *cdata = (unsigned char *)data;
	buffer->insert(buffer->end(), cdata, cdata + size);
}

static PyObject *saveJpegInMemory(PyObject *obj, PyObject *args)
{
	int width = 0, height = 0;
	PyObject *byteArray = NULL;

	if (!PyArg_ParseTuple(args, "iiO", &width, &height, &byteArray))
	{
		PyErr_SetString(PyExc_TypeError, "Expected arguments: int, int, PyByteArray");
		return NULL;
	}

	unsigned char *data = (unsigned char *)PyByteArray_AsString(byteArray);
	Py_ssize_t size = PyByteArray_Size(byteArray);

	if (size != (Py_ssize_t)(width * height * 3))
	{
		PyErr_SetString(PyExc_ValueError, "Invalid byte array size for the given width and height.");
		return NULL;
	}

	std::vector<unsigned char> buffer;
	if (stbi_write_jpg_to_func(write_to_memory, &buffer, width, height, 3, data, 90))
	{
		return PyBytes_FromStringAndSize((const char *)buffer.data(), buffer.size());
	}
	else
	{
		PyErr_SetString(PyExc_RuntimeError, "Failed to save JPEG to memory.");
		return NULL;
	}
}

static PyObject *savePngInMemory(PyObject *obj, PyObject *args)
{
	int width = 0, height = 0;
	PyObject *byteArray = NULL;

	if (!PyArg_ParseTuple(args, "iiO", &width, &height, &byteArray))
	{
		PyErr_SetString(PyExc_TypeError, "Expected arguments: int, int, PyByteArray");
		return NULL;
	}

	unsigned char *data = (unsigned char *)PyByteArray_AsString(byteArray);
	Py_ssize_t size = PyByteArray_Size(byteArray);

	if (size != (Py_ssize_t)(width * height * 3))
	{
		PyErr_SetString(PyExc_ValueError, "Invalid byte array size for the given width and height.");
		return NULL;
	}

	std::vector<unsigned char> buffer;
	// stbi_write_png_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data, int stride_in_bytes);
	if (stbi_write_png_to_func(write_to_memory, &buffer, width, height, 3, data, width * 3))
	{
		return PyBytes_FromStringAndSize((const char *)buffer.data(), buffer.size());
	}
	else
	{
		PyErr_SetString(PyExc_RuntimeError, "Failed to save PNG to memory.");
		return NULL;
	}
}

static PyObject *savePng(PyObject *obj, PyObject *args)
{
	const char *filename = NULL;
	int width = 0, height = 0;
	PyObject *byteArray = NULL;

	if (!PyArg_ParseTuple(args, "siiO", &filename, &width, &height, &byteArray))
	{
		PyErr_SetString(PyExc_TypeError, "Expected arguments: str, int, int, PyByteArray");
		return NULL;
	}

	unsigned char *data = (unsigned char *)PyByteArray_AsString(byteArray);
	Py_ssize_t size = PyByteArray_Size(byteArray);

	if (size != (Py_ssize_t)(width * height * 3))
	{
		PyErr_SetString(PyExc_ValueError, "Invalid byte array size for the given width and height.");
		return NULL;
	}

	if (stbi_write_png(filename, width, height, 3, data, width * 3))
	{
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString(PyExc_RuntimeError, "Failed to save PNG file.");
		return NULL;
	}
}

static PyMethodDef litecam_methods[] = {
	{"getDeviceList", getDeviceList, METH_VARARGS, "Get available cameras"},
	{"saveJpeg", saveJpeg, METH_VARARGS, "Save the frame as a JPEG image"},
	{"savePng", savePng, METH_VARARGS, "Save the frame as a PNG image"},
	{"saveJpegInMemory", saveJpegInMemory, METH_VARARGS, "Save the frame as a JPEG image in memory"},
	{"savePngInMemory", savePngInMemory, METH_VARARGS, "Save the frame as a PNG image in memory"},
	{NULL, NULL, 0, NULL}};

static struct PyModuleDef litecam_module_def = {
	PyModuleDef_HEAD_INIT,
	"litecam",
	"Internal \"litecam\" module",
	-1,
	litecam_methods};

PyMODINIT_FUNC PyInit_litecam(void)
{
	PyObject *module = PyModule_Create(&litecam_module_def);
	if (module == NULL)
		INITERROR;

	if (PyType_Ready(&PyCameraType) < 0)
	{
		printf("Failed to initialize PyCameraType\n");
		Py_DECREF(module);
		return NULL;
	}

	if (PyModule_AddObject(module, "PyCamera", (PyObject *)&PyCameraType) < 0)
	{
		printf("Failed to add PyCamera to the module\n");
		Py_DECREF(&PyCameraType);
		Py_DECREF(module);
		INITERROR;
	}

	if (PyType_Ready(&PyWindowType) < 0)
	{
		printf("Failed to initialize PyWindowType\n");
		Py_DECREF(module);
		return NULL;
	}

	if (PyModule_AddObject(module, "PyWindow", (PyObject *)&PyWindowType) < 0)
	{
		printf("Failed to add PyWindow to the module\n");
		Py_DECREF(&PyWindowType);
		Py_DECREF(module);
		INITERROR;
	}

	return module;
}
