#pragma once

#include <hyprland/src/layout/IHyprLayout.hpp>
#include <hyprland/src/SharedDefs.hpp>
#include <utility>

struct SOvGridNodeData
{
  PHLWINDOW pWindow = nullptr;
  WORKSPACEID ovbk_windowWorkspaceId = WORKSPACE_INVALID;
  std::string ovbk_windowWorkspaceName;
  MONITORID ovbk_windowMonitorId = MONITOR_INVALID;
  std::string workspaceName;
  bool ovbk_windowIsFloating = false;
  bool ovbk_windowIsFullscreen = false;
  eFullscreenMode ovbk_windowFullscreenMode = FSMODE_NONE;
  Vector2D ovbk_position;
  Vector2D ovbk_size;
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


class OvGridLayout : public IHyprLayout
{
public:
  virtual void onWindowCreatedTiling(PHLWINDOW , eDirection direction = DIRECTION_DEFAULT);
  virtual void onWindowRemovedTiling(PHLWINDOW );
  virtual void onWindowRemoved(PHLWINDOW );
  virtual bool isWindowTiled(PHLWINDOW );
  virtual PHLWINDOW getNextWindowCandidate(PHLWINDOW);
  virtual void recalculateMonitor(const MONITORID &);
  virtual void recalculateWindow(PHLWINDOW );
  virtual void resizeActiveWindow(const Vector2D &, eRectCorner corner = CORNER_NONE, PHLWINDOW pWindow = nullptr);
  virtual void fullscreenRequestForWindow(PHLWINDOW , const eFullscreenMode, const eFullscreenMode);
  virtual std::any layoutMessage(SLayoutMessageHeader, std::string);
  virtual SWindowRenderLayoutHints requestRenderHints(PHLWINDOW );
  virtual void switchWindows(PHLWINDOW , PHLWINDOW );
  virtual void alterSplitRatio(PHLWINDOW , float, bool);
  virtual std::string getLayoutName();
  virtual Vector2D predictSizeForNewWindowTiled();
  virtual void replaceWindowDataWith(PHLWINDOW from, PHLWINDOW to);
  virtual void moveWindowTo(PHLWINDOW, const std::string& direction, bool silent = false);
  virtual void onEnable();
  virtual void onDisable();
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
private:
};
