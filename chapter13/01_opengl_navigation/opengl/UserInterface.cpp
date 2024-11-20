#include <string>
#include <tuple>
#include <map>
#include <cctype>
#include <limits>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <misc/cpp/imgui_stdlib.h> // ImGui::InputText with std::string

#include <ImGuiFileDialog.h>

#include <filesystem>

#include "UserInterface.h"
#include "AssimpModel.h"
#include "AssimpAnimClip.h"
#include "AssimpInstance.h"
#include "AssimpSettingsContainer.h"
#include "ModelSettings.h"
#include "Camera.h"
#include "SingleInstanceBehavior.h"
#include "AssimpLevel.h"
#include "LevelSettings.h"
#include "Logger.h"

void UserInterface::init(OGLRenderData &renderData) {
  IMGUI_CHECKVERSION();

  ImGui::CreateContext();
  ImNodes::CreateContext();

  ImGui_ImplGlfw_InitForOpenGL(renderData.rdWindow, true);

  const char *glslVersion = "#version 460 core";
  ImGui_ImplOpenGL3_Init(glslVersion);

  ImGui::StyleColorsLight();
  ImNodes::StyleColorsDark();

  /* init plot vectors */
  mFPSValues.resize(mNumFPSValues);
  mFrameTimeValues.resize(mNumFrameTimeValues);
  mModelUploadValues.resize(mNumModelUploadValues);
  mMatrixGenerationValues.resize(mNumMatrixGenerationValues);
  mMatrixUploadValues.resize(mNumMatrixUploadValues);
  mMatrixDownloadValues.resize(mNumMatrixDownloadValues);
  mUiGenValues.resize(mNumUiGenValues);
  mUiDrawValues.resize(mNumUiDrawValues);
  mCollisionDebugDrawValues.resize(mNumCollisionDebugDrawValues);
  mCollisionCheckValues.resize(mNumCollisionCheckValues);
  mNumCollisionsValues.resize(mNumNumCollisionValues);
  mBehaviorValues.resize(mNumBehaviorValues);
  mInteractionValues.resize(mNumInteractionValues);
  mFaceAnimValues.resize(mNumFaceAnimValues);
  mLevelCollisionCheckValues.resize(mNumLevelCollisionCheckValues);
  mIKValues.resize(mNumIKValues);
  mLevelGroundNeighborUpdateValues.resize(mNumLevelGroundNeighborUpdateValues);
  mPathFindingValues.resize(mNumPathFindingValues);

  /* Use CTRL to detach links */
  ImNodesIO& io = ImNodes::GetIO();
  io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
}

void UserInterface::hideMouse(bool hide) {
  /* v1.89.8 removed the check for disabled mouse cursor in GLFW
   * we need to ignore the mouse postion if the mouse lock is active */
  if (hide) {
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
  } else {
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
  }
}

void UserInterface::createFrame(OGLRenderData &renderData) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  static float newFps = 0.0f;
  /* avoid inf values (division by zero) */
  if (renderData.rdFrameTime > 0.0) {
    newFps = 1.0f / renderData.rdFrameTime * 1000.f;
  }

  /* make an averge value to avoid jumps */
  mFramesPerSecond = (mAveragingAlpha * mFramesPerSecond) + (1.0f - mAveragingAlpha) * newFps;
}

void UserInterface::createSettingsWindow(OGLRenderData& renderData, ModelInstanceCamData& modInstCamData) {
  ImGuiWindowFlags imguiWindowFlags = 0;

  ImGui::SetNextWindowBgAlpha(0.8f);

  /* dim background for modal dialogs */
  ImGuiStyle& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.75f);

  ImGui::Begin("Control", nullptr, imguiWindowFlags);

  bool loadModelRequest = false;
  bool loadLevelRequest = false;

  bool openUnsavedChangesNewDialog = false;
  bool openUnsavedChangesLoadDialog = false;
  bool openUnsavedChangesExitDialog = false;

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      ImGui::MenuItem("New Config", "CTRL+N", &renderData.rdNewConfigRequest);
      ImGui::MenuItem("Load Config", "CTRL+L", &renderData.rdLoadConfigRequest);
      if (modInstCamData.micModelList.size() == 1) {
        ImGui::BeginDisabled();
      }
      ImGui::MenuItem("Save Config", "CTRL+S", &renderData.rdSaveConfigRequest);
      if (modInstCamData.micModelList.size() == 1) {
        ImGui::EndDisabled();
      }
      ImGui::MenuItem("Exit", "CTRL+Q", &renderData.rdRequestApplicationExit);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      if (modInstCamData.micSettingsContainer->getUndoSize() == 0) {
        ImGui::BeginDisabled();
      }
      if (ImGui::MenuItem("Undo", "CTRL+Z")) {
        modInstCamData.micUndoCallbackFunction();
      }
      if (modInstCamData.micSettingsContainer->getUndoSize() == 0) {
        ImGui::EndDisabled();
      }

      if (modInstCamData.micSettingsContainer->getRedoSize() == 0) {
        ImGui::BeginDisabled();
      }
      if (ImGui::MenuItem("Redo", "CTRL+Y")) {
        modInstCamData.micRedoCallbackFunction();
      }
      if (modInstCamData.micSettingsContainer->getRedoSize() == 0) {
        ImGui::EndDisabled();
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Models")) {
      ImGui::MenuItem("Load Model...", nullptr, &loadModelRequest);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Levels")) {
      ImGui::MenuItem("Load Level...", nullptr, &loadLevelRequest);
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }

  /* application exit */
  if (renderData.rdRequestApplicationExit) {
    ImGuiFileDialog::Instance()->Close();
    ImGui::SetNextWindowPos(ImVec2(renderData.rdWidth / 2.0f, renderData.rdHeight / 2.0f), ImGuiCond_Always);
    ImGui::OpenPopup("Do you want to quit?");
  }

  if (ImGui::BeginPopupModal("Do you want to quit?", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
    ImGui::Text("  Exit Application?  ");

    /* cheating a bit to get buttons more to the center */
    ImGui::Indent();
    if (ImGui::Button("OK")) {
      if (modInstCamData.micGetConfigDirtyCallbackFunction()) {
        openUnsavedChangesExitDialog = true;
        renderData.rdRequestApplicationExit = false;
      } else {
        renderData.rdAppExitCallback();
      }
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      renderData.rdRequestApplicationExit = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* unsaved changes, ask */
  if (openUnsavedChangesExitDialog) {
    ImGui::SetNextWindowPos(ImVec2(renderData.rdWidth / 2.0f, renderData.rdHeight / 2.0f), ImGuiCond_Always);
    ImGui::OpenPopup("Exit - Unsaved Changes");
  }

  if (ImGui::BeginPopupModal("Exit - Unsaved Changes", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
    ImGui::Text("You have unsaved Changes!");
    ImGui::Text("Still exit?");

    /* cheating a bit to get buttons more to the center */
    ImGui::Indent();
    if (ImGui::Button("OK")) {
      renderData.rdAppExitCallback();
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      renderData.rdRequestApplicationExit = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* new config */
  if (renderData.rdNewConfigRequest) {
    if (modInstCamData.micGetConfigDirtyCallbackFunction()) {
      openUnsavedChangesNewDialog = true;
    } else {
      renderData.rdNewConfigRequest = false;
      modInstCamData.micNewConfigCallbackFunction();
    }
  }

  /* unsaved changes, ask */
  if (openUnsavedChangesNewDialog) {
    ImGui::SetNextWindowPos(ImVec2(renderData.rdWidth / 2.0f, renderData.rdHeight / 2.0f), ImGuiCond_Always);
    ImGui::OpenPopup("New - Unsaved Changes");
  }

  if (ImGui::BeginPopupModal("New - Unsaved Changes", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
    ImGui::Text("You have unsaved Changes!");
    ImGui::Text("Continue?");

    /* cheating a bit to get buttons more to the center */
    ImGui::Indent();
    if (ImGui::Button("OK")) {
      renderData.rdNewConfigRequest = false;
      modInstCamData.micNewConfigCallbackFunction();
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      renderData.rdNewConfigRequest = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* load config */
  if (renderData.rdLoadConfigRequest) {
    IGFD::FileDialogConfig config;
    config.path = ".";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_Modal;
    const std::string defaultFileName = "config/conf.acfg";
    config.filePathName = defaultFileName.c_str();
    ImGuiFileDialog::Instance()->OpenDialog("LoadConfigFile", "Load Configuration File",
      ".acfg", config);
  }

  bool loadSuccessful = true;
  if (ImGuiFileDialog::Instance()->Display("LoadConfigFile")) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      if (modInstCamData.micGetConfigDirtyCallbackFunction()) {
        openUnsavedChangesLoadDialog = true;
      } else {
        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
        loadSuccessful = modInstCamData.micLoadConfigCallbackFunction(filePathName);
      }
    }
    renderData.rdLoadConfigRequest = false;
    ImGuiFileDialog::Instance()->Close();
  }

  /* ask for replacement  */
  if (openUnsavedChangesLoadDialog) {
    ImGui::SetNextWindowPos(ImVec2(renderData.rdWidth / 2.0f, renderData.rdHeight / 2.0f), ImGuiCond_Always);
    ImGui::OpenPopup("Load - Unsaved Changes");
  }

  if (ImGui::BeginPopupModal("Load - Unsaved Changes", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
    ImGui::Text("You have unsaved Changes!");
    ImGui::Text("Continue?");

    /* cheating a bit to get buttons more to the center */
    ImGui::Indent();
    if (ImGui::Button("OK")) {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      loadSuccessful = modInstCamData.micLoadConfigCallbackFunction(filePathName);
      if (loadSuccessful) {
        renderData.rdLoadConfigRequest = false;
      }
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      renderData.rdLoadConfigRequest = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* show error message if load was not successful */
  if (!loadSuccessful) {
    ImGui::SetNextWindowPos(ImVec2(renderData.rdWidth / 2.0f, renderData.rdHeight / 2.0f), ImGuiCond_Always);
    ImGui::OpenPopup("Load Error!");
  }

  if (ImGui::BeginPopupModal("Load Error!", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
    ImGui::Text("Error loading config!");
    ImGui::Text("Check console output!");

    /* cheating a bit to get buttons more to the center */
    ImGui::Indent();
    ImGui::Indent();
    ImGui::Indent();
    if (ImGui::Button("OK")) {
      renderData.rdLoadConfigRequest = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* save config*/
  if (renderData.rdSaveConfigRequest) {
    IGFD::FileDialogConfig config;
    config.path = ".";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_ConfirmOverwrite;
    const std::string defaultFileName = "config/conf.acfg";
    config.filePathName = defaultFileName.c_str();
    ImGuiFileDialog::Instance()->OpenDialog("SaveConfigFile", "Save Configuration File",
      ".acfg", config);
  }

  bool saveSuccessful = true;
  if (ImGuiFileDialog::Instance()->Display("SaveConfigFile")) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      saveSuccessful = modInstCamData.micSaveConfigCallbackFunction(filePathName);

      if (saveSuccessful) {
        modInstCamData.micSetConfigDirtyCallbackFunction(false);
      }
    }
    renderData.rdSaveConfigRequest = false;
    ImGuiFileDialog::Instance()->Close();
  }

  /* show error message if save was not successful */
  if (!saveSuccessful) {
    ImGui::SetNextWindowPos(ImVec2(renderData.rdWidth / 2.0f, renderData.rdHeight / 2.0f), ImGuiCond_Always);
    ImGui::OpenPopup("Save Error!");
  }

  if (ImGui::BeginPopupModal("Save Error!", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
    ImGui::Text("Error saving config!");
    ImGui::Text("Check console output!");

    /* cheating a bit to get buttons more to the center */
    ImGui::Indent();
    ImGui::Indent();
    ImGui::Indent();
    if (ImGui::Button("OK")) {
      renderData.rdSaveConfigRequest = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* load model */
  if (loadModelRequest) {
    IGFD::FileDialogConfig config;
    config.path = ".";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_Modal;
    ImGuiFileDialog::Instance()->OpenDialog("ChooseModelFile", "Choose Model File",
      "Supported Model Files{.gltf,.glb,.obj,.fbx,.dae,.mdl,.md3,.pk3}", config);
  }

  if (ImGuiFileDialog::Instance()->Display("ChooseModelFile")) {
    if (ImGuiFileDialog::Instance()->IsOk()) {

      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();

      /* try to construct a relative path */
      std::filesystem::path currentPath = std::filesystem::current_path();
      std::string relativePath =  std::filesystem::relative(filePathName, currentPath).generic_string();

      if (!relativePath.empty()) {
        filePathName = relativePath;
      }
      /* Windows does understand forward slashes, but std::filesystem preferres backslashes... */
      std::replace(filePathName.begin(), filePathName.end(), '\\', '/');

      if (!modInstCamData.micModelAddCallbackFunction(filePathName, true, true)) {
        Logger::log(1, "%s error: unable to load model file '%s', unnown error \n", __FUNCTION__, filePathName.c_str());
      }
    }
    ImGuiFileDialog::Instance()->Close();
  }

  /* load level */
  if (loadLevelRequest) {
    IGFD::FileDialogConfig config;
    config.path = ".";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_Modal;
    ImGuiFileDialog::Instance()->OpenDialog("ChooseLevelFile", "Choose Level File",
      "Supported Level Files{.gltf,.glb,.obj,.fbx,.dae,.pk3}", config);
  }

  if (ImGuiFileDialog::Instance()->Display("ChooseLevelFile")) {
    if (ImGuiFileDialog::Instance()->IsOk()) {

      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();

      /* try to construct a relative path */
      std::filesystem::path currentPath = std::filesystem::current_path();
      std::string relativePath =  std::filesystem::relative(filePathName, currentPath).generic_string();

      if (!relativePath.empty()) {
        filePathName = relativePath;
      }
      /* Windows does understand forward slashes, but std::filesystem preferres backslashes... */
      std::replace(filePathName.begin(), filePathName.end(), '\\', '/');

      if (!modInstCamData.micLevelAddCallbackFunction(filePathName)) {
        Logger::log(1, "%s error: unable to load level file '%s', unnown error \n", __FUNCTION__, filePathName.c_str());
      }
    }
    ImGuiFileDialog::Instance()->Close();
  }

  /* clamp manual input on all sliders to min/max */
  ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;

  static double updateTime = 0.0;

  /* avoid literal double compares */
  if (updateTime < 0.000001) {
    updateTime = ImGui::GetTime();
  }

  static int fpsOffset = 0;
  static int frameTimeOffset = 0;
  static int modelUploadOffset = 0;
  static int matrixGenOffset = 0;
  static int matrixUploadOffset = 0;
  static int matrixDownloadOffset = 0;
  static int uiGenOffset = 0;
  static int uiDrawOffset = 0;
  static int collisionDebugDrawOffset = 0;
  static int collisionCheckOffset = 0;
  static int numCollisionOffset = 0;
  static int behaviorOffset = 0;
  static int interactionOffset = 0;
  static int faceAnimOffset = 0;
  static int levelCollisionOffset = 0;
  static int ikOffset = 0;
  static int levelGroundNeighborOffset = 0;
  static int pathFindingOffset= 0;

  while (updateTime < ImGui::GetTime()) {
    mFPSValues.at(fpsOffset) = mFramesPerSecond;
    fpsOffset = ++fpsOffset % mNumFPSValues;

    mFrameTimeValues.at(frameTimeOffset) = renderData.rdFrameTime;
    frameTimeOffset = ++frameTimeOffset % mNumFrameTimeValues;

    mModelUploadValues.at(modelUploadOffset) = renderData.rdUploadToVBOTime;
    modelUploadOffset = ++modelUploadOffset % mNumModelUploadValues;

    mMatrixGenerationValues.at(matrixGenOffset) = renderData.rdMatrixGenerateTime;
    matrixGenOffset = ++matrixGenOffset % mNumMatrixGenerationValues;

    mMatrixUploadValues.at(matrixUploadOffset) = renderData.rdUploadToUBOTime;
    matrixUploadOffset = ++matrixUploadOffset % mNumMatrixUploadValues;

    mMatrixDownloadValues.at(matrixDownloadOffset) = renderData.rdDownloadFromUBOTime;
    matrixDownloadOffset = ++matrixDownloadOffset % mNumMatrixDownloadValues;

    mUiGenValues.at(uiGenOffset) = renderData.rdUIGenerateTime;
    uiGenOffset = ++uiGenOffset % mNumUiGenValues;

    mUiDrawValues.at(uiDrawOffset) = renderData.rdUIDrawTime;
    uiDrawOffset = ++uiDrawOffset % mNumUiDrawValues;

    mCollisionDebugDrawValues.at(collisionDebugDrawOffset) = renderData.rdCollisionDebugDrawTime;
    collisionDebugDrawOffset = ++collisionDebugDrawOffset % mNumCollisionDebugDrawValues;

    mCollisionCheckValues.at(collisionCheckOffset) = renderData.rdCollisionCheckTime;
    collisionCheckOffset = ++collisionCheckOffset % mNumCollisionCheckValues;

    mNumCollisionsValues.at(numCollisionOffset) = renderData.rdNumberOfCollisions;
    numCollisionOffset = ++numCollisionOffset % mNumNumCollisionValues;

    mBehaviorValues.at(behaviorOffset) = renderData.rdBehaviorTime;
    behaviorOffset = ++behaviorOffset % mNumBehaviorValues;

    mInteractionValues.at(interactionOffset) = renderData.rdInteractionTime;
    interactionOffset = ++interactionOffset % mNumInteractionValues;

    mFaceAnimValues.at(faceAnimOffset) = renderData.rdFaceAnimTime;
    faceAnimOffset = ++faceAnimOffset % mNumFaceAnimValues;

    mLevelCollisionCheckValues.at(levelCollisionOffset) = renderData.rdLevelCollisionTime;
    levelCollisionOffset = ++levelCollisionOffset % mNumLevelCollisionCheckValues;

    mIKValues.at(ikOffset) = renderData.rdIKTime;
    ikOffset = ++ikOffset % mNumIKValues;

    mLevelGroundNeighborUpdateValues.at(levelGroundNeighborOffset) = renderData.rdLevelGRoundNeighborUpdateTime;
    levelGroundNeighborOffset = ++levelGroundNeighborOffset % mNumLevelGroundNeighborUpdateValues;

    mPathFindingValues.at(pathFindingOffset) = renderData.rdPathFindingTime;
    pathFindingOffset = ++pathFindingOffset % mNumPathFindingValues;

    updateTime += 1.0 / 30.0;
  }

  ImGui::Text("FPS: %10.4f", mFramesPerSecond);

  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    float averageFPS = 0.0f;
    for (const auto value : mFPSValues) {
      averageFPS += value;
    }
    averageFPS /= static_cast<float>(mNumFPSValues);
    std::string fpsOverlay = "now:     " + std::to_string(mFramesPerSecond) + "\n30s avg: " + std::to_string(averageFPS);
    ImGui::Text("FPS");
    ImGui::SameLine();
    ImGui::PlotLines("##FrameTimes", mFPSValues.data(), mFPSValues.size(), fpsOffset, fpsOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(),
      ImVec2(0, 80));
    ImGui::EndTooltip();
  }

  if (ImGui::CollapsingHeader("Info")) {
    ImGui::Text("Triangles:              %10i", renderData.rdTriangleCount);
    ImGui::Text("Level Triangles:        %10i", renderData.rdLevelTriangleCount);

    std::string unit = "B";
    float memoryUsage = renderData.rdMatricesSize;

    if (memoryUsage > 1024.0f * 1024.0f) {
      memoryUsage /= 1024.0f * 1024.0f;
      unit = "MB";
    } else  if (memoryUsage > 1024.0f) {
      memoryUsage /= 1024.0f;
      unit = "KB";
    }

    ImGui::Text("Instance Matrix Size:  %8.2f %2s", memoryUsage, unit.c_str());


    std::string windowDims = std::to_string(renderData.rdWidth) + "x" + std::to_string(renderData.rdHeight);
    ImGui::Text("Window Dimensions:      %10s", windowDims.c_str());

    std::string imgWindowPos = std::to_string(static_cast<int>(ImGui::GetWindowPos().x)) + "/" + std::to_string(static_cast<int>(ImGui::GetWindowPos().y));
    ImGui::Text("ImGui Window Position:  %10s", imgWindowPos.c_str());
  }


  if (ImGui::CollapsingHeader("Timers")) {
    ImGui::Text("Frame Time:              %10.4f ms", renderData.rdFrameTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageFrameTime = 0.0f;
      for (const auto value : mFrameTimeValues) {
        averageFrameTime += value;
      }
      averageFrameTime /= static_cast<float>(mNumMatrixGenerationValues);
      std::string frameTimeOverlay = "now:     " + std::to_string(renderData.rdFrameTime)
        + " ms\n30s avg: " + std::to_string(averageFrameTime) + " ms";
      ImGui::Text("Frame Time       ");
      ImGui::SameLine();
      ImGui::PlotLines("##FrameTime", mFrameTimeValues.data(), mFrameTimeValues.size(), frameTimeOffset,
        frameTimeOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Model Upload Time:       %10.4f ms", renderData.rdUploadToVBOTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageModelUpload = 0.0f;
      for (const auto value : mModelUploadValues) {
        averageModelUpload += value;
      }
      averageModelUpload /= static_cast<float>(mNumModelUploadValues);
      std::string modelUploadOverlay = "now:     " + std::to_string(renderData.rdUploadToVBOTime)
        + " ms\n30s avg: " + std::to_string(averageModelUpload) + " ms";
      ImGui::Text("VBO Upload");
      ImGui::SameLine();
      ImGui::PlotLines("##ModelUploadTimes", mModelUploadValues.data(), mModelUploadValues.size(), modelUploadOffset,
        modelUploadOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Matrix Generation Time:  %10.4f ms", renderData.rdMatrixGenerateTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageMatGen = 0.0f;
      for (const auto value : mMatrixGenerationValues) {
        averageMatGen += value;
      }
      averageMatGen /= static_cast<float>(mNumMatrixGenerationValues);
      std::string matrixGenOverlay = "now:     " + std::to_string(renderData.rdMatrixGenerateTime)
        + " ms\n30s avg: " + std::to_string(averageMatGen) + " ms";
      ImGui::Text("Matrix Generation");
      ImGui::SameLine();
      ImGui::PlotLines("##MatrixGenTimes", mMatrixGenerationValues.data(), mMatrixGenerationValues.size(), matrixGenOffset,
        matrixGenOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Matrix Upload Time:      %10.4f ms", renderData.rdUploadToUBOTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageMatrixUpload = 0.0f;
      for (const auto value : mMatrixUploadValues) {
        averageMatrixUpload += value;
      }
      averageMatrixUpload /= static_cast<float>(mNumMatrixUploadValues);
      std::string matrixUploadOverlay = "now:     " + std::to_string(renderData.rdUploadToUBOTime)
        + " ms\n30s avg: " + std::to_string(averageMatrixUpload) + " ms";
      ImGui::Text("UBO Upload");
      ImGui::SameLine();
      ImGui::PlotLines("##MatrixUploadTimes", mMatrixUploadValues.data(), mMatrixUploadValues.size(), matrixUploadOffset,
        matrixUploadOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Matrix Download Time:    %10.4f ms", renderData.rdDownloadFromUBOTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageMatrixDownload = 0.0f;
      for (const auto value : mMatrixDownloadValues) {
        averageMatrixDownload += value;
      }
      averageMatrixDownload /= static_cast<float>(mNumMatrixDownloadValues);
      std::string matrixDownloadOverlay = "now:     " + std::to_string(renderData.rdDownloadFromUBOTime)
        + " ms\n30s avg: " + std::to_string(averageMatrixDownload) + " ms";
      ImGui::Text("UBO Download");
      ImGui::SameLine();
      ImGui::PlotLines("##MatrixDownloadTimes", mMatrixDownloadValues.data(), mMatrixDownloadValues.size(), matrixDownloadOffset,
        matrixDownloadOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("UI Generation Time:      %10.4f ms", renderData.rdUIGenerateTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageUiGen = 0.0f;
      for (const auto value : mUiGenValues) {
        averageUiGen += value;
      }
      averageUiGen /= static_cast<float>(mNumUiGenValues);
      std::string uiGenOverlay = "now:     " + std::to_string(renderData.rdUIGenerateTime)
        + " ms\n30s avg: " + std::to_string(averageUiGen) + " ms";
      ImGui::Text("UI Generation");
      ImGui::SameLine();
      ImGui::PlotLines("##UIGenTimes", mUiGenValues.data(), mUiGenValues.size(), uiGenOffset,
        uiGenOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("UI Draw Time:            %10.4f ms", renderData.rdUIDrawTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageUiDraw = 0.0f;
      for (const auto value : mUiDrawValues) {
        averageUiDraw += value;
      }
      averageUiDraw /= static_cast<float>(mNumUiDrawValues);
      std::string uiDrawOverlay = "now:     " + std::to_string(renderData.rdUIDrawTime)
        + " ms\n30s avg: " + std::to_string(averageUiDraw) + " ms";
      ImGui::Text("UI Draw");
      ImGui::SameLine();
      ImGui::PlotLines("##UIDrawTimes", mUiDrawValues.data(), mUiDrawValues.size(), uiDrawOffset,
        uiDrawOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Collision Debug Draw:    %10.4f ms", renderData.rdCollisionDebugDrawTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageCollisionDebugDraw = 0.0f;
      for (const auto value : mCollisionDebugDrawValues) {
        averageCollisionDebugDraw += value;
      }
      averageCollisionDebugDraw /= static_cast<float>(mNumCollisionDebugDrawValues);
      std::string collisionDebugOverlay = "now:     " + std::to_string(renderData.rdCollisionDebugDrawTime)
        + " ms\n30s avg: " + std::to_string(averageCollisionDebugDraw) + " ms";
      ImGui::Text("Collision Debug Draw");
      ImGui::SameLine();
      ImGui::PlotLines("##CollisionDebugDrawTimes", mCollisionDebugDrawValues.data(), mCollisionDebugDrawValues.size(), collisionDebugDrawOffset,
        collisionDebugOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Collision Check Time:    %10.4f ms", renderData.rdCollisionCheckTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageCollisionCheck = 0.0f;
      for (const auto value : mCollisionCheckValues) {
        averageCollisionCheck += value;
      }
      averageCollisionCheck /= static_cast<float>(mNumCollisionCheckValues);
      std::string collisionCheckOverlay = "now:     " + std::to_string(renderData.rdCollisionCheckTime)
        + " ms\n30s avg: " + std::to_string(averageCollisionCheck) + " ms";
      ImGui::Text("Collision Check");
      ImGui::SameLine();
      ImGui::PlotLines("##CollisionCheckTimes", mCollisionCheckValues.data(), mCollisionCheckValues.size(), collisionCheckOffset,
        collisionCheckOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Behavior Update Time:    %10.4f ms", renderData.rdBehaviorTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageBehavior = 0.0f;
      for (const auto value : mBehaviorValues) {
        averageBehavior += value;
      }
      averageBehavior /= static_cast<float>(mNumBehaviorValues);
      std::string behaviorOverlay = "now:     " + std::to_string(renderData.rdBehaviorTime)
        + " ms\n30s avg: " + std::to_string(averageBehavior) + " ms";
      ImGui::Text("Behavior Update");
      ImGui::SameLine();
      ImGui::PlotLines("##BehaviorUpdateTimes", mBehaviorValues.data(), mBehaviorValues.size(), behaviorOffset,
        behaviorOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Interaction Update Time: %10.4f ms", renderData.rdInteractionTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageInteraction = 0.0f;
      for (const auto value : mInteractionValues) {
        averageInteraction += value;
      }
      averageInteraction /= static_cast<float>(mNumInteractionValues);
      std::string interactionOverlay = "now:     " + std::to_string(renderData.rdInteractionTime)
        + " ms\n30s avg: " + std::to_string(averageInteraction) + " ms";
      ImGui::Text("Interaction Update");
      ImGui::SameLine();
      ImGui::PlotLines("##InteractionUpdateTimes", mInteractionValues.data(), mInteractionValues.size(), interactionOffset,
        interactionOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Face Animation Time:     %10.4f ms", renderData.rdFaceAnimTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageFaceAnim = 0.0f;
      for (const auto value : mFaceAnimValues) {
        averageFaceAnim += value;
      }
      averageFaceAnim /= static_cast<float>(mNumFaceAnimValues);
      std::string faceAnimOverlay = "now:     " + std::to_string(renderData.rdFaceAnimTime)
        + " ms\n30s avg: " + std::to_string(averageFaceAnim) + " ms";
      ImGui::Text("Face Anim Time");
      ImGui::SameLine();
      ImGui::PlotLines("##FaceAnimTimes", mFaceAnimValues.data(), mFaceAnimValues.size(), faceAnimOffset,
        faceAnimOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Level Collision Check:   %10.4f ms", renderData.rdLevelCollisionTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageLevelCollisionCheck = 0.0f;
      for (const auto value : mLevelCollisionCheckValues) {
        averageLevelCollisionCheck += value;
      }
      averageLevelCollisionCheck /= static_cast<float>(mNumLevelCollisionCheckValues);
      std::string levelCollisionCheckOverlay = "now:     " + std::to_string(renderData.rdLevelCollisionTime)
        + " ms\n30s avg: " + std::to_string(averageLevelCollisionCheck) + " ms";
      ImGui::Text("Level Collision Check");
      ImGui::SameLine();
      ImGui::PlotLines("##LevelCollisionCheck", mLevelCollisionCheckValues.data(), mLevelCollisionCheckValues.size(), levelCollisionOffset,
        levelCollisionCheckOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Inverse Kinematics:      %10.4f ms", renderData.rdIKTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageIk = 0.0f;
      for (const auto value : mIKValues) {
        averageIk += value;
      }
      averageIk /= static_cast<float>(mNumIKValues);
      std::string ikOverlay = "now:     " + std::to_string(renderData.rdIKTime)
        + " ms\n30s avg: " + std::to_string(averageIk) + " ms";
      ImGui::Text("Inverse Kinematics");
      ImGui::SameLine();
      ImGui::PlotLines("##InverseKinematice", mIKValues.data(), mIKValues.size(), ikOffset,
        ikOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();

    }

    ImGui::Text("Ground Neighbor Update:  %10.4f ms", renderData.rdLevelGRoundNeighborUpdateTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageNeighborUpdate = 0.0f;
      for (const auto value : mLevelGroundNeighborUpdateValues) {
        averageNeighborUpdate += value;
      }
      averageNeighborUpdate /= static_cast<float>(mNumLevelCollisionCheckValues);
      std::string neighborUpdateOverlay = "now:     " + std::to_string(renderData.rdLevelGRoundNeighborUpdateTime)
        + " ms\n30s avg: " + std::to_string(averageNeighborUpdate) + " ms";
      ImGui::Text("Ground Neighbor Update");
      ImGui::SameLine();
      ImGui::PlotLines("##GroundNeighborUpdate", mLevelGroundNeighborUpdateValues.data(), mLevelGroundNeighborUpdateValues.size(), levelGroundNeighborOffset,
        neighborUpdateOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Path Finding:            %10.4f ms", renderData.rdPathFindingTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averagePathFinding = 0.0f;
      for (const auto value : mPathFindingValues) {
        averagePathFinding += value;
      }
      averagePathFinding /= static_cast<float>(mNumPathFindingValues);
      std::string pathFindingOverlay = "now:     " + std::to_string(renderData.rdPathFindingTime)
        + " ms\n30s avg: " + std::to_string(averagePathFinding) + " ms";
      ImGui::Text("Path Finding");
      ImGui::SameLine();
      ImGui::PlotLines("##PathFinding", mPathFindingValues.data(), mPathFindingValues.size(), pathFindingOffset,
        pathFindingOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }
  }

  if (ImGui::CollapsingHeader("Camera")) {

    static CameraSettings savedCameraSettings{};
    static std::shared_ptr<Camera> currentCamera = nullptr;
    static std::vector<std::string> boneNames{};

    std::shared_ptr<Camera> cam = modInstCamData.micCameras.at(modInstCamData.micSelectedCamera);
    CameraSettings settings = cam->getCameraSettings();

    /* overwrite saved settings on camera change */
    if (currentCamera != modInstCamData.micCameras.at(modInstCamData.micSelectedCamera)) {
      currentCamera = modInstCamData.micCameras.at(modInstCamData.micSelectedCamera);
      savedCameraSettings = settings;
      boneNames = cam->getBoneNames();
    }

    /* same hack as for instances */
    int numCameras = modInstCamData.micCameras.size() - 1;
    if (numCameras == 0) {
      ImGui::BeginDisabled();
    }

    ImGui::Text("Cameras:         ");
    ImGui::SameLine();
    ImGui::PushItemWidth(180.0f);

    std::string selectedCamName = "None";

    if (ImGui::ArrowButton("##CamLeft", ImGuiDir_Left) &&
      modInstCamData.micSelectedCamera > 0) {
      modInstCamData.micSelectedCamera--;
    }

    ImGui::SameLine();
    if (ImGui::BeginCombo("##CamCombo",
      settings.csCamName.c_str())) {
      for (int i = 0; i < modInstCamData.micCameras.size(); ++i) {
        const bool isSelected = (modInstCamData.micSelectedCamera == i);
        if (ImGui::Selectable(modInstCamData.micCameras.at(i)->getName().c_str(), isSelected)) {
          modInstCamData.micSelectedCamera = i;
          selectedCamName = modInstCamData.micCameras.at(modInstCamData.micSelectedCamera)->getName().c_str();
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::ArrowButton("##CamRight", ImGuiDir_Right) &&
      modInstCamData.micSelectedCamera < modInstCamData.micCameras.size() - 1) {
      modInstCamData.micSelectedCamera++;
    }

    if (numCameras == 0) {
      ImGui::EndDisabled();
    }

    ImGui::Text("                 ");
    ImGui::SameLine();
    if (ImGui::Button("Clone Current Camera")) {
      modInstCamData.micCameraCloneCallbackFunction();
      numCameras = modInstCamData.micCameras.size() - 1;
    }

    if (numCameras == 0 || modInstCamData.micSelectedCamera == 0) {
      ImGui::BeginDisabled();
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete Camera")) {
      modInstCamData.micCameraDeleteCallbackFunction();
      numCameras = modInstCamData.micCameras.size() - 1;
    }
    if (numCameras == 0 || modInstCamData.micSelectedCamera == 0) {
      ImGui::EndDisabled();
    }

    /* Disallow changing default 'FreeCam' name or type */
    if (modInstCamData.micSelectedCamera == 0) {
      ImGui::BeginDisabled();
    }

    ImGuiInputTextFlags textinputFlags = ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCharFilter;
    static bool showDuplicateCamNameDialog = false;
    std::string camName = settings.csCamName;
    ImGui::Text("Camera Name:     ");
    ImGui::SameLine();
    if (ImGui::InputText("##CamName", &camName, textinputFlags, nameInputFilter)) {
      if (modInstCamData.micCameraNameCheckCallbackFunction(camName)) {
        showDuplicateCamNameDialog = true;
      } else {
        settings.csCamName = camName;
        modInstCamData.micSettingsContainer->applyEditCameraSettings(modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
                                                                     settings, savedCameraSettings);
        savedCameraSettings = settings;
        modInstCamData.micSetConfigDirtyCallbackFunction(true);
      }
    }

    if (showDuplicateCamNameDialog) {
      ImGui::SetNextWindowPos(ImVec2(renderData.rdWidth / 2.0f, renderData.rdHeight / 2.0f), ImGuiCond_Always);
      ImGui::OpenPopup("Duplicate Camera Name");
      showDuplicateCamNameDialog = false;
    }

    if (ImGui::BeginPopupModal("Duplicate Camera Name", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
      ImGui::Text("Camera Name '%s' alread exists!", camName.c_str());

      /* cheating a bit to get buttons more to the center */
      ImGui::Indent();
      ImGui::Indent();
      ImGui::Indent();
      ImGui::Indent();
      ImGui::Indent();
      if (ImGui::Button("OK")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::Text("Camera Type:     ");
    ImGui::SameLine();
    ImGui::PushItemWidth(250.0f);

    if (ImGui::BeginCombo("##CamTypeCombo",
      modInstCamData.micCameraTypeMap.at(settings.csCamType).c_str())) {
      for (int i = 0; i < modInstCamData.micCameraTypeMap.size(); ++i) {
        const bool isSelected = (static_cast<int>(settings.csCamType) == i);
        if (ImGui::Selectable(modInstCamData.micCameraTypeMap[static_cast<cameraType>(i)].c_str(), isSelected)) {
          settings.csCamType = static_cast<cameraType>(i);
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    int followInstanceIndex = 0;
    std::string followInstanceId = "-";
    std::shared_ptr<AssimpInstance> followInstance = cam->getInstanceToFollow();
    if (followInstance) {
      followInstanceIndex = followInstance->getInstanceSettings().isInstanceIndexPosition;
      followInstanceId = std::to_string(followInstanceIndex);
    }

    if (settings.csCamType == cameraType::firstPerson || settings.csCamType == cameraType::thirdPerson || settings.csCamType == cameraType::stationaryFollowing) {
      ImGui::Text("Following:  %4s ", followInstanceId.c_str());
      ImGui::SameLine();

      if (modInstCamData.micSelectedInstance == 0) {
        ImGui::BeginDisabled();
      }

      if (ImGui::Button("Use Selected Instance")) {
        std::shared_ptr<AssimpInstance> selectedInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
        /* this call also fills in the bone list */
        cam->setInstanceToFollow(selectedInstance);
        boneNames = cam->getBoneNames();

        settings = cam->getCameraSettings();
      }
      if (modInstCamData.micSelectedInstance == 0) {
        ImGui::EndDisabled();
      }

      ImGui::SameLine();
      if (!followInstance) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Button("Clear Selection")) {
        cam->clearInstanceToFollow();
        boneNames = cam->getBoneNames();

        settings = cam->getCameraSettings();
      }

      ImGui::Text("                 ");
      ImGui::SameLine();
      if (ImGui::Button("Selected Following Instance")) {
        modInstCamData.micSelectedInstance = followInstanceIndex;
        std::shared_ptr<AssimpInstance> selectedInstance = modInstCamData.micAssimpInstances.at(followInstanceIndex);
        /* this call also fills in the bone list */
        cam->setInstanceToFollow(selectedInstance);
        boneNames = cam->getBoneNames();

        settings = cam->getCameraSettings();
      }

      if (settings.csCamType == cameraType::thirdPerson && followInstance) {
        ImGui::Text("Distance:        ");
        ImGui::SameLine();
        ImGui::SliderFloat("##3rdPersonDistance", &settings.csThirdPersonDistance, 3.0f, 10.0f, "%.3f", flags);

        ImGui::Text("Camera Height:   ");
        ImGui::SameLine();
        ImGui::SliderFloat("##3rdPersonOffset", &settings.csThirdPersonHeightOffset, 0.0f, 3.0f, "%.3f", flags);
      }

      if (settings.csCamType == cameraType::firstPerson && followInstance) {
        ImGui::Text("Lock View:       ");
        ImGui::SameLine();
        ImGui::Checkbox("##1stPersonLockView", &settings.csFirstPersonLockView);

        if (cam->getBoneNames().size() > 0) {
          ImGui::Text("Bone to Follow:  ");
          ImGui::SameLine();
          ImGui::PushItemWidth(250.0f);

          if (ImGui::BeginCombo("##1stPersonBoneNameCombo",
            boneNames.at(settings.csFirstPersonBoneToFollow).c_str())) {
            for (int i = 0; i < boneNames.size(); ++i) {
              const bool isSelected = (settings.csFirstPersonBoneToFollow == i);
              if (ImGui::Selectable(boneNames.at(i).c_str(), isSelected)) {
                settings.csFirstPersonBoneToFollow = i;
              }

              if (isSelected) {
                ImGui::SetItemDefaultFocus();
              }
            }
            ImGui::EndCombo();
          }
          ImGui::PopItemWidth();
        }

        ImGui::Text("View Offsets:    ");
        ImGui::SameLine();
        ImGui::SliderFloat3("##1stPersonOffset", glm::value_ptr(settings.csFirstPersonOffsets), -1.0f, 1.0f, "%.3f", flags);
      }

      if (settings.csCamType == cameraType::stationaryFollowing && followInstance) {
        ImGui::Text("Camera Height:   ");
        ImGui::SameLine();
        ImGui::SliderFloat("##3rdPersonOffset", &settings.csFollowCamHeightOffset, 0.0f, 5.0f, "%.3f", flags);
      }

      if (!followInstance) {
        ImGui::EndDisabled();
      }

    }

    if (modInstCamData.micSelectedCamera == 0) {
      ImGui::EndDisabled();
    }

    /* disable settings in locked 3rd person mode */
    if (!(followInstance || settings.csCamType == cameraType::stationary)) {
      ImGui::Text("Camera Position: ");
      ImGui::SameLine();
      ImGui::SliderFloat3("##CameraPos", glm::value_ptr(settings.csWorldPosition), -75.0f, 75.0f, "%.3f", flags);
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        modInstCamData.micSettingsContainer->applyEditCameraSettings(modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
                                                                       settings, savedCameraSettings);
        savedCameraSettings = settings;
        modInstCamData.micSetConfigDirtyCallbackFunction(true);
      }

      ImGui::Text("View Azimuth:    ");
      ImGui::SameLine();
      ImGui::SliderFloat("##CamAzimuth", &settings.csViewAzimuth, 0.0f, 360.0f, "%.3f", flags);
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        modInstCamData.micSettingsContainer->applyEditCameraSettings(modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
                                                                       settings, savedCameraSettings);
        savedCameraSettings = settings;
        modInstCamData.micSetConfigDirtyCallbackFunction(true);
      }

      ImGui::Text("View Elevation:  ");
      ImGui::SameLine();
      ImGui::SliderFloat("##CamElevation", &settings.csViewElevation, -89.0f, 89.0f, "%.3f", flags);
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        modInstCamData.micSettingsContainer->applyEditCameraSettings(modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
                                                                       settings, savedCameraSettings);
        savedCameraSettings = settings;
        modInstCamData.micSetConfigDirtyCallbackFunction(true);
      }
    } // end of locked cam type third person

    /* force projection for first and  third person cam */
    if (settings.csCamType == cameraType::firstPerson || settings.csCamType == cameraType::thirdPerson) {
      settings.csCamProjection = cameraProjection::perspective;
    }

    /* remove perspective settings in third person mode */
    if (settings.csCamType != cameraType::firstPerson && settings.csCamType != cameraType::thirdPerson) {
      ImGui::Text("Projection:      ");
      ImGui::SameLine();
      if (ImGui::RadioButton("Perspective",
        settings.csCamProjection == cameraProjection::perspective)) {
        settings.csCamProjection = cameraProjection::perspective;

        modInstCamData.micSettingsContainer->applyEditCameraSettings(modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
                                                                       settings, savedCameraSettings);
        savedCameraSettings = settings;
        modInstCamData.micSetConfigDirtyCallbackFunction(true);
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("Orthogonal",
        settings.csCamProjection == cameraProjection::orthogonal)) {
        settings.csCamProjection = cameraProjection::orthogonal;

        modInstCamData.micSettingsContainer->applyEditCameraSettings(modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
                                                                       settings, savedCameraSettings);
        savedCameraSettings = settings;
        modInstCamData.micSetConfigDirtyCallbackFunction(true);
      }
    }

    if (settings.csCamProjection == cameraProjection::orthogonal) {
      ImGui::BeginDisabled();
    }

    ImGui::Text("Field of View:   ");
    ImGui::SameLine();
    ImGui::SliderInt("##CamFOV", &settings.csFieldOfView, 40, 100, "%d", flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      Logger::log(1, "%s: old FOV is %i\n", __FUNCTION__, savedCameraSettings.csFieldOfView);
      Logger::log(1, "%s: new FOV is %i\n", __FUNCTION__, settings.csFieldOfView);
      modInstCamData.micSettingsContainer->applyEditCameraSettings(modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
                                                                     settings, savedCameraSettings);
      savedCameraSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    if (settings.csCamProjection == cameraProjection::orthogonal) {
      ImGui::EndDisabled();
    }

    /* disable orthoginal scaling in 1st and 3rd person mode, only perspective is allowed  */
    if (settings.csCamType != cameraType::firstPerson && settings.csCamType != cameraType::thirdPerson) {
      if (settings.csCamProjection == cameraProjection::perspective) {
        ImGui::BeginDisabled();
      }

      ImGui::Text("Ortho Scaling:   ");
      ImGui::SameLine();
      ImGui::SliderFloat("##CamOrthoScale", &settings.csOrthoScale, 1.0f, 50.0f, "%.3f", flags);
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        modInstCamData.micSettingsContainer->applyEditCameraSettings(modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
                                                                       settings, savedCameraSettings);
        savedCameraSettings = settings;
        modInstCamData.micSetConfigDirtyCallbackFunction(true);
      }

      if (settings.csCamProjection == cameraProjection::perspective) {
        ImGui::EndDisabled();
      }
    }

    cam->setCameraSettings(settings);
  }

  if (ImGui::CollapsingHeader("Models")) {
    /* state is changed during model deletion, so save it first */
    bool modelListEmtpy = modInstCamData.micModelList.size() == 1;
    std::string selectedModelName = "None";
    std::shared_ptr<AssimpModel> selectedModel = nullptr;
    bool modelIsStatic = true;

    if (!modelListEmtpy) {
      selectedModel = modInstCamData.micModelList.at(modInstCamData.micSelectedModel);
      selectedModelName = selectedModel->getModelFileName().c_str();
      modelIsStatic = !selectedModel->hasAnimations();
    }

    if (modelListEmtpy) {
      ImGui::BeginDisabled();
    }

    ImGui::Text("Models:          ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    if (ImGui::BeginCombo("##ModelCombo",
      /* avoid access the empty model vector or the null model name */
      selectedModelName.c_str())) {
      for (int i = 1; i < modInstCamData.micModelList.size(); ++i) {
        const bool isSelected = (modInstCamData.micSelectedModel == i);
        if (ImGui::Selectable(modInstCamData.micModelList.at(i)->getModelFileName().c_str(), isSelected)) {
          modInstCamData.micSelectedModel = i;
          selectedModelName = modInstCamData.micModelList.at(modInstCamData.micSelectedModel)->getModelFileName().c_str();
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::Text("                 ");
    ImGui::SameLine();
    if (ImGui::Button("Create New Instance")) {
      std::shared_ptr<AssimpModel> currentModel = modInstCamData.micModelList[modInstCamData.micSelectedModel];
      modInstCamData.micInstanceAddCallbackFunction(currentModel);
      /* select new instance */
      modInstCamData.micSelectedInstance = modInstCamData.micAssimpInstances.size() - 1;
    }

    ImGui::SameLine();
    if (ImGui::Button("Delete Model")) {
      ImGui::SetNextWindowPos(ImVec2(renderData.rdWidth / 2.0f, renderData.rdHeight / 2.0f), ImGuiCond_Always);
      ImGui::OpenPopup("Delete Model?");
    }

    if (ImGui::BeginPopupModal("Delete Model?", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
      ImGui::Text("Delete Model '%s'?", modInstCamData.micModelList.at(modInstCamData.micSelectedModel)->getModelFileName().c_str());

      /* cheating a bit to get buttons more to the center */
      ImGui::Indent();
      ImGui::Indent();
      if (ImGui::Button("OK")) {
        modInstCamData.micModelDeleteCallbackFunction(modInstCamData.micModelList.at(modInstCamData.micSelectedModel)->getModelFileName().c_str(), true);

        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    static int manyInstanceCreateNum = 1;
    ImGui::Text("Create Instances:");
    ImGui::SameLine();
    ImGui::PushItemWidth(300.0f);
    ImGui::SliderInt("##MassInstanceCreation", &manyInstanceCreateNum, 1, 100, "%d", flags);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Go!##Create")) {
      std::shared_ptr<AssimpModel> currentModel = modInstCamData.micModelList[modInstCamData.micSelectedModel];
      modInstCamData.micInstanceAddManyCallbackFunction(currentModel, manyInstanceCreateNum);
    }

    if (modelListEmtpy) {
     ImGui::EndDisabled();
    }

    if (modelIsStatic) {
      ImGui::BeginDisabled();
    }

    size_t numTrees = modInstCamData.micBehaviorData.size();
    static std::string selectedTreeName;
    static std::shared_ptr<SingleInstanceBehavior> behavior = nullptr;

    if (numTrees == 0)  {
      selectedTreeName = "None";
      behavior.reset();
      ImGui::BeginDisabled();
    } else {
      if (selectedTreeName.empty() || selectedTreeName == "None") {
        selectedTreeName = modInstCamData.micBehaviorData.begin()->first;
      }
      if (!behavior) {
        behavior = modInstCamData.micBehaviorData.begin()->second;
      }
    }

    ImGui::Text("Change Tree:     ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    if (ImGui::BeginCombo("##ModelTreeCombo", selectedTreeName.c_str())) {
      for (const auto& tree : modInstCamData.micBehaviorData) {
        const bool isSelected = (tree.first == selectedTreeName);
        if (ImGui::Selectable(tree.first.c_str(), isSelected)) {
          selectedTreeName = tree.first;
          behavior = tree.second;
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Set##Model")) {
      modInstCamData.micModelAddBehaviorCallbackFunction(selectedModelName, behavior);
    }
    ImGui::SameLine();

    if (numTrees == 0) {
      ImGui::EndDisabled();
    }

    if (ImGui::Button("Clear##Model")) {
      modInstCamData.micModelDelBehaviorCallbackFunction(selectedModelName);
    }

    if (modelIsStatic) {
      ImGui::EndDisabled();
    }

    bool isNavTarget = false;
    if (modelListEmtpy) {
      ImGui::BeginDisabled();
    } else{
      std::shared_ptr<AssimpModel> currentModel = modInstCamData.micModelList[modInstCamData.micSelectedModel];
      isNavTarget = currentModel->isNavigationTarget();
    }
    ImGui::Text("Use as NavTarget:");
    ImGui::SameLine();
    ImGui::Checkbox("##ModelIsNavTarget", &isNavTarget);
    if (modelListEmtpy) {
      ImGui::EndDisabled();
    } else {
      std::shared_ptr<AssimpModel> currentModel = modInstCamData.micModelList[modInstCamData.micSelectedModel];
      currentModel->setAsNavigationTarget(isNavTarget);
    }
  }

  if (ImGui::CollapsingHeader("Levels")) {
    /* state is changed during model deletion, so save it first */
    bool levelListEmtpy = modInstCamData.micLevels.size() == 1;
    bool nullLevelSelected = modInstCamData.micSelectedLevel == 0;
    std::string selectedLevelName = "None";

    static std::shared_ptr<AssimpLevel> currentLevel = nullptr;
    LevelSettings settings;
    if (!nullLevelSelected) {
      if (currentLevel != modInstCamData.micLevels.at(modInstCamData.micSelectedLevel)) {
        currentLevel = modInstCamData.micLevels.at(modInstCamData.micSelectedLevel);
      }
      settings = currentLevel->getLevelSettings();
      selectedLevelName = currentLevel->getLevelFileName().c_str();
    }

    if (levelListEmtpy) {
      ImGui::BeginDisabled();
    }

    /* map with loaded levels */
    ImGui::Text("Levels:            ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    if (ImGui::BeginCombo("##LevelCombo",
      selectedLevelName.c_str())) {
      for (int i = 1; i < modInstCamData.micLevels.size(); ++i) {
        const bool isSelected = (modInstCamData.micSelectedLevel == i);
        if (ImGui::Selectable(modInstCamData.micLevels.at(i)->getLevelFileName().c_str(), isSelected)) {
          modInstCamData.micSelectedLevel = i;
          settings = modInstCamData.micLevels.at(modInstCamData.micSelectedLevel)->getLevelSettings();
          selectedLevelName = modInstCamData.micLevels.at(modInstCamData.micSelectedLevel)->getLevelFileName().c_str();
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::Button("Delete Level")) {
      ImGui::SetNextWindowPos(ImVec2(renderData.rdWidth / 2.0f, renderData.rdHeight / 2.0f), ImGuiCond_Always);
      ImGui::OpenPopup("Delete Level?");
    }

    if (ImGui::BeginPopupModal("Delete Level?", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
      ImGui::Text("Delete Level '%s'?", modInstCamData.micLevels.at(modInstCamData.micSelectedLevel)->getLevelFileName().c_str());

      /* cheating a bit to get buttons more to the center */
      ImGui::Indent();
      ImGui::Indent();
      if (ImGui::Button("OK")) {
        modInstCamData.micLevelDeleteCallbackFunction(selectedLevelName);
        settings = modInstCamData.micLevels.at(modInstCamData.micSelectedLevel)->getLevelSettings();
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    /* level settings, like instance */
    bool recreateLevelData = false;
    ImGui::Text("Swap Y/Z axes:     ");
    ImGui::SameLine();
    if (ImGui::Checkbox("##LevelAxisSwap", &settings.lsSwapYZAxis)) {
      recreateLevelData = true;
    }

    ImGui::Text("Pos (X/Y/Z):       ");
    ImGui::SameLine();
    ImGui::SliderFloat3("##LevelPos", glm::value_ptr(settings.lsWorldPosition),
      -150.0f, 150.0f, "%.3f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      recreateLevelData = true;
    }

    ImGui::Text("Rotation (X/Y/Z):  ");
    ImGui::SameLine();
    ImGui::SliderFloat3("##LevelRot", glm::value_ptr(settings.lsWorldRotation),
      -180.0f, 180.0f, "%.3f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      recreateLevelData = true;
    }

    ImGui::Text("Scale:             ");
    ImGui::SameLine();
    ImGui::SliderFloat("##LevelScale", &settings.lsScale,
      0.001f, 10.0f, "%.4f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      recreateLevelData = true;
    }

    ImGui::Text("                   ");
    ImGui::SameLine();
    if (ImGui::Button("Reset Values to Zero##Level")) {
      LevelSettings defaultSettings{};
      std::string levelFileName = settings.lsLevelFilename;
      std::string levelFileNamePath = settings.lsLevelFilenamePath;

      settings = defaultSettings;
      settings.lsLevelFilename = levelFileName;
      settings.lsLevelFilenamePath = levelFileNamePath;

      recreateLevelData = true;
    }

    ImGui::Text("Colliding Tris:    %10i", renderData.rdNumberOfCollidingTriangles);
    ImGui::Text("Ground Tris:       %10i", renderData.rdNumberOfCollidingGroundTriangles);

    ImGui::Text("Max Ground Slope:  ");
    ImGui::SameLine();
    ImGui::SliderFloat("##MaxSlope", &renderData.rdMaxLevelGroundSlopeAngle,
      0.0f, 45.0f, "%.2f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      recreateLevelData = true;
    }

    ImGui::Text("Max Stair Height:  ");
    ImGui::SameLine();
    ImGui::SliderFloat("##MaxStairHeight", &renderData.rdMaxStairstepHeight,
      0.1f, 3.0f, "%.2f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      recreateLevelData = true;
    }

    ImGui::Text("Simple Gravity:    ");
    ImGui::SameLine();
    ImGui::Checkbox("##EnableGravity", &renderData.rdEnableSimpleGravity);

    ImGui::Text("Draw AABB:         ");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawLevelAABB", &renderData.rdDrawLevelAABB);

    ImGui::Text("Draw Wireframe:    ");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawLevelWireframe", &renderData.rdDrawLevelWireframe);

    ImGui::Text("Draw Octree:       ");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawLevelOctree", &renderData.rdDrawLevelOctree);

    ImGui::Text("Octree Max Depth:  ");
    ImGui::SameLine();
    ImGui::SliderInt("##LevelOctreeMaxDepth", &renderData.rdLevelOctreeMaxDepth, 1, 10, "%d", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      recreateLevelData = true;
    }

    ImGui::Text("Octree Threshold:  ");
    ImGui::SameLine();
    ImGui::SliderInt("##LevelOctreeThreshold", &renderData.rdLevelOctreeThreshold, 1, 20, "%d", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      recreateLevelData = true;
    }

    ImGui::Text("Draw Ground Tris:  ");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawGroundTriangles", &renderData.rdDrawGroundTriangles);

    ImGui::Text("Draw Collisions:   ");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawLevelCollidingTriangles", &renderData.rdDrawLevelCollisionTriangles);

    ImGui::Text("Draw Neighbor Tris:");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawGroundNeihgbors", &renderData.rdDrawNeighborTriangles);

    ImGui::Text("Draw Instance Path:");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawInstancePaths", &renderData.rdDrawInstancePaths);

    if (!nullLevelSelected) {
      modInstCamData.micLevels.at(modInstCamData.micSelectedLevel)->setLevelSettings(settings);
      if (recreateLevelData) {
        modInstCamData.micLevelGenerateLevelDataCallbackFunction();
      }
    }

    if (levelListEmtpy) {
     ImGui::EndDisabled();
    }

    // AABB recalc button, if too much to be done when changing orientation
    // later: Normal angle etc. for nav mesh
  }

  if (ImGui::CollapsingHeader("Model Idle/Walk/Run Blendings")) {
    /* close the other animation header*/
    ImGui::GetStateStorage()->SetInt(ImGui::GetID("Model Animation Mappings"), 0);
    ImGui::GetStateStorage()->SetInt(ImGui::GetID("Model Allowed Clip Orders"), 0);

    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    static std::shared_ptr<AssimpInstance> currentInstance = nullptr;
    static std::shared_ptr<AssimpModel> currentModel = nullptr;

    InstanceSettings settings;
    ModelSettings modSettings;
    size_t numberOfClips = 0;

    static int clipOne = 0;
    static int clipTwo = 0;
    static int clipThree = 0;

    static float clipOneSpeed = 1.0f;
    static float clipTwoSpeed = 1.0f;
    static float clipThreeSpeed = 1.0f;

    static moveDirection direction = moveDirection::any;

    static float blendFactor = 0.0f;

    if (numberOfInstances > 0) {
      settings = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getInstanceSettings();
      currentModel = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getModel();

      numberOfClips = currentModel->getAnimClips().size();
      modSettings = currentModel->getModelSettings();

      if (currentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
        currentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);

        currentModel = currentInstance->getModel();
        numberOfClips = currentModel->getAnimClips().size();
        modSettings = currentModel->getModelSettings();

        if (modSettings.msIWRBlendings.size() > 0) {
          direction = modSettings.msIWRBlendings.begin()->first;
          IdleWalkRunBlending blend = modSettings.msIWRBlendings.begin()->second;
          clipOne = blend.iwrbIdleClipNr;
          clipOneSpeed = blend.iwrbIdleClipSpeed;
          clipTwo = blend.iwrbWalkClipNr;
          clipTwoSpeed = blend.iwrbWalkClipSpeed;
          clipThree = blend.iwrbRunClipNr;
          clipThreeSpeed = blend.iwrbRunClipSpeed;
        } else {
          clipOne = 0;
          clipTwo = 0;
          clipThree = 0;

          clipOneSpeed = 1.0f;
          clipTwoSpeed = 1.0f;
          clipThreeSpeed = 1.0f;

          direction = moveDirection::any;
        }

        blendFactor = 0.0f;
        currentModel->setModelSettings(modSettings);
      }
    }

    if (numberOfInstances > 0 && numberOfClips > 0) {
      std::vector<std::shared_ptr<AssimpAnimClip>> animClips =
        modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getModel()->getAnimClips();

      ImGui::Text("Dir: ");
      ImGui::SameLine();
      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##DirCombo",
        modInstCamData.micMoveDirectionMap.at(direction).c_str())) {
        for (int i = 0; i < modInstCamData.micMoveDirectionMap.size(); ++i) {
          if (modInstCamData.micMoveDirectionMap[static_cast<moveDirection>(i)].empty()) {
            continue;
          }
          const bool isSelected = (static_cast<int>(direction) == i);
          if (ImGui::Selectable(modInstCamData.micMoveDirectionMap[static_cast<moveDirection>(i)].c_str(), isSelected)) {
            direction = static_cast<moveDirection>(i);
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      ImGui::Text("Idle:");
      ImGui::SameLine();
      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##FirstClipCombo",
        animClips.at(clipOne)->getClipName().c_str())) {
        for (int i = 0; i < animClips.size(); ++i) {
          const bool isSelected = (clipOne == i);
          if (ImGui::Selectable(animClips.at(i)->getClipName().c_str(), isSelected)) {
            clipOne = i;
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::PushItemWidth(200.0f);
      ImGui::SliderFloat("##ClipOneSpeed", &clipOneSpeed,
        0.0f, 15.0f, "%.4f", flags);
      ImGui::PopItemWidth();

      ImGui::Text("Walk:");
      ImGui::SameLine();
      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##SecondClipCombo",
        animClips.at(clipTwo)->getClipName().c_str())) {
        for (int i = 0; i < animClips.size(); ++i) {
          const bool isSelected = (clipTwo == i);
          if (ImGui::Selectable(animClips.at(i)->getClipName().c_str(), isSelected)) {
            clipTwo = i;
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::PushItemWidth(200.0f);
      ImGui::SliderFloat("##ClipTwoSpeed", &clipTwoSpeed,
        0.0f, 15.0f, "%.4f", flags);
      ImGui::PopItemWidth();

      ImGui::Text("Run: ");
      ImGui::SameLine();
      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##ThirdClipCombo",
        animClips.at(clipThree)->getClipName().c_str())) {
        for (int i = 0; i < animClips.size(); ++i) {
          const bool isSelected = (clipThree == i);
          if (ImGui::Selectable(animClips.at(i)->getClipName().c_str(), isSelected)) {
            clipThree = i;
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::PushItemWidth(200.0f);
      ImGui::SliderFloat("##ClipThreeSpeed", &clipThreeSpeed,
        0.0f, 15.0f, "%.4f", flags);
      ImGui::PopItemWidth();

      ImGui::SameLine();
      if (ImGui::Button("Save##Blending")) {
        IdleWalkRunBlending blend;
        blend.iwrbIdleClipNr = clipOne;
        blend.iwrbIdleClipSpeed = clipOneSpeed;
        blend.iwrbWalkClipNr = clipTwo;
        blend.iwrbWalkClipSpeed = clipTwoSpeed;
        blend.iwrbRunClipNr = clipThree;
        blend.iwrbRunClipSpeed = clipThreeSpeed;

        modSettings.msIWRBlendings[direction] = blend;
      }

      ImGui::Text("      %-12s %14s %22s", animClips.at(clipOne)->getClipName().c_str(), animClips.at(clipTwo)->getClipName().c_str(), animClips.at(clipThree)->getClipName().c_str());
      ImGui::Text("Test:");
      ImGui::SameLine();
      ImGui::PushItemWidth(350.0f);
      ImGui::SliderFloat("##ClipBlending", &blendFactor, 0.0f, 2.0f, "", flags);
      ImGui::PopItemWidth();

      if (blendFactor <= 1.0f) {
        settings.isFirstAnimClipNr = clipOne;
        settings.isSecondAnimClipNr = clipTwo;
        settings.isAnimBlendFactor = blendFactor;
        settings.isAnimSpeedFactor = glm::mix(clipOneSpeed, clipTwoSpeed, settings.isAnimBlendFactor);
      } else {
        settings.isFirstAnimClipNr = clipTwo;
        settings.isSecondAnimClipNr = clipThree;
        settings.isAnimBlendFactor = blendFactor - 1.0f;
        settings.isAnimSpeedFactor = glm::mix(clipTwoSpeed, clipThreeSpeed, settings.isAnimBlendFactor);
      }

      unsigned int buttonId = 0;
      for (auto iter = modSettings.msIWRBlendings.begin(); iter != modSettings.msIWRBlendings.end(); /* done while erasing */) {
        moveDirection dir = (*iter).first;
        IdleWalkRunBlending blend = (*iter).second;
        ImGui::Text("%8s: %s(%2.2f)/%s(%2.2f)/%s(%2.2f)", modInstCamData.micMoveDirectionMap[dir].c_str(),
          animClips.at(blend.iwrbIdleClipNr)->getClipName().c_str(),
          blend.iwrbIdleClipSpeed,
          animClips.at(blend.iwrbWalkClipNr)->getClipName().c_str(),
          blend.iwrbWalkClipSpeed,
          animClips.at(blend.iwrbRunClipNr)->getClipName().c_str(),
          blend.iwrbRunClipSpeed
        );

        ImGui::SameLine();
        ImGui::PushID(buttonId++);
        if (ImGui::Button("Edit##Blending")) {
          direction = dir;
          clipOne = blend.iwrbIdleClipNr;
          clipOneSpeed = blend.iwrbIdleClipSpeed;
          clipTwo = blend.iwrbWalkClipNr;
          clipTwoSpeed = blend.iwrbWalkClipSpeed;
          clipThree = blend.iwrbRunClipNr;
          clipThreeSpeed = blend.iwrbRunClipSpeed;
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(buttonId++);
        if (ImGui::Button("Remove##Blending")) {
          iter = modSettings.msIWRBlendings.erase(iter);
        } else {
          ++iter;
        }
        ImGui::PopID();
      }

      currentInstance->setInstanceSettings(settings);
      currentModel->setModelSettings(modSettings);
    }
  }

  if (ImGui::CollapsingHeader("Model Animation Mappings")) {
    /* close the other animation header*/
    ImGui::GetStateStorage()->SetInt(ImGui::GetID("Model Idle/Walk/Run Blendings"), 0);
    ImGui::GetStateStorage()->SetInt(ImGui::GetID("Model Allowed Clip Orders"), 0);


    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    static std::shared_ptr<AssimpInstance> currentInstance = nullptr;
    static std::shared_ptr<AssimpModel> currentModel = nullptr;

    InstanceSettings settings;
    ModelSettings modSettings;
    size_t numberOfClips = 0;

    static moveState state = static_cast<moveState>(0);
    static int clipNr = 0;
    static float clipSpeed = 1.0f;

    if (numberOfInstances > 0) {
      settings = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getInstanceSettings();
      currentModel = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getModel();

      numberOfClips = currentModel->getAnimClips().size();
      modSettings = currentModel->getModelSettings();

      if (currentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
        currentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);

        currentModel = currentInstance->getModel();
        numberOfClips = currentModel->getAnimClips().size();
        modSettings = currentModel->getModelSettings();

        if (modSettings.msActionClipMappings.size() > 0) {
          state = modSettings.msActionClipMappings.begin()->first;
          ActionAnimation savedAnim = modSettings.msActionClipMappings.begin()->second;
          clipNr = savedAnim.aaClipNr;
          clipSpeed = savedAnim.aaClipSpeed;
        } else {
          state = static_cast<moveState>(0);
          clipNr = 0;
          clipSpeed = 1.0f;
        }

        currentModel->setModelSettings(modSettings);
      }
    }

    if (numberOfInstances > 0 && numberOfClips > 0) {
      std::vector<std::shared_ptr<AssimpAnimClip>> animClips =
        modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getModel()->getAnimClips();

      ImGui::Text("State           Clip           Speed");
      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##StateCombo",
        modInstCamData.micMoveStateMap.at(state).c_str())) {
        /* skip idle/walk/run */
        for (int i = 3; i < static_cast<int>(moveState::NUM); ++i) {
          const bool isSelected = (static_cast<int>(state) == i);
          if (ImGui::Selectable(modInstCamData.micMoveStateMap[static_cast<moveState>(i)].c_str(), isSelected)) {
            state = static_cast<moveState>(i);
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##ActionClipCombo",
        animClips.at(clipNr)->getClipName().c_str())) {
        for (int i = 0; i < animClips.size(); ++i) {
          const bool isSelected = (clipNr == i);
          if (ImGui::Selectable(animClips.at(i)->getClipName().c_str(), isSelected)) {
            clipNr = i;
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::PushItemWidth(200.0f);
      ImGui::SliderFloat("##ActionClipSpeed", &clipSpeed,
        0.0f, 15.0f, "%.4f", flags);
      ImGui::PopItemWidth();

      ImGui::SameLine();
      if (ImGui::Button("Save##Action")) {
        ActionAnimation anim;
        anim.aaClipNr = clipNr;
        anim.aaClipSpeed = clipSpeed;

        modSettings.msActionClipMappings[state] = anim;
      }

      unsigned int buttonId = 0;
      for (auto iter = modSettings.msActionClipMappings.begin(); iter != modSettings.msActionClipMappings.end(); /* done while erasing */) {
        moveState savedState = (*iter).first;
        ActionAnimation anim = (*iter).second;
        ImGui::Text("%8s: %s(%2.2f)", modInstCamData.micMoveStateMap[savedState].c_str(),
          animClips.at(anim.aaClipNr)->getClipName().c_str(),
          anim.aaClipSpeed
        );

        ImGui::SameLine();
        ImGui::PushID(buttonId++);
        if (ImGui::Button("Edit##Action")) {
          state = savedState;
          clipNr = anim.aaClipNr;
          clipSpeed = anim.aaClipSpeed;
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(buttonId++);
        if (ImGui::Button("Remove##Action")) {
          iter = modSettings.msActionClipMappings.erase(iter);
        } else {
          ++iter;
        }
        ImGui::PopID();
      }

      settings.isFirstAnimClipNr = clipNr;
      settings.isSecondAnimClipNr = clipNr;
      settings.isAnimSpeedFactor = clipSpeed;
      settings.isAnimBlendFactor = 0.0f;

      currentInstance->setInstanceSettings(settings);
      currentModel->setModelSettings(modSettings);
    }
  }

  if (ImGui::CollapsingHeader("Model Allowed Clip Orders")) {
    /* close the other animation header*/
    ImGui::GetStateStorage()->SetInt(ImGui::GetID("Model Idle/Walk/Run Blendings"), 0);
    ImGui::GetStateStorage()->SetInt(ImGui::GetID("Model Animation Mappings"), 0);

    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    static std::shared_ptr<AssimpInstance> currentInstance = nullptr;
    static std::shared_ptr<AssimpModel> currentModel = nullptr;

    ModelSettings modSettings;
    size_t numberOfClips = 0;

    static moveState stateOne = moveState::idle;
    static moveState stateTwo = moveState::idle;

    if (numberOfInstances > 0) {
      currentModel = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getModel();

      numberOfClips = currentModel->getAnimClips().size();
      modSettings = currentModel->getModelSettings();

      if (currentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
        currentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
      }
    }

    if (numberOfInstances > 0 && numberOfClips > 0) {
      std::vector<std::shared_ptr<AssimpAnimClip>> animClips =
        modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getModel()->getAnimClips();

      ImGui::Text("Source          Destination");

      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##SourceStateCombo",
        modInstCamData.micMoveStateMap.at(stateOne).c_str())) {
        for (int i = 0; i < static_cast<int>(moveState::NUM); ++i) {
          const bool isSelected = (static_cast<int>(stateOne) == i);
          if (ImGui::Selectable(modInstCamData.micMoveStateMap[static_cast<moveState>(i)].c_str(), isSelected)) {
            stateOne = static_cast<moveState>(i);
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##DestStateCombo",
        modInstCamData.micMoveStateMap.at(stateTwo).c_str())) {
        for (int i = 0; i < static_cast<int>(moveState::NUM); ++i) {
          const bool isSelected = (static_cast<int>(stateTwo) == i);
          if (ImGui::Selectable(modInstCamData.micMoveStateMap[static_cast<moveState>(i)].c_str(), isSelected)) {
            stateTwo = static_cast<moveState>(i);
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      if (ImGui::Button("Save##Order")) {
        std::pair<moveState, moveState> order = std::make_pair(stateOne, stateTwo);
        modSettings.msAllowedStateOrder.insert(order);
      }

      unsigned int buttonId = 0;
      for (auto iter = modSettings.msAllowedStateOrder.begin(); iter != modSettings.msAllowedStateOrder.end(); /* done while erasing */) {
        std::pair<moveState, moveState> order = *iter;

        ImGui::Text("From: %s to %s (and back)", modInstCamData.micMoveStateMap.at(order.first).c_str(), modInstCamData.micMoveStateMap.at(order.second).c_str());

        ImGui::SameLine();
        ImGui::PushID(buttonId++);
        if (ImGui::Button("Edit##Order")) {
          stateOne = order.first;
          stateTwo = order.second;
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(buttonId++);
        if (ImGui::Button("Remove##Order")) {
          iter = modSettings.msAllowedStateOrder.erase(iter);
        } else {
          ++iter;
        }
        ImGui::PopID();
      }

      currentModel->setModelSettings(modSettings);
    }
  }

  if (ImGui::CollapsingHeader("Model Head Movement Animation Mappings")) {
    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    static std::shared_ptr<AssimpInstance> currentInstance = nullptr;
    static std::shared_ptr<AssimpModel> currentModel = nullptr;

    InstanceSettings settings;
    ModelSettings modSettings;
    static int clipNr = 0;

    if (numberOfInstances > 0) {
      if (currentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
        currentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
        currentModel = currentInstance->getModel();
        clipNr = 0;
      }
    }

    if (numberOfInstances > 0 && currentModel->hasAnimations()) {
      settings = currentInstance->getInstanceSettings();
      modSettings = currentModel->getModelSettings();

      std::vector<std::shared_ptr<AssimpAnimClip>> animClips =
        modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getModel()->getAnimClips();

      /* init mapping with default values if empty */
      if (modSettings.msHeadMoveClipMappings.size() == 0) {
        for (int i = 0; i < static_cast<int>(headMoveDirection::NUM); ++i) {
          modSettings.msHeadMoveClipMappings[static_cast<headMoveDirection>(i)] = -1;
        }
      }

      ImGui::Text("       Clip:");
      ImGui::SameLine();
      ImGui::PushItemWidth(160.0f);
      if (ImGui::BeginCombo("##HeadMoveClipCombo",
        animClips.at(clipNr)->getClipName().c_str())) {
        for (int i = 0; i < animClips.size(); ++i) {
          const bool isSelected = (clipNr == i);
          if (ImGui::Selectable(animClips.at(i)->getClipName().c_str(), isSelected)) {
            clipNr = i;
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      unsigned int buttonId = 0;
      for (int i = 0; i < static_cast<int>(headMoveDirection::NUM); ++i) {
        headMoveDirection headMoveDir = static_cast<headMoveDirection>(i);
        ImGui::Text("%10s:", modInstCamData.micHeadMoveAnimationNameMap[headMoveDir].c_str());

        ImGui::SameLine();
        if (modSettings.msHeadMoveClipMappings[headMoveDir] >= 0) {
          ImGui::Text("%20s", animClips.at(modSettings.msHeadMoveClipMappings[headMoveDir])->getClipName().c_str());
        } else {
          ImGui::Text("%20s", "None");
        }

        ImGui::SameLine();
        ImGui::PushID(buttonId++);
        if (ImGui::Button("Set##HeadMove")) {
          modSettings.msHeadMoveClipMappings[headMoveDir] = clipNr;
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(buttonId++);
        if (ImGui::Button("Remove##HeadMove")) {
          modSettings.msHeadMoveClipMappings[headMoveDir] = -1;
        }
        ImGui::PopID();
      }

      ImGui::Text("Test Left/Right: ");
      ImGui::SameLine();
      ImGui::PushItemWidth(150.0f);
      ImGui::SliderFloat("##HeadLeftRightTest", &settings.isHeadLeftRightMove,
            -1.0f, 1.0f, "%.2f", flags);
      ImGui::PopItemWidth();

      ImGui::Text("Test Up/Down:    ");
      ImGui::SameLine();
      ImGui::PushItemWidth(150.0f);
      ImGui::SliderFloat("##HeadUpDownTeast", &settings.isHeadUpDownMove,
            -1.0f, 1.0f, "%.2f", flags);
      ImGui::PopItemWidth();

      currentInstance->setInstanceSettings(settings);
      currentModel->setModelSettings(modSettings);
    }

  }

  if (ImGui::CollapsingHeader("Model Bounding Sphere Adjustment")) {
    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    static std::shared_ptr<AssimpInstance> currentInstance = nullptr;
    static std::shared_ptr<AssimpModel> currentModel = nullptr;

    InstanceSettings settings;
    ModelSettings modSettings;

    static std::vector<std::string> nodeNames;
    static int selectedNode = 0;
    static float adjustmentValue = 1.0f;
    static glm::vec3 positionOffset = glm::vec3(0.0f);

    if (numberOfInstances > 0 && modInstCamData.micSelectedInstance > 0) {
      if (currentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
        currentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
        settings = currentInstance->getInstanceSettings();
        currentModel = currentInstance->getModel();

        nodeNames = currentModel->getBoneNameList();

        selectedNode = 0;
      }

      modSettings = currentModel->getModelSettings();
      glm::vec4 value = modSettings.msBoundingSphereAdjustments.at(selectedNode);
      adjustmentValue = value.w;
      positionOffset = glm::vec3(value.x, value.y, value.z);

      if (modInstCamData.micModelList.at(modInstCamData.micSelectedModel)->getBoneNameList().size() > 0) {
        ImGui::Text("Node:    ");
        ImGui::SameLine();
        ImGui::PushItemWidth(150.0f);
        if (ImGui::BeginCombo("##NodeListCombo",
          nodeNames.at(selectedNode).c_str())) {
          for (int i = 0; i < nodeNames.size(); ++i) {
            const bool isSelected = (selectedNode == i);
            if (ImGui::Selectable(nodeNames.at(i).c_str(), isSelected)) {
              selectedNode = i;

              glm::vec4 value = modSettings.msBoundingSphereAdjustments.at(selectedNode);
              adjustmentValue = value.w;
              positionOffset = glm::vec3(value.x, value.y, value.z);
            }

            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::Text("Scaling: ");
        ImGui::SameLine();
        ImGui::SliderFloat("##SphereScale", &adjustmentValue,
          0.01f, 10.0f, "%.4f", flags);

        ImGui::Text("Position:");
        ImGui::SameLine();
        ImGui::SliderFloat3("##SphereOffset", glm::value_ptr(positionOffset), -1.0f, 1.0f, "%.3f", flags);

        modSettings.msBoundingSphereAdjustments.at(selectedNode) = glm::vec4(positionOffset, adjustmentValue);
      }

      currentModel->setModelSettings(modSettings);
    }
  }

  if (ImGui::CollapsingHeader("Model Feet Inverse Kinematics")) {
    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    static std::shared_ptr<AssimpInstance> currentInstance = nullptr;
    static std::shared_ptr<AssimpModel> currentModel = nullptr;

    ModelSettings modSettings;

    static std::vector<std::string> nodeNames;

    if (numberOfInstances > 0 && modInstCamData.micSelectedInstance > 0) {
      if (currentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
        currentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
        currentModel = currentInstance->getModel();
        nodeNames = currentModel->getBoneNameList();
      }

      ImGui::Text("Enable IK:      ");
      ImGui::SameLine();
      ImGui::Checkbox("##FeetIK", &renderData.rdEnableFeetIK);

      if (!renderData.rdEnableFeetIK) {
        ImGui::BeginDisabled();
      }

      ImGui::Text("IK Iterations:  ");
      ImGui::SameLine();
      ImGui::PushItemWidth(300.0f);
      ImGui::SliderInt("##IKIterations", &renderData.rdNumberOfIkIteratons, 1, 15, "%d", flags);
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        modInstCamData.micIkIterationsCallbackFunction(renderData.rdNumberOfIkIteratons);
      }

      modSettings = currentModel->getModelSettings();

      /* read out values to use shorter lines */
      int leftEffector = modSettings.msFootIKChainPair.at(0).first;
      int leftRoot = modSettings.msFootIKChainPair.at(0).second;
      int rightEffector = modSettings.msFootIKChainPair.at(1).first;
      int rightRoot = modSettings.msFootIKChainPair.at(1).second;

      bool leftFootChainChanged = false;
      bool rightFootChainChanged = false;

      if (currentModel->getBoneNameList().size() > 0) {
        ImGui::Text("                  Effector Node         Root Node");
        ImGui::Text("Left Foot:      ");
        ImGui::SameLine();
        ImGui::PushItemWidth(150.0f);
        if (ImGui::BeginCombo("##LeftFootEffectorCombo",
          nodeNames.at(leftEffector).c_str())) {
          for (int i = 0; i < nodeNames.size(); ++i) {
            const bool isSelected = (leftEffector == i);
            if (ImGui::Selectable(nodeNames.at(i).c_str(), isSelected)) {
              leftEffector = i;
              leftFootChainChanged = true;
            }

            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();
        ImGui::PushItemWidth(150.0f);
        if (ImGui::BeginCombo("##LeftFootRootCombo",
          nodeNames.at(leftRoot).c_str())) {
          for (int i = 0; i < nodeNames.size(); ++i) {
            const bool isSelected = (leftRoot == i);
            if (ImGui::Selectable(nodeNames.at(i).c_str(), isSelected)) {
              leftRoot = i;
              leftFootChainChanged = true;
            }

            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::Text("Right Foot:     ");
        ImGui::SameLine();
        ImGui::PushItemWidth(150.0f);
        if (ImGui::BeginCombo("##RightFootEffectorCombo",
          nodeNames.at(rightEffector).c_str())) {
          for (int i = 0; i < nodeNames.size(); ++i) {
            const bool isSelected = (rightEffector == i);
            if (ImGui::Selectable(nodeNames.at(i).c_str(), isSelected)) {
              rightEffector = i;
              rightFootChainChanged = true;
            }

            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();
        ImGui::PushItemWidth(150.0f);
        if (ImGui::BeginCombo("##RightFootRootCombo",
          nodeNames.at(rightRoot).c_str())) {
          for (int i = 0; i < nodeNames.size(); ++i) {
            const bool isSelected = (rightRoot == i);
            if (ImGui::Selectable(nodeNames.at(i).c_str(), isSelected)) {
              rightRoot = i;
              rightFootChainChanged = true;
            }

            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
      }

      ImGui::Text("Draw Debbug:    ");
      ImGui::SameLine();
      ImGui::Checkbox("##IKDebug", &renderData.rdDrawIKDebugLines);

      /* write (possibly updated) values back */
      modSettings.msFootIKChainPair.at(0).first = leftEffector;
      modSettings.msFootIKChainPair.at(0).second = leftRoot;
      modSettings.msFootIKChainPair.at(1).first = rightEffector;
      modSettings.msFootIKChainPair.at(1).second= rightRoot;

      currentModel->setModelSettings(modSettings);

      if (leftFootChainChanged) {
        currentModel->setIkNodeChain(0, leftEffector, leftRoot);
      }
      if (rightFootChainChanged) {
        currentModel->setIkNodeChain(1, rightEffector, rightRoot);
      }

      if (!renderData.rdEnableFeetIK) {
        ImGui::EndDisabled();
      }
    }

  }

  if (ImGui::CollapsingHeader("Instances")) {
    bool modelListEmtpy = modInstCamData.micModelList.size() == 1;
    bool nullInstanceSelected = modInstCamData.micSelectedInstance == 0;
    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    ImGui::Text("Total Instances:   %ld", numberOfInstances);

    if (modelListEmtpy) {
     ImGui::BeginDisabled();
    }

    ImGui::Text("Select Instance:  ");
    ImGui::SameLine();
    ImGui::PushButtonRepeat(true);
    if (ImGui::ArrowButton("##Left", ImGuiDir_Left) &&
      modInstCamData.micSelectedInstance > 1) {
      modInstCamData.micSelectedInstance--;
    }

    if (modelListEmtpy || nullInstanceSelected) {
     ImGui::BeginDisabled();
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(30);
    ImGui::DragInt("##SelInst", &modInstCamData.micSelectedInstance, 1, 1,
                   modInstCamData.micAssimpInstances.size() - 1, "%3d", flags);
    ImGui::PopItemWidth();

    /* DragInt does not like clamp flag */
    modInstCamData.micSelectedInstance = std::clamp(modInstCamData.micSelectedInstance, 0,
                                                static_cast<int>(modInstCamData.micAssimpInstances.size() - 1));

    if (modelListEmtpy || nullInstanceSelected) {
     ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::ArrowButton("##Right", ImGuiDir_Right) &&
      modInstCamData.micSelectedInstance < (modInstCamData.micAssimpInstances.size() - 1)) {
      modInstCamData.micSelectedInstance++;
    }
    ImGui::PopButtonRepeat();

    if (modelListEmtpy || nullInstanceSelected) {
     ImGui::BeginDisabled();
    }

    static InstanceSettings savedInstanceSettings{};
    static std::shared_ptr<AssimpInstance> currentInstance = nullptr;

    static bool modelHasFaceAnims = false;

    InstanceSettings settings;
    if (numberOfInstances > 0) {
      settings = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getInstanceSettings();
      if (currentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
        currentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
        /* overwrite saved settings on instance change */
        savedInstanceSettings = settings;

        std::shared_ptr<AssimpModel> currentModel = currentInstance->getModel();
        modelHasFaceAnims = currentModel->hasAnimMeshes();
      }
    }

    if (modelListEmtpy || nullInstanceSelected) {
     ImGui::EndDisabled();
    }

    if (modelListEmtpy) {
     ImGui::EndDisabled();
    }

    std::string baseModelName = "None";
    if (numberOfInstances > 0 && !nullInstanceSelected) {
      baseModelName = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getModel()->getModelFileName();
    }
    ImGui::Text("Base Model:         %s", baseModelName.c_str());


    if (modelListEmtpy || nullInstanceSelected) {
      ImGui::BeginDisabled();
    }

    ImGui::Text("                  ");
    ImGui::SameLine();
    if (ImGui::Button("Center This Instance##Instance")) {
      modInstCamData.micInstanceCenterCallbackFunction(currentInstance);
    }

    ImGui::SameLine();

    /* we MUST retain the last model */
    unsigned int numberOfInstancesPerModel = 0;
    if (modInstCamData.micAssimpInstances.size() > 1) {
      std::string currentModelName = currentInstance->getModel()->getModelFileName();
      numberOfInstancesPerModel = modInstCamData.micAssimpInstancesPerModel[currentModelName].size();
    }

    if (numberOfInstancesPerModel < 2) {
      ImGui::BeginDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Delete Instance")) {
      modInstCamData.micInstanceDeleteCallbackFunction(currentInstance, true);

      /* read back settings for UI */
      settings = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getInstanceSettings();
    }

    if (numberOfInstancesPerModel < 2) {
      ImGui::EndDisabled();
    }

    ImGui::Text("                  ");
    ImGui::SameLine();
    if (ImGui::Button("Clone Instance")) {
      modInstCamData.micInstanceCloneCallbackFunction(currentInstance);

      /* read back settings for UI */
      settings = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getInstanceSettings();
    }

    static int manyInstanceCloneNum = 1;
    ImGui::Text("Create Clones:    ");
    ImGui::SameLine();
    ImGui::PushItemWidth(300.0f);
    ImGui::SliderInt("##MassInstanceCloning", &manyInstanceCloneNum, 1, 100, "%d", flags);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Go!##Clone")) {
      modInstCamData.micInstanceCloneManyCallbackFunction(currentInstance, manyInstanceCloneNum);

      /* read back settings for UI */
      settings = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getInstanceSettings();
    }

    /* get the new size, in case of a deletion */
    numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    ImGui::Text("Hightlight:       ");
    ImGui::SameLine();
    ImGui::Checkbox("##HighlightInstance", &renderData.rdHighlightSelectedInstance);

    ImGui::Text("Stop Movement:    ");
    ImGui::SameLine();
    ImGui::Checkbox("##StopMovement", &settings.isNoMovement);


    ImGui::Text("Swap Y/Z axes:    ");
    ImGui::SameLine();
    ImGui::Checkbox("##ModelAxisSwap", &settings.isSwapYZAxis);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstCamData.micSettingsContainer->applyEditInstanceSettings(modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance),
        settings, savedInstanceSettings);
      savedInstanceSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    ImGui::Text("Pos (X/Y/Z):      ");
    ImGui::SameLine();
    ImGui::SliderFloat3("##ModelPos", glm::value_ptr(settings.isWorldPosition),
      -75.0f, 75.0f, "%.3f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstCamData.micSettingsContainer->applyEditInstanceSettings(modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance),
        settings, savedInstanceSettings);
      savedInstanceSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    ImGui::Text("Rotation (X/Y/Z): ");
    ImGui::SameLine();
    ImGui::SliderFloat3("##ModelRot", glm::value_ptr(settings.isWorldRotation),
      -180.0f, 180.0f, "%.3f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstCamData.micSettingsContainer->applyEditInstanceSettings(modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance),
        settings, savedInstanceSettings);
      savedInstanceSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    ImGui::Text("Scale:            ");
    ImGui::SameLine();
    ImGui::SliderFloat("##ModelScale", &settings.isScale,
      0.001f, 10.0f, "%.4f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstCamData.micSettingsContainer->applyEditInstanceSettings(modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance),
        settings, savedInstanceSettings);
      savedInstanceSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    ImGui::Text("                  ");
    ImGui::SameLine();
    if (ImGui::Button("Reset Values to Zero##Instance")) {
      modInstCamData.micSettingsContainer->applyEditInstanceSettings(modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance),
        settings, savedInstanceSettings);
      InstanceSettings defaultSettings{};

      /* save and restore index positions */
      int instanceIndex = settings.isInstanceIndexPosition;
      int modelInstanceIndex = settings.isInstancePerModelIndexPosition;
      settings = defaultSettings;
      settings.isInstanceIndexPosition = instanceIndex;
      settings.isInstancePerModelIndexPosition = modelInstanceIndex;

      savedInstanceSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    /* ignore nav target settings on static models */
    std::shared_ptr<AssimpModel> currentModel = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getModel();
    bool modelIsStatic = !currentModel->hasAnimations();

    size_t numTrees = modInstCamData.micBehaviorData.size();
    static std::string selectedTreeName;
    static std::shared_ptr<SingleInstanceBehavior> behavior;

    if (numTrees == 0)  {
      selectedTreeName = "None";
      behavior.reset();
      ImGui::BeginDisabled();
    } else {
      if (selectedTreeName.empty() || selectedTreeName == "None") {
        selectedTreeName = modInstCamData.micBehaviorData.begin()->first;
      }
      if (!behavior) {
        behavior = modInstCamData.micBehaviorData.begin()->second;
      }
    }

    if (modelIsStatic) {
      ImGui::BeginDisabled();
    }

    ImGui::Text("Model Tree:         %s", settings.isNodeTreeName.empty() ? "None" : settings.isNodeTreeName.c_str());
    ImGui::Text("Change Tree:      ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    if (ImGui::BeginCombo("##NodeTreeCombo", selectedTreeName.c_str())) {
      for (const auto& tree : modInstCamData.micBehaviorData) {
        const bool isSelected = (tree.first == selectedTreeName);
        if (ImGui::Selectable(tree.first.c_str(), isSelected)) {
          selectedTreeName = tree.first;
          behavior = tree.second;
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Set##Instance")) {
      settings.isNodeTreeName = selectedTreeName;
      modInstCamData.micInstanceAddBehaviorCallbackFunction(settings.isInstanceIndexPosition, behavior);
    }
    ImGui::SameLine();

    if (numTrees == 0) {
      ImGui::EndDisabled();
    }

    bool nodeTreeEmpty = settings.isNodeTreeName.empty();
    if (nodeTreeEmpty) {
      ImGui::BeginDisabled();
    }
    if (ImGui::Button("Clear##Instance")) {
      modInstCamData.micInstanceDelBehaviorCallbackFunction(settings.isInstanceIndexPosition);
      settings.isNodeTreeName.clear();

      /* HACK to change data in instance while settngs are used */
      modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->setInstanceSettings(settings);
      currentInstance->updateInstanceState(moveState::idle, moveDirection::none);
      settings = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getInstanceSettings();
    }
    if (nodeTreeEmpty) {
      ImGui::EndDisabled();
    }

    if (modelIsStatic){
      ImGui::EndDisabled();
    }


    if (!modelHasFaceAnims) {
      ImGui::BeginDisabled();
    }

    ImGui::Text("Movement State:     %s", modInstCamData.micMoveStateMap.at(settings.isMoveState).c_str());

    ImGui::Text("Face Anim Clip:   ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    if (ImGui::BeginCombo("##FaceAnimClipCombo", modInstCamData.micFaceAnimationNameMap.at(settings.isFaceAnim).c_str())) {
      for (unsigned int i = 0; i < modInstCamData.micFaceAnimationNameMap.size(); ++i) {
        const bool isSelected = (static_cast<int>(settings.isFaceAnim) == i);

        if (ImGui::Selectable(modInstCamData.micFaceAnimationNameMap.at(static_cast<faceAnimation>(i)).c_str(), isSelected)) {
          settings.isFaceAnimWeight = 0.0f;
          settings.isFaceAnim = static_cast<faceAnimation>(i);
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();


    ImGui::Text("MorphAnim Weight: ");
    ImGui::SameLine();
    ImGui::SliderFloat("##MorphAnimWeight", &settings.isFaceAnimWeight, 0.0f, 1.0f, "%.2f", flags);

    if (!modelHasFaceAnims) {
      ImGui::EndDisabled();
    }

    ImGui::Text("Ground Tri:      %10i", settings.isCurrentGroundTriangleIndex);
    ImGui::Text("Neighbor Tris:   %10li", settings.isNeighborGroundTriangles.size());

    std::vector<int> navTargets = modInstCamData.micGetNavTargetsCallbackFunction();
    static int selectedNavTarget = 0;
    size_t numNavTargets = navTargets.size();

    if (selectedNavTarget > numNavTargets) {
      selectedNavTarget = 0;
    }

    if (numNavTargets == 0 || modelIsStatic) {
      ImGui::BeginDisabled();
    }

    ImGui::Text("Enable Navigation:");
    ImGui::SameLine();
    ImGui::Checkbox("##EnableNav", &settings.isNavigationEnabled);

    if (!settings.isNavigationEnabled) {
      ImGui::BeginDisabled();
    }

    ImGui::Text("Nav Target:      %10i", settings.isPathTargetInstance);
    ImGui::Text("Nav Targets:      ");
    ImGui::SameLine();

    if (numNavTargets > 0) {
      ImGui::PushItemWidth(250.0f);
      if (ImGui::BeginCombo("##NavTargetCombo",
        std::to_string(navTargets.at(selectedNavTarget)).c_str())) {
        for (int i = 0; i < numNavTargets; ++i) {
          const bool isSelected = (selectedNavTarget == i);
          if (ImGui::Selectable(std::to_string(navTargets.at(i)).c_str(), isSelected)) {
            selectedNavTarget = i;
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      ImGui::PopItemWidth();
      ImGui::SameLine();

      if (ImGui::Button("Set##Target")) {
        settings.isPathTargetInstance = navTargets.at(selectedNavTarget);
      }
      ImGui::SameLine();

      bool noTargetSelected = (settings.isPathTargetInstance == -1);
      if (noTargetSelected) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Button("Clear##Target")) {
        settings.isPathTargetInstance = -1;
      }
      if (noTargetSelected) {
        ImGui::EndDisabled();
      }

      ImGui::Text("                  ");
      ImGui::SameLine();
      if (ImGui::Button("Center Target##NavTarget")) {
        std::shared_ptr<AssimpInstance> instance =
          modInstCamData.micAssimpInstances.at(navTargets.at(selectedNavTarget));
        modInstCamData.micInstanceCenterCallbackFunction(instance);
      }
    } else {
      ImGui::Text("None");
    }

    if (!settings.isNavigationEnabled) {
      ImGui::EndDisabled();
    }

    if (numNavTargets == 0 || modelIsStatic) {
      ImGui::EndDisabled();
    }

    if (numberOfInstances == 0 || nullInstanceSelected) {
      ImGui::EndDisabled();
    }

    if (numberOfInstances > 0) {
      modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->setInstanceSettings(settings);
    }
  }

  if (ImGui::CollapsingHeader("Node Tree")) {
    ImGuiInputTextFlags textinputFlags = ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCharFilter;

    bool showDuplicateNameDialog = false;
    static std::string newName = "Tree1";

    ImGui::Text("Tree Name: ");
    ImGui::SameLine();
    ImGui::PushItemWidth(150.0f);
    if (ImGui::InputText("##TreeName", &newName, textinputFlags, nameInputFilter)) {
      if (modInstCamData.micBehaviorData.count(newName) > 0) {
        showDuplicateNameDialog = true;
      }
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Create Node Tree")) {
      if (modInstCamData.micBehaviorData.count(newName) > 0) {
        showDuplicateNameDialog = true;
      } else {
        modInstCamData.micBehaviorData[newName] = modInstCamData.micCreateEmptyNodeGraphCallbackFunction();
        modInstCamData.micBehaviorData[newName]->getBehaviorData()->bdName = newName;
      }
    }

    if (showDuplicateNameDialog) {
      ImGui::SetNextWindowPos(ImVec2(renderData.rdWidth / 2.0f, renderData.rdHeight / 2.0f), ImGuiCond_Always);
      ImGui::OpenPopup("Duplicate Tree Name");
      showDuplicateNameDialog = false;
    }

    if (ImGui::BeginPopupModal("Duplicate Tree Name", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
      ImGui::Text("Tree Name '%s' alread exists!", newName.c_str());

      /* cheating a bit to get buttons more to the center */
      ImGui::Indent();
      ImGui::Indent();
      ImGui::Indent();
      ImGui::Indent();
      ImGui::Indent();
      if (ImGui::Button("OK")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    unsigned int buttonId = 0;
    bool showDeleteRequest = false;
    static std::string treeToDelete;

    for (auto iter = modInstCamData.micBehaviorData.begin(); iter != modInstCamData.micBehaviorData.end(); /* done while erasing */) {
      std::string treeName = (*iter).first;
      std::shared_ptr<BehaviorData> treeData = (*iter).second->getBehaviorData();

      size_t nodeSize = treeData->bdGraphNodes.size();
      size_t linkSize = treeData->bdGraphLinks.size();
      ImGui::Text("%8s: %lu node%s, %lu link%s", treeName.c_str(), nodeSize, nodeSize == 1 ? "" : "s",
                  linkSize, linkSize == 1 ? "" : "s");

      ImGui::SameLine();
      ImGui::PushID(buttonId++);
      if (ImGui::Button("Edit##Tree")) {
        modInstCamData.micEditNodeGraphCallbackFunction(treeName);
      }
      ImGui::PopID();
      ImGui::SameLine();
      ImGui::PushID(buttonId++);
      if (ImGui::Button("Remove##Tree")) {
        /* delete empty trees without rquest */
        if (nodeSize > 1) {
          treeToDelete = treeName;
          showDeleteRequest = true;
          ++iter;
        } else {
          iter = modInstCamData.micBehaviorData.erase(iter);
          modInstCamData.micPostNodeTreeDelBehaviorCallbackFunction(treeName);
        }
      } else {
        ++iter;
      }
      ImGui::PopID();
    }

    if (showDeleteRequest) {
      ImGui::SetNextWindowPos(ImVec2(renderData.rdWidth / 2.0f, renderData.rdHeight / 2.0f), ImGuiCond_Always);
      ImGui::OpenPopup("Delete Tree?");
      showDeleteRequest = false;
    }

    if (ImGui::BeginPopupModal("Delete Tree?", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
      ImGui::Text(" Delete Tree '%s'?  ", treeToDelete.c_str());

      /* cheating a bit to get buttons more to the center */
      ImGui::Indent();
      if (ImGui::Button("OK")) {
        modInstCamData.micBehaviorData.erase(treeToDelete);
        modInstCamData.micPostNodeTreeDelBehaviorCallbackFunction(treeToDelete);
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }

  if (ImGui::CollapsingHeader("Collisions")) {
    ImGui::Text("Number of Collisions:  %4li", renderData.rdNumberOfCollisions);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      int averageNumCollisions = 0;
      for (const auto value : mNumCollisionsValues) {
        averageNumCollisions += value;
      }
      averageNumCollisions /= static_cast<float>(mNumNumCollisionValues);
      std::string numCoillisionsOverlay = "now:     " + std::to_string(renderData.rdNumberOfCollisions)
        + "\n30s avg: " + std::to_string(averageNumCollisions);
      ImGui::Text("Collisions");
      ImGui::SameLine();
      ImGui::PlotLines("##NumCollisions", mNumCollisionsValues.data(), mNumCollisionsValues.size(), numCollisionOffset,
        numCoillisionsOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Collisions:             ");
    ImGui::SameLine();
    if (ImGui::RadioButton("None##CollCheck",
      renderData.rdCheckCollisions == collisionChecks::none)) {
      renderData.rdCheckCollisions = collisionChecks::none;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("2D Bounding Box##CollCheck",
      renderData.rdCheckCollisions == collisionChecks::boundingBox)) {
      renderData.rdCheckCollisions = collisionChecks::boundingBox;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Bounding Spheres##CollCheck",
      renderData.rdCheckCollisions == collisionChecks::boundingSpheres)) {
      renderData.rdCheckCollisions = collisionChecks::boundingSpheres;
    }

    ImGui::Text("Draw AABB Lines:        ");
    ImGui::SameLine();
    if (ImGui::RadioButton("None##AABB",
      renderData.rdDrawCollisionAABBs == collisionDebugDraw::none)) {
      renderData.rdDrawCollisionAABBs = collisionDebugDraw::none;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Colliding##AABB",
      renderData.rdDrawCollisionAABBs == collisionDebugDraw::colliding)) {
      renderData.rdDrawCollisionAABBs = collisionDebugDraw::colliding;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("All##AABB",
      renderData.rdDrawCollisionAABBs == collisionDebugDraw::all)) {
      renderData.rdDrawCollisionAABBs = collisionDebugDraw::all;
    }
    ImGui::Text("Draw Bounding Spheres:  ");
    ImGui::SameLine();
    if (ImGui::RadioButton("None##Sphere",
      renderData.rdDrawBoundingSpheres == collisionDebugDraw::none)) {
      renderData.rdDrawBoundingSpheres = collisionDebugDraw::none;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Colliding##Sphere",
      renderData.rdDrawBoundingSpheres == collisionDebugDraw::colliding)) {
      renderData.rdDrawBoundingSpheres = collisionDebugDraw::colliding;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Selected##Sphere",
      renderData.rdDrawBoundingSpheres == collisionDebugDraw::selected)) {
      renderData.rdDrawBoundingSpheres = collisionDebugDraw::selected;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("All##Sphere",
      renderData.rdDrawBoundingSpheres == collisionDebugDraw::all)) {
      renderData.rdDrawBoundingSpheres = collisionDebugDraw::all;
    }
  }

  if (ImGui::CollapsingHeader("Interaction")) {
    ImGui::Text("Interaction:           ");
    ImGui::SameLine();
    ImGui::Checkbox("##EnableInteraction", &renderData.rdInteraction);

    if (!renderData.rdInteraction) {
      ImGui::BeginDisabled();
    }

    ImGui::Text("Number Of Candidates:   %li", renderData.rdNumberOfInteractionCandidates);
    ImGui::Text("Interaction Candidate:  %i", renderData.rdInteractWithInstanceId);

    ImGui::Text("Min Interaction Range: ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    ImGui::SliderFloat("##MinInteractionRange", &renderData.rdInteractionMinRange, 0.0f, 20.0f, "%.3f", flags);
    ImGui::PopItemWidth();

    if (renderData.rdInteractionMinRange > renderData.rdInteractionMaxRange) {
      renderData.rdInteractionMaxRange = renderData.rdInteractionMinRange;
    }

    ImGui::Text("Max Interaction Range: ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    ImGui::SliderFloat("##MaxInteractionRange", &renderData.rdInteractionMaxRange, 0.0f, 20.0f, "%.3f", flags);
    ImGui::PopItemWidth();

    if (renderData.rdInteractionMaxRange < renderData.rdInteractionMinRange) {
      renderData.rdInteractionMinRange = renderData.rdInteractionMaxRange;
    }

    ImGui::Text("Interaction FOV:       ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    ImGui::SliderFloat("##InteractionFOV", &renderData.rdInteractionFOV, 30.0f, 60.0f, "%.3f", flags);
    ImGui::PopItemWidth();

    ImGui::NewLine();

    ImGui::Text("Draw Interaction Range:");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawInteractionRange", &renderData.rdDrawInteractionRange);

    ImGui::Text("Draw Interaction FOV:  ");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawInteractionFOV", &renderData.rdDrawInteractionFOV);

    ImGui::Text("Draw Interaction Debug:");
    ImGui::SameLine();
    if (ImGui::RadioButton("None##Interaction",
      renderData.rdDrawInteractionAABBs == interactionDebugDraw::none)) {
      renderData.rdDrawInteractionAABBs = interactionDebugDraw::none;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("All in Range##Interaction",
      renderData.rdDrawInteractionAABBs == interactionDebugDraw::distance)) {
      renderData.rdDrawInteractionAABBs = interactionDebugDraw::distance;
    }
    ImGui::Text("                       ");
    ImGui::SameLine();
    if (ImGui::RadioButton("Correct Facing##Interaction",
      renderData.rdDrawInteractionAABBs == interactionDebugDraw::facingTowardsUs)) {
      renderData.rdDrawInteractionAABBs = interactionDebugDraw::facingTowardsUs;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Nearest Candidata##Interaction",
      renderData.rdDrawInteractionAABBs == interactionDebugDraw::nearestCandidate)) {
      renderData.rdDrawInteractionAABBs = interactionDebugDraw::nearestCandidate;
    }

    if (!renderData.rdInteraction) {
      ImGui::EndDisabled();
    }
  }

  ImGui::End();
}

void UserInterface::createPositionsWindow(OGLRenderData& renderData, ModelInstanceCamData& modInstCamData) {
  std::shared_ptr<BoundingBox3D> worldBoundaries = modInstCamData.micWorldGetBoundariesCallbackFunction();

  ImGuiWindowFlags posWinFlags = 0;
  ImGui::SetNextWindowBgAlpha(0.5f);

  ImGui::Begin("Instance Positions", nullptr, posWinFlags);

  if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) {
    /* zoom in/out with mouse wheel */
    ImGuiIO& io = ImGui::GetIO();
    mOctreeZoomFactor += 0.025f * io.MouseWheel;
    mOctreeZoomFactor = std::clamp(mOctreeZoomFactor, 0.1f, 5.0f);

    /* rotate octree view when right mouse button is pressed */
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
      mOctreeRotation.y += io.MouseDelta.x;
      mOctreeRotation.x += io.MouseDelta.y;
    }

    /* move octree view when middle mouse button is pressed */
    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
      mOctreeTranslation.x += io.MouseDelta.x;
      mOctreeTranslation.y += io.MouseDelta.y;
    }
  }

  glm::vec4 red = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
  glm::vec4 yellow = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
  glm::vec4 green = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
  glm::vec4 white = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

  mOctreeLines.vertices.clear();
  /* draw octree boxes first */
  const auto treeBoxes = modInstCamData.micOctreeGetBoxesCallback();
  for (const auto& box : treeBoxes) {
    AABB boxAABB{};
    boxAABB.create(box.getFrontTopLeft());
    boxAABB.addPoint(box.getFrontTopLeft() + box.getSize());

    std::shared_ptr<OGLLineMesh> instanceLines = boxAABB.getAABBLines(white);
    mOctreeLines.vertices.insert(mOctreeLines.vertices.end(), instanceLines->vertices.begin(), instanceLines->vertices.end());
  }

  /* draw instance AABBs second */
  for (const auto& instance : modInstCamData.micAssimpInstances) {
    InstanceSettings instSettings = instance->getInstanceSettings();
    int instanceId = instSettings.isInstanceIndexPosition;
    /* skip null instance */
    if (instanceId == 0) {
      continue;
    }

    AABB instanceAABB = instance->getModel()->getAABB(instSettings);

    const auto iter = std::find_if(modInstCamData.micInstanceCollisions.begin(), modInstCamData.micInstanceCollisions.end(),
       [instanceId](std::pair<int, int> values) {
      return instanceId == values.first || instanceId == values.second;
    });

    std::shared_ptr<OGLLineMesh> instanceLines = nullptr;
    if (iter != modInstCamData.micInstanceCollisions.end()) {
      /* colliding instances in red */
      instanceLines = instanceAABB.getAABBLines(red);
    } else {
      /* all other instances in yellow */
      instanceLines = instanceAABB.getAABBLines(yellow);
    }
    mOctreeLines.vertices.insert(mOctreeLines.vertices.end(), instanceLines->vertices.begin(), instanceLines->vertices.end());

    /* draw a green box around the selected instance */
    if (modInstCamData.micSelectedInstance == instanceId) {
      instanceAABB.setMinPos(instanceAABB.getMinPos() - glm::vec3(1.0f));
      instanceAABB.setMaxPos(instanceAABB.getMaxPos() + glm::vec3(1.0f));
      instanceLines = instanceAABB.getAABBLines(green);
    }
    mOctreeLines.vertices.insert(mOctreeLines.vertices.end(), instanceLines->vertices.begin(), instanceLines->vertices.end());
  }

  ImDrawList* drawList = ImGui::GetWindowDrawList();

  ImVec2 cursorPos = ImGui::GetCursorScreenPos();
  ImVec2 windowSize = ImGui::GetWindowSize();

  ImVec2 drawArea = ImVec2(cursorPos.x + windowSize.x - 16.0f, cursorPos.y + windowSize.y - 32.0f);
  ImVec2 drawAreaCenter = ImVec2(cursorPos.x + windowSize.x / 2.0f - 8.0f, cursorPos.y + windowSize.y / 2.0f - 16.0f);

  drawList->AddRect(cursorPos, drawArea, IM_COL32(255, 255, 255, 192));
  drawList->AddRectFilled(cursorPos, drawArea, IM_COL32(64, 64, 64, 128));
  drawList->PushClipRect(cursorPos, drawArea, true);

  mScaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(mOctreeZoomFactor));
  mRotationMat = glm::rotate(mScaleMat, glm::radians(mOctreeRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
  mOctreeViewMat = glm::rotate(mRotationMat, glm::radians(mOctreeRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));

  for (int i = 0; i < mOctreeLines.vertices.size(); i += 2) {
    OGLLineVertex startVert = mOctreeLines.vertices.at(i);
    OGLLineVertex endVert = mOctreeLines.vertices.at(i+1);

    glm::vec3 startPos = mOctreeViewMat * startVert.position;
    glm::vec3 endPos = mOctreeViewMat * endVert.position;

    ImVec2 pointStart = ImVec2(drawAreaCenter.x + startPos.x + mOctreeTranslation.x,
                                drawAreaCenter.y + startPos.z + mOctreeTranslation.y);
    ImVec2 pointEnd = ImVec2(drawAreaCenter.x + endPos.x + mOctreeTranslation.x,
                                 drawAreaCenter.y + endPos.z + mOctreeTranslation.y);

    drawList->AddLine(pointStart, pointEnd,
                      ImColor(startVert.color.r, startVert.color.g, startVert.color.b, 0.6f));
  }

  drawList->PopClipRect();

  ImGui::End();
}

void UserInterface::resetPositionWindowOctreeView() {
  mOctreeZoomFactor = 1.0f;
  mOctreeRotation = glm::vec3(0.0f);
  mOctreeTranslation = glm::vec3(0.0f);
}

void UserInterface::createStatusBar(OGLRenderData& renderData, ModelInstanceCamData& modInstCamData) {
  ImGuiWindowFlags statusBarFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize;

  ImGui::SetNextWindowPos(ImVec2(0.0f, renderData.rdHeight -30.0f), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(renderData.rdWidth, 30.0f));
  ImGui::SetNextWindowBgAlpha(0.5f);

  InstanceSettings settings = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getInstanceSettings();

  ImGui::Begin("Status", nullptr, statusBarFlags);
  ImGui::Text("Mode: %8s | Active Camera:  %16s | FPS:  %7.2f | Speed: %2.4f | Accel: %2.4f | State: %6s",
              renderData.mAppModeMap.at(renderData.rdApplicationMode).c_str(),
              modInstCamData.micCameras.at(modInstCamData.micSelectedCamera)->getName().c_str(), mFramesPerSecond,
              glm::length(settings.isSpeed), glm::length(settings.isAccel),
              modInstCamData.micMoveStateMap.at(settings.isMoveState).c_str());

  ImGui::End();
}

void UserInterface::render() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UserInterface::cleanup() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();

  ImNodes::DestroyContext();
  ImGui::DestroyContext();
}

int UserInterface::nameInputFilter(ImGuiInputTextCallbackData* data) {
  ImWchar c = data->EventChar;
  if (std::isdigit(c) || std::isalnum(c) || c == '-' || c == '_') {
    return 0;
  }
  return 1;
}
