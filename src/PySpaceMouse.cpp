// Copyright (c) 2020 FlyingSamson.
// SpaceMouseTool is released under the terms of the AGPLv3 or higher.

#include <Python.h>

#include "SpaceMouse.hpp"

extern "C" {

static PyObject* set_logger(PyObject* /*self*/, PyObject* args) {
  PyObject* pyLogFun;
  if (!PyArg_ParseTuple(args, "O", &pyLogFun))
    return nullptr;
  if (!PyCallable_Check(pyLogFun)) {
    PyErr_SetString(PyExc_TypeError, "First argument (logFun) is not a function!");
  } else {
    spacemouse::logFun = std::function<void(const char*)>([pyLogFun](const char* string) {
      if (!Py_IsInitialized())
        return;
      PyGILState_STATE gil = PyGILState_Ensure();

      PyObject* arglist = Py_BuildValue("(s)", string);
      PyObject* result = PyObject_CallObject(pyLogFun, arglist);

      Py_DECREF(arglist);
      Py_XDECREF(result);
      PyGILState_Release(gil);
    });
  }

  Py_INCREF(Py_None);
  return Py_None;
}

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
      if (!Py_IsInitialized())
        return;
      PyGILState_STATE gil = PyGILState_Ensure();

      PyObject* arglist =
          Py_BuildValue("(iiidddd)", e.tx, e.ty, e.tz, e.angle, e.axisX, e.axisY, e.axisZ);
      PyObject* result = PyObject_CallObject(pyMoveCallback, arglist);

      Py_DECREF(arglist);
      Py_XDECREF(result);
      PyGILState_Release(gil);
    });
    smDaemon.setButtonPressCallback(
        [pyButtonPressCallback](spacemouse::SpaceMouseButtonEvent e) -> void {
          if (!Py_IsInitialized())
            return;
          PyGILState_STATE gil = PyGILState_Ensure();

          PyObject* arglist = Py_BuildValue("(ii)", (int)e.button, (int)e.modifierKeys.modifiers());
          PyObject* result = PyObject_CallObject(pyButtonPressCallback, arglist);

          Py_DECREF(arglist);
          Py_XDECREF(result);
          PyGILState_Release(gil);
        });
    smDaemon.setButtonReleaseCallback(
        [pyButtonReleaseCallback](spacemouse::SpaceMouseButtonEvent e) -> void {
          if (!Py_IsInitialized())
            return;
          PyGILState_STATE gil = PyGILState_Ensure();

          PyObject* arglist = Py_BuildValue("(ii)", (int)e.button, (int)e.modifierKeys.modifiers());
          PyObject* result = PyObject_CallObject(pyButtonReleaseCallback, arglist);

          Py_DECREF(arglist);
          Py_XDECREF(result);
          PyGILState_Release(gil);
        });
  }

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* release_spacemouse_daemon(PyObject* /*self*/, PyObject* args) {
  #ifndef NDEBUG
  spacemouse::logFun("Releasing daemon");
  #endif
  auto& smDaemon = spacemouse::SpaceMouseDaemon::instance();
  smDaemon.setMoveCallback([](spacemouse::SpaceMouseMoveEvent e) -> void {});
  smDaemon.setButtonPressCallback([](spacemouse::SpaceMouseButtonEvent e) -> void {});
  smDaemon.setButtonReleaseCallback([](spacemouse::SpaceMouseButtonEvent e) -> void {});
  spacemouse::logFun = std::function<void(const char*)>([](const char* string) {});

  Py_INCREF(Py_None);
  return Py_None;
}

#ifdef WITH_LIB3DX_WIN
static PyObject* set_window_handle(PyObject* /*self*/, PyObject* args) {
  HWND winId;

  PyObject* capsule;
  if (!PyArg_ParseTuple(args, "O", &capsule)) {
    PyErr_SetString(PyExc_TypeError, "Unable to extract capsule from tuple");
  } else if (!PyCapsule_IsValid(capsule, nullptr)) {
    PyErr_SetString(PyExc_TypeError, "Capsule is not valid");
  } else {
    winId = (HWND)PyCapsule_GetPointer(capsule, NULL);
  }

  auto& smDaemon = spacemouse::SpaceMouseDaemon::instance();
  smDaemon.setWindowHandle(winId);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* process_win_event(PyObject* /*self*/, PyObject* args) {
  MSG message;

  PyObject* capsule;
  if (!PyArg_ParseTuple(args, "O", &capsule)) {
    PyErr_SetString(PyExc_TypeError, "Unable to extract capsule from tuple");
  } else if (!PyCapsule_IsValid(capsule, nullptr)) {
    PyErr_SetString(PyExc_TypeError, "Capsule is not valid");
  } else {
    message = *(MSG*)PyCapsule_GetPointer(capsule, NULL);
  }

  auto& smDaemon = spacemouse::SpaceMouseDaemon::instance();
  auto handled = smDaemon.processWinEvent(message);

  return PyBool_FromLong(handled);
}
#endif

static const char* docSetLogger =
  "Sets the logger function that will be used for printing logging information regarding the"
  " spacemouse\n"
  "\n"
  "Parameters:\n"
  "logFun (function(str) -> None): Callback function that is called to log space mouse activity\n"
  "\n"
  "Returns:\n"
  "None";
static const char* docStart =
  "Starts the space mouse daemon in the background\n"
  "\n"
  "Parameters:\n"
  "moveCallback (function(int, int, int, float, float, float, float) -> None): "
    "The callback that is executed when a move event occurs\n"
  "buttonPressCallback (function(int, int) -> None):"
    "The callback that is executed when a button is pressed\n"
  "buttonReleaseCallback (function(int, int) -> None):"
    "The callback that is executed when a button is released\n"
  "\n"
  "Returns:\n"
  "None";
static const char* docRelease =
  "Releases the space mouse daemon by resetting the callback functions and the logger function"
  " to no-ops\n"
  "\n"
  "Returns:\n"
  "None";
#ifdef WITH_LIB3DX_WIN
static const char* docSetHwnd =
  "Sets the hwnd window handle\n"
  "\n"
  "Parameters:\n"
  "winID (Capsule): A python capsule containing the hwnd pointer\n"
  "\n"
  "Returns:\n"
  "None";
static const char* docProcessWinEvent =
  "Processes a Windows MSG message\n"
  "Parameters:\n"
  "msg (Capsule): A python capsule containing a pointer to the MSG message\n"
  "\n"
  "Returns:\n"
  "Bool: Whether or not the event has be handled";
#endif  // WITH_LIB3DX_WIN

static PyMethodDef SpaceMouseMethods[] = {
    {"set_logger", set_logger, METH_VARARGS, docSetLogger},
    {"start_spacemouse_daemon", start_spacemouse_daemon, METH_VARARGS, docStart},
    {"release_spacemouse_daemon", release_spacemouse_daemon, METH_NOARGS, docRelease},
#ifdef WITH_LIB3DX_WIN
    {"set_window_handle", set_window_handle, METH_VARARGS, docSetHwnd},
    {"process_win_event", process_win_event, METH_VARARGS, docProcessWinEvent},
#endif  // WITH_LIB3DX_WIN
    {nullptr, nullptr, 0, nullptr}
};

static struct PyModuleDef PySpaceMouseModule = {PyModuleDef_HEAD_INIT, "pyspacemouse", nullptr, -1,
                                                SpaceMouseMethods};

PyMODINIT_FUNC PyInit_pyspacemouse(void) { return PyModule_Create(&PySpaceMouseModule); }

} // extern "C"
