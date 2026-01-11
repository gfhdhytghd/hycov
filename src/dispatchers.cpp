#include "dispatchers.hpp"

static const std::string overviewWorksapceName = "OVERVIEW";
static std::string workspaceNameBackup;
static WORKSPACEID workspaceIdBackup;

void recalculateAllMonitor() {
	for (auto &m : g_pCompositor->m_monitors) {
		PHLMONITOR pMonitor = m;
		g_pLayoutManager->getCurrentLayout()->recalculateMonitor(pMonitor->m_id);
	}
}

// only change layout,keep data of previous layout
void switchToLayoutWithoutReleaseData(std::string layout) {
    for (size_t i = 0; i < g_pLayoutManager->m_layouts.size(); ++i) {
        if (g_pLayoutManager->m_layouts[i].first == layout) {
            if (i == (size_t)g_pLayoutManager->m_currentLayoutID)
                return;
            g_pLayoutManager->m_currentLayoutID = i;
            return;
        }
    }
    hycov_log(Log::ERR, "Unknown layout!");
}

bool want_auto_fullscren(PHLWINDOW pWindow) {
	int nodeNumInTargetWorkspace = 1;

	if(!pWindow) {
		return false;
	}

	auto pNode = g_hycov_OvGridLayout->getNodeFromWindow(pWindow);

	if(!pNode) {
		return true;
	}

	// if client is fullscreen before,don't make it fullscreen
	if (pNode->ovbk_windowIsFullscreen) {
		return false;
	}

	// caculate the number of clients that will be in the same workspace with pWindow(don't contain itself)
	for (auto &n : g_hycov_OvGridLayout->m_lOvGridNodesData) {
		if(n.pWindow != pNode->pWindow && n.ovbk_windowWorkspaceId == pNode->ovbk_windowWorkspaceId) {
			nodeNumInTargetWorkspace++;
		}
	}
	
	// if only one client in workspace(pWindow itself), don't make it fullscreen
	if(nodeNumInTargetWorkspace > 1) {
		return true;
	} else {
		return false;
	}
}

bool isDirectionArg(std::string arg) {
	if (arg == "l" || arg == "r" || arg == "u" || arg == "d" || arg == "left" || arg == "right" || arg == "up" || arg == "down"  || arg == "leftcross" || arg == "rightcross" || arg == "upcross" || arg == "downcross") {
		return true;
	} else {
		return false;
	}
}

bool isCrossMonitor(std::string arg) {
	if (arg == "leftcross" || arg == "rightcross" || arg == "upcross" || arg == "downcross") {
		return true;
	} else {
		return false;
	}
}

std::optional<ShiftDirection> parseShiftArg(std::string arg) {
	if (arg == "l" || arg == "left" || arg == "leftcross") return ShiftDirection::Left;
	else if (arg == "r" || arg == "right" || arg == "rightcross") return ShiftDirection::Right;
	else if (arg == "u" || arg == "up" || arg == "upcross") return ShiftDirection::Up;
	else if (arg == "d" || arg == "down" || arg == "downcross") return ShiftDirection::Down;
	else return {};
}

PHLWINDOW direction_select(std::string arg){
	PHLWINDOW pTempClient = Desktop::focusState()->window();
	auto dataSize =  g_pCompositor->m_windows.size();
	auto pTempCWindows = new PHLWINDOW[dataSize + 1];
	PHLWINDOW pTempFocusCWindows = nullptr;
	int last = -1;
	if(!pTempClient){
		delete[] pTempCWindows;
		return nullptr;
	}else if (pTempClient->isFullscreen()){
		delete[] pTempCWindows;
		return nullptr;
	}

    if (!isDirectionArg(arg)) {
        hycov_log(Log::ERR, "Cannot move focus in direction {}, unsupported direction. Supported: l/left/leftcross,r/right/rightcross,u/up/upcross,d/down/downcross", arg);
		delete[] pTempCWindows;
        return nullptr;
    }

    for (auto &w : g_pCompositor->m_windows)
    {
		PHLWINDOW pWindow = w;

        if (pTempClient == pWindow || pWindow->isHidden() || !pWindow->m_isMapped || pWindow->m_fadingOut || pWindow->isFullscreen()) {
			continue;
		}
        
		auto pMonitor = pWindow->m_monitor.lock();

		if (!((isCrossMonitor(arg) && pWindow->monitorID() != pTempClient->monitorID() && !pTempClient->m_workspace->m_isSpecialWorkspace && pWindow->m_workspace == pMonitor->m_activeWorkspace ) || pTempClient->m_workspace == pWindow->m_workspace)) {
			continue;
		}
			
		last++;
		pTempCWindows[last] = pWindow;			
    }
	
  	if (last < 0) {
		delete[] pTempCWindows;
  		return nullptr;
	}
  	int sel_x = pTempClient->m_realPosition->goal().x;
  	int sel_y = pTempClient->m_realPosition->goal().y;
  	long long int distance = LLONG_MAX;;
  	// int temp_focus = 0;

	auto values = CVarList(arg);
	auto shift = parseShiftArg(values[0]);
  	switch (shift.value()) {
  	case ShiftDirection::Up:
		// Find the window with the closest coordinates 
		// in the top left corner of the window (is limited to same x)
  		for (int _i = 0; _i <= last; _i++) {
  		  if (pTempCWindows[_i]->m_realPosition->goal().y < sel_y && pTempCWindows[_i]->m_realPosition->goal().x == sel_x) {
  		    int dis_x = pTempCWindows[_i]->m_realPosition->goal().x - sel_x;
  		    int dis_y = pTempCWindows[_i]->m_realPosition->goal().y - sel_y;
  		    long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; 
  		    if (tmp_distance < distance) {
  		      distance = tmp_distance;
  		      pTempFocusCWindows = pTempCWindows[_i];
  		    }
  		  }
  		}
		// if find nothing above 
		// find again(is unlimited to x)
		if(!pTempFocusCWindows){
  			for (int _i = 0; _i <= last; _i++) {
  			  if (pTempCWindows[_i]->m_realPosition->goal().y < sel_y ) {
  			    int dis_x = pTempCWindows[_i]->m_realPosition->goal().x - sel_x;
  			    int dis_y = pTempCWindows[_i]->m_realPosition->goal().y - sel_y;
  			    long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; 
  			    if (tmp_distance < distance) {
  			      distance = tmp_distance;
  			      pTempFocusCWindows = pTempCWindows[_i];
  			    }
  			  }
  			}		
		}
  		break;
  	case ShiftDirection::Down:
  		for (int _i = 0; _i <= last; _i++) {
  		  if (pTempCWindows[_i]->m_realPosition->goal().y > sel_y && pTempCWindows[_i]->m_realPosition->goal().x == sel_x) {
  		    int dis_x = pTempCWindows[_i]->m_realPosition->goal().x - sel_x;
  		    int dis_y = pTempCWindows[_i]->m_realPosition->goal().y - sel_y;
  		    long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; 
  		    if (tmp_distance < distance) {
  		      distance = tmp_distance;
  		      pTempFocusCWindows = pTempCWindows[_i];
  		    }
  		  }
  		}
		if(!pTempFocusCWindows){
  			for (int _i = 0; _i <= last; _i++) {
  			  if (pTempCWindows[_i]->m_realPosition->goal().y > sel_y ) {
  			    int dis_x = pTempCWindows[_i]->m_realPosition->goal().x - sel_x;
  			    int dis_y = pTempCWindows[_i]->m_realPosition->goal().y - sel_y;
  			    long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; 
  			    if (tmp_distance < distance) {
  			      distance = tmp_distance;
  			      pTempFocusCWindows = pTempCWindows[_i];
  			    }
  			  }
  			}		
		}
  		break;
  	case ShiftDirection::Left:
  		for (int _i = 0; _i <= last; _i++) {
  		  if (pTempCWindows[_i]->m_realPosition->goal().x < sel_x && pTempCWindows[_i]->m_realPosition->goal().y == sel_y) {
  		    int dis_x = pTempCWindows[_i]->m_realPosition->goal().x - sel_x;
  		    int dis_y = pTempCWindows[_i]->m_realPosition->goal().y - sel_y;
  		    long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; 
  		    if (tmp_distance < distance) {
  		      distance = tmp_distance;
  		      pTempFocusCWindows = pTempCWindows[_i];
  		    }
  		  }
  		}
		if(!pTempFocusCWindows){
  			for (int _i = 0; _i <= last; _i++) {
  			  if (pTempCWindows[_i]->m_realPosition->goal().x < sel_x) {
  			    int dis_x = pTempCWindows[_i]->m_realPosition->goal().x - sel_x;
  			    int dis_y = pTempCWindows[_i]->m_realPosition->goal().y - sel_y;
  			    long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; 
  			    if (tmp_distance < distance) {
  			      distance = tmp_distance;
  			      pTempFocusCWindows = pTempCWindows[_i];
  			    }
  			  }
  			}		
		}
  		break;
  	case ShiftDirection::Right:
  		for (int _i = 0; _i <= last; _i++) {
  		  if (pTempCWindows[_i]->m_realPosition->goal().x > sel_x  && pTempCWindows[_i]->m_realPosition->goal().y == sel_y) {
  		    int dis_x = pTempCWindows[_i]->m_realPosition->goal().x - sel_x;
  		    int dis_y = pTempCWindows[_i]->m_realPosition->goal().y - sel_y;
  		    long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; 
  		    if (tmp_distance < distance) {
  		      distance = tmp_distance;
  		      pTempFocusCWindows = pTempCWindows[_i];
  		    }
  		  }
  		}
		if(!pTempFocusCWindows){
  			for (int _i = 0; _i <= last; _i++) {
  			  if (pTempCWindows[_i]->m_realPosition->goal().x > sel_x) {
  			    int dis_x = pTempCWindows[_i]->m_realPosition->goal().x - sel_x;
  			    int dis_y = pTempCWindows[_i]->m_realPosition->goal().y - sel_y;
  			    long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; 
  			    if (tmp_distance < distance) {
  			      distance = tmp_distance;
  			      pTempFocusCWindows = pTempCWindows[_i];
  			    }
  			  }
  			}		
		}
  		break;
  	}
	delete[] pTempCWindows;
  	return pTempFocusCWindows;
}

PHLWINDOW get_circle_next_window (std::string arg) {
	bool next_ready = false;
	PHLWINDOW pTempClient = Desktop::focusState()->window();

	if(!pTempClient)
		return nullptr;

    for (auto &w : g_pCompositor->m_windows)
    {
		PHLWINDOW pWindow = w;
        if (pTempClient->m_workspace != pWindow->m_workspace || pWindow->isHidden() || !pWindow->m_isMapped || pWindow->m_fadingOut || pWindow->isFullscreen())
            continue;
		if (next_ready)
			return 	pWindow;
		if (pWindow == pTempClient)
			next_ready = true;	
    }

    for (auto &w : g_pCompositor->m_windows)
    {
		PHLWINDOW pWindow = w;
        if (pTempClient->m_workspace != pWindow->m_workspace || pWindow->isHidden() || !pWindow->m_isMapped || pWindow->m_fadingOut || pWindow->isFullscreen())
            continue;
		return pWindow;
    }
	return nullptr;
}

void warpcursor_and_focus_to_window(PHLWINDOW pWindow) {
	Desktop::focusState()->fullWindowFocus(pWindow);
	g_pCompositor->warpCursorTo(pWindow->middle());
}

void dispatch_circle(std::string arg)
{
	PHLWINDOW pWindow;
	pWindow = get_circle_next_window(arg);
	if(pWindow){
		warpcursor_and_focus_to_window(pWindow);
	}
}

void dispatch_focusdir(std::string arg)
{
	PHLWINDOW pWindow;
	pWindow = direction_select(arg);
	if(pWindow){
		warpcursor_and_focus_to_window(pWindow);
	}
}

void dispatch_toggleoverview(std::string arg)
{
	if (g_hycov_isOverView && (!g_hycov_enable_alt_release_exit || arg == "internalToggle")) {
		dispatch_leaveoverview("");
		hycov_log(LOG,"leave overview:toggleMethod:{},enable_alt_release_exit:{}",arg,g_hycov_enable_alt_release_exit);
	} else if (g_hycov_isOverView && g_hycov_enable_alt_release_exit && arg != "internalToggle") {
		dispatch_circle("");
		hycov_log(LOG,"toggle overview:switch focus circlely");
	} else if(g_hycov_enable_alt_release_exit && g_hycov_alt_toggle_auto_next && arg != "internalToggle") {
		dispatch_enteroverview(arg);
		dispatch_circle("");
		hycov_log(LOG,"enter overview:alt switch mode auto next");
	} else {
		dispatch_enteroverview(arg);
		hycov_log(LOG,"enter overview:toggleMethod:{}",arg);
	}
}

void dispatch_enteroverview(std::string arg)
{ 
	if(g_hycov_isOverView) {
		return;
	}

	const auto pMonitor = Desktop::focusState()->monitor();
	if(pMonitor->activeSpecialWorkspaceID() != 0)
		pMonitor->setSpecialWorkspace(nullptr);

	//force display all workspace window,ignore `only_active_worksapce` and `only_active_monitor`
	if (arg == "forceall") {
		g_hycov_forece_display_all = true;
		hycov_log(LOG,"force display all clients");
	} else {
		g_hycov_forece_display_all = false;
	}

	//force display all workspace window in one monitor,ignore `only_active_worksapce` and `only_active_monitor`
	if (arg == "forceallinone") {
		g_hycov_forece_display_all_in_one_monitor = true;
		hycov_log(LOG,"force display all clients in one monitor");
	} else {
		g_hycov_forece_display_all_in_one_monitor = false;
	}

	//force only display current workspace,ignore `only_active_worksapce` and `only_active_monitor`
	if (arg == "onlycurrentworkspace") {
		g_hycov_force_display_only_current_workspace = true;
		hycov_log(LOG,"force display only current workspace");
	} else {
		g_hycov_force_display_only_current_workspace = false;
	}

	//ali clients exit fullscreen status before enter overview
	PHLWINDOW pFullscreenWindow;
	PHLWINDOW pActiveWindow = Desktop::focusState()->window();
	PHLWORKSPACE pActiveWorkspace;
	PHLMONITOR pActiveMonitor;

	bool isNoShouldTileWindow = true;

    for (auto &w : g_pCompositor->m_windows)
    {
		PHLWINDOW pWindow = w;
        if (pWindow->isHidden() || !pWindow->m_isMapped || pWindow->m_fadingOut || pWindow->m_workspace->m_isSpecialWorkspace)
            continue;
		isNoShouldTileWindow = false;
	}

	//if no clients, forbit enter overview 
	if(isNoShouldTileWindow){
		return;
	}

	hycov_log(LOG,"enter overview");
	g_hycov_isOverView = true;

	//make all fullscreen window exit fullscreen state
	for (auto &w : g_pCompositor->getWorkspaces())
	{
		PHLWORKSPACE pWorkspace = w.lock();
		if (pWorkspace && pWorkspace->m_hasFullscreenWindow)
		{
			pFullscreenWindow = pWorkspace->getFullscreenWindow();
			g_pCompositor->setWindowFullscreenInternal(pFullscreenWindow, FSMODE_NONE);

			//let overview know the client is a fullscreen before - tracked in our node data
		}
	}

	//enter overview layout
	// g_pLayoutManager->switchToLayout("ovgrid");
	switchToLayoutWithoutReleaseData("ovgrid");
	g_pLayoutManager->getCurrentLayout()->onEnable();

	//change workspace name to OVERVIEW
	pActiveMonitor	= Desktop::focusState()->monitor();
	pActiveWorkspace = pActiveMonitor->m_activeWorkspace;
	workspaceNameBackup = pActiveWorkspace->m_name;
	workspaceIdBackup = pActiveWorkspace->m_id;
	pActiveWorkspace->rename(overviewWorksapceName);

	//Preserve window focus
	if(pActiveWindow){
		Desktop::focusState()->fullWindowFocus(pActiveWindow); //restore the focus to before active window

	} else { // when no window is showed in current window,find from other workspace to focus(exclude special workspace)
    	for (auto &w : g_pCompositor->m_windows) {
			PHLWINDOW pWindow = w;
			auto node = g_hycov_OvGridLayout->getNodeFromWindow(pWindow);
    	    if ( !node || g_pCompositor->isWorkspaceSpecial(node->workspaceID) || pWindow->isHidden() || !pWindow->m_isMapped || pWindow->m_fadingOut || pWindow->isFullscreen())
    	        continue;
			Desktop::focusState()->fullWindowFocus(pWindow); // find the last window that is in same workspace with the remove window
    	}

	}

	// enable hook fullscreenActive funciton
  	g_hycov_pFullscreenActiveHook->hook();

	//disable changeworkspace
	if(g_hycov_disable_workspace_change) {
  		g_hycov_pChangeworkspaceHook->hook();
		g_hycov_pMoveActiveToWorkspaceHook->hook();
	}

	//disable spawn
	if(g_hycov_disable_spawn) {
		g_hycov_pSpawnHook->hook();
	}

	g_hycov_pCKeybindManager_changeGroupActiveHook->hook();
	g_hycov_pCKeybindManager_toggleGroupHook->hook();
	g_hycov_pCKeybindManager_moveOutOfGroupHook->hook();

	return;
}

void dispatch_leaveoverview(std::string arg)
{ 
	if(!g_hycov_isOverView) {
		return;
	}

	const auto pMonitor = Desktop::focusState()->monitor();
	if(pMonitor->activeSpecialWorkspaceID() != 0)
		pMonitor->setSpecialWorkspace(nullptr);
	
	// get default layout
	std::string *configLayoutName = &g_hycov_configLayoutName;
	
	hycov_log(LOG,"leave overview");
	g_hycov_isOverView = false;
	//mark exiting overview mode
	g_hycov_isOverViewExiting = true;
	
	//restore workspace name
	if(auto pWorkspace = g_pCompositor->getWorkspaceByID(workspaceIdBackup))
		pWorkspace->rename(workspaceNameBackup);

	//enable changeworkspace
	if(g_hycov_disable_workspace_change) {
  		g_hycov_pChangeworkspaceHook->unhook();
		g_hycov_pMoveActiveToWorkspaceHook->unhook();
	}

	//enable spawn
	if(g_hycov_disable_spawn) {
		g_hycov_pSpawnHook->unhook();
	}

	g_hycov_pCKeybindManager_changeGroupActiveHook->unhook();
	g_hycov_pCKeybindManager_toggleGroupHook->unhook();
	g_hycov_pCKeybindManager_moveOutOfGroupHook->unhook();

	// if no clients, just exit overview, don't restore client's state
	if (g_hycov_OvGridLayout->m_lOvGridNodesData.empty())
	{
		switchToLayoutWithoutReleaseData(*configLayoutName);
		recalculateAllMonitor();
		g_hycov_OvGridLayout->m_lOvGridNodesData.clear();
		g_hycov_isOverViewExiting = false;
		return;
	}

	//move clients to it's original workspace 
	g_hycov_OvGridLayout->moveWindowToSourceWorkspace();
	// go to the workspace where the active client was before
	g_hycov_OvGridLayout->changeToActivceSourceWorkspace();
	
	for (auto &n : g_hycov_OvGridLayout->m_lOvGridNodesData)
	{	
			if (n.ovbk_windowIsFloating)
		{
			//make floating client restore it's floating status
			n.pWindow->m_isFloating = true;
			g_pLayoutManager->getCurrentLayout()->onWindowCreatedFloating(n.pWindow);

			// make floating client restore it's position and size
			*n.pWindow->m_realSize = n.ovbk_size;
			*n.pWindow->m_realPosition = n.ovbk_position;

			auto calcPos = n.ovbk_position;
			auto calcSize = n.ovbk_size;

			*n.pWindow->m_realSize = calcSize;
			*n.pWindow->m_realPosition = calcPos;

			n.pWindow->sendWindowSize(true);

		} else if(!n.ovbk_windowIsFloating && !n.ovbk_windowIsFullscreen) {
			// make nofloating client restore it's position and size
			*n.pWindow->m_realSize = n.ovbk_size;
			*n.pWindow->m_realPosition = n.ovbk_position;

			// auto calcPos = n.ovbk_position;
			// auto calcSize = n.ovbk_size;

			// n.pWindow->m_vRealSize = calcSize;
			// n.pWindow->m_vRealPosition = calcPos;

			// some app sometime can't catch window size to restore,don't use dirty data,remove refer data in old layout.
			if (n.ovbk_size.x == 0 && n.ovbk_size.y == 0 && n.isInOldLayout) {
				g_hycov_OvGridLayout->removeOldLayoutData(n.pWindow);
				n.isInOldLayout = false;
			} else {
				n.pWindow->sendWindowSize(true);	
			}	

			// restore active window in group
			if(n.isGroupActive) {
				n.pWindow->setGroupCurrent(n.pWindow);
			}	
		}
	}

	//exit overview layout,go back to old layout
	PHLWINDOW pActiveWindow = Desktop::focusState()->window();
	Desktop::focusState()->rawWindowFocus(nullptr);
	// g_pLayoutManager->switchToLayout(*configLayoutName);
	// g_pLayoutManager->getCurrentLayout()->onDisable();
	switchToLayoutWithoutReleaseData(*configLayoutName);
	recalculateAllMonitor();

	//Preserve window focus
	if(pActiveWindow){
		if(g_hycov_forece_display_all_in_one_monitor && pActiveWindow->monitorID() != Desktop::focusState()->monitor()->m_id) {
			warpcursor_and_focus_to_window(pActiveWindow); //restore the focus to before active window.when cross monitor,warpcursor to monitor of current active window is in
		} else {
			Desktop::focusState()->fullWindowFocus(pActiveWindow); //restore the focus to before active window
		}

		if(pActiveWindow->m_isFloating && g_hycov_raise_float_to_top) {
			g_pCompositor->changeWindowZOrder(pActiveWindow, true);
		} else if(g_hycov_auto_fullscreen && want_auto_fullscren(pActiveWindow)) { // if enale auto_fullscreen after exit overview
			g_pCompositor->setWindowFullscreenInternal(pActiveWindow, FSMODE_MAXIMIZED);
		}
	}

	for (auto &n : g_hycov_OvGridLayout->m_lOvGridNodesData)
	{
		//make all fullscrenn windwo restore it's status
		if (n.ovbk_windowIsFullscreen)
		{
			if (!Desktop::focusState()->window()) {
				continue;
			}

			if (n.pWindow != Desktop::focusState()->window() && n.pWindow->m_workspace == Desktop::focusState()->window()->m_workspace)
			{
				continue;
			}	
			g_pCompositor->setWindowFullscreenInternal(n.pWindow, n.ovbk_windowFullscreenMode);
		}
	}

	for (auto &n : g_hycov_OvGridLayout->m_lOvGridNodesData)
	{
		// if client not in old layout,create tiling of the client
		if(!n.isInOldLayout)
		{
			if (n.pWindow->m_fadingOut || !n.pWindow->m_isMapped || n.pWindow->isHidden()) {
				continue;
			}
			hycov_log(LOG,"create tiling window in old layout,window:{},workspace:{},inoldlayout:{}",n.pWindow,n.workspaceID,n.isInOldLayout);
			g_pLayoutManager->getCurrentLayout()->onWindowCreatedTiling(n.pWindow);
		}
		// restore active window in group
		if(n.isGroupActive) {
			n.pWindow->setGroupCurrent(n.pWindow);
		}	
	}

	//clean overview layout node date
	g_hycov_OvGridLayout->m_lOvGridNodesData.clear();

	//mark has exited overview mode
	g_hycov_isOverViewExiting = false;

	// disable hook fullscreenActive funciton
  	g_hycov_pFullscreenActiveHook->unhook();

	// Apply pending move from drag-to-monitor
	if (g_hycov_pendingMoveWindow && g_hycov_pendingMoveMonitor) {
		hycov_log(LOG, "Applying pending move to monitor {}", g_hycov_pendingMoveMonitor->m_id);
		
		Desktop::focusState()->fullWindowFocus(g_hycov_pendingMoveWindow);
		
		std::string moveArg = "mon:" + std::to_string(g_hycov_pendingMoveMonitor->m_id);
		g_pKeybindManager->m_dispatchers["movewindow"](moveArg);
		
		g_pLayoutManager->getCurrentLayout()->recalculateMonitor(g_hycov_pendingMoveMonitor->m_id);
		g_pLayoutManager->getCurrentLayout()->recalculateMonitor(g_hycov_dragStartMonitor);
		
		g_hycov_pendingMoveWindow = nullptr;
		g_hycov_pendingMoveMonitor = nullptr;
	}

	return;
}

SDispatchResult dispatch_enteroverview_v2(std::string arg) {
	dispatch_enteroverview(arg);
	return {};
}

SDispatchResult dispatch_leaveoverview_v2(std::string arg) {
	dispatch_leaveoverview(arg);
	return {};
}

SDispatchResult dispatch_toggleoverview_v2(std::string arg) {
	dispatch_toggleoverview(arg);
	return {};
}

SDispatchResult dispatch_focusdir_v2(std::string arg) {
	dispatch_focusdir(arg);
	return {};
}

void registerDispatchers()
{
	HyprlandAPI::addDispatcherV2(PHANDLE, "hycov:enteroverview", dispatch_enteroverview_v2);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hycov:leaveoverview", dispatch_leaveoverview_v2);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hycov:toggleoverview", dispatch_toggleoverview_v2);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hycov:movefocus", dispatch_focusdir_v2);
}
