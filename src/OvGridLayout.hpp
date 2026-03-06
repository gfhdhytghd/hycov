#pragma once

#include <utility>

struct SOvGridNodeData
{
  PHLWINDOW pWindow = nullptr;
  WORKSPACEID ovbk_windowWorkspaceId = WORKSPACE_INVALID;
  std::string ovbk_windowWorkspaceName;
  MONITORID ovbk_windowMonitorId = MONITOR_INVALID;
  std::string workspaceName;
  bool ovbk_windowIsFloating = false;
  bool ovbk_windowWasPinned = false;
  bool ovbk_windowHadXDGSizeLimits = false;
  bool ovbk_windowIsFullscreen = false;
  eFullscreenMode ovbk_windowFullscreenMode = FSMODE_NONE;
  Vector2D ovbk_position;
  Vector2D ovbk_size;
  Vector2D ovbk_xdgMinSizeCurrent = {1, 1};
  Vector2D ovbk_xdgMaxSizeCurrent = {1337420, 694200};
  Vector2D ovbk_xdgMinSizePending = {1, 1};
  Vector2D ovbk_xdgMaxSizePending = {1337420, 694200};
  Vector2D position;
  Vector2D size;
  bool ovbk_windowIsWithBorder = true;
  bool ovbk_windowIsWithDecorate = true;
  bool ovbk_windowIsWithRounding = true;
  bool ovbk_windowIsWithShadow = true;
  bool ovbk_movedForOverview = false;
  bool isInOldLayout = false;
  bool isGroupActive = false;

  WORKSPACEID workspaceID = WORKSPACE_INVALID;

  bool operator==(const SOvGridNodeData &rhs) const
  {
    return pWindow == rhs.pWindow;
  }
};


struct SOldLayoutRecordNodeData
{
  PHLWINDOW pWindow = nullptr;
  bool operator==(const SOldLayoutRecordNodeData &rhs) const
  {
    return pWindow == rhs.pWindow;
  }
};


class OvGridLayout
{
public:
  void onWindowCreatedTiling(PHLWINDOW, Math::eDirection direction = Math::DIRECTION_DEFAULT);
  void onWindowRemovedTiling(PHLWINDOW);
  void onWindowRemoved(PHLWINDOW);
  bool isWindowTiled(PHLWINDOW);
  PHLWINDOW getNextWindowCandidate(PHLWINDOW);
  void recalculateMonitor(const MONITORID&);
  void recalculateWindow(PHLWINDOW);
  void resizeActiveWindow(const Vector2D&, Layout::eRectCorner corner = Layout::CORNER_NONE, PHLWINDOW pWindow = nullptr);
  void fullscreenRequestForWindow(PHLWINDOW, const eFullscreenMode, const eFullscreenMode);
  void switchWindows(PHLWINDOW, PHLWINDOW);
  void alterSplitRatio(PHLWINDOW, float, bool);
  std::string getLayoutName();
  Vector2D predictSizeForNewWindowTiled();
  void replaceWindowDataWith(PHLWINDOW from, PHLWINDOW to);
  void moveWindowTo(PHLWINDOW, const std::string& direction, bool silent = false);
  void onEnable();
  void onDisable();
  void applyNodeDataToWindow(SOvGridNodeData *);
  void calculateWorkspace(const WORKSPACEID &);
  int getNodesNumOnWorkspace(const WORKSPACEID &);
  SOvGridNodeData *getNodeFromWindow(PHLWINDOW );
  SOldLayoutRecordNodeData *getOldLayoutRecordNodeFromWindow(PHLWINDOW );
  void resizeNodeSizePos(SOvGridNodeData *, int, int, int, int);
  void moveWindowToWorkspaceSilent(PHLWINDOW , const WORKSPACEID &);
  std::list<SOvGridNodeData> m_lOvGridNodesData; 
  std::list<SOldLayoutRecordNodeData> m_lSOldLayoutRecordNodeData; 
  std::pair<int, int> moveWindowToSourceWorkspace();
  void changeToActivceSourceWorkspace();
  void removeOldLayoutData(PHLWINDOW pWindow);
  std::vector<PHLWORKSPACE> activeOverviewWorkspaces() const;
private:
};
