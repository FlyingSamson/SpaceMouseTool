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
      PyGILState_STATE gil = PyGILState_Ensure();

      PyObject* arglist = Py_BuildValue("(s)", string);
      PyObject* result = PyEval_CallObject(pyLogFun, arglist);

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

          PyObject* arglist = Py_BuildValue("(ii)", (int)e.button, (int)e.modifierKeys.modifiers());
          PyObject* result = PyEval_CallObject(pyButtonPressCallback, arglist);

          Py_DECREF(arglist);
          Py_XDECREF(result);
          PyGILState_Release(gil);
        });
    smDaemon.setButtonReleaseCallback(
        [pyButtonReleaseCallback](spacemouse::SpaceMouseButtonEvent e) -> void {
          PyGILState_STATE gil = PyGILState_Ensure();

          PyObject* arglist = Py_BuildValue("(ii)", (int)e.button, (int)e.modifierKeys.modifiers());
          PyObject* result = PyEval_CallObject(pyButtonReleaseCallback, arglist);

          Py_DECREF(arglist);
          Py_XDECREF(result);
          PyGILState_Release(gil);
        });
  }

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

static const char* docString =
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

static PyMethodDef SpaceMouseMethods[] = {
    {"set_logger", set_logger, METH_VARARGS, "bla"},
    {"start_spacemouse_daemon", start_spacemouse_daemon, METH_VARARGS, docString},
#ifdef WITH_LIB3DX_WIN
    {"set_window_handle", set_window_handle, METH_VARARGS, "bla"},
    {"process_win_event", process_win_event, METH_VARARGS, "bla"},
#endif
    {nullptr, nullptr, 0, nullptr}
};

static struct PyModuleDef PySpaceMouseModule = {PyModuleDef_HEAD_INIT, "pyspacemouse", nullptr, -1,
                                                SpaceMouseMethods};

PyMODINIT_FUNC PyInit_pyspacemouse(void) { return PyModule_Create(&PySpaceMouseModule); }

} // extern "C"
