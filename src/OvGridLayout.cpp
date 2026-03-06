
#include "globals.hpp"
#include "OvGridLayout.hpp"
#include "dispatchers.hpp"

static void forceWindowFloating(PHLWINDOW pWindow, bool floating) {
    if (!pWindow || !pWindow->m_workspace || !pWindow->m_workspace->m_space) {
        return;
    }

    auto target = pWindow->layoutTarget();
    if (target && target->floating() != floating) {
        pWindow->m_workspace->m_space->toggleTargetFloating(target);
    }

    pWindow->m_isFloating = floating;
    pWindow->updateWindowData();
}

static void moveWindowToWorkspaceCompat(PHLWINDOW pWindow, PHLWORKSPACE pWorkspace) {
    if (!pWindow || !pWorkspace) {
        return;
    }

    if (pWindow->m_workspace != pWorkspace) {
        g_pCompositor->moveWindowToWorkspaceSafe(pWindow, pWorkspace);
    }

    if (auto pMonitor = pWorkspace->m_monitor.lock()) {
        pWindow->m_monitor = pMonitor;
    }
}

static void setWindowPinnedForOverview(PHLWINDOW pWindow, bool pinned) {
    if (!pWindow) {
        return;
    }

    pWindow->m_pinned = pinned;
    pWindow->updateWindowData();
    g_pHyprRenderer->damageWindow(pWindow);
}

static void relaxWindowSizeLimitsForOverview(PHLWINDOW pWindow, SOvGridNodeData* pNode) {
    if (!pWindow || !pNode || !pWindow->m_xdgSurface) {
        return;
    }

    const auto xdgSurface = pWindow->m_xdgSurface.lock();
    if (!xdgSurface || !xdgSurface->m_toplevel) {
        return;
    }

    const auto toplevel = xdgSurface->m_toplevel.lock();
    if (!toplevel) {
        return;
    }

    pNode->ovbk_windowHadXDGSizeLimits = true;
    pNode->ovbk_xdgMinSizeCurrent = toplevel->m_current.minSize;
    pNode->ovbk_xdgMaxSizeCurrent = toplevel->m_current.maxSize;
    pNode->ovbk_xdgMinSizePending = toplevel->m_pending.minSize;
    pNode->ovbk_xdgMaxSizePending = toplevel->m_pending.maxSize;

    toplevel->m_current.minSize = {1, 1};
    toplevel->m_current.maxSize = {1337420, 694200};
    toplevel->m_pending.minSize = {1, 1};
    toplevel->m_pending.maxSize = {1337420, 694200};
}

static void placeWindowForOverview(PHLWINDOW pWindow, const Vector2D& position, const Vector2D& size) {
    if (!pWindow) {
        return;
    }

    pWindow->m_position = position;
    pWindow->m_size = size;

    // Overview is a compositor-side preview. Write the rendered box directly
    // and do not route through target geometry, otherwise min/max-size clients
    // such as Electron utility windows keep enforcing their requested size.
    pWindow->m_realSize->setValueAndWarp(size);
    pWindow->m_realPosition->setValueAndWarp(position);
    g_pHyprRenderer->damageWindow(pWindow);

    pWindow->updateWindowDecos();
}

// find next focus window after remove a window
PHLWINDOW OvGridLayout::getNextWindowCandidate(PHLWINDOW plastWindow) {

    PHLWINDOW targetWindow =  nullptr;
    for (auto &w : g_pCompositor->m_windows)
    {
		PHLWINDOW pWindow = w;
        if (pWindow->m_workspace != plastWindow->m_workspace || pWindow->isHidden() || !pWindow->m_isMapped || pWindow->m_fadingOut || pWindow->isFullscreen())
            continue;
		targetWindow = pWindow; // find the last window that is in same workspace with the remove window
    }

    return targetWindow;

}

SOvGridNodeData *OvGridLayout::getNodeFromWindow(PHLWINDOW pWindow)
{
    for (auto &nd : m_lOvGridNodesData)
    {
        if (nd.pWindow == pWindow)
            return &nd;
    }

    return nullptr;
}


SOldLayoutRecordNodeData *OvGridLayout::getOldLayoutRecordNodeFromWindow(PHLWINDOW pWindow)
{
    for (auto &nd : m_lSOldLayoutRecordNodeData)
    {
        if (nd.pWindow == pWindow)
            return &nd;
    }

    return nullptr;
}

int OvGridLayout::getNodesNumOnWorkspace(const WORKSPACEID &ws)
{
    int no = 0;
    for (auto &n : m_lOvGridNodesData)
    {
        if (n.workspaceID == ws)
            no++;
    }

    return no;
}


Vector2D OvGridLayout::predictSizeForNewWindowTiled() {
    return {};
}

void OvGridLayout::resizeNodeSizePos(SOvGridNodeData *node, int x, int y, int width, int height)
{   
    const int groupbar_height_fix = 0;
    node->size = Vector2D(width, height - g_hycov_height_of_titlebar - groupbar_height_fix);
    node->position = Vector2D(x, y + g_hycov_height_of_titlebar + groupbar_height_fix);
    applyNodeDataToWindow(node);
}

void OvGridLayout::onWindowCreatedTiling(PHLWINDOW pWindow, Math::eDirection direction)
{
    PHLMONITOR pTargetMonitor;
    if(g_hycov_forece_display_all_in_one_monitor) {
        pTargetMonitor = Desktop::focusState()->monitor();
    } else {
      pTargetMonitor = pWindow->m_monitor.lock();
    }

    const auto pNode = &m_lOvGridNodesData.emplace_back(); // make a new node in list back

    auto pActiveWorkspace = pTargetMonitor->m_activeWorkspace;

    auto pWindowOriWorkspace = pWindow->m_workspace;

    pNode->workspaceID = pWindow->m_workspace->m_id; // encapsulate window objects as node objects to bind more properties
    pNode->pWindow = pWindow;
    pNode->workspaceName = pWindowOriWorkspace->m_name;
    
    //record the window stats which are used by restore
    pNode->ovbk_windowMonitorId = pWindow->monitorID();
    pNode->ovbk_windowWorkspaceId = pWindow->m_workspace->m_id;
    pNode->ovbk_windowFullscreenMode  = pWindowOriWorkspace->m_fullscreenMode;
    pNode->ovbk_position = pWindow->m_realPosition->goal();
    pNode->ovbk_size = pWindow->m_realSize->goal();
    pNode->ovbk_windowIsFloating = pWindow->m_isFloating;
    pNode->ovbk_windowWasPinned = pWindow->m_pinned;
    pNode->ovbk_windowIsFullscreen = pWindow->isFullscreen();
    pNode->ovbk_windowWorkspaceName = pWindowOriWorkspace->m_name;

    pNode->ovbk_windowIsWithBorder = true;
    pNode->ovbk_windowIsWithDecorate = true;
    pNode->ovbk_windowIsWithRounding = true;
    pNode->ovbk_windowIsWithShadow = true;

    const bool scrollingSourceLayout =
        g_hycov_compat_scrolling_active &&
        (g_hycov_overview_source_layout.empty() ? g_hycov_configLayoutName : g_hycov_overview_source_layout) == "scrolling";


    //change all client(exclude special workspace) to active worksapce 
    if ((!g_pCompositor->isWorkspaceSpecial(pNode->workspaceID) || g_hycov_show_special) && (pWindowOriWorkspace->m_id != pActiveWorkspace->m_id || pWindowOriWorkspace->m_name != pActiveWorkspace->m_name) && (!(g_hycov_only_active_workspace || g_hycov_force_display_only_current_workspace) || g_hycov_forece_display_all || g_hycov_forece_display_all_in_one_monitor)) {
        moveWindowToWorkspaceCompat(pWindow, pActiveWorkspace);
        pNode->workspaceID = pActiveWorkspace->m_id;
        pNode->workspaceName = pActiveWorkspace->m_name;
        pNode->ovbk_movedForOverview = true;
    }

    // clean fullscreen status
    if (pWindow->isFullscreen()) {   
        g_pCompositor->setWindowFullscreenInternal(pWindow, FSMODE_NONE);
    }

    // Pinned floating windows are rendered in a dedicated "always above" pass.
    // Disable pinned while in overview so they only render at their preview box.
    if (pNode->ovbk_windowWasPinned) {
        setWindowPinnedForOverview(pWindow, false);
    }

    relaxWindowSizeLimitsForOverview(pWindow, pNode);

    // Keep overview-managed windows detached from the active tiled layout.
    if (!pWindow->m_isFloating && !scrollingSourceLayout) {
        forceWindowFloating(pWindow, true);
    }

    recalculateMonitor(pWindow->monitorID());    
}


void OvGridLayout::removeOldLayoutData(PHLWINDOW pWindow) { 
    hycov_log(LOG, "removeOldLayoutData noop on 0.54 for window:{}", pWindow);
}

void OvGridLayout::onWindowRemoved(PHLWINDOW pWindow) {
    const auto pNode = getNodeFromWindow(pWindow);

    if (!pNode)
        return;

    if (pWindow->isFullscreen())
        g_pCompositor->setWindowFullscreenInternal(pWindow, FSMODE_NONE);

    onWindowRemovedTiling(pWindow);
}

void OvGridLayout::onWindowRemovedTiling(PHLWINDOW pWindow)
{
    hycov_log(LOG,"remove tiling windwo:{}",pWindow);

    const auto pNode = getNodeFromWindow(pWindow);

    if (!pNode)
        return;

    m_lOvGridNodesData.remove(*pNode);

    if(m_lOvGridNodesData.empty()){
        return;
    }

    recalculateMonitor(pWindow->monitorID());

}

bool OvGridLayout::isWindowTiled(PHLWINDOW pWindow)
{
    return getNodeFromWindow(pWindow) != nullptr;
}

void OvGridLayout::calculateWorkspace(const WORKSPACEID &ws)
{
    const auto pWorksapce = g_pCompositor->getWorkspaceByID(ws); 
    auto dataSize = m_lOvGridNodesData.size();
    auto pTempNodes = new SOvGridNodeData*[dataSize + 1];
    SOvGridNodeData *pNode;
    int i, n = 0;
    int cx, cy;
    int dx, cw, ch;;
    int cols, rows, overcols,NODECOUNT;

    if (!pWorksapce) {
        delete[] pTempNodes;
        return;
    }

    NODECOUNT = getNodesNumOnWorkspace(pWorksapce->m_id);          
    const auto pMonitor = pWorksapce->m_monitor.lock(); 

    if (NODECOUNT == 0) {
        delete[] pTempNodes;
        return;
    }

    static const auto *PBORDERSIZE = &g_hycov_bordersize;
    static const auto *GAPPO = &g_hycov_overview_gappo;
    static const auto *GAPPI = &g_hycov_overview_gappi;

    /*
    m is region that is moniotr,
    w is region that is monitor but don not contain bar  
    */
    const auto RESERVED = pMonitor->logicalBoxMinusReserved();
    int m_x = pMonitor->m_position.x;
    int m_y = pMonitor->m_position.y;
    int w_x = RESERVED.x;
    int w_y = RESERVED.y;
    int m_width = pMonitor->m_size.x;
    int m_height = pMonitor->m_size.y;
    int w_width = RESERVED.w;
    int w_height = RESERVED.h;

    for (auto &node : m_lOvGridNodesData)
    {
        if (node.workspaceID == ws)
        {
            pTempNodes[n] = &node;
            n++;
        }
    }

    pTempNodes[n] = NULL;

    if (NODECOUNT == 0) {
        delete[] pTempNodes;
        return;
    }

    // one client arrange
    if (NODECOUNT == 1)
    {
        pNode = pTempNodes[0];
        cw = (w_width - 2 * (*GAPPO)) * 0.7;
        ch = (w_height - 2 * (*GAPPO)) * 0.8;
        resizeNodeSizePos(pNode, w_x + (int)((m_width - cw) / 2), w_y + (int)((w_height - ch) / 2),
                          cw - 2 * (*PBORDERSIZE), ch - 2 * (*PBORDERSIZE));
        delete[] pTempNodes;
        return;
    }

    // two client arrange
    if (NODECOUNT == 2)
    {
        pNode = pTempNodes[0];
        cw = (w_width - 2 * (*GAPPO) - (*GAPPI)) / 2;
        ch = (w_height - 2 * (*GAPPO)) * 0.65;
        resizeNodeSizePos(pNode, m_x + cw + (*GAPPO) + (*GAPPI), m_y + (m_height - ch) / 2 + (*GAPPO),
                          cw - 2 * (*PBORDERSIZE), ch - 2 * (*PBORDERSIZE));
        resizeNodeSizePos(pTempNodes[1], m_x + (*GAPPO), m_y + (m_height - ch) / 2 + (*GAPPO),
                          cw - 2 * (*PBORDERSIZE), ch - 2 * (*PBORDERSIZE));
        delete[] pTempNodes;
        return;
    }

    //more than two client arrange

    //Calculate the integer part of the square root of the number of nodes
    for (cols = 0; cols <= NODECOUNT / 2; cols++)
        if (cols * cols >= NODECOUNT)
            break;
            
    //The number of rows and columns multiplied by the number of nodes
    // must be greater than the number of nodes to fit all the Windows
    rows = (cols && (cols - 1) * cols >= NODECOUNT) ? cols - 1 : cols;

    //Calculate the width and height of the layout area based on 
    //the number of rows and columns
    ch = (int)((w_height - 2 * (*GAPPO) - (rows - 1) * (*GAPPI)) / rows);
    cw = (int)((w_width - 2 * (*GAPPO) - (cols - 1) * (*GAPPI)) / cols);

    //If the nodes do not exactly fill all rows, 
    //the number of Windows in the unfilled rows is
    overcols = NODECOUNT % cols;

    if (overcols)
        dx = (int)((w_width - overcols * cw - (overcols - 1) * (*GAPPI)) / 2) - (*GAPPO);
    for (i = 0, pNode = pTempNodes[0]; pNode; pNode = pTempNodes[i + 1], i++)
    {
        cx = w_x + (i % cols) * (cw + (*GAPPI));
        cy = w_y + (int)(i / cols) * (ch + (*GAPPI));
        if (overcols && i >= (NODECOUNT-overcols))
        {
            cx += dx;
        }
        resizeNodeSizePos(pNode, cx + (*GAPPO), cy + (*GAPPO), cw - 2 * (*PBORDERSIZE), ch - 2 * (*PBORDERSIZE));
    }
    delete[] pTempNodes;
}

void OvGridLayout::recalculateMonitor(const MONITORID &monid)
{
    const auto pMonitor = g_pCompositor->getMonitorFromID(monid);                       // 根据monitor id获取monitor对象

    if(!pMonitor || !pMonitor->m_activeWorkspace)
        return;

    g_pHyprRenderer->damageMonitor(pMonitor); // Use local rendering

    if (pMonitor->activeSpecialWorkspaceID()) {
        calculateWorkspace(pMonitor->activeSpecialWorkspaceID());
        return;
    }

    const auto pWorksapce = g_pCompositor->getWorkspaceByID(pMonitor->activeWorkspaceID()); // 获取当前workspace对象
    if (!pWorksapce)
        return;

    calculateWorkspace(pWorksapce->m_id); // calculate windwo's size and position
}

// set window's size and position
void OvGridLayout::applyNodeDataToWindow(SOvGridNodeData *pNode)
{ 

    const auto pWindow = pNode->pWindow;

    // force disable decorate and shadow
    // pWindow->m_sSpecialRenderData.decorate = false;
    // pWindow->m_sSpecialRenderData.shadow   = false;

    // force enable bordear and rounding (Note: These APIs may have changed in new Hyprland)
    // pWindow->m_sSpecialRenderData.border   = true;
    // pWindow->m_sSpecialRenderData.rounding = true;

    // Overview should visually place windows without asking the client to
    // reconfigure. This keeps min/max-size constrained windows shrinkable.
    placeWindowForOverview(pWindow, pNode->position, pNode->size);
}

void OvGridLayout::recalculateWindow(PHLWINDOW pWindow)
{
    ; // empty
}


void OvGridLayout::resizeActiveWindow(const Vector2D &pixResize, Layout::eRectCorner corner, PHLWINDOW pWindow)
{
    ; // empty
}

void OvGridLayout::fullscreenRequestForWindow(PHLWINDOW pWindow, const eFullscreenMode currentMode, const eFullscreenMode effectiveMode)
{
    ; // empty
}

void OvGridLayout::switchWindows(PHLWINDOW pWindowA, PHLWINDOW pWindowB)
{
    ; // empty
}

void OvGridLayout::alterSplitRatio(PHLWINDOW pWindow, float delta, bool exact)
{
    ; // empty
}

std::string OvGridLayout::getLayoutName()
{
    return "ovgrid";
}

void OvGridLayout::replaceWindowDataWith(PHLWINDOW from, PHLWINDOW to)
{
    ; // empty
}

void OvGridLayout::moveWindowTo(PHLWINDOW , const std::string &dir, bool silent)
{
    ; // empty
}

void OvGridLayout::changeToActivceSourceWorkspace()
{
    PHLWINDOW pWindow = nullptr;
    SOvGridNodeData *pNode;
    PHLWORKSPACE pWorksapce;
    hycov_log(LOG,"changeToActivceSourceWorkspace");
    pWindow = Desktop::focusState()->window();
    pNode = getNodeFromWindow(pWindow);
    if(pNode) {
        pWorksapce = g_pCompositor->getWorkspaceByID(pNode->ovbk_windowWorkspaceId); 
    } else if(pWindow) {
        pWorksapce = pWindow->m_workspace; 
    } else {
        pWorksapce = Desktop::focusState()->monitor()->m_activeWorkspace;
    }
    if (!pWorksapce) {
        return;
    }

    auto pMonitor = g_pCompositor->getMonitorFromID(pWorksapce->monitorID());
    if (!pMonitor) {
        return;
    }

    hycov_log(LOG,"changeToWorkspace:{}",pWorksapce->m_id);
    pMonitor->changeWorkspace(pWorksapce, false, false, false);
}

std::pair<int, int> OvGridLayout::moveWindowToSourceWorkspace()
{
    PHLWORKSPACE pWorkspace;
    int restored = 0;
    int failed = 0;
    
    hycov_log(LOG,"moveWindowToSourceWorkspace");

    for (auto &nd : m_lOvGridNodesData)
    {
        if (!nd.pWindow) {
            continue;
        }

        const bool needsRestore = nd.ovbk_movedForOverview || nd.pWindow->m_workspace->m_id != nd.ovbk_windowWorkspaceId || nd.workspaceName != nd.ovbk_windowWorkspaceName;
        if (needsRestore)
        {
            pWorkspace = g_pCompositor->getWorkspaceByID(nd.ovbk_windowWorkspaceId);
            if (!pWorkspace){
                hycov_log(LOG,"source workspace no exist");
                g_hycov_pSpawnHook->hook(); // disable on-emptty-create workspace rule
                pWorkspace = g_pCompositor->createNewWorkspace(nd.ovbk_windowWorkspaceId,nd.ovbk_windowMonitorId,nd.ovbk_windowWorkspaceName);
                g_hycov_pSpawnHook->unhook();
                hycov_log(LOG,"create workspace: id:{} monitor:{} name:{}",nd.ovbk_windowWorkspaceId,nd.pWindow->monitorID(),nd.ovbk_windowWorkspaceName);
            }

            auto pMonitor = g_pCompositor->getMonitorFromID(nd.ovbk_windowMonitorId);
            if (!pWorkspace || !pMonitor) {
                failed++;
                hycov_log(Log::ERR,"restore source workspace failed,window:{} workspace:{} monitor:{}",nd.pWindow,nd.ovbk_windowWorkspaceId,nd.ovbk_windowMonitorId);
                continue;
            }

            moveWindowToWorkspaceCompat(nd.pWindow, pWorkspace);
            nd.workspaceID = nd.ovbk_windowWorkspaceId;
            nd.workspaceName = nd.ovbk_windowWorkspaceName;
            nd.pWindow->m_position = nd.ovbk_position;
            nd.pWindow->m_size = nd.ovbk_size;
            g_pHyprRenderer->damageWindow(nd.pWindow);
            nd.ovbk_movedForOverview = false;
            restored++;
        }
    }

    hycov_log(LOG,"moveWindowToSourceWorkspace done,restored:{} failed:{}",restored,failed);
    return {restored, failed};
}

// it will exec once when change layout enable
void OvGridLayout::onEnable()
{

    for (auto &w : g_pCompositor->m_windows)
    {        
        PHLWINDOW pWindow = w;

        if (pWindow->isHidden() || !pWindow->m_isMapped || pWindow->m_fadingOut)
            continue;

        if(pWindow->monitorID() != Desktop::focusState()->monitor()->m_id && g_hycov_only_active_monitor && !g_hycov_forece_display_all && !g_hycov_forece_display_all_in_one_monitor)
            continue;

        onWindowCreatedTiling(pWindow);
    }
}

// it will exec once when change layout disable
void OvGridLayout::onDisable()
{
    dispatch_leaveoverview("");
}

std::vector<PHLWORKSPACE> OvGridLayout::activeOverviewWorkspaces() const {
    std::vector<PHLWORKSPACE> workspaces;

    for (const auto& node : m_lOvGridNodesData) {
        if (!node.pWindow || !node.pWindow->m_workspace) {
            continue;
        }

        auto it = std::ranges::find(workspaces, node.pWindow->m_workspace);
        if (it == workspaces.end()) {
            workspaces.push_back(node.pWindow->m_workspace);
        }
    }

    return workspaces;
}
