#ifndef SPACEMOUSE_HPP
#define SPACEMOUSE_HPP

#include <functional>
#include <memory>

namespace spacemouse {

extern std::function<void(const char*)> logFun;

/*--------------------------------------------------------------------------*/
/* Spacemouse events                                                        */
/*--------------------------------------------------------------------------*/
/**
 * @brief Event that represents a translation and/or rotation movement.
 */
struct SpaceMouseMoveEvent {
  SpaceMouseMoveEvent() = default;
  SpaceMouseMoveEvent(int tx, int ty, int tz, double angle, double axisX, double axisY,
                      double axisZ)
      : tx(tx), ty(ty), tz(tz), angle(angle), axisX(axisX), axisY(axisY), axisZ(axisZ) {}
  int tx; /**< Translation x coordinate */
  int ty; /**< Translation y coordinate */
  int tz; /**< Translation z coordinate */

  double angle; /**< Rotation angle */
  double axisX; /**< Rotation axis x coordinate */
  double axisY; /**< Rotation axis y coordinate */
  double axisZ; /**< Rotation axis z coordinate */
};

/**
 * @brief Enumerates the buttons on the spacemouse
 */
enum SpaceMouseButton {
  // buttons on the 3DConnexion Spacemouse Wireles Pro:
  // view buttons:
  SPMB_TOP = 0,      /**< Top view button */
  SPMB_RIGHT = 1,    /**< Right view button */
  SPMB_FRONT = 2,    /**< Front view button */
  SPMB_ROLL_CW = 3,  /**< Roll the view clock-wise in the plane orthogonal to the
                  direction of view */
  SPMB_LOCK_ROT = 4, /**< Lock rotation */
  // configurable buttons 1,2,3, and 4
  SPMB_1 = 5, /**< Configurable button 1 */
  SPMB_2 = 6, /**< Configurable button 2 */
  SPMB_3 = 7, /**< Configurable button 3 */
  SPMB_4 = 8, /**< Configurable button 4 */
  // modifier keys
  SPMB_ESC = 9,    /**< Escape key */
  SPMB_SHIFT = 10, /**< Shift key */
  SPMB_CTRL = 11,  /**< Control key */
  SPMB_ALT = 12,   /**< Alternate key */
  // menu button
  SPMB_MENU = 12, /**< Menu button */
  // fit to screen button
  SPMB_FIT = 13, /**< Fit shown objects to screen */
  // if you own another spacemouse feel free to add further buttons

  // undefined button
  SPMB_UNDEFINED = 14 /**< Undefined button */
};


enum class SpaceMouseModifierKey : int {
  SPMM_SHIFT = 1,
  SPMM_CTRL = 2,
  SPMM_ALT = 4
};

class SpaceMouseModifierKeys {
 public:
  SpaceMouseModifierKeys() : mModifiers(SpaceMouseModifierKey(0)) {}
  void add(SpaceMouseModifierKey key) {
    mModifiers = SpaceMouseModifierKey(static_cast<int>(mModifiers) | static_cast<int>(key));
  }
  void remove(SpaceMouseModifierKey key) {
    mModifiers = SpaceMouseModifierKey(static_cast<int>(mModifiers) & (~static_cast<int>(key)));
  }
  bool contains(SpaceMouseModifierKey key) const {
    return (static_cast<int>(mModifiers) & static_cast<int>(key)) != 0;
  }
  bool isEmpty() const {
    return (static_cast<int>(mModifiers) == 0);
  }
  SpaceMouseModifierKey modifiers() const {
    return mModifiers;
  }

 private:
  SpaceMouseModifierKey mModifiers;
};

/**
 * @brief Event describing the press or release of a button
 */
struct SpaceMouseButtonEvent {
  SpaceMouseButton button; /**< The pressed button */
  SpaceMouseModifierKeys modifierKeys;
};
}  // namespace spacemouse

namespace spacemouse {
/*--------------------------------------------------------------------------*/
/* Abstract base class defining core functionality of spacemouse            */
/*--------------------------------------------------------------------------*/
class SpaceMouseAbstract {
 public:
  /**
   * @brief Initializes the communication to the spacemouse daemon (3dxsrv or
   * spacenavd)
   */
  virtual void Initialize() = 0;
  /**
   * @brief Closes the connection to the 3DConnexion client.
   * @note You do not have to take care of this yourself, as the destructor will
   * also close the connection. You can, however, also do it yourself.
   *
   */
  virtual void Close() = 0;

  /**
   * Checks whether the communication to the daemon (3dxsrv or
   * spacenavd) was successfully established
   */
  bool isInitialized() const { return mInitialized; }
  /** @brief Sets the callback for move (i.e. translate and rotate) events
   *  @note The callback might get called from another thread then the one that
   *  instantiated the daemon
   */
  void setMoveCallback(std::function<void(SpaceMouseMoveEvent)> callback) {
    mMoveCallback = callback;
  }
  /** @brief Sets the callback for button pressed events
   *  @note The callback might get called from another thread then the one that
   *  instantiated the daemon
   */
  void setButtonPressCallback(std::function<void(SpaceMouseButtonEvent)> callback) {
    mButtonPressCallback = callback;
  }
  /** @brief Sets the callback for button released events
   *  @note The callback might get called from another thread then the one that
   *  instantiated the daemon
   */
  void setButtonReleaseCallback(std::function<void(SpaceMouseButtonEvent)> callback) {
    mButtonReleaseCallback = callback;
  }

 protected:
  SpaceMouseAbstract();
  virtual ~SpaceMouseAbstract();
  bool mInitialized;
  SpaceMouseModifierKeys mModifiers;
  std::function<void(SpaceMouseMoveEvent)> mMoveCallback;
  std::function<void(SpaceMouseButtonEvent)> mButtonPressCallback;
  std::function<void(SpaceMouseButtonEvent)> mButtonReleaseCallback;
};
}  // namespace spacemouse

#ifdef WITH_LIBSPACENAV
/*--------------------------------------------------------------------------*/
/* Spacemouse support using libspacenav                                     */
/*--------------------------------------------------------------------------*/
#include <X11/Xlib.h>
#include <spnav.h>

#include <future>
#include <memory>
#include <thread>

namespace spacemouse {
class SpaceMouseSpnav : public SpaceMouseAbstract {
 public:
  static SpaceMouseSpnav &instance();
  void Initialize();
  void Close();

 protected:
  SpaceMouseSpnav();
  virtual ~SpaceMouseSpnav();

 private:
  std::unique_ptr<std::thread> mThread;
  std::promise<void> mSignalExit;

  /**
   * @brief Processes a spacenav event calling the appropriate callbacks for
   * move, button press and button release events
   */
  void ProcessEvent(spnav_event sev);

  SpaceMouseSpnav(const SpaceMouseSpnav &);
  SpaceMouseSpnav &operator=(const SpaceMouseSpnav &);
};

}  // namespace spacemouse
#endif  // WITH_LIBSPACENAV

#ifdef WITH_LIB3DX
/*--------------------------------------------------------------------------*/
/* Spacemouse support using 3DX Client API                                  */
/*--------------------------------------------------------------------------*/
#include <3DconnexionClient/ConnexionClientAPI.h>

#include <iostream>

namespace spacemouse {
class SpaceMouse3DX : public SpaceMouseAbstract {
 public:
  static SpaceMouse3DX &instance();
  void Initialize();
  void Close();

 protected:
  SpaceMouse3DX();
  virtual ~SpaceMouse3DX();

 private:
  uint64_t mClientID;

  int mLastButtonConfig;

  SpaceMouse3DX(const SpaceMouse3DX &);
  SpaceMouse3DX &operator=(const SpaceMouse3DX &);

  static void handleMessage(unsigned int productID, unsigned int messageType, void *messageArg);
  void ProcessEvent(const ConnexionDeviceState *state);
};

}  // namespace spacemouse

#endif  // WITH_LIB3DX

namespace spacemouse {
/*--------------------------------------------------------------------------*/
/* Daemon for processing spacemouse events and calling the callbacks        */
/*--------------------------------------------------------------------------*/
/** Daemon that wraps the connection to the spacemouse internally using either
 * the libs provided by 3DConnexion or libspacenav (configure which one is used
 * using cmake, or directly via the flags -DWITH_LIB3DX or -DWITH_LIBSPACENAV).
 * If libspacenav is used communication to either the 3DConnexion daemon or the
 * spacenavd daemon is supported (again configure which one is used using
 * cmake, or directly via the flags -DWITH_DAEMONSPACENAV or -DWITH_DAEMON3DX).
 */
class SpaceMouseDaemon {
 public:
  static SpaceMouseDaemon &instance();

  /**
   * Checks whether the daemon was successfully initialized
   */
  bool isInitialized() const { return spaceMouse && spaceMouse->isInitialized(); }

  /** @brief Sets the callback for move (i.e. translate and rotate) events
   *  @note The callback might get called from another thread then the one that
   *  instantiated the daemon
   */
  void setMoveCallback(std::function<void(SpaceMouseMoveEvent)> callback) {
    spaceMouse->setMoveCallback(callback);
  }
  /** @brief Sets the callback for button pressed events
   *  @note The callback might get called from another thread then the one that
   *  instantiated the daemon
   */
  void setButtonPressCallback(std::function<void(SpaceMouseButtonEvent)> callback) {
    spaceMouse->setButtonPressCallback(callback);
  }
  /** @brief Sets the callback for button released events
   *  @note The callback might get called from another thread then the one that
   *  instantiated the daemon
   */
  void setButtonReleaseCallback(std::function<void(SpaceMouseButtonEvent)> callback) {
    spaceMouse->setButtonReleaseCallback(callback);
  }

 protected:
  SpaceMouseDaemon();
  virtual ~SpaceMouseDaemon();

 private:
  SpaceMouseAbstract *spaceMouse;

  SpaceMouseDaemon(const SpaceMouseDaemon &);             // not implemented
  SpaceMouseDaemon &operator=(const SpaceMouseDaemon &);  // not implemented
};

}  // namespace spacemouse

#endif  // SPACEMOUSE_HPP
