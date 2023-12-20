# Copyright (c) 2020 FlyingSamson.
# SpaceMouseTool is released under the terms of the AGPLv3 or higher.

from UM.Application import Application
from UM.Logger import Logger
from UM.Math.Matrix import Matrix
from UM.Math.Vector import Vector
from UM.Qt.Bindings.MainWindow import MainWindow
from UM.Qt.QtApplication import QtApplication
from UM.Scene.Selection import Selection
from UM.Extension import Extension
from UM.i18n import i18nCatalog

from PyQt6 import QtCore
from PyQt6.QtCore import QAbstractNativeEventFilter, Qt
from PyQt6.QtGui import QGuiApplication

from enum import IntEnum
import math
import numpy as np
from typing import cast

import platform
if platform.system() == "Darwin":
    if platform.machine() == "arm64":
        from .lib.darwin_arm64.pyspacemouse import set_logger, start_spacemouse_daemon, release_spacemouse_daemon
    else:
        from .lib.darwin_x86_64.pyspacemouse import set_logger, start_spacemouse_daemon, release_spacemouse_daemon
elif platform.system() == "Linux":
    from .lib.linux.pyspacemouse import set_logger, start_spacemouse_daemon, release_spacemouse_daemon
elif platform.system() == "Windows":
    from .lib.windows.pyspacemouse import set_logger, start_spacemouse_daemon, release_spacemouse_daemon
    from .lib.windows.pyspacemouse import set_window_handle, process_win_event


catalog = i18nCatalog("cura")


def homogenize(vec4: np.array) -> np.array:  # vec3
    vec4 /= vec4[3]
    return vec4[0:3]


def debugLog(s: str) -> None:
    Logger.log("d", s)


set_logger(debugLog)


class SpaceMouseTool(Extension):
    _scene = None
    _cameraTool = None
    _rotScaleFree = 0.0001
    _rotScaleConstrained = 0.00004
    _transScale = 0.015
    _zoomScale = 0.00001
    _zoomMin = -0.495  # same as used in CameraTool
    _zoomMax = 1       # same as used in CameraTool
    _fitBorderPercentage = 0.1
    _rotationLocked = False
    _constrainedOrbit = False

    class SpaceMouseButton(IntEnum):
        # buttons on the 3DConnexion Spacemouse Wireles Pro:
        # view buttons:
        SPMB_TOP = 0         # Top view button
        SPMB_RIGHT = 1       # Right view button
        SPMB_FRONT = 2       # Front view button
        SPMB_ROLL_CW = 3     # Roll the view clock-wise in the plane orthogonal
                             # to the direction of view
        SPMB_LOCK_ROT = 4    # Lock rotation
        # configurable buttons 1, 2, 3, and 4
        SPMB_1 = 5           # Configurable button 1 * /
        SPMB_2 = 6           # Configurable button 2 * /
        SPMB_3 = 7           # Configurable button 3 * /
        SPMB_4 = 8           # Configurable button 4 * /
        # modifier keys
        SPMB_ESC = 9         # Escape key * /
        SPMB_SHIFT = 10      # Shift key * /
        SPMB_CTRL = 11       # Control key * /
        SPMB_ALT = 12        # Alternate key * /
        # menu button
        SPMB_MENU = 12       # Menu button * /
        # fit to screen button
        SPMB_FIT = 13        # Fit shown objects to screen * /

        # if you own another spacemouse feel free to add further buttons

        # undefined button
        SPMB_UNDEFINED = 14  # Undefined button * /

    class SpaceMouseModifierKey(IntEnum):
        SPMM_SHIFT = 1
        SPMM_CTRL = 2
        SPMM_ALT = 4

    def __init__(self):
        super().__init__()
        SpaceMouseTool._scene = Application.getInstance().getController().getScene()
        SpaceMouseTool._cameraTool = Application.getInstance().getController().getTool("CameraTool")

        # get the preference for free/constrained orbit from cura preferences pane
        self.setMenuName(catalog.i18nc("@item:inmenu", "Space Mouse Tool"))
        self.addMenuItem(catalog.i18nc("@item:inmenu", "Toggle free/constrained orbit"),
                         SpaceMouseTool._toggleOrbit)

        # init space mouse when engine was created as we require the hwnd of the
        # MainWindow on Windows
        Application.getInstance().engineCreatedSignal.connect(SpaceMouseTool._onEngineCreated)
        Application.getInstance().applicationShuttingDown.connect(release_spacemouse_daemon)

    @staticmethod
    def _toggleOrbit() -> None:
        SpaceMouseTool._constrainedOrbit = not SpaceMouseTool._constrainedOrbit

    @staticmethod
    def _translateCamera(tx: int, ty: int, tz: int) -> None:
        camera = SpaceMouseTool._scene.getActiveCamera()
        if not camera or not camera.isEnabled():
            Logger.log("d", "No camera available")
            return
        moveVec = Vector(0, 0, 0)
        # Translate camera by tx and tz. We have to reverse x and use z as y
        # in order to achieve the default behavior of the space mouse (c.f.
        # cube example of 3DX). If you prefer it another way change the setting
        # of the 3D mouse
        moveVec = moveVec.set(x=-tx, y=tz)

        # Zoom camera using negated ty. Again you should/can change this
        # behavior for space mouse
        if camera.isPerspective():
            moveVec = moveVec.set(z=-ty)
            camera.translate(SpaceMouseTool._transScale * moveVec)
        else:  # orthographic
            camera.translate(SpaceMouseTool._transScale * moveVec)
            zoomFactor = camera.getZoomFactor() - SpaceMouseTool._zoomScale * ty
            # clamp to [zoomMin, zoomMax]
            zoomFactor = min(SpaceMouseTool._zoomMax, max(SpaceMouseTool._zoomMin, zoomFactor))
            camera.setZoomFactor(zoomFactor)

    @staticmethod
    def _rotateCameraFree(angle: float, axisX: float, axisY: float, axisZ: float) -> None:
        if SpaceMouseTool._rotationLocked:
            return
        camera = SpaceMouseTool._scene.getActiveCamera()
        if not camera or not camera.isEnabled():
            return

        # compute axis in view space:
        # space mouse system: x: right, y: front, z: down
        # camera system:     x: right, y: up,    z: front
        # i.e. rotate the vector about x by 90 degrees in mathematical positive sense
        axisInViewSpace = np.array([-axisX, axisZ, -axisY, 1])

        # get inverse view matrix
        invViewMatrix = camera.getWorldTransformation().getData()

        # compute rotation axis in world space
        axisInWorldSpace = homogenize(np.dot(invViewMatrix, axisInViewSpace))
        originInWorldSpace = homogenize(np.dot(invViewMatrix, np.array([0, 0, 0, 1])))

        axisInWorldSpace = axisInWorldSpace - originInWorldSpace
        axisInWorldSpace = Vector(data=axisInWorldSpace)

        # rotate camera around that axis by angle
        rotOrigin = SpaceMouseTool._cameraTool.getOrigin()

        # rotation matrix around the axis
        rotMat = Matrix()
        rotMat.setByRotationAxis(angle, axisInWorldSpace, rotOrigin.getData())
        camera.setTransformation(camera.getLocalTransformation().preMultiply(rotMat))

    @staticmethod
    def _rotateCameraConstrained(angleAzim: float, angleIncl: float) -> None:
        if SpaceMouseTool._rotationLocked:
            return
        camera = SpaceMouseTool._scene.getActiveCamera()
        if not camera or not camera.isEnabled():
            return

        # we need to compute the translation and lookAt in one step as we otherwise get artifacts
        # from first setting the camera to a new position and then telling it to look at the
        # rotation center

        up = Vector.Unit_Y
        target = SpaceMouseTool._cameraTool.getOrigin()

        oldEye = camera.getWorldPosition()
        camToTarget = (target - oldEye).normalized()

        # compute angle between up axis and current camera ray
        try:
            angleToY = math.acos(up.dot(camToTarget))
        except ValueError:
            return

        # compute new position of camera
        rotMat = Matrix()
        rotMat.rotateByAxis(angleAzim, up, target.getData())
        # prevent camera from rotating to close to the poles
        if (angleToY > 0.1 or angleIncl > 0) and (angleToY < math.pi - 0.1 or angleIncl < 0):
            rotMat.rotateByAxis(angleIncl, up.cross(camToTarget).normalized(), target.getData())
        newEye = oldEye.preMultiply(rotMat)

        # look at (from UM.Scene.SceneNode)
        f = (target - newEye).normalized()
        s = f.cross(up).normalized()
        u = s.cross(f).normalized()

        # construct new matrix for camera including the new position and orientation from looking at
        # the rotation center
        newTrafo = Matrix([
            [s.x,  u.x,  -f.x, newEye.x],
            [s.y,  u.y,  -f.y, newEye.y],
            [s.z,  u.z,  -f.z, newEye.z],
            [0.0,  0.0,  0.0,  1.0]
        ])

        camera.setTransformation(newTrafo)

    @staticmethod
    def _fitSelection() -> None:
        camera = SpaceMouseTool._scene.getActiveCamera()
        if not camera or not camera.isEnabled():
            Logger.log("d", "No camera available")
            return

        if not Selection.hasSelection():
            Logger.log("d", "Nothing selected to fit")
            return

        aabb = Selection.getBoundingBox()
        minAabb = aabb.minimum  # type: Vector
        maxAabb = aabb.maximum  # type: Vector
        centerAabb = aabb.center  # type: Vector

        # get center in viewspace:
        viewMatrix = camera.getInverseWorldTransformation()
        centerInViewSpace = homogenize(np.dot(viewMatrix.getData(),
                                              np.append(centerAabb.getData(), 1)))
        # translate camera in xy-plane such that it is looking on the center
        centerInViewSpace[2] = 0
        centerInViewSpace = Vector(data=centerInViewSpace)
        camera.translate(centerInViewSpace)

        if camera.isPerspective():
            # compute the smaller of the two field of views
            aspect = camera.getViewportWidth() / camera.getViewportHeight()
            halfFovY = math.radians(30) / 2.  # from Camera.py _updatePerspectiveMarix
            halfFovX = math.atan(aspect * math.tan(halfFovY))
            halfFov = min(halfFovX, halfFovY)

            # radius of the bounding sphere containing the bounding box
            boundingSphereRadius = (centerAabb - minAabb).length()

            # compute distance of camera to center
            # (sin, as we use a point tangential to the bounding sphere)
            distCamCenter = boundingSphereRadius / math.sin(halfFov)

            # unit vector pointing from center to camera
            centerToCam = (camera.getWorldPosition() - centerAabb).normalized()

            # compute new position for camera
            newCamPos = centerAabb + centerToCam * distCamCenter

            camera.setPosition(newCamPos)
        else:
            minX = None
            maxX = None
            minY = None
            maxY = None
            for x in [minAabb.x, maxAabb.x]:
                for y in [minAabb.y, maxAabb.y]:
                    for z in [minAabb.z, maxAabb.z]:
                        # get corner of aabb in view space
                        cornerInViewSpace = homogenize(np.dot(viewMatrix.getData(),
                                                       np.array([x, y, z, 1])))
                        cornerInViewSpace = Vector(data=cornerInViewSpace)
                        minX = cornerInViewSpace.x if minX is None\
                            else min(minX, cornerInViewSpace.x)
                        maxX = cornerInViewSpace.x if maxX is None\
                            else max(maxX, cornerInViewSpace.x)
                        minY = cornerInViewSpace.y if minY is None\
                            else min(minY, cornerInViewSpace.y)
                        maxY = cornerInViewSpace.y if maxY is None\
                            else max(maxY, cornerInViewSpace.y)
            widthWithBorder = (1 + 2 * SpaceMouseTool._fitBorderPercentage) * (maxX-minX)
            heightWithBorder = (1 + 2 * SpaceMouseTool._fitBorderPercentage) * (maxY-minY)

            # set zoom factor in such a way, that the width or height with border fills the width
            # or height of the viewport, respectively
            zoomFactorHor = widthWithBorder / 2. / camera.getViewportWidth() - 0.5
            zoomFactorVer = heightWithBorder / 2. / camera.getViewportHeight() - 0.5

            # use max as zoom factor get more negative if we zoom into the scene
            zoomFactor = max(zoomFactorHor, zoomFactorVer)
            camera.setZoomFactor(zoomFactor)

    @staticmethod
    def _setCameraRotation(view: str) -> None:
        controller = Application.getInstance().getController()

        if view == "TOP":
            controller.setCameraRotation("y", 90)
        elif view == "RIGHT":
            controller.setCameraRotation("x", -90)
        elif view == "FRONT":
            controller.setCameraRotation("home", 0)
        elif view == "BOTTOM":
            # this work around isn't pretty but setCameraRotation's implementation is quite strange
            camera = SpaceMouseTool._scene.getActiveCamera()
            if not camera:
                return
            camera.setZoomFactor(camera.getDefaultZoomFactor())
            camera.setPosition(Vector(0, -800, 0))
            SpaceMouseTool._cameraTool.setOrigin(Vector(0, 100, .1))
            camera.lookAt(Vector(0, 100, .1), Vector(0, 1, 0))
        elif view == "LEFT":
            controller.setCameraRotation("x", 90)
        elif view == "REAR":
            # this work around isn't pretty but setCameraRotation's implementation is quite strange
            camera = SpaceMouseTool._scene.getActiveCamera()
            if not camera:
                return
            camera.setZoomFactor(camera.getDefaultZoomFactor())
            SpaceMouseTool._cameraTool.setOrigin(Vector(0, 100, 0))
            camera.setPosition(Vector(0, 100, -700))
            SpaceMouseTool._cameraTool.rotateCamera(0, 0)
        else:
            pass

    @staticmethod
    def spacemouse_move_callback(
            tx: int, ty: int, tz: int,
            angle: float, axisX: float, axisY: float, axisZ: float) -> None:
        if SpaceMouseTool._constrainedOrbit:
            # translate and zoom:
            SpaceMouseTool._translateCamera(0, ty, 0)

            # rotate
            angleAzim = angle * axisZ * SpaceMouseTool._rotScaleConstrained
            angleIncl = angle * axisX * SpaceMouseTool._rotScaleConstrained
            SpaceMouseTool._rotateCameraConstrained(angleAzim, angleIncl)
        else:
            # translate and zoom:
            SpaceMouseTool._translateCamera(tx, ty, tz)

            # rotate
            SpaceMouseTool._rotateCameraFree(angle * SpaceMouseTool._rotScaleFree,
                                             axisX, axisY, axisZ)

    @staticmethod
    def spacemouse_button_press_callback(button: int, modifiers: int):
        keyboardModifiers = QGuiApplication.queryKeyboardModifiers()
        if (keyboardModifiers & Qt.KeyboardModifier.ShiftModifier) == Qt.KeyboardModifier.ShiftModifier:
            modifiers |= SpaceMouseTool.SpaceMouseModifierKey.SPMM_SHIFT
        if (keyboardModifiers & Qt.KeyboardModifier.ControlModifier) == Qt.KeyboardModifier.ControlModifier:
            modifiers |= SpaceMouseTool.SpaceMouseModifierKey.SPMM_CTRL
        if (keyboardModifiers & Qt.KeyboardModifier.AltModifier) == Qt.KeyboardModifier.ControlModifier:
            modifiers |= SpaceMouseTool.SpaceMouseModifierKey.SPMM_ALT

        if button == SpaceMouseTool.SpaceMouseButton.SPMB_TOP:
            if (modifiers & SpaceMouseTool.SpaceMouseModifierKey.SPMM_SHIFT) != 0:
                SpaceMouseTool._setCameraRotation("BOTTOM")
            else:
                SpaceMouseTool._setCameraRotation("TOP")
        elif button == SpaceMouseTool.SpaceMouseButton.SPMB_RIGHT:
            if (modifiers & SpaceMouseTool.SpaceMouseModifierKey.SPMM_SHIFT) != 0:
                SpaceMouseTool._setCameraRotation("LEFT")
            else:
                SpaceMouseTool._setCameraRotation("RIGHT")
        elif button == SpaceMouseTool.SpaceMouseButton.SPMB_FRONT:
            if (modifiers & SpaceMouseTool.SpaceMouseModifierKey.SPMM_SHIFT) != 0:
                SpaceMouseTool._setCameraRotation("REAR")
            else:
                SpaceMouseTool._setCameraRotation("FRONT")
        elif button == SpaceMouseTool.SpaceMouseButton.SPMB_ROLL_CW:
            if (modifiers & SpaceMouseTool.SpaceMouseModifierKey.SPMM_SHIFT) != 0:
                SpaceMouseTool._rotateCamera(math.pi/2, 0, 1, 0)
            else:
                SpaceMouseTool._rotateCamera(-math.pi/2, 0, 1, 0)
        elif button == SpaceMouseTool.SpaceMouseButton.SPMB_FIT:
            SpaceMouseTool._fitSelection()
        elif button == SpaceMouseTool.SpaceMouseButton.SPMB_LOCK_ROT:
            if platform.system() == "Linux":
                SpaceMouseTool._rotationLocked = not SpaceMouseTool._rotationLocked
            # on Windows and OSX this is taken care of by the driver

    @staticmethod
    def spacemouse_button_release_callback(button: int, modifiers: int):
        pass

    if platform.system() == "Windows":
        _filterObj = None

    @staticmethod
    def _onEngineCreated() -> None:
        start_spacemouse_daemon(
            SpaceMouseTool.spacemouse_move_callback,
            SpaceMouseTool.spacemouse_button_press_callback,
            SpaceMouseTool.spacemouse_button_release_callback)

        if platform.system() == "Windows":
            # the windows api requires the hwnd (window id)
            mainWindow = cast(MainWindow, QtApplication.getInstance().getMainWindow())
            windowId = mainWindow.winId()
            set_window_handle(windowId.ascapsule())

            # windows api cannot poll for events in parallel to the qt event loop
            # so we have pass the events to the daemon so that it can check for
            # space mouse events
            SpaceMouseTool._filterObj = WinEventFilterObj()
            QtApplication.getInstance().installNativeEventFilter(SpaceMouseTool._filterObj)

        Logger.log("d", "Initialized SpaceMouseTool")


class WinEventFilterObj(QAbstractNativeEventFilter):
    def nativeEventFilter(self, eventType, message):
        process_win_event(message.ascapsule())
        return False, 0
