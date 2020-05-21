#include <Python.h>

#include "SpaceMouse.hpp"

extern "C" {

static PyObject* get_spacemouse_daemon_instance(PyObject* /*self*/, PyObject* args) {
  PyObject *pyMoveCallback, *pyButtonPressCallback, *pyButtonReleaseCallback;

  if (!PyArg_ParseTuple(args, "OOO", &pyMoveCallback, &pyButtonPressCallback,
                        &pyButtonReleaseCallback))
    return nullptr;

  if (!PyCallable_Check(pyMoveCallback)) {
    PyErr_SetString(PyExc_TypeError, "First argument (moveCallback) is not a function!");
  } else if (!PyCallable_Check(pyButtonPressCallback)) {
    PyErr_SetString(PyExc_TypeError, "Second argument (buttonPressCallback) is not a function!");
  } else if (!PyCallable_Check(pyButtonReleaseCallback)) {
    PyErr_SetString(PyExc_TypeError, "Second argument (buttonReleasCallback) is not a function!");
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
        [pyButtonPressCallback](spacemouse::SpaceMouseButtonPressEvent e) -> void {
          PyGILState_STATE gil = PyGILState_Ensure();

          PyObject* arglist = Py_BuildValue("(i)", (int)e.button);
          PyObject* result = PyEval_CallObject(pyButtonPressCallback, arglist);

          Py_DECREF(arglist);
          Py_XDECREF(result);
          PyGILState_Release(gil);
        });
    smDaemon.setButtonReleaseCallback(
        [pyButtonReleaseCallback](spacemouse::SpaceMouseButtonReleaseEvent e) -> void {
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

static PyMethodDef SpaceMouseMethods[] = {
    {"get_spacemouse_daemon_instance", get_spacemouse_daemon_instance, METH_VARARGS, "Blabla"}};

static struct PyModuleDef PySpaceMouseModule = {PyModuleDef_HEAD_INIT, "pyspacemouse", nullptr, -1,
                                                SpaceMouseMethods};

PyMODINIT_FUNC PyInit_pyspacemouse(void) { return PyModule_Create(&PySpaceMouseModule); }
}
