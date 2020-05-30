from . import SpaceMouseTool


def getMetaData():
    return {
        "tool": {
            "visible": False
        }
    }


def register(app):
    return {"tool": SpaceMouseTool.SpaceMouseTool()}
