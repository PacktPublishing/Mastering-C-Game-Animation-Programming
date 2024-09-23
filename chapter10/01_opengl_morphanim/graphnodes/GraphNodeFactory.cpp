#include "GraphNodeFactory.h"

#include "RootNode.h"
#include "TestNode.h"
#include "WaitNode.h"
#include "RandowWaitNode.h"
#include "SequenceNode.h"
#include "SelectorNode.h"
#include "InstanceNode.h"
#include "EventNode.h"
#include "ActionNode.h"
#include "DebugLogNode.h"
#include "FaceAnimNode.h"
#include "HeadAnimNode.h"

#include "Logger.h"

GraphNodeFactory::GraphNodeFactory(fireNodeOutputCallback callback) :
 mFireNodeOutputCallback(callback) {
  mGraphNodeTypeMap[graphNodeType::root] = "Root";
  mGraphNodeTypeMap[graphNodeType::test] = "Test";
  mGraphNodeTypeMap[graphNodeType::wait] = "Wait";
  mGraphNodeTypeMap[graphNodeType::randomWait] = "RandomWait";
  mGraphNodeTypeMap[graphNodeType::selector] = "Selector";
  mGraphNodeTypeMap[graphNodeType::sequence] = "Sequence";
  mGraphNodeTypeMap[graphNodeType::instance] = "Instance";
  mGraphNodeTypeMap[graphNodeType::event] = "Event";
  mGraphNodeTypeMap[graphNodeType::action] = "Action";
  mGraphNodeTypeMap[graphNodeType::debugLog] = "DebugLog";
  mGraphNodeTypeMap[graphNodeType::faceAnim] = "FaceAnim";
  mGraphNodeTypeMap[graphNodeType::headAmin] = "HeadAnim";
}

std::string GraphNodeFactory::getNodeTypeName(graphNodeType nodeType) {
  return mGraphNodeTypeMap[nodeType];
}

std::shared_ptr<GraphNodeBase> GraphNodeFactory::makeNode(graphNodeType type, int nodeId) {
  std::shared_ptr<GraphNodeBase> newNode = nullptr;
  if (!mFireNodeOutputCallback) {
    Logger::log(1, "%s error: node fire callback not set\n", __FUNCTION__);
    return newNode;
  }

  switch (type) {
    case graphNodeType::root:
      newNode = std::make_shared<RootNode>();
      break;
    case graphNodeType::test:
      newNode = std::make_shared<TestNode>(nodeId);
      break;
    case graphNodeType::wait:
      newNode = std::make_shared<WaitNode>(nodeId);
      break;
    case graphNodeType::randomWait:
      newNode = std::make_shared<RandomWaitNode>(nodeId);
      break;
    case graphNodeType::selector:
      newNode = std::make_shared<SelectorNode>(nodeId);
      break;
    case graphNodeType::sequence:
      newNode = std::make_shared<SequenceNode>(nodeId);
      break;
    case graphNodeType::instance:
      newNode = std::make_shared<InstanceNode>(nodeId);
      break;
    case graphNodeType::event:
      newNode = std::make_shared<EventNode>(nodeId);
      break;
    case graphNodeType::action:
      newNode = std::make_shared<ActionNode>(nodeId);
      break;
    case graphNodeType::debugLog:
      newNode = std::make_shared<DebugLogNode>(nodeId);
      break;
    case graphNodeType::faceAnim:
      newNode = std::make_shared<FaceAnimNode>(nodeId);
      break;
    case graphNodeType::headAmin:
      newNode = std::make_shared<HeadAnimNode>(nodeId);
      break;
    default:
      Logger::log(1, "%s error: invalid node type %i", __FUNCTION__, static_cast<int>(type));
      break;
  }

  newNode->setNodeOutputTriggerCallback(mFireNodeOutputCallback);
  newNode->mNodeName = mGraphNodeTypeMap.at(type);
  newNode->mNodeType = type;

  return newNode;
}
