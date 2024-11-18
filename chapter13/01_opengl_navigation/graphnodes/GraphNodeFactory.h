#pragma once

#include <unordered_map>
#include <memory>

#include "Enums.h"
#include "OGLRenderData.h"

#include "GraphNodeBase.h"

class GraphNodeFactory {
  public:
    GraphNodeFactory(fireNodeOutputCallback callback);
    std::shared_ptr<GraphNodeBase> makeNode(graphNodeType type, int nodeId);
    std::string getNodeTypeName(graphNodeType nodeType);

  private:
    fireNodeOutputCallback mFireNodeOutputCallback;
    std::unordered_map<graphNodeType, std::string> mGraphNodeTypeMap{};
};

