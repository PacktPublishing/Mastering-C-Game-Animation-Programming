/* separate settings file to avoid cicrula dependecies */
#pragma once

#include <memory>
#include <vector>
#include <map>
#include <set>
#include <functional>

#include "Callbacks.h"

// forward declaration
class AssimpModel;
class AssimpInstance;
class AssimpSettingsContainer;

struct ModelAndInstanceData {
  std::vector<std::shared_ptr<AssimpModel>> miModelList{};
  int miSelectedModel = 0;

  std::vector<std::shared_ptr<AssimpInstance>> miAssimpInstances{};
  std::map<std::string, std::vector<std::shared_ptr<AssimpInstance>>> miAssimpInstancesPerModel{};
  int miSelectedInstance = 0;

  std::shared_ptr<AssimpSettingsContainer> miSettingsContainer{};

  /* we can only delete models in Vulkan outside the command buffers,
   * so let's use a separate pending list */
  std::set<std::shared_ptr<AssimpModel>> miPendingDeleteAssimpModels{};
  /* we need a flag to signal the real deletion (undo/redo would break) */
  bool miDoDeletePendingAssimpModels = false;

  /* callbacks */
  setWindowTitleCallback miSetWindowTitleFunction;
  getWindowTitleCallback miGetWindowTitleFunction;

  modelCheckCallback miModelCheckCallbackFunction;
  modelAddCallback miModelAddCallbackFunction;
  modelDeleteCallback miModelDeleteCallbackFunction;

  instanceAddCallback miInstanceAddCallbackFunction;
  instanceAddManyCallback miInstanceAddManyCallbackFunction;
  instanceDeleteCallback miInstanceDeleteCallbackFunction;
  instanceCloneCallback miInstanceCloneCallbackFunction;
  instanceCloneManyCallback miInstanceCloneManyCallbackFunction;

  instanceCenterCallback miInstanceCenterCallbackFunction;

  undoRedoCallback miUndoCallbackFunction;
  undoRedoCallback miRedoCallbackFunction;

  loadSaveCallback miSaveConfigCallbackFunction;
  loadSaveCallback miLoadConfigCallbackFunction;
};
