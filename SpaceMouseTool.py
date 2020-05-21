import math
import numpy as np

from UM.Application import Application
from UM.Tool import Tool
from UM.Logger import Logger
from UM.Math.Matrix import Matrix
from UM.Math.Vector import Vector
from UM.Math.Quaternion import Quaternion
from UM.Scene.SceneNode import SceneNode as SceneNode

from .lib.pyspacemouse import get_spacemouse_daemon_instance
import time


def homogenize(vec4: np.array) -> np.array:  # vec3
    vec4 /= vec4[3]
    return vec4[0:3]


# class SpaceMouseTool():
class SpaceMouseTool(Tool):
    _scene = None
    _cameraTool = None
    _rotCenter = Vector(0, 0, 0)
    _transScale = 0.05
    _zoomScale = 0.00005
    _zoomMin = -0.495  # same as used in CameraTool
    _zoomMax = 1       # same as used in CameraTool

    def __init__(self):
        super().__init__()
        SpaceMouseTool._scene = \
            Application.getInstance().getController().getScene()
        SpaceMouseTool._cameraTool = \
            Application.getInstance().getController().getTool("CameraTool")
        get_spacemouse_daemon_instance(
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
        # Translate camera by tx and tz. We have to reverse x and unse z as y
        # in order to achieve the default behaviour of the spacemouse (c.f.
        # cube example of 3DX). If you prefer it another way change the setting
        # of the 3D mouse
        moveVec = moveVec.set(x=-tx, y=tz)

        # Zoom camera using negated ty. Again you should/can change this
        # beahviour for space mouse
        if camera.isPerspective():
            moveVec = moveVec.set(z=-ty)
            camera.translate(SpaceMouseTool._transScale * moveVec)
        else:  # orthografic
            # camera_position = camera.getWorldPosition()
            camera.translate(SpaceMouseTool._transScale * moveVec)
            # SpaceMouseTool._origin += \
            #   camera.getWorldPosition() - camera_position
            zoomFactor = \
                camera.getZoomFactor() - SpaceMouseTool._zoomScale * ty
            # clamp to [zoomMin, zoomMax]
            zoomFactor = min(SpaceMouseTool._zoomMax,
                             max(SpaceMouseTool._zoomMin, zoomFactor))
            # Logger.log("d", "New zoomFactor" + str(zoomFactor))
            camera.setZoomFactor(zoomFactor)

    @staticmethod
    def _rotateCamera(
            angle: float, axisX: float, axisY: float, axisZ: float) -> None:
        camera = SpaceMouseTool._scene.getActiveCamera()
        if not camera or not camera.isEnabled():
            return

        # compute axis in view space:
        # spacemouse system: x: right, y: front, z: down
        # camera system:     x: right, y: up,    z: front
        # i.e. rotate the vector about x by 90 degrees in mathmatical positive sense
        axisInViewSpace = np.array([-axisX, axisZ, -axisY, 1])

        # get inverse view matrix
        invViewMatrix = camera.getWorldTransformation().getData()

        # compute rotation axis in world space
        axisInWorldSpace = homogenize(np.dot(invViewMatrix, axisInViewSpace))
        originWorldSpace = homogenize(np.dot(invViewMatrix, np.array([0, 0, 0, 1])))

        # subtract origin in worldspace to obtain direction rather then points in 3D space
        axisInWorldSpace = axisInWorldSpace - originWorldSpace

        # rotate camera arround that axis by angle
        origin = camera.getWorldPosition()
        rotOrigin = SpaceMouseTool._cameraTool.getOrigin()
        camToOrigin = origin - rotOrigin

        # rotation matrix around the axis
        rotMat = Matrix()
        rotMat.setByRotationAxis(angle * 0.0001, Vector(data=axisInWorldSpace))

        # translation matrix to shift camera to origing i.e. 0,0,0 in world space
        transToOrigMat = Matrix()
        transToOrigMat.setByTranslation(-origin)

        # compute new position for camera
        newPos = camToOrigin.preMultiply(rotMat) + rotOrigin

        # translation matrix to shift camera to new position
        transToNewPosMat = Matrix()
        transToNewPosMat.setByTranslation(newPos)

        # combine to final transforamtion
        trafo = transToOrigMat               # shift to origin
        trafo.preMultiply(rotMat)            # rotate camera in place
        trafo.preMultiply(transToNewPosMat)  # shift to new position

        # apply transforamtion
        camera.setTransformation(camera.getLocalTransformation().preMultiply(trafo))

    @staticmethod
    def spacemouse_move_callback(
            tx: int, ty: int, tz: int,
            angle: float, axisX: float, axisY: float, axisZ: float) -> None:
        # translate and zoom:
        # SpaceMouseTool._translateCamera(tx, ty, tz)
        SpaceMouseTool._rotateCamera(angle, axisX, axisY, axisZ)

        # Logger.log("d",
        #          "Move:" + str(tx) + " " + str(ty) + " " + str(tz) + " "
        #           + str(angle) + " "
        #           + str(axisX) + " " + str(axisY) + " " + str(axisZ) + "\n")

    @staticmethod
    def spacemouse_button_press_callback(button):
        Logger.log("d", "Press " + str(button))

    @staticmethod
    def spacemouse_button_release_callback(button):
        Logger.log("d", "Release " + str(button))


def main():
    spacemousetool = SpaceMouseTool()

    time.sleep(10)


if __name__ == "__main__":
    main()
