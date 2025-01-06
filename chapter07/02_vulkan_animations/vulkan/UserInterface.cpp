#include <string>
#include <limits>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <misc/cpp/imgui_stdlib.h> // ImGui::InputText with std::string

#include <ImGuiFileDialog.h>

#include <filesystem>

#include "UserInterface.h"
#include "CommandBuffer.h"
#include "AssimpModel.h"
#include "AssimpAnimClip.h"
#include "AssimpInstance.h"
#include "AssimpSettingsContainer.h"
#include "InstanceSettings.h"
#include "Logger.h"

bool UserInterface::init(VkRenderData& renderData) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  std::vector<VkDescriptorPoolSize> imguiPoolSizes =
  {
    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
  };

  VkDescriptorPoolCreateInfo imguiPoolInfo{};
  imguiPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  imguiPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  imguiPoolInfo.maxSets = 1000;
  imguiPoolInfo.poolSizeCount = static_cast<uint32_t>( imguiPoolSizes.size());
  imguiPoolInfo.pPoolSizes = imguiPoolSizes.data();

  VkResult result = vkCreateDescriptorPool(renderData.rdVkbDevice.device, &imguiPoolInfo, nullptr, &renderData.rdImguiDescriptorPool);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not init ImGui descriptor pool \n", __FUNCTION__);
    return false;
  }

  if (!ImGui_ImplGlfw_InitForVulkan(renderData.rdWindow, true)) {
    Logger::log(1, "%s error: could not init ImGui GLFW for Vulkan \n", __FUNCTION__);
    return false;
  }

  ImGui_ImplVulkan_InitInfo imguiIinitInfo{};
  imguiIinitInfo.Instance = renderData.rdVkbInstance.instance;
  imguiIinitInfo.PhysicalDevice = renderData.rdVkbPhysicalDevice.physical_device;
  imguiIinitInfo.Device = renderData.rdVkbDevice.device;
  imguiIinitInfo.Queue = renderData.rdGraphicsQueue;
  imguiIinitInfo.DescriptorPool = renderData.rdImguiDescriptorPool;
  imguiIinitInfo.MinImageCount = 2;
  imguiIinitInfo.ImageCount = renderData.rdSwapchainImages.size();
  imguiIinitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  imguiIinitInfo.RenderPass = renderData.rdImGuiRenderpass;

  if (!ImGui_ImplVulkan_Init(&imguiIinitInfo)) {
    Logger::log(1, "%s error: could not init ImGui for Vulkan \n", __FUNCTION__);
    return false;
  }

  ImGui::StyleColorsDark();

  /* init plot vectors */
  mFPSValues.resize(mNumFPSValues);
  mFrameTimeValues.resize(mNumFrameTimeValues);
  mModelUploadValues.resize(mNumModelUploadValues);
  mMatrixGenerationValues.resize(mNumMatrixGenerationValues);
  mMatrixUploadValues.resize(mNumMatrixUploadValues);
  mUiGenValues.resize(mNumUiGenValues);
  mUiDrawValues.resize(mNumUiDrawValues);

  return true;
}

void UserInterface::createFrame(VkRenderData &renderData) {
  ImGui_ImplVulkan_NewFrame();
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

void UserInterface::createSettingsWindow(VkRenderData& renderData, ModelInstanceCamData& modInstCamData) {
  ImGuiWindowFlags imguiWindowFlags = 0;
  //imguiWindowFlags |= ImGuiWindowFlags_NoCollapse;
  //imguiWindowFlags |= ImGuiWindowFlags_NoResize;
  //imguiWindowFlags |= ImGuiWindowFlags_NoMove;

  ImGui::SetNextWindowBgAlpha(0.8f);

  /* dim background for modal dialogs */
  ImGuiStyle& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.75f);

  ImGui::Begin("Control", nullptr, imguiWindowFlags);

  bool loadModelRequest = false;

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

  /* load model*/
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
  static int uiGenOffset = 0;
  static int uiDrawOffset = 0;

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

    mUiGenValues.at(uiGenOffset) = renderData.rdUIGenerateTime;
    uiGenOffset = ++uiGenOffset % mNumUiGenValues;

    mUiDrawValues.at(uiDrawOffset) = renderData.rdUIDrawTime;
    uiDrawOffset = ++uiDrawOffset % mNumUiDrawValues;

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
    ImGui::PlotLines("##FrameTimes", mFPSValues.data(), mFPSValues.size(), fpsOffset, fpsOverlay.c_str(), 0.0f,
                     std::numeric_limits<float>::max(), ImVec2(0, 80));
    ImGui::EndTooltip();
  }

  if (ImGui::CollapsingHeader("Info")) {
    ImGui::Text("Triangles:              %10i", renderData.rdTriangleCount);

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
    ImGui::Text("Frame Time:             %10.4f ms", renderData.rdFrameTime);

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

    ImGui::Text("Model Upload Time:      %10.4f ms", renderData.rdUploadToVBOTime);

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

    ImGui::Text("Matrix Generation Time: %10.4f ms", renderData.rdMatrixGenerateTime);

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

    ImGui::Text("Matrix Upload Time:     %10.4f ms", renderData.rdUploadToUBOTime);

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

    ImGui::Text("UI Generation Time:     %10.4f ms", renderData.rdUIGenerateTime);

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

    ImGui::Text("UI Draw Time:           %10.4f ms", renderData.rdUIDrawTime);

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
      if (ImGui::InputText("##CamName", &camName, textinputFlags, cameraNameInputFilter)) {
        if (modInstCamData.micCameraNameCheckCallback(camName)) {
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


    if (!modelListEmtpy) {
      selectedModelName = modInstCamData.micModelList.at(modInstCamData.micSelectedModel)->getModelFileName().c_str();
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
      if (ImGui::BeginCombo("##StateCombo",
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
            if (ImGui::Button("Remove##order")) {
              iter = modSettings.msAllowedStateOrder.erase(iter);
            } else {
              ++iter;
            }
            ImGui::PopID();
          }

          currentModel->setModelSettings(modSettings);
    }
  }

  if (ImGui::CollapsingHeader("Instances")) {
    bool modelListEmtpy = modInstCamData.micModelList.size() == 1;
    bool nullInstanceSelected = modInstCamData.micSelectedInstance == 0;
    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    ImGui::Text("Total Instances:  %ld", numberOfInstances);

    if (modelListEmtpy) {
      ImGui::BeginDisabled();
    }

    ImGui::Text("Select Instance: ");
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

    if (modelListEmtpy || nullInstanceSelected) {
      ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::ArrowButton("##Right", ImGuiDir_Right) &&
      modInstCamData.micSelectedInstance < (modInstCamData.micAssimpInstances.size() - 1)) {
      modInstCamData.micSelectedInstance++;
      }
      ImGui::PopButtonRepeat();

    ImGui::Text("Hightlight:      ");
    ImGui::SameLine();
    ImGui::Checkbox("##HighlightInstance", &renderData.rdHighlightSelectedInstance);

    if (modelListEmtpy) {
      ImGui::EndDisabled();
    }

    if (modelListEmtpy || nullInstanceSelected) {
      ImGui::BeginDisabled();
    }

    /* DragInt does not like clamp flag */
    modInstCamData.micSelectedInstance = std::clamp(modInstCamData.micSelectedInstance, 0,
                                                    static_cast<int>(modInstCamData.micAssimpInstances.size() - 1));

    static InstanceSettings savedInstanceSettings{};
    static std::shared_ptr<AssimpInstance> currentInstance = nullptr;

    InstanceSettings settings;
    if (numberOfInstances > 0) {
      settings = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getInstanceSettings();
      /* overwrite saved settings on instance change */
      if (currentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
        currentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
        savedInstanceSettings = settings;
      }
    }

    ImGui::Text("                 ");
    ImGui::SameLine();
    if (ImGui::Button("Center This Instance")) {
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

    ImGui::Text("                 ");
    ImGui::SameLine();
    if (ImGui::Button("Clone Instance")) {
      modInstCamData.micInstanceCloneCallbackFunction(currentInstance);

      /* read back settings for UI */
      settings = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getInstanceSettings();
    }

    static int manyInstanceCloneNum = 1;
    ImGui::Text("Create Clones:   ");
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

    if (modelListEmtpy || nullInstanceSelected) {
      ImGui::EndDisabled();
    }

    /* get the new size, in case of a deletion */
    numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    std::string baseModelName = "None";
    if (numberOfInstances > 0 && !nullInstanceSelected) {
      baseModelName = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getModel()->getModelFileName();
    }
    ImGui::Text("Base Model:        %s", baseModelName.c_str());

    if (numberOfInstances == 0 || nullInstanceSelected) {
      ImGui::BeginDisabled();
    }

    ImGui::Text("Swap Y/Z axes:   ");
    ImGui::SameLine();
    ImGui::Checkbox("##ModelAxisSwap", &settings.isSwapYZAxis);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstCamData.micSettingsContainer->applyEditInstanceSettings(modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance),
                                                                     settings, savedInstanceSettings);
      savedInstanceSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    ImGui::Text("Pos (X/Y/Z):     ");
    ImGui::SameLine();
    ImGui::SliderFloat3("##ModelPos", glm::value_ptr(settings.isWorldPosition),
                        -75.0f, 75.0f, "%.3f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstCamData.micSettingsContainer->applyEditInstanceSettings(modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance),
                                                                     settings, savedInstanceSettings);
      savedInstanceSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    ImGui::Text("Rotation (X/Y/Z):");
    ImGui::SameLine();
    ImGui::SliderFloat3("##ModelRot", glm::value_ptr(settings.isWorldRotation),
                        -180.0f, 180.0f, "%.3f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstCamData.micSettingsContainer->applyEditInstanceSettings(modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance),
                                                                     settings, savedInstanceSettings);
      savedInstanceSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    ImGui::Text("Scale:           ");
    ImGui::SameLine();
    ImGui::SliderFloat("##ModelScale", &settings.isScale,
                       0.001f, 10.0f, "%.4f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstCamData.micSettingsContainer->applyEditInstanceSettings(modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance),
                                                                     settings, savedInstanceSettings);
      savedInstanceSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    ImGui::Text("                 ");
    ImGui::SameLine();
    if (ImGui::Button("Reset Values to Zero")) {
      modInstCamData.micSettingsContainer->applyEditInstanceSettings(modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance),
                                                                     settings, savedInstanceSettings);
      InstanceSettings defaultSettings{};
      settings = defaultSettings;
      savedInstanceSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    if (numberOfInstances == 0 || nullInstanceSelected) {
      ImGui::EndDisabled();
    }

    if (numberOfInstances > 0) {
      modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->setInstanceSettings(settings);
    }
  }

  ImGui::End();
}

void UserInterface::createStatusBar(VkRenderData& renderData, ModelInstanceCamData& modInstCamData) {
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

void UserInterface::render(VkRenderData& renderData) {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), renderData.rdImGuiCommandBuffer);
}

void UserInterface::cleanup(VkRenderData& renderData) {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();

  vkDestroyDescriptorPool(renderData.rdVkbDevice.device, renderData.rdImguiDescriptorPool, nullptr);
  ImGui::DestroyContext();
}

int UserInterface::cameraNameInputFilter(ImGuiInputTextCallbackData* data) {
  ImWchar c = data->EventChar;
  if (std::isdigit(c) || std::isalnum(c) || c == '-' || c == '_') {
    return 0;
  }
  return 1;
}
