#include <Python.h>

#include "SpaceMouse.hpp"

extern "C" {

static PyObject* start_spacemouse_daemon(PyObject* /*self*/, PyObject* args) {
  PyObject *pyMoveCallback, *pyButtonPressCallback, *pyButtonReleaseCallback;

  if (!PyArg_ParseTuple(args, "OOO", &pyMoveCallback, &pyButtonPressCallback,
                        &pyButtonReleaseCallback))
    return nullptr;

  if (!PyCallable_Check(pyMoveCallback)) {
    PyErr_SetString(PyExc_TypeError, "First argument (moveCallback) is not a function!");
  } else if (!PyCallable_Check(pyButtonPressCallback)) {
    PyErr_SetString(PyExc_TypeError, "Second argument (buttonPressCallback) is not a function!");
  } else if (!PyCallable_Check(pyButtonReleaseCallback)) {
    PyErr_SetString(PyExc_TypeError, "Third argument (buttonReleasCallback) is not a function!");
  } else {
    auto& smDaemon = spacemouse::SpaceMouseDaemon::instance();
    smDaemon.setMoveCallback([pyMoveCallback](spacemouse::SpaceMouseMoveEvent e) -> void {
      PyGILState_STATE gil = PyGILState_Ensure();

      PyObject* arglist =
          Py_BuildValue("(iiidddd)", e.tx, e.ty, e.tz, e.angle, e.axisX, e.axisY, e.axisZ);
      PyObject* result = PyEval_CallObject(pyMoveCallback, arglist);

      Py_DECREF(arglist);
      Py_XDECREF(result);
      PyGILState_Release(gil);
    });
    smDaemon.setButtonPressCallback(
        [pyButtonPressCallback](spacemouse::SpaceMouseButtonEvent e) -> void {
          PyGILState_STATE gil = PyGILState_Ensure();

          PyObject* arglist = Py_BuildValue("(i)", (int)e.button);
          PyObject* result = PyEval_CallObject(pyButtonPressCallback, arglist);

          Py_DECREF(arglist);
          Py_XDECREF(result);
          PyGILState_Release(gil);
        });
    smDaemon.setButtonReleaseCallback(
        [pyButtonReleaseCallback](spacemouse::SpaceMouseButtonEvent e) -> void {
          PyGILState_STATE gil = PyGILState_Ensure();

          PyObject* arglist = Py_BuildValue("(i)", (int)e.button);
          PyObject* result = PyEval_CallObject(pyButtonReleaseCallback, arglist);

          Py_DECREF(arglist);
          Py_XDECREF(result);
          PyGILState_Release(gil);
        });
  }

  Py_INCREF(Py_None);
  return Py_None;
}

static const char* docString =
  "Starts the space mouse daemon in the background\n"
  "\n"
  "Parameters:\n"
  "moveCallback (function(int, int, int, float, float, float, float) -> None): "
    "The callback that is executed when a move event occurs\n"
  "buttonPressCallback (function(int) -> None):"
    "The callback that is executed when a button is pressed\n"
  "buttonReleaseCallback (function(int) -> None):"
    "The callback that is executed when a button is released\n"
  "\n"
  "Returns:\n"
  "None";

static PyMethodDef SpaceMouseMethods[] = {
    {"start_spacemouse_daemon", start_spacemouse_daemon, METH_VARARGS, docString},
    {nullptr, nullptr, 0, nullptr}
};

static struct PyModuleDef PySpaceMouseModule = {PyModuleDef_HEAD_INIT, "pyspacemouse", nullptr, -1,
                                                SpaceMouseMethods};

PyMODINIT_FUNC PyInit_pyspacemouse(void) { return PyModule_Create(&PySpaceMouseModule); }
}
