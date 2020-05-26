from UM.Application import Application
from UM.Tool import Tool
from UM.Logger import Logger
from UM.Math.Matrix import Matrix
from UM.Math.Quaternion import Quaternion
from UM.Math.Vector import Vector

from PyQt5 import QtCore
from PyQt5.QtGui import QGuiApplication

from enum import IntEnum
import math
import numpy as np
import time


import platform
if platform.system() == "Darwin":
    from .lib.darwin.pyspacemouse import start_spacemouse_daemon
elif platform.system() == "Linux":
    from .lib.x86_64.pyspacemouse import start_spacemouse_daemon


def homogenize(vec4: np.array) -> np.array:  # vec3
    vec4 /= vec4[3]
    return vec4[0:3]

class SpaceMouseTool(Tool):
    _scene = None
    _cameraTool = None
    _rotScale = 0.0001
    _transScale = 0.05
    _zoomScale = 0.00005
    _zoomMin = -0.495  # same as used in CameraTool
    _zoomMax = 1       # same as used in CameraTool

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
        start_spacemouse_daemon(
            SpaceMouseTool.spacemouse_move_callback,
            SpaceMouseTool.spacemouse_button_press_callback,
            SpaceMouseTool.spacemouse_button_release_callback)
        Logger.log("d", "Initialized SpaceMouseTool")

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
    def _rotateCamera(angle: float, axisX: float, axisY: float, axisZ: float) -> None:
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
        origin = camera.getWorldPosition()
        rotOrigin = SpaceMouseTool._cameraTool.getOrigin()
        camToOrigin = origin - rotOrigin

        Logger.log("d", invViewMatrix)

        rotMat = Matrix()
        rotMat = Quaternion.fromAngleAxis(angle, axisInWorldSpace).toMatrix()
        #rotMat.setByRotationAxis(angle * 0.0001, Vector(data=axisInWorldSpace))

        # translation matrix to shift camera to origin i.e. 0,0,0 in world space
        transToOrigMat = Matrix()
        transToOrigMat.setByTranslation(-origin)

        # compute new position for camera
        newPos = camToOrigin.preMultiply(rotMat) + rotOrigin

        # translation matrix to shift camera to new position
        transToNewPosMat = Matrix()
        transToNewPosMat.setByTranslation(newPos)

        trafo = transToOrigMat               # shift to origin
        trafo.preMultiply(rotMat)            # rotate camera in place
        trafo.preMultiply(transToNewPosMat)  # shift to new position

        camera.setTransformation(camera.getLocalTransformation().preMultiply(trafo))

        #euler_angles = camera.getOrientation().toMatrix().getEuler()
        #Logger.log("d", str(euler_angles))

        # rotation matrix around the axis
        #rotMat = Matrix()
        #rotMat.setByRotationAxis(angle, axisInWorldSpace, rotOrigin.getData())

        # apply transformation
        #camera.setTransformation(camera.getLocalTransformation().preMultiply(rotMat))

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
        # translate and zoom:
        #SpaceMouseTool._translateCamera(tx, ty, tz)
        SpaceMouseTool._rotateCamera(angle * SpaceMouseTool._rotScale, axisX, axisY, axisZ)

    @staticmethod
    def spacemouse_button_press_callback(button: int, modifiers: int):
        keyboardModifiers = QGuiApplication.queryKeyboardModifiers()
        Logger.log("d", repr(keyboardModifiers))
        if (keyboardModifiers & QtCore.Qt.ShiftModifier) == QtCore.Qt.ShiftModifier:
            modifiers |= SpaceMouseTool.SpaceMouseModifierKey.SPMM_SHIFT
        if (keyboardModifiers & QtCore.Qt.ControlModifier) == QtCore.Qt.ControlModifier:
            modifiers |= SpaceMouseTool.SpaceMouseModifierKey.SPMM_CTRL
        if (keyboardModifiers & QtCore.Qt.AltModifier) == QtCore.Qt.ControlModifier:
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
        elif(button == SpaceMouseTool.SpaceMouseButton.SPMB_FRONT):
            if (modifiers & SpaceMouseTool.SpaceMouseModifierKey.SPMM_SHIFT) != 0:
                SpaceMouseTool._setCameraRotation("REAR")
            else:
                SpaceMouseTool._setCameraRotation("FRONT")
        elif(button == SpaceMouseTool.SpaceMouseButton.SPMB_ROLL_CW):
            if (modifiers & SpaceMouseTool.SpaceMouseModifierKey.SPMM_SHIFT) != 0:
                SpaceMouseTool._rotateCamera(math.pi/2, 0, 1, 0)
            else:
                SpaceMouseTool._rotateCamera(-math.pi/2, 0, 1, 0)

        Logger.log("d", "Press " + str(button) + " " + str(modifiers))

    @staticmethod
    def spacemouse_button_release_callback(button: int, modifiers: int):
        Logger.log("d", "Release " + str(button) + " " + str(modifiers))


def main():
    spacemousetool = SpaceMouseTool()

    time.sleep(10)


if __name__ == "__main__":
    main()
