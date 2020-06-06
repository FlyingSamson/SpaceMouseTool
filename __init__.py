# Copyright (c) 2020 FlyingSamson.
# SpaceMouseTool is released under the terms of the AGPLv3 or higher.

from . import SpaceMouseTool


def getMetaData():
    return {
        "tool": {
            "visible": False
        }
    }


def register(app):
    return {"tool": SpaceMouseTool.SpaceMouseTool()}
