
#include "globaleventhook.hpp"
#include "dispatchers.hpp"
#include <regex>
#include <set>
#include <hyprland/src/SharedDefs.hpp>
#include "OvGridLayout.hpp"

typedef void (*origOnSwipeBegin)(void*, IPointer::SSwipeBeginEvent e);
typedef void (*origOnSwipeEnd)(void*, IPointer::SSwipeEndEvent e);
typedef void (*origOnSwipeUpdate)(void*, IPointer::SSwipeUpdateEvent e);
typedef void (*origCWindow_onUnmap)(void*);
typedef void (*origStartAnim)(void*, bool in, bool left, bool instant);
typedef void (*origFullscreenActive)(std::string args);
typedef void (*origOnKeyboardKey)(void*, std::any e, SP<IKeyboard> pKeyboard);
typedef void (*origCInputManager_mouseMoveUnified)(void* , uint32_t time, bool refocus);
typedef void (*origCInputManager_onMouseButton)(void*, IPointer::SButtonEvent e);

static double gesture_dx,gesture_previous_dx;
static double gesture_dy,gesture_previous_dy;
static std::set<uint32_t> g_consumedButtonPresses;

std::string getKeynameFromKeycode(IKeyboard::SKeyEvent e, SP<IKeyboard> pKeyboard) {
  xkb_keycode_t keycode = e.keycode + 8;
  xkb_keysym_t keysym = xkb_state_key_get_one_sym(pKeyboard->m_xkbState, keycode);
  char *tmp_keyname = new char[64];
  xkb_keysym_get_name(keysym, tmp_keyname, 64);
  std::string keyname = tmp_keyname;
  delete[] tmp_keyname;
  return keyname;
}

bool isKeyReleaseToggleExitOverviewHit(IKeyboard::SKeyEvent e, SP<IKeyboard> pKeyboard) {
  if (g_hycov_alt_replace_key == "")
    return false;

  if (isNumber(g_hycov_alt_replace_key) && std::stoi(g_hycov_alt_replace_key) > 9 && std::stoi(g_hycov_alt_replace_key) == (e.keycode + 8)) {
    return true;
  } else if (g_hycov_alt_replace_key.find("code:") == 0 && isNumber(g_hycov_alt_replace_key.substr(5)) && std::stoi(g_hycov_alt_replace_key.substr(5)) == (e.keycode + 8)) {
    return true;
  } else {
    std::string keyname = getKeynameFromKeycode(e,pKeyboard);
    if (keyname == g_hycov_alt_replace_key) {
      return true;
    }
  }

  return false;
}

static void hkOnSwipeUpdate(void* thisptr, IPointer::SSwipeUpdateEvent e) {
  if(g_hycov_isOverView){
    gesture_dx = gesture_dx + e.delta.x;
    gesture_dy = gesture_dy + e.delta.y;
    if(e.delta.x > 0 && gesture_dx - gesture_previous_dx > g_hycov_move_focus_distance){
      dispatch_focusdir("r");
      gesture_previous_dx = gesture_dx;
      hycov_log(LOG,"OnSwipeUpdate hook focus right");
    } else if(e.delta.x < 0 && gesture_previous_dx - gesture_dx > g_hycov_move_focus_distance){
      dispatch_focusdir("l");
      gesture_previous_dx = gesture_dx;
      hycov_log(LOG,"OnSwipeUpdate hook focus left");
    } else if(e.delta.y > 0 && gesture_dy - gesture_previous_dy > g_hycov_move_focus_distance){
      dispatch_focusdir("d");
      gesture_previous_dy = gesture_dy;
      hycov_log(LOG,"OnSwipeUpdate hook focus down");
    } else if(e.delta.y < 0 && gesture_previous_dy - gesture_dy > g_hycov_move_focus_distance){
      dispatch_focusdir("u");
      gesture_previous_dy = gesture_dy;
      hycov_log(LOG,"OnSwipeUpdate hook focus up");
    }
    return;
  }
  // call the original function,Let it do what it should do
  (*(origOnSwipeUpdate)g_hycov_pOnSwipeUpdateHook->m_original)(thisptr, e);
}

static void hkOnSwipeBegin(void* thisptr, IPointer::SSwipeBeginEvent e) {
  if(e.fingers == g_hycov_swipe_fingers){
    g_hycov_isGestureBegin = true;
    return;
  } 
  hycov_log(LOG,"OnSwipeBegin hook toggle");

  // call the original function,Let it do what it should do
  (*(origOnSwipeBegin)g_hycov_pOnSwipeBeginHook->m_original)(thisptr, e);
}

static void hkOnSwipeEnd(void* thisptr, IPointer::SSwipeEndEvent e) {
  gesture_dx = 0;
  gesture_previous_dx = 0;
  gesture_dy = 0;
  gesture_previous_dy = 0;
  
  if(g_hycov_isGestureBegin){
    g_hycov_isGestureBegin = false;
    dispatch_toggleoverview("internalToggle");
    return;
  }
  hycov_log(LOG,"OnSwipeEnd hook toggle");
  // call the original function,Let it do what it should do
  (*(origOnSwipeEnd)g_hycov_pOnSwipeEndHook->m_original)(thisptr, e);
}

static void toggle_hotarea(int x_root, int y_root)
{
  PHLMONITOR pMonitor = Desktop::focusState()->monitor();

  if (g_hycov_hotarea_monitor != "all" && pMonitor->m_name != g_hycov_hotarea_monitor)
    return;

  auto m_x = pMonitor->m_position.x;
  auto m_y = pMonitor->m_position.y;
  auto m_width = pMonitor->m_size.x;
  auto m_height = pMonitor->m_size.y;

  if (!g_hycov_isInHotArea &&
    ((g_hycov_hotarea_pos == 1 && x_root < (m_x + g_hycov_hotarea_size) && y_root > (m_y + m_height - g_hycov_hotarea_size)) ||
    (g_hycov_hotarea_pos == 2 && x_root > (m_x + m_width - g_hycov_hotarea_size) && y_root > (m_y + m_height - g_hycov_hotarea_size)) ||
    (g_hycov_hotarea_pos == 3 && x_root < (m_x + g_hycov_hotarea_size) && y_root < (m_y + g_hycov_hotarea_size)) ||
    (g_hycov_hotarea_pos == 4 && x_root > (m_x + m_width - g_hycov_hotarea_size) && y_root < (m_y + g_hycov_hotarea_size))))
  {
    g_hycov_isInHotArea = true;
    hycov_log(LOG,"cursor enter hotarea");
    dispatch_toggleoverview("internalToggle");
  }
  else if (g_hycov_isInHotArea &&
    !((g_hycov_hotarea_pos == 1 && x_root < (m_x + g_hycov_hotarea_size) && y_root > (m_y + m_height - g_hycov_hotarea_size)) ||
    (g_hycov_hotarea_pos == 2 && x_root > (m_x + m_width - g_hycov_hotarea_size) && y_root > (m_y + m_height - g_hycov_hotarea_size)) ||
    (g_hycov_hotarea_pos == 3 && x_root < (m_x + g_hycov_hotarea_size) && y_root < (m_y + g_hycov_hotarea_size)) ||
    (g_hycov_hotarea_pos == 4 && x_root > (m_x + m_width - g_hycov_hotarea_size) && y_root < (m_y + g_hycov_hotarea_size))))
  {
      g_hycov_isInHotArea = false;
  }
}

static void hkCInputManager_mouseMoveUnified(void* thisptr, uint32_t time, bool refocus)
{
  (*(origCInputManager_mouseMoveUnified)g_hycov_pCInputManager_mouseMoveUnifiedHook->m_original)(thisptr, time, refocus);

  Vector2D   mouseCoords        = g_pInputManager->getMouseCoordsInternal();
  const auto MOUSECOORDSFLOORED = mouseCoords.floor();

  toggle_hotarea(MOUSECOORDSFLOORED.x, MOUSECOORDSFLOORED.y);
}

static void hkCInputManager_onMouseButton(void* thisptr, IPointer::SButtonEvent e)
{
  if (e.state != WL_POINTER_BUTTON_STATE_PRESSED) {
    if (g_consumedButtonPresses.count(e.button)) {
      g_consumedButtonPresses.erase(e.button);
      return;
    }
    (*(origCInputManager_onMouseButton)g_hycov_pCInputManager_onMouseButtonHook->m_original)(thisptr, e);
    return;
  }

  if(g_hycov_isOverView && (e.button == BTN_LEFT || e.button == BTN_RIGHT)) {
    
    if (g_hycov_click_in_cursor) {
        g_pInputManager->refocus();
    }

    auto focusedWindow = Desktop::focusState()->window();
    
    if (!focusedWindow) {
        Vector2D mouseCoords = g_pInputManager->getMouseCoordsInternal();
        focusedWindow = g_pCompositor->vectorToWindowUnified(mouseCoords, 0, 0);
        if (focusedWindow) {
            Desktop::focusState()->fullWindowFocus(focusedWindow);
        }
    }
    
    if (!focusedWindow) {
      (*(origCInputManager_onMouseButton)g_hycov_pCInputManager_onMouseButtonHook->m_original)(thisptr, e);
      return;
    }

    g_consumedButtonPresses.insert(e.button);

    if (e.button == BTN_LEFT) {
        dispatch_toggleoverview("internalToggle");
    } else if (e.button == BTN_RIGHT) {
        g_pCompositor->closeWindow(focusedWindow);
    }
    return;
  }

  (*(origCInputManager_onMouseButton)g_hycov_pCInputManager_onMouseButtonHook->m_original)(thisptr, e);
}

void hkOnMouseButton(void* self, SCallbackInfo& info, std::any data)
{
  auto e = std::any_cast<IPointer::SButtonEvent>(data);
  
  if (!g_hycov_isOverView) {
    return;
  }
  
  if (e.button != BTN_LEFT && e.button != BTN_RIGHT) {
    return;
  }
  
  if (e.state != WL_POINTER_BUTTON_STATE_PRESSED) {
    return;
  }
  
  if (g_hycov_click_in_cursor) {
    g_pInputManager->refocus();
  }

  auto focusedWindow = Desktop::focusState()->window();
  
  if (!focusedWindow) {
    Vector2D mouseCoords = g_pInputManager->getMouseCoordsInternal();
    focusedWindow = g_pCompositor->vectorToWindowUnified(mouseCoords, 0, 0);
    if (focusedWindow) {
      Desktop::focusState()->fullWindowFocus(focusedWindow);
    }
  }
  
  if (!focusedWindow) {
    return;
  }

  info.cancelled = true;

  if (e.button == BTN_LEFT) {
    dispatch_toggleoverview("internalToggle");
  } else if (e.button == BTN_RIGHT) {
    g_pCompositor->closeWindow(focusedWindow);
  }
}


static void hkCWindow_onUnmap(void* thisptr) {
  // call the original function,Let it do what it should do
  (*(origCWindow_onUnmap)g_hycov_pCWindow_onUnmap->m_original)(thisptr);

  // after done original thing,The workspace automatically exit overview if no client exists 
  auto nodeNumInSameMonitor = 0;
  auto nodeNumInSameWorkspace = 0;
	for (auto &n : g_hycov_OvGridLayout->m_lOvGridNodesData) {
		if(n.pWindow->monitorID() == Desktop::focusState()->monitor()->m_id && !g_pCompositor->isWorkspaceSpecial(n.workspaceID)) {
			nodeNumInSameMonitor++;
		}
		if(n.pWindow->m_workspace == Desktop::focusState()->monitor()->m_activeWorkspace) {
			nodeNumInSameWorkspace++;
		}
	}

  if (g_hycov_isOverView && nodeNumInSameMonitor == 0) {
    hycov_log(LOG,"no tiling window in same monitor,auto exit overview");
    dispatch_leaveoverview("");
    return;
  }

  if (g_hycov_isOverView && nodeNumInSameWorkspace == 0 && (g_hycov_only_active_workspace || g_hycov_force_display_only_current_workspace)) {
    hycov_log(LOG,"no tiling windwo in same workspace,auto exit overview");
    dispatch_leaveoverview("");
    return;
  }

}

static void hkChangeworkspace(std::string args) {
  // just log a message and do nothing, mean the original function is disabled
  hycov_log(LOG,"ChangeworkspaceHook hook toggle");
}

static void hkMoveActiveToWorkspace(std::string args) {
  // just log a message and do nothing, mean the original function is disabled
  hycov_log(LOG,"MoveActiveToWorkspace hook toggle");
}

static void hkSpawn(std::string args) {
  // just log a message and do nothing, mean the original function is disabled
  hycov_log(LOG,"Spawn hook toggle");
}

static void hkStartAnim(void* thisptr,bool in, bool left, bool instant = false) {
  // if is exiting overview, omit the animation of workspace change (instant = true)
  if (g_hycov_isOverViewExiting) {
    (*(origStartAnim)g_hycov_pStartAnimHook->m_original)(thisptr, in, left, true);
    hycov_log(LOG,"hook startAnim,disable workspace change anim,in:{},isOverview:{}",in,g_hycov_isOverView);
  } else {
    (*(origStartAnim)g_hycov_pStartAnimHook->m_original)(thisptr, in, left, instant);
    // hycov_log(LOG,"hook startAnim,enable workspace change anim,in:{},isOverview:{}",in,g_hycov_isOverView);
  }
}

static void hkOnKeyboardKey(void* thisptr,std::any event, SP<IKeyboard> pKeyboard) {

  (*(origOnKeyboardKey)g_hycov_pOnKeyboardKeyHook->m_original)(thisptr, event, pKeyboard);

  auto e = std::any_cast<IKeyboard::SKeyEvent>(event);
  // hycov_log(LOG,"alt key,keycode:{}",e.keycode);
  if(g_hycov_enable_alt_release_exit && g_hycov_isOverView && e.state == WL_KEYBOARD_KEY_STATE_RELEASED) {
    if (!isKeyReleaseToggleExitOverviewHit(e,pKeyboard))
      return;
    dispatch_leaveoverview("");
    hycov_log(LOG,"alt key release toggle leave overview");
  }

}

static void hkFullscreenActive(std::string args) {
  // auto exit overview and fullscreen window when toggle fullscreen in overview mode
  hycov_log(LOG,"FullscreenActive hook toggle");

  // (*(origFullscreenActive)g_hycov_pFullscreenActiveHook->m_pOriginal)(args);
  const auto pWindow = Desktop::focusState()->window();

  if (!pWindow)
        return;

  if (pWindow->m_workspace->m_isSpecialWorkspace)
        return;

  if (g_hycov_isOverView && want_auto_fullscren(pWindow) && !g_hycov_auto_fullscreen) {
    hycov_log(LOG,"FullscreenActive toggle leave overview with fullscreen");
    dispatch_toggleoverview("internalToggle");
    g_pCompositor->setWindowFullscreenInternal(pWindow, pWindow->isFullscreen() ? FSMODE_NONE : (args == "1" ? FSMODE_MAXIMIZED : FSMODE_FULLSCREEN));
  } else if (g_hycov_isOverView && (!want_auto_fullscren(pWindow) || g_hycov_auto_fullscreen)) {
    hycov_log(LOG,"FullscreenActive toggle leave overview without fullscreen");
    dispatch_toggleoverview("internalToggle");
  } else {
    hycov_log(LOG,"FullscreenActive set fullscreen");
    g_pCompositor->setWindowFullscreenInternal(pWindow, pWindow->isFullscreen() ? FSMODE_NONE : (args == "1" ? FSMODE_MAXIMIZED : FSMODE_FULLSCREEN));
  }
}

void hkHyprDwindleLayout_recalculateMonitor(void* thisptr,const int& ID) {
  ;
}

void hkHyprMasterLayout_recalculateMonitor(void* thisptr,const int& ID) {
  ;
}

void hkHyprDwindleLayout_recalculateWindow(void* thisptr,CWindow* pWindow) {
  ;
}

void hkSDwindleNodeData_recalcSizePosRecursive(void* thisptr,bool force, bool horizontalOverride, bool verticalOverride) {
  ;
}

void hkCKeybindManager_toggleGroup(std::string args) {
  ;
}

void hkCKeybindManager_moveOutOfGroup(std::string args) {
  ;
}

void hkCKeybindManager_changeGroupActive(std::string args) {
    const auto PWINDOW = Desktop::focusState()->window();
    PHLWINDOW pTargetWindow;
    if (!PWINDOW)
        return;

    if (!PWINDOW->m_groupData.pNextWindow.lock())
        return;

    if (PWINDOW->m_groupData.pNextWindow.lock() == PWINDOW)
        return;

    auto pNode =  g_hycov_OvGridLayout->getNodeFromWindow(PWINDOW);
    if (!pNode)
      return;

    if (args != "b" && args != "prev") {
        pTargetWindow = PWINDOW->m_groupData.pNextWindow.lock();
    } else {
        pTargetWindow = PWINDOW->getGroupPrevious();
    }  

    hycov_log(LOG,"changeGroupActive,pTargetWindow:{}",pTargetWindow);

    if(pNode->isInOldLayout) { // if client is taken from the old layout
        g_hycov_OvGridLayout->removeOldLayoutData(PWINDOW);
        pNode->isInOldLayout = false;
    }

    pNode->pWindow = pTargetWindow;
    pNode->pWindow->m_workspace = g_pCompositor->getWorkspaceByID(pNode->workspaceID);
    
    PWINDOW->setGroupCurrent(pTargetWindow);
    g_hycov_OvGridLayout->applyNodeDataToWindow(pNode);
}


void registerGlobalEventHook()
{
  g_hycov_isInHotArea = false;
  g_hycov_isGestureBegin = false;
  g_hycov_isOverView = false;
  g_hycov_isOverViewExiting = false;
  gesture_dx = 0;
  gesture_dy = 0;
  gesture_previous_dx = 0;
  gesture_previous_dy = 0;
  
  // HyprlandAPI::registerCallbackStatic(PHANDLE, "mouseMove", mouseMoveHookPtr.get());
  // HyprlandAPI::registerCallbackStatic(PHANDLE, "mouseButton", mouseButtonHookPtr.get());
  
  //create public function hook

  // hook function of Swipe gesture event handle 
  g_hycov_pOnSwipeBeginHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CInputManager::onSwipeBegin, (void*)&hkOnSwipeBegin);
  g_hycov_pOnSwipeEndHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CInputManager::onSwipeEnd, (void*)&hkOnSwipeEnd);
  g_hycov_pOnSwipeUpdateHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CInputManager::onSwipeUpdate, (void*)&hkOnSwipeUpdate);

  // hook function of Gridlayout Remove a node from tiled list
  g_hycov_pCWindow_onUnmap = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CWindow::onUnmap, (void*)&hkCWindow_onUnmap);

  // hook function of workspace change animation start
  // NOTE: CWorkspace::startAnim no longer exists in new Hyprland - animations now handled by CDesktopAnimationManager
  // g_hycov_pStartAnimHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CWorkspace::startAnim, (void*)&hkStartAnim);
  // g_hycov_pStartAnimHook->hook();

  //  hook function of keypress
  g_hycov_pOnKeyboardKeyHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CInputManager::onKeyboardKey, (void*)&hkOnKeyboardKey);

  // layotu reculate
  g_hycov_pHyprDwindleLayout_recalculateMonitorHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CHyprDwindleLayout::recalculateMonitor, (void*)&hkHyprDwindleLayout_recalculateMonitor);
  g_hycov_pHyprMasterLayout_recalculateMonitorHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CHyprMasterLayout::recalculateMonitor, (void*)&hkHyprMasterLayout_recalculateMonitor);
  g_hycov_pHyprDwindleLayout_recalculateWindowHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CHyprDwindleLayout::recalculateWindow, (void*)&hkHyprDwindleLayout_recalculateWindow);
  g_hycov_pSDwindleNodeData_recalcSizePosRecursiveHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&SDwindleNodeData::recalcSizePosRecursive, (void*)&hkSDwindleNodeData_recalcSizePosRecursive);


  //mouse
  g_hycov_pCInputManager_mouseMoveUnifiedHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CInputManager::mouseMoveUnified, (void*)&hkCInputManager_mouseMoveUnified);

  // Guard scrolling follow-focus callback while in overview.
  // This avoids layout-owned activeWindow handlers from fighting ovgrid geometry.
  g_hycov_pActiveWindowGuardCallback = HyprlandAPI::registerCallbackDynamic(PHANDLE, "activeWindow", [](void*, SCallbackInfo& info, std::any) {
    if (!g_hycov_scrolling_guard_activewindow) {
      return;
    }
    if (!g_hycov_isOverView || !g_hycov_compat_scrolling_active) {
      return;
    }

    info.cancelled = true;
  });

  //mousebutton - use dynamic callback (safe approach, doesn't corrupt mouse input)
  if(g_hycov_enable_click_action) {
    static auto pMouseButtonCallback = HyprlandAPI::registerCallbackDynamic(PHANDLE, "mouseButton", [](void* self, SCallbackInfo& info, std::any data) {
      auto e = std::any_cast<IPointer::SButtonEvent>(data);
      
      if (!g_hycov_isOverView) {
        return;
      }
      
      if (e.state != WL_POINTER_BUTTON_STATE_PRESSED) {
        if (g_hycov_isDragging && e.button == BTN_LEFT) {
          Vector2D mouseCoords = g_pInputManager->getMouseCoordsInternal();
          auto targetMonitor = g_pCompositor->getMonitorFromVector(mouseCoords);
          
          if (g_hycov_draggedWindow && targetMonitor && targetMonitor->m_id != g_hycov_dragStartMonitor) {
            // Save for pending move AFTER overview exits
            g_hycov_pendingMoveWindow = g_hycov_draggedWindow;
            g_hycov_pendingMoveMonitor = targetMonitor;
            hycov_log(LOG, "Drag release: pending move to monitor {}", targetMonitor->m_id);
          }
          
          g_hycov_isDragging = false;
          g_hycov_draggedWindow = nullptr;
          
          dispatch_toggleoverview("internalToggle");
          info.cancelled = true;
        }
        return;
      }
      
      if (e.button != BTN_LEFT && e.button != BTN_RIGHT) {
        return;
      }
      
      if (g_hycov_click_in_cursor) {
        g_pInputManager->refocus();
      }

      auto focusedWindow = Desktop::focusState()->window();
      
      if (!focusedWindow) {
        Vector2D mouseCoords = g_pInputManager->getMouseCoordsInternal();
        focusedWindow = g_pCompositor->vectorToWindowUnified(mouseCoords, 0, 0);
        if (focusedWindow) {
          Desktop::focusState()->fullWindowFocus(focusedWindow);
        }
      }
      
      if (!focusedWindow) {
        return;
      }

      info.cancelled = true;

      if (e.button == BTN_LEFT) {
        Vector2D mouseCoords = g_pInputManager->getMouseCoordsInternal();
        auto currentMonitor = g_pCompositor->getMonitorFromVector(mouseCoords);
        
        g_hycov_isDragging = true;
        g_hycov_draggedWindow = focusedWindow;
        g_hycov_dragStartPos = mouseCoords;
        g_hycov_dragStartMonitor = currentMonitor ? currentMonitor->m_id : -1;
      } else if (e.button == BTN_RIGHT) {
        g_pCompositor->closeWindow(focusedWindow);
      }
    });
    
    // Register mouse move callback for visual dragging
    static auto pMouseMoveCallback = HyprlandAPI::registerCallbackDynamic(PHANDLE, "mouseMove", [](void* self, SCallbackInfo& info, std::any data) {
      if (!g_hycov_isDragging || !g_hycov_draggedWindow) {
        return;
      }
      
      Vector2D mouseCoords = g_pInputManager->getMouseCoordsInternal();
      Vector2D delta = mouseCoords - g_hycov_dragStartPos;
      
      // Cursor mouse drag, PLEASE WORK PLEASE PLEASE PLEASE
      Vector2D newPos = g_hycov_draggedWindow->m_position + delta;
      g_hycov_draggedWindow->m_position = newPos;
      *g_hycov_draggedWindow->m_realPosition = newPos;
      g_hycov_draggedWindow->m_realPosition->warp();
      g_hycov_dragStartPos = mouseCoords;
      
      // Try to update multi-monitor render on drag, meh it might work even if it isnt the most beautiful solution
      auto currentMonitor = g_pCompositor->getMonitorFromVector(mouseCoords);
      if (currentMonitor && g_hycov_draggedWindow->m_monitor.get() != currentMonitor.get()) {
        g_hycov_draggedWindow->m_monitor = currentMonitor;
      }
      
      g_pHyprRenderer->damageWindow(g_hycov_draggedWindow);
    });
    
    hycov_log(LOG, "hycov: Registered mouseButton callback (dynamic)");
  }

  // Register keyPress callback for alt-release 
  static auto pKeyPressCallback = HyprlandAPI::registerCallbackDynamic(PHANDLE, "keyPress", [](void* self, SCallbackInfo& info, std::any data) {
    // Read config value dynamically to see if we bypass dumbass caching
    static const auto *pEnableAltReleaseExit = (Hyprlang::INT* const*)(HyprlandAPI::getConfigValue(PHANDLE, "plugin:hycov:enable_alt_release_exit")->getDataStaticPtr());
    static const auto *pAltReplaceKey = (Hyprlang::STRING const*)(HyprlandAPI::getConfigValue(PHANDLE, "plugin:hycov:alt_replace_key")->getDataStaticPtr());
    
    const bool altReleaseEnabled = **pEnableAltReleaseExit;
    
    // Early exit if feature disabled or not in overview
    if (!altReleaseEnabled || !g_hycov_isOverView) {
      return;
    }

    // Extract keyboard and event from the data map
    auto dataMap = std::any_cast<std::unordered_map<std::string, std::any>>(data);
    auto pKeyboard = std::any_cast<SP<IKeyboard>>(dataMap["keyboard"]);
    auto event = std::any_cast<IKeyboard::SKeyEvent>(dataMap["event"]);

    if (event.state != WL_KEYBOARD_KEY_STATE_RELEASED) {
      return;
    }

    std::string altKey = *pAltReplaceKey;
    int xkbKeycode = event.keycode + 8;
    
    // numeric keycode match
    bool matched = false;
    if (isNumber(altKey) && std::stoi(altKey) > 9 && std::stoi(altKey) == xkbKeycode) {
      matched = true;
    } else if (altKey.find("code:") == 0 && isNumber(altKey.substr(5)) && std::stoi(altKey.substr(5)) == xkbKeycode) {
      matched = true;
    } else {
      // symbolic keyname match
      std::string keyname = getKeynameFromKeycode(event, pKeyboard);
      if (!keyname.empty() && keyname == altKey) {
        matched = true;
      }
    }

    if (!matched) {
      return;
    }

    hycov_log(LOG, "[hycov] Alt key released (keycode {}), exiting overview", xkbKeycode);
    dispatch_leaveoverview("");
  });
  hycov_log(LOG, "hycov: Registered keyPress callback for alt-release exit");

  //changeGroupActive
  g_hycov_pCKeybindManager_changeGroupActiveHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CKeybindManager::changeGroupActive, (void*)&hkCKeybindManager_changeGroupActive);

  //toggleGroup
  g_hycov_pCKeybindManager_toggleGroupHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CKeybindManager::toggleGroup, (void*)&hkCKeybindManager_toggleGroup);

  //moveOutOfGroup
  g_hycov_pCKeybindManager_moveOutOfGroupHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CKeybindManager::moveOutOfGroup, (void*)&hkCKeybindManager_moveOutOfGroup);

  //create private function hook

  // hook function of changeworkspace
  static const auto ChangeworkspaceMethods = HyprlandAPI::findFunctionsByName(PHANDLE, "changeworkspace");
  if (!ChangeworkspaceMethods.empty()) {
    g_hycov_pChangeworkspaceHook = HyprlandAPI::createFunctionHook(PHANDLE, ChangeworkspaceMethods[0].address, (void*)&hkChangeworkspace);
  } else {
    hycov_log(Log::ERR, "hycov: Failed to find changeworkspace function");
  }

  // hook function of moveActiveToWorkspace
  static const auto MoveActiveToWorkspaceMethods = HyprlandAPI::findFunctionsByName(PHANDLE, "moveActiveToWorkspace");
  if (!MoveActiveToWorkspaceMethods.empty()) {
    g_hycov_pMoveActiveToWorkspaceHook = HyprlandAPI::createFunctionHook(PHANDLE, MoveActiveToWorkspaceMethods[0].address, (void*)&hkMoveActiveToWorkspace);
  } else {
    hycov_log(Log::ERR, "hycov: Failed to find moveActiveToWorkspace function");
  }

  // hook function of spawn (bindkey will call spawn to excute a command or a dispatch)
  static const auto SpawnMethods = HyprlandAPI::findFunctionsByName(PHANDLE, "spawn");
  if (!SpawnMethods.empty()) {
    g_hycov_pSpawnHook = HyprlandAPI::createFunctionHook(PHANDLE, SpawnMethods[0].address, (void*)&hkSpawn);
  } else {
    hycov_log(Log::ERR, "hycov: Failed to find spawn function");
  }

  //hook function of fullscreenActive
  static const auto FullscreenActiveMethods = HyprlandAPI::findFunctionsByName(PHANDLE, "fullscreenActive");
  if (!FullscreenActiveMethods.empty()) {
    g_hycov_pFullscreenActiveHook = HyprlandAPI::createFunctionHook(PHANDLE, FullscreenActiveMethods[0].address, (void*)&hkFullscreenActive);
  } else {
    hycov_log(Log::ERR, "hycov: Failed to find fullscreenActive function");
  }

  // Hotarea feature disabled - causes mouse input corruption on Hyprland v0.53+

  // mouseButton callback is already registered above when g_hycov_enable_click_action is true
  if(g_hycov_enable_click_action) {
    HyprlandAPI::addNotification(PHANDLE, "[hycov] Mouse click enabled!", CHyprColor{0.0, 1.0, 0.0, 1.0}, 3000);
  }

  //if enable gesture, apply hook Swipe function 
  if(g_hycov_enable_gesture){
    g_hycov_pOnSwipeBeginHook->hook();
    g_hycov_pOnSwipeEndHook->hook();
    g_hycov_pOnSwipeUpdateHook->hook();
  }

  //if enable auto_exit, apply hook RemovedTiling function
  if(g_hycov_auto_exit){
    g_hycov_pCWindow_onUnmap->hook();
  }

  // Alt-release exit is now handled by keyPress callback registered above

}
