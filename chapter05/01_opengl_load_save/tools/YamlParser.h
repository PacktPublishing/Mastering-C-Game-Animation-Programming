#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

#include "YamlParserTypes.h"
#include "ModelAndInstanceData.h"
#include "InstanceSettings.h"
#include "OGLRenderData.h"

class YamlParser {
  public:
    /* loading */
    bool loadYamlFile(std::string fileName);
    std::string getFileName();

    std::vector<std::string> getModelFileNames();
    std::vector<InstanceSettings> getInstanceConfigs();
    int getSelectedModelNum();
    int getSelectedInstanceNum();
    bool getHighlightActivated();

    glm::vec3 getCameraPosition();
    float getCameraElevation();
    float getCameraAzimuth();

    /* saving */
    bool createConfigFile(OGLRenderData renderData, ModelAndInstanceData modInstData);
    bool writeYamlFile(std::string fileName);

    /* misc */
    bool hasKey(std::string key);
    bool getValue(std::string key, int& value);

  private:
    std::string mYamlFileName;
    YAML::Node mYamlNode{};
    YAML::Emitter mYamlEmit{};
};
