#include "SpaceMouse.hpp"

#include <cmath>
#include <iostream>

namespace spacemouse {

/*--------------------------------------------------------------------------*/
/* Abstract base class defining core functionality of spacemouse            */
/*--------------------------------------------------------------------------*/
SpaceMouseAbstract::SpaceMouseAbstract()
    : mInitialized(false),
      mMoveCallback([](SpaceMouseMoveEvent) {}),
      mButtonPressCallback([](SpaceMouseButtonEvent) {}),
      mButtonReleaseCallback([](SpaceMouseButtonEvent) {}) {}

SpaceMouseAbstract::~SpaceMouseAbstract() {}

#ifdef WITH_LIBSPACENAV
/*--------------------------------------------------------------------------*/
/* Spacemouse support using libspacenav                                     */
/*--------------------------------------------------------------------------*/
enum SpaceMouseButtonSpnav {
  // buttons on the 3DConnexion Spacemouse Wireles Pro
  SPMB_SPNAV_TOP = 2,
  SPMB_SPNAV_RIGHT = 4,
  SPMB_SPNAV_FRONT = 5,
  SPMB_SPNAV_ROLL_CW = 8,
  SPMB_SPNAV_LOCK_ROT = 26,
  SPMB_SPNAV_1 = 12,
  SPMB_SPNAV_2 = 13,
  SPMB_SPNAV_3 = 14,
  SPMB_SPNAV_4 = 15,
  SPMB_SPNAV_ESC = 22,
  SPMB_SPNAV_SHIFT = 24,
  SPMB_SPNAV_CTRL = 25,
  SPMB_SPNAV_ALT = 23,
  SPMB_SPNAV_MENU = 0,
  SPMB_SPNAV_FIT = 1

  // if you own an other spacemouse feel free to add further buttons
};

void SpaceMouseSpnav::ProcessEvent(spnav_event sev) {
  if (sev.type == SPNAV_EVENT_MOTION) {
    SpaceMouseMoveEvent moveEvent;
    double axis[3];

    // set translation
    moveEvent.tx = sev.motion.x;
    moveEvent.ty = sev.motion.y;
    moveEvent.tz = sev.motion.z;

    axis[0] = sev.motion.rx;
    axis[1] = sev.motion.ry;
    axis[2] = sev.motion.rz;

    // compute and set angle = norm of the rotation axis
    moveEvent.angle = sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);

    // set (normalized) rotation axis
    if (moveEvent.angle == 0) {
      moveEvent.axisX = 0;
      moveEvent.axisY = 0;
      moveEvent.axisZ = 1;
    } else {
      moveEvent.axisX = axis[0] / moveEvent.angle;
      moveEvent.axisY = axis[1] / moveEvent.angle;
      moveEvent.axisZ = axis[2] / moveEvent.angle;
    }

    // call the callback with that event
    mMoveCallback(std::move(moveEvent));
  } else if (sev.type == SPNAV_EVENT_BUTTON) {
    SpaceMouseButton bnum = SPMB_UNDEFINED;
    switch (sev.button.bnum) {
      case SpaceMouseButtonSpnav::SPMB_SPNAV_TOP:
        bnum = SPMB_TOP;
        break;
      case SpaceMouseButtonSpnav::SPMB_SPNAV_RIGHT:
        bnum = SPMB_RIGHT;
        break;
      case SpaceMouseButtonSpnav::SPMB_SPNAV_FRONT:
        bnum = SPMB_FRONT;
        break;
      case SpaceMouseButtonSpnav::SPMB_SPNAV_LOCK_ROT:
        bnum = SPMB_LOCK_ROT;
        break;
      case SpaceMouseButtonSpnav::SPMB_SPNAV_ROLL_CW:
        bnum = SPMB_ROLL_CW;
        break;
      case SpaceMouseButtonSpnav::SPMB_SPNAV_1:
        bnum = SPMB_1;
        break;
      case SpaceMouseButtonSpnav::SPMB_SPNAV_2:
        bnum = SPMB_2;
        break;
      case SpaceMouseButtonSpnav::SPMB_SPNAV_3:
        bnum = SPMB_3;
        break;
      case SpaceMouseButtonSpnav::SPMB_SPNAV_4:
        bnum = SPMB_4;
        break;
      case SpaceMouseButtonSpnav::SPMB_SPNAV_ESC:
        bnum = SPMB_ESC;
        break;
      case SpaceMouseButtonSpnav::SPMB_SPNAV_SHIFT:
        bnum = SPMB_SHIFT;
        break;
      case SpaceMouseButtonSpnav::SPMB_SPNAV_CTRL:
        bnum = SPMB_CTRL;
        break;
      case SpaceMouseButtonSpnav::SPMB_SPNAV_ALT:
        bnum = SPMB_ALT;
        break;
      case SpaceMouseButtonSpnav::SPMB_SPNAV_MENU:
        bnum = SPMB_MENU;
        break;
      case SpaceMouseButtonSpnav::SPMB_SPNAV_FIT:
        bnum = SPMB_FIT;
        break;
      default:
        bnum = SPMB_UNDEFINED;
        break;
    }
    if (sev.button.press) {
      if (bnum == SPMB_SHIFT)
        mModifiers.add(SpaceMouseModifierKey::SPMM_SHIFT);
      else if (bnum == SPMB_CTRL)
        mModifiers.add(SpaceMouseModifierKey::SPMM_CTRL);
      else if (bnum == SPMB_ALT)
        mModifiers.add(SpaceMouseModifierKey::SPMM_ALT);
      mButtonPressCallback({bnum, mModifiers});
    } else {
      if (bnum == SPMB_SHIFT)
        mModifiers.remove(SpaceMouseModifierKey::SPMM_SHIFT);
      else if (bnum == SPMB_CTRL)
        mModifiers.remove(SpaceMouseModifierKey::SPMM_CTRL);
      else if (bnum == SPMB_ALT)
        mModifiers.remove(SpaceMouseModifierKey::SPMM_ALT);
      mButtonReleaseCallback({bnum, mModifiers});
    }
  }
}

SpaceMouseSpnav &SpaceMouseSpnav::instance() {
  static SpaceMouseSpnav pInstance;
  return pInstance;
}

void SpaceMouseSpnav::Initialize() {
  #ifndef NDEBUG
  std::cout << "Init Spnav" << std::endl;
  #endif  // NDEBUG
  if (!mInitialized) {
    auto error = spnav_open();
    mInitialized = (error != -1);
    if (mInitialized) {
      mThread = std::unique_ptr<std::thread>(new std::thread(
          [](std::future<void> signalExit, std::function<void(spnav_event)> callback) {
            spnav_event sev;
            while (signalExit.wait_for(std::chrono::milliseconds(1)) ==
                   std::future_status::timeout) {
              if (spnav_poll_event(&sev)) {
                callback(sev);
              }
            }
          },
          mSignalExit.get_future(),
          std::bind(&SpaceMouseSpnav::ProcessEvent, this, std::placeholders::_1)));
    }
  }
}

void SpaceMouseSpnav::Close() {
  #ifndef NDEBUG
  std::cout << "Close Spnav" << std::endl;
  #endif  // NDEBUG
  if (mInitialized) {
    spnav_close();
    mInitialized = false;
    mSignalExit.set_value();
    mThread->join();
  }
}

SpaceMouseSpnav::SpaceMouseSpnav() {}

SpaceMouseSpnav::~SpaceMouseSpnav() {
  if (mInitialized) Close();
}
#endif  // WITH_LIBSPACENAV

#ifdef WITH_LIB3DX
/*--------------------------------------------------------------------------*/
/* Spacemouse support using 3DX Client API                                  */
/*--------------------------------------------------------------------------*/

SpaceMouse3DX *This = nullptr;

void SpaceMouse3DX::handleMessage(unsigned int productID, unsigned int messageType,
                                  void *messageArg) {
  ConnexionDeviceState *state;

  switch (messageType) {
    case kConnexionMsgDeviceState:
      state = static_cast<ConnexionDeviceState *>(messageArg);
      if (state->client == This->mClientID) {
        // message was meant for us
        This->ProcessEvent(state);
      }
      break;
    default:
      break;
  }
}

enum SpaceMouseButton3DX {
  // buttons on the 3DConnexion Spacemouse Wireles Pro
  SPMB_3DX_TOP = 4,
  SPMB_3DX_RIGHT = 16,
  SPMB_3DX_FRONT = 32,
  SPMB_3DX_ROLL_CW = 256,
  SPMB_3DX_LOCK_ROT = 67108864,
  SPMB_3DX_1 = 4096,          // Not communicated by the api :(
  SPMB_3DX_2 = 8192,          // Not communicated by the api :(
  SPMB_3DX_3 = 16384,         // Not communicated by the api :(
  SPMB_3DX_4 = 32768,         // Not communicated by the api :(
  SPMB_3DX_ESC = 4194304,     // Not communicated by the api :(
  SPMB_3DX_SHIFT = 16777216,  // Not communicated by the api :(
  SPMB_3DX_CTRL = 33554432,   // Not communicated by the api :(
  SPMB_3DX_ALT = 8388608,     // Not communicated by the api :(
  SPMB_3DX_MENU = 1,
  SPMB_3DX_FIT = 2

  // if you own an other spacemouse feel free to add further buttons
};

void SpaceMouse3DX::ProcessEvent(const ConnexionDeviceState *state) {
  SpaceMouseMoveEvent moveEvent;
  double axis[3];
  SpaceMouseButton bnum = SPMB_UNDEFINED;
  SpaceMouseButton3DX changedButton;
  bool pressed;

  switch (state->command) {
    case kConnexionCmdHandleAxis:
      // set translation
      moveEvent.tx = state->axis[0];
      moveEvent.ty = state->axis[1];
      moveEvent.tz = state->axis[2];

      // compute and set angle = norm of the rotation axis
      for (size_t i = 0; i < 3; ++i) axis[i] = state->axis[i + 3];
      moveEvent.angle = sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);

      // set (normalized) rotation axis
      if (moveEvent.angle == 0) {
        moveEvent.axisX = 0;
        moveEvent.axisY = 0;
        moveEvent.axisZ = 1;
      } else {
        moveEvent.axisX = axis[0] / moveEvent.angle;
        moveEvent.axisY = axis[1] / moveEvent.angle;
        moveEvent.axisZ = axis[2] / moveEvent.angle;
      }

      // call the callback with that event
      mMoveCallback(std::move(moveEvent));
      break;
    case kConnexionCmdHandleButtons:
      // extract the button that has changed:
      changedButton = static_cast<SpaceMouseButton3DX>(mLastButtonConfig ^ state->buttons);
      // button was pressed if the bit of the button was set in state->buttons
      pressed = ((changedButton & state->buttons) != 0);
      switch (changedButton) {
        case SpaceMouseButton3DX::SPMB_3DX_TOP:
          bnum = SPMB_TOP;
          break;
        case SpaceMouseButton3DX::SPMB_3DX_RIGHT:
          bnum = SPMB_RIGHT;
          break;
        case SpaceMouseButton3DX::SPMB_3DX_FRONT:
          bnum = SPMB_FRONT;
          break;
        case SpaceMouseButton3DX::SPMB_3DX_LOCK_ROT:
          bnum = SPMB_LOCK_ROT;
          break;
        case SpaceMouseButton3DX::SPMB_3DX_ROLL_CW:
          bnum = SPMB_ROLL_CW;
          break;
        case SpaceMouseButton3DX::SPMB_3DX_1:
          bnum = SPMB_1;
          break;
        case SpaceMouseButton3DX::SPMB_3DX_2:
          bnum = SPMB_2;
          break;
        case SpaceMouseButton3DX::SPMB_3DX_3:
          bnum = SPMB_3;
          break;
        case SpaceMouseButton3DX::SPMB_3DX_4:
          bnum = SPMB_4;
          break;
        case SpaceMouseButton3DX::SPMB_3DX_ESC:
          bnum = SPMB_ESC;
          break;
        case SpaceMouseButton3DX::SPMB_3DX_SHIFT:
          bnum = SPMB_SHIFT;
          break;
        case SpaceMouseButton3DX::SPMB_3DX_CTRL:
          bnum = SPMB_CTRL;
          break;
        case SpaceMouseButton3DX::SPMB_3DX_ALT:
          bnum = SPMB_ALT;
          break;
        case SpaceMouseButton3DX::SPMB_3DX_MENU:
          bnum = SPMB_MENU;
          break;
        case SpaceMouseButton3DX::SPMB_3DX_FIT:
          bnum = SPMB_FIT;
          break;
        default:
          bnum = SPMB_UNDEFINED;
          break;
      }

      mLastButtonConfig = state->buttons;
      if (pressed) {
        if (bnum == SPMB_SHIFT)
          mModifiers.add(SpaceMouseModifierKey::SPMM_SHIFT);
        else if (bnum == SPMB_CTRL)
          mModifiers.add(SpaceMouseModifierKey::SPMM_CTRL);
        else if (bnum == SPMB_ALT)
          mModifiers.add(SpaceMouseModifierKey::SPMM_ALT);
        mButtonPressCallback({bnum, mModifiers});
      } else {
        if (bnum == SPMB_SHIFT)
          mModifiers.remove(SpaceMouseModifierKey::SPMM_SHIFT);
        else if (bnum == SPMB_CTRL)
          mModifiers.remove(SpaceMouseModifierKey::SPMM_CTRL);
        else if (bnum == SPMB_ALT)
          mModifiers.remove(SpaceMouseModifierKey::SPMM_ALT);
        mButtonReleaseCallback({bnum, mModifiers});
      }
      break;
    default:
      break;
  }
}

SpaceMouse3DX &SpaceMouse3DX::instance() {
  static SpaceMouse3DX pInstance;
  return pInstance;
}

void SpaceMouse3DX::Initialize() {
  #ifndef NDEBUG
  std::cout << "Init 3DX" << std::endl;
  #endif  // NDEBUG
  if (!mInitialized) {
    auto error = SetConnexionHandlers(handleMessage, nullptr, nullptr, true);
    mInitialized = (error == 0);
    mLastButtonConfig = 0;  // all buttons released
    uint8_t name[] = "test";
    mClientID = RegisterConnexionClient(kConnexionClientWildcard, (uint8_t *)name,
                                        kConnexionClientModeTakeOver, kConnexionMaskAll);
    This = this;
  }
}

/**
 * @brief Closes the connection to the 3DConnexion client.
 * @note You do not have to take care of this yourself, as the destructor will
 * also close the connection. You can, however, also do it yourself.
 *
 */
void SpaceMouse3DX::Close() {
  #ifndef NDEBUG
  std::cout << "Close 3DX" << std::endl;
  #endif  // NDEBUG
  UnregisterConnexionClient(mClientID);
  This = nullptr;
  CleanupConnexionHandlers();
  mClientID = 0;
  mInitialized = false;
  mLastButtonConfig = 0;  // all buttons released
}

SpaceMouse3DX::SpaceMouse3DX() {
  mClientID = 0;
  mInitialized = false;
  mLastButtonConfig = 0;  // all buttons released
}

SpaceMouse3DX::~SpaceMouse3DX() {
  if (mInitialized) Close();
}
#endif  // WITH_LIB3DX

/*--------------------------------------------------------------------------*/
/* Daemon for processing spacemouse events and calling the callbacks        */
/*--------------------------------------------------------------------------*/
SpaceMouseDaemon &SpaceMouseDaemon::instance() {
  static SpaceMouseDaemon pInstance;
  return pInstance;
}

SpaceMouseDaemon::SpaceMouseDaemon() {
#ifdef WITH_LIB3DX
  spaceMouse = &SpaceMouse3DX::instance();
#elif WITH_LIBSPACENAV
#if WITH_DAEMONSPACENAV
  spaceMouse = &SpaceMouseSpnav::instance();
#elif WITH_DAEMON3DX
#error Libspacenav with 3dx daemon not yet supported
#else
#error You have to specify which daemon is used
#endif  // WITH_DAEMONSPACENAV OR WITH_DEAMON3DX
#else
#error You have to specify which library (3dx or libspacenav) is used
#endif  // WITH_LIB3DX OR WITH_LIBSPACENAV

  if (!spaceMouse->isInitialized()) spaceMouse->Initialize();
}

SpaceMouseDaemon::~SpaceMouseDaemon() {}

}  // namespace spacemouse
