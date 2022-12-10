//
// Created on 12/10/2022.
//

#ifndef ONLINE_TRAFFIC_SIM_MAIN_H
#define ONLINE_TRAFFIC_SIM_MAIN_H

#include <emscripten/val.h>
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/fetch.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/functional/hash.hpp>
#include <iostream>
#include <utility>
#include <vector>
#include <optional>
#include <random>
#include <unordered_map>
#include <string>
#include <numbers>
#include "json.hpp"

class Pedestrian;
class Car;
class Vertex;
class Edge;

class Pedestrian
{
public:
  boost::uuids::uuid uuid;
  double posX;
  double posY;
  double velX;
  double velY;
  double accX;
  double accY;
  double jrkX;
  double jrkY;
};

class Car
{
public:
  boost::uuids::uuid uuid;
  Edge* currentRoadPointer;
  double posX;
  double posY;
  double velX;
  double velY;
  double accX;
  double accY;
  double jrkX;
  double jrkY;
  double maxSpeed;
  double maxAcceleration;
  double maxJerk;
};

class Vertex
{
private:
  boost::uuids::uuid uuid;
  double posX;
  double posY;
  std::vector<Edge*> edgePointerVector;
public:
  Vertex(boost::uuids::uuid uuid, double posX, double posY)
  {
    this->uuid = uuid;
    this->posX = posX;
    this->posY = posY;
  }
  [[nodiscard]] boost::uuids::uuid get_uuid() const
  {
    return uuid;
  }
  [[nodiscard]] double get_x() const
  {
    return posX;
  }
  [[nodiscard]] double get_y() const
  {
    return posY;
  }
  void add_edge(Edge& edge)
  {
    edgePointerVector.emplace_back(&edge);
  }
  [[nodiscard]] const std::vector<Edge*>& get_edge_pointer_vector() const
  {
    return edgePointerVector;
  }
};

class Edge
{
private:
  boost::uuids::uuid uuid;
  std::vector<Vertex*> vertexPointerVector;
public:
  Edge(boost::uuids::uuid uuid, std::vector<Vertex*>& vertexPointerVector)
  {
    this->uuid = uuid;
    this->vertexPointerVector = std::move(vertexPointerVector);
  }
  [[nodiscard]] boost::uuids::uuid get_uuid() const
  {
    return uuid;
  }
  void add_vertex(Vertex& vertex)
  {
    vertexPointerVector.emplace_back(&vertex);
  }
  [[nodiscard]] const std::vector<Vertex*>& get_vertex_pointer_vector() const
  {
    return vertexPointerVector;
  }
};

class Workspace
{
protected:
  std::unordered_map<boost::uuids::uuid, Car, boost::hash<boost::uuids::uuid>> cars;
  std::unordered_map<boost::uuids::uuid, Pedestrian, boost::hash<boost::uuids::uuid>> pedestrians;
  std::unordered_map<boost::uuids::uuid, Vertex, boost::hash<boost::uuids::uuid>> vertices;
  std::unordered_map<boost::uuids::uuid, Edge, boost::hash<boost::uuids::uuid>> edges;
  std::optional<nlohmann::json> cachedEdgeData;
  std::size_t ticks;
  bool verticesLoaded;
  bool edgesLoaded;
public:
  Workspace()
  {
    verticesLoaded = false;
    edgesLoaded = false;
    ticks = 0;
  }
protected:
  void parse_edges_helper(nlohmann::json jsonData, bool clearCachedEdgeData) {
    static boost::uuids::string_generator gen;
    edges.clear();
    for (auto& dataEntry : jsonData)
    {
      auto uuid = gen(dataEntry["uuid"].get<std::string>());
      auto nodeUuidList = dataEntry["nodeUuidList"];
      std::vector<Vertex*> vertexPointerVector;
      Edge edge(uuid, vertexPointerVector);
      edges.try_emplace(uuid, edge);
      Edge& currentEdge = edges.at(uuid);
      for (auto& nodeUuidJson : nodeUuidList)
      {
        auto nodeUuid = gen(nodeUuidJson.get<std::string>());
        Vertex& currentVertex = vertices.at(nodeUuid);
        currentEdge.add_vertex(currentVertex);
        currentVertex.add_edge(currentEdge);
      }
    }
    if (clearCachedEdgeData)
    {
      cachedEdgeData.reset();
    }
    edgesLoaded = true;
  }
public:
  void parse_edges(emscripten_fetch_t *fetch)
  {
    auto data = nlohmann::json::parse(fetch->data);
    emscripten_fetch_close(fetch); // Free data associated with the fetch.
    if (verticesLoaded) {
      parse_edges_helper(data, false);
    } else {
      cachedEdgeData = data;
    }
  }
  void parse_vertices(emscripten_fetch_t *fetch)
  {
    static boost::uuids::string_generator gen;
    auto data = nlohmann::json::parse(fetch->data);
    emscripten_fetch_close(fetch); // Free data associated with the fetch.
    vertices.clear();
    for (auto& dataEntry : data)
    {
      auto uuid = gen(dataEntry["uuid"].get<std::string>());
      auto posX = dataEntry["x"].get<double>();
      auto posY = dataEntry["y"].get<double>();
      Vertex vertex(uuid, posX, posY);
      vertices.try_emplace(uuid, vertex);
    }
    verticesLoaded = true;
    if (cachedEdgeData.has_value())
    {
      parse_edges_helper(cachedEdgeData.value(), true);
    }
  }
  bool initialize_workspace()
  {
    if (!verticesLoaded or !edgesLoaded)
    {
      return false;
    }
    return true;
  }
  void render_to_canvas(emscripten::val canvas) const
  {
    auto ctx = canvas.call<emscripten::val>("getContext", emscripten::val("2d"));
    ctx.call<void>("clearRect", emscripten::val(0), emscripten::val(0), canvas["width"], canvas["height"]);
    for (const auto& [uuid, vertex] : vertices)
    {
      ctx.call<void>("beginPath");
      ctx.call<void>("arc", vertex.get_x(), vertex.get_y(), 10, 0, 2 * std::numbers::pi);
      ctx.call<void>("stroke");
    }
    ctx.call<void>("beginPath");
    for (const auto& [uuid, edge] : edges)
    {
      auto vertexPointerVector = edge.get_vertex_pointer_vector();
      ctx.call<void>("moveTo", vertexPointerVector.at(0)->get_x(), vertexPointerVector.at(0)->get_y());
      for (const auto& vertexPointer : vertexPointerVector)
      {
        ctx.call<void>("lineTo", vertexPointer->get_x(), vertexPointer->get_y());
      }
    }
    ctx.call<void>("stroke");
  }
  void tick()
  {
    ticks++;
  }
};

Workspace workspace;

boost::uuids::uuid GenerateUuid() {
  // this is not thread safe probably
  static boost::uuids::random_generator uuidGenerator;
  return uuidGenerator();
}

void FetchVertices();
void FetchEdges();

void VertexDownloadHandler(emscripten_fetch_t *fetch, bool wasSuccessful)
{
  if (wasSuccessful) {
    workspace.parse_vertices(fetch);
  } else {
    emscripten_fetch_close(fetch);
    // TODO: this is bad and will probably spam the user if they don't have internet
    FetchVertices();
  }
}

void EdgeDownloadHandler(emscripten_fetch_t *fetch, bool wasSuccessful)
{
  if (wasSuccessful) {
    workspace.parse_edges(fetch);
  } else {
    emscripten_fetch_close(fetch);
    // TODO: this is bad and will probably spam the user if they don't have internet
    FetchEdges();
  }
}

void VerticesDownloadSucceeded(emscripten_fetch_t *fetch) {
  // The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
  VertexDownloadHandler(fetch, true);
}

void VerticesDownloadFailed(emscripten_fetch_t *fetch) {
  VertexDownloadHandler(fetch, false);
}

void EdgesDownloadSucceeded(emscripten_fetch_t *fetch) {
  EdgeDownloadHandler(fetch, true);
}

void EdgesDownloadFailed(emscripten_fetch_t *fetch) {
  EdgeDownloadHandler(fetch, false);
}

void FetchVertices()
{
  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  attr.onsuccess = VerticesDownloadSucceeded;
  attr.onerror = VerticesDownloadFailed;
  emscripten_fetch(&attr, "vertices.json");
}

void FetchEdges()
{
  // must be called AFTER FetchVertices
  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  attr.onsuccess = EdgesDownloadSucceeded;
  attr.onerror = EdgesDownloadFailed;
  emscripten_fetch(&attr, "edges.json");
}

double RandomDouble()
{
  static std::random_device rd;
  static std::mt19937 rng(rd());
  static std::uniform_real_distribution<double> dist(0, 1);
  return dist(rng);
}

void RenderCanvas(double DOMHighResTimeStamp)
{
  static bool isInitialized = false;
  emscripten::val window = emscripten::val::global("window");
  emscripten::val document = emscripten::val::global("document");
  if (!isInitialized)
  {
    bool initializationSuccessful = workspace.initialize_workspace();
    window.call<void>("requestAnimationFrame", emscripten::val::module_property("RenderCanvas"));
    if (!initializationSuccessful)
    {
      return;
    } else {
      isInitialized = true;
    }
  }
  auto canvas = document.call<emscripten::val>("getElementById", emscripten::val("canvas"));
  workspace.render_to_canvas(canvas);
  window.call<void>("requestAnimationFrame", emscripten::val::module_property("RenderCanvas"));
}

void InitializeCanvas(emscripten::val canvas)
{
    emscripten::val window = emscripten::val::global("window");
    auto parentPanel = canvas["parentElement"];
    canvas.set("width", parentPanel["offsetWidth"]);
    canvas.set("height", parentPanel["offsetHeight"]);
    emscripten::val ctx = canvas.call<emscripten::val>("getContext", emscripten::val("2d"));
    ctx.set("textAlign", emscripten::val("center"));
    ctx.set("textBaseline", emscripten::val("middle"));
    ctx.set("font", emscripten::val("20px Arial"));
}

int main()
{
    emscripten::val window = emscripten::val::global("window");
    emscripten::val document = emscripten::val::global("document");
    auto canvas = document.call<emscripten::val>("getElementById", emscripten::val("canvas"));
    FetchVertices();
    FetchEdges();
    InitializeCanvas(canvas);
    canvas.call<void>("addEventListener", emscripten::val("resize"), emscripten::val::module_property("InitializeCanvas"));
    RenderCanvas(0);
    return 0;
}

EMSCRIPTEN_BINDINGS(bindings)\
{\
  emscripten::function("InitializeCanvas", InitializeCanvas);\
  emscripten::function("RenderCanvas", RenderCanvas);\
};

#endif //ONLINE_TRAFFIC_SIM_MAIN_H
