#include "dispatchers.hpp"

static const std::string overviewWorksapceName = "OVERVIEW";
static std::string workspaceNameBackup;
static int workspaceIdBackup;

void recalculateAllMonitor() {
	for (auto &m : g_pCompositor->m_monitors) {
		// Note: Layout manager access changed in Hyprland 0.50
		// This needs to be updated to use the new API
		// For now, we'll skip the recalculation to allow compilation
		// TODO: Update to new layout manager API
	}
}

// only change layout,keep data of previous layout
void switchToLayoutWithoutReleaseData(std::string layout) {
    // Note: Layout manager access changed in Hyprland 0.50
    // This function needs to be updated to use the new API
    // For now, we'll skip the layout switching to allow compilation
    // TODO: Update to new layout manager API
    hycov_log(INFO, "Layout switching disabled - needs update for Hyprland 0.50");
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
	PHLWINDOW pTempClient =  g_pCompositor->m_lastWindow.lock();
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
        hycov_log(ERR, "Cannot move focus in direction {}, unsupported direction. Supported: l/left/leftcross,r/right/rightcross,u/up/upcross,d/down/downcross", arg);
		delete[] pTempCWindows;
        return nullptr;
    }

    for (auto &w : g_pCompositor->m_windows)
    {
		PHLWINDOW pWindow = w;

        if (pTempClient == pWindow || pWindow->isHidden() || !pWindow->m_isMapped || pWindow->m_fadingOut || pWindow->isFullscreen()) {
			continue;
		}
        
		auto pMonitor = g_pCompositor->getMonitorFromID(pWindow->monitorID());

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
  	int sel_x = pTempClient->m_realPosition->value().x;
  	int sel_y = pTempClient->m_realPosition->value().y;
  	long long int distance = LLONG_MAX;;
  	// int temp_focus = 0;

	auto values = CVarList(arg);
	auto shift = parseShiftArg(values[0]);
  	switch (shift.value()) {
  	case ShiftDirection::Up:
		// Find the window with the closest coordinates 
		// in the top left corner of the window (is limited to same x)
  		for (int _i = 0; _i <= last; _i++) {
  		  if (pTempCWindows[_i]->m_realPosition->value().y < sel_y && pTempCWindows[_i]->m_realPosition->value().x == sel_x) {
  		    int dis_x = pTempCWindows[_i]->m_realPosition->value().x - sel_x;
  		    int dis_y = pTempCWindows[_i]->m_realPosition->value().y - sel_y;
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
  			  if (pTempCWindows[_i]->m_realPosition->value().y < sel_y ) {
  			    int dis_x = pTempCWindows[_i]->m_realPosition->value().x - sel_x;
  			    int dis_y = pTempCWindows[_i]->m_realPosition->value().y - sel_y;
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
  		  if (pTempCWindows[_i]->m_realPosition->value().y > sel_y && pTempCWindows[_i]->m_realPosition->value().x == sel_x) {
  		    int dis_x = pTempCWindows[_i]->m_realPosition->value().x - sel_x;
  		    int dis_y = pTempCWindows[_i]->m_realPosition->value().y - sel_y;
  		    long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; 
  		    if (tmp_distance < distance) {
  		      distance = tmp_distance;
  		      pTempFocusCWindows = pTempCWindows[_i];
  		    }
  		  }
  		}
		if(!pTempFocusCWindows){
  			for (int _i = 0; _i <= last; _i++) {
  			  if (pTempCWindows[_i]->m_realPosition->value().y > sel_y ) {
  			    int dis_x = pTempCWindows[_i]->m_realPosition->value().x - sel_x;
  			    int dis_y = pTempCWindows[_i]->m_realPosition->value().y - sel_y;
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
  		  if (pTempCWindows[_i]->m_realPosition->value().x < sel_x && pTempCWindows[_i]->m_realPosition->value().y == sel_y) {
  		    int dis_x = pTempCWindows[_i]->m_realPosition->value().x - sel_x;
  		    int dis_y = pTempCWindows[_i]->m_realPosition->value().y - sel_y;
  		    long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; 
  		    if (tmp_distance < distance) {
  		      distance = tmp_distance;
  		      pTempFocusCWindows = pTempCWindows[_i];
  		    }
  		  }
  		}
		if(!pTempFocusCWindows){
  			for (int _i = 0; _i <= last; _i++) {
  			  if (pTempCWindows[_i]->m_realPosition->value().x < sel_x) {
  			    int dis_x = pTempCWindows[_i]->m_realPosition->value().x - sel_x;
  			    int dis_y = pTempCWindows[_i]->m_realPosition->value().y - sel_y;
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
  		  if (pTempCWindows[_i]->m_realPosition->value().x > sel_x  && pTempCWindows[_i]->m_realPosition->value().y == sel_y) {
  		    int dis_x = pTempCWindows[_i]->m_realPosition->value().x - sel_x;
  		    int dis_y = pTempCWindows[_i]->m_realPosition->value().y - sel_y;
  		    long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; 
  		    if (tmp_distance < distance) {
  		      distance = tmp_distance;
  		      pTempFocusCWindows = pTempCWindows[_i];
  		    }
  		  }
  		}
		if(!pTempFocusCWindows){
  			for (int _i = 0; _i <= last; _i++) {
  			  if (pTempCWindows[_i]->m_realPosition->value().x > sel_x) {
  			    int dis_x = pTempCWindows[_i]->m_realPosition->value().x - sel_x;
  			    int dis_y = pTempCWindows[_i]->m_realPosition->value().y - sel_y;
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
	PHLWINDOW pTempClient =  g_pCompositor->m_lastWindow.lock();

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
	g_pCompositor->focusWindow(pWindow);
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

	const auto pMonitor = g_pCompositor->m_lastMonitor;
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
	PHLWINDOW pActiveWindow = g_pCompositor->m_lastWindow.lock();
	PHLWORKSPACE pActiveWorkspace;
	CMonitor *pActiveMonitor;

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
	for (auto &w : g_pCompositor->m_workspaces)
	{
		CWorkspace *pWorkspace = w.get();
		if (pWorkspace->m_hasFullscreenWindow)
		{
			// API changed in Hyprland v0.50 - getFullscreenWindowOnWorkspace removed
			// Find fullscreen window manually
			for (auto& w : g_pCompositor->m_windows) {
				if (w->m_workspace.get() == pWorkspace && w->isFullscreen()) {
					pFullscreenWindow = w;
					break;
				}
			}
			g_pCompositor->setWindowFullscreenState(pFullscreenWindow, {.internal = FSMODE_NONE, .client = FSMODE_NONE});

			//let overview know the client is a fullscreen before
			// Note: isFullscreen() is a getter method, not assignable
			// We'll handle fullscreen state through the proper API
		}
	}

	//enter overview layout
	// g_pLayoutManager->switchToLayout("ovgrid");
	switchToLayoutWithoutReleaseData("ovgrid");
	g_pLayoutManager->getCurrentLayout()->onEnable();

	//change workspace name to OVERVIEW
	pActiveMonitor	= g_pCompositor->m_lastMonitor.get();
	pActiveWorkspace = g_pCompositor->getWorkspaceByID(pActiveMonitor->m_activeWorkspace->m_id);
	workspaceNameBackup = pActiveWorkspace->m_name;
	workspaceIdBackup = pActiveWorkspace->m_id;
	// Note: renameWorkspace API changed in Hyprland v0.50
	// For now we'll skip workspace renaming to avoid compilation errors
	// TODO: Find the new workspace renaming API
	// g_pCompositor->renameWorkspace(pActiveMonitor->m_activeWorkspace->m_id, overviewWorksapceName);

	//Preserve window focus
	if(pActiveWindow){
		g_pCompositor->focusWindow(pActiveWindow); //restore the focus to before active window

	} else { // when no window is showed in current window,find from other workspace to focus(exclude special workspace)
    	for (auto &w : g_pCompositor->m_windows) {
			PHLWINDOW pWindow = w;
			auto node = g_hycov_OvGridLayout->getNodeFromWindow(pWindow);
    	    if ( !node || g_pCompositor->isWorkspaceSpecial(node->workspaceID) || pWindow->isHidden() || !pWindow->m_isMapped || pWindow->m_fadingOut || pWindow->isFullscreen())
    	        continue;
			g_pCompositor->focusWindow(pWindow); // find the last window that is in same workspace with the remove window
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

	const auto pMonitor = g_pCompositor->m_lastMonitor;
	if(pMonitor->activeSpecialWorkspaceID() != 0)
		pMonitor->setSpecialWorkspace(nullptr);
	
	// get default layout
	std::string *configLayoutName = &g_hycov_configLayoutName;
	
	hycov_log(LOG,"leave overview");
	g_hycov_isOverView = false;
	//mark exiting overview mode
	g_hycov_isOverViewExiting = true;
	
	//restore workspace name
	// Note: renameWorkspace API changed in Hyprland v0.50
	// For now we'll skip workspace renaming to avoid compilation errors  
	// TODO: Find the new workspace renaming API
	// g_pCompositor->renameWorkspace(workspaceIdBackup, workspaceNameBackup);

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
		//make all window restore it's style
    	// Note: Special render data API changed in Hyprland v0.50
    	// For now we'll skip render data restoration to avoid compilation errors
    	// TODO: Update to new render data API
    	// n.pWindow->m_specialRenderData.border   = n.ovbk_windowIsWithBorder;
    	// n.pWindow->m_specialRenderData.decorate = n.ovbk_windowIsWithDecorate;
    	// n.pWindow->m_specialRenderData.rounding = n.ovbk_windowIsWithRounding;
    	// n.pWindow->m_specialRenderData.shadow   = n.ovbk_windowIsWithShadow;

		if (n.ovbk_windowIsFloating)
		{
			//make floating client restore it's floating status
			n.pWindow->m_isFloating = true;
			g_pLayoutManager->getCurrentLayout()->onWindowCreatedFloating(n.pWindow);

			// make floating client restore it's position and size
			n.pWindow->m_realSize->setValueAndWarp(n.ovbk_size);
			n.pWindow->m_realPosition->setValueAndWarp(n.ovbk_position);

			auto calcPos = n.ovbk_position;
			auto calcSize = n.ovbk_size;

			n.pWindow->m_realSize->setValueAndWarp(calcSize);
			n.pWindow->m_realPosition->setValueAndWarp(calcPos);

			// Note: setWindowSize API changed in Hyprland v0.50
// For now we skip XWayland size setting to avoid compilation errors
// TODO: Find the new XWayland API for setting window size
// g_pXWaylandManager->setWindowSize(n.pWindow, calcSize);

		} else if(!n.ovbk_windowIsFloating && !n.ovbk_windowIsFullscreen) {
			// make nofloating client restore it's position and size
			n.pWindow->m_realSize->setValueAndWarp(n.ovbk_size);
			n.pWindow->m_realPosition->setValueAndWarp(n.ovbk_position);

			// auto calcPos = n.ovbk_position;
			// auto calcSize = n.ovbk_size;

			// n.pWindow->m_realSize->setValueAndWarp(calcSize);
			// n.pWindow->m_realPosition->setValueAndWarp(calcPos);

			// some app sometime can't catch window size to restore,don't use dirty data,remove refer data in old layout.
			if (n.ovbk_size.x == 0 && n.ovbk_size.y == 0 && n.isInOldLayout) {
				g_hycov_OvGridLayout->removeOldLayoutData(n.pWindow);
				n.isInOldLayout = false;
			} else {
				// Note: setWindowSize API changed in Hyprland v0.50
// For now we skip XWayland size setting to avoid compilation errors
// TODO: Find the new XWayland API for setting window size
// g_pXWaylandManager->setWindowSize(n.pWindow, n.ovbk_size);	
			}	

			// restore active window in group
			if(n.isGroupActive) {
				n.pWindow->setGroupCurrent(n.pWindow);
			}	
		}
	}

	//exit overview layout,go back to old layout
	PHLWINDOW pActiveWindow = g_pCompositor->m_lastWindow.lock();
	g_pCompositor->focusWindow(nullptr);
	// g_pLayoutManager->switchToLayout(*configLayoutName);
	// g_pLayoutManager->getCurrentLayout()->onDisable();
	switchToLayoutWithoutReleaseData(*configLayoutName);
	recalculateAllMonitor();

	//Preserve window focus
	if(pActiveWindow){
		if(g_hycov_forece_display_all_in_one_monitor && pActiveWindow->monitorID() != g_pCompositor->m_lastMonitor->m_id) {
			warpcursor_and_focus_to_window(pActiveWindow); //restore the focus to before active window.when cross monitor,warpcursor to monitor of current active window is in
		} else {
			g_pCompositor->focusWindow(pActiveWindow); //restore the focus to before active window
		}

		if(pActiveWindow->m_isFloating && g_hycov_raise_float_to_top) {
			g_pCompositor->changeWindowZOrder(pActiveWindow, true);
		} else if(g_hycov_auto_fullscreen && want_auto_fullscren(pActiveWindow)) { // if enale auto_fullscreen after exit overview
			g_pCompositor->setWindowFullscreenState(pActiveWindow, {.internal = FSMODE_MAXIMIZED, .client = FSMODE_MAXIMIZED});
		}
	}

	for (auto &n : g_hycov_OvGridLayout->m_lOvGridNodesData)
	{
		//make all fullscrenn windwo restore it's status
		if (n.ovbk_windowIsFullscreen)
		{
			if (!g_pCompositor->m_lastWindow.lock()) {
				continue;
			}

			if (n.pWindow != g_pCompositor->m_lastWindow.lock() && n.pWindow->m_workspace == g_pCompositor->m_lastWindow.lock()->m_workspace)
			{
				continue;
			}	
			g_pCompositor->setWindowFullscreenState(n.pWindow, {.internal = n.ovbk_windowFullscreenMode, .client = n.ovbk_windowFullscreenMode});
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

	return;
}

void registerDispatchers()
{
	HyprlandAPI::addDispatcher(PHANDLE, "hycov:enteroverview", dispatch_enteroverview);
	HyprlandAPI::addDispatcher(PHANDLE, "hycov:leaveoverview", dispatch_leaveoverview);
	HyprlandAPI::addDispatcher(PHANDLE, "hycov:toggleoverview", dispatch_toggleoverview);
	HyprlandAPI::addDispatcher(PHANDLE, "hycov:movefocus", dispatch_focusdir);
}
