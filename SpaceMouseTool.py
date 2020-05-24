from UM.Application import Application
from UM.Tool import Tool
from UM.Logger import Logger
from UM.Math.Matrix import Matrix
from UM.Math.Vector import Vector

from enum import IntEnum
import time

import platform
if platform.system() == "Darwin":
    from .lib.darwin.pyspacemouse import start_spacemouse_daemon
elif platform.system() == "Linux":
    from .lib.x86_64.pyspacemouse import start_spacemouse_daemon


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
        axisInViewSpace = Vector(-axisX, axisZ, -axisY)

        # get inverse view matrix
        invViewMatrix = camera.getWorldOrientation().toMatrix()

        # compute rotation axis in world space
        axisInWorldSpace = axisInViewSpace.preMultiply(invViewMatrix)

        # rotate camera around that axis by angle
        rotOrigin = SpaceMouseTool._cameraTool.getOrigin()

        # rotation matrix around the axis
        rotMat = Matrix()
        rotMat.setByRotationAxis(
            angle * SpaceMouseTool._rotScale, axisInWorldSpace, rotOrigin.getData())

        # apply transformation
        camera.setTransformation(camera.getLocalTransformation().preMultiply(rotMat))

    @staticmethod
    def spacemouse_move_callback(
            tx: int, ty: int, tz: int,
            angle: float, axisX: float, axisY: float, axisZ: float) -> None:
        # translate and zoom:
        SpaceMouseTool._translateCamera(tx, ty, tz)
        SpaceMouseTool._rotateCamera(angle, axisX, axisY, axisZ)

    @staticmethod
    def spacemouse_button_press_callback(button: int):
        controller = Application.getInstance().getController()
        if(button == SpaceMouseTool.SpaceMouseButton.SPMB_TOP):
            controller.setCameraRotation("y", 90)
        elif(button == SpaceMouseTool.SpaceMouseButton.SPMB_RIGHT):
            controller.setCameraRotation("x", -90)
        elif(button == SpaceMouseTool.SpaceMouseButton.SPMB_FRONT):
            controller.setCameraRotation("home", 0)
        Logger.log("d", "Press " + str(button))

    @staticmethod
    def spacemouse_button_release_callback(button: int):
        Logger.log("d", "Release " + str(button))


def main():
    spacemousetool = SpaceMouseTool()

    time.sleep(10)


if __name__ == "__main__":
    main()
