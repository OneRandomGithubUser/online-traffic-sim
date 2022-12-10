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
#include <vector>
#include <array>
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
public:
  Vertex(boost::uuids::uuid uuid, double posX, double posY)
  {
    this->uuid = uuid;
    this->posX = posX;
    this->posY = posY;
  }
  boost::uuids::uuid get_uuid() const
  {
    return uuid;
  }
  double get_x() const
  {
    return posX;
  }
  double get_y() const
  {
    return posY;
  }
};

class Edge
{
public:
  boost::uuids::uuid uuid;
  double posX;
  double posY;
};

class Workspace
{
protected:
  std::unordered_map<boost::uuids::uuid, Car, boost::hash<boost::uuids::uuid>> cars;
  std::unordered_map<boost::uuids::uuid, Pedestrian, boost::hash<boost::uuids::uuid>> pedestrians;
  std::unordered_map<boost::uuids::uuid, Vertex, boost::hash<boost::uuids::uuid>> vertices;
  std::unordered_map<boost::uuids::uuid, Edge, boost::hash<boost::uuids::uuid>> edges;
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
  void load_vertices(std::unordered_map<boost::uuids::uuid, Vertex, boost::hash<boost::uuids::uuid>>& vertices)
  {
    this->vertices = vertices;
    verticesLoaded = true;
  }
  void load_edges(std::unordered_map<boost::uuids::uuid, Edge, boost::hash<boost::uuids::uuid>>& edges)
  {
    this->edges = edges;
    edgesLoaded = true;
  }
  bool initialize_workspace()
  {
    if (!verticesLoaded)
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

std::unordered_map<boost::uuids::uuid, Vertex, boost::hash<boost::uuids::uuid>> ParseVertices(std::string jsonData)
{
  static boost::uuids::string_generator gen;
  nlohmann::json data = nlohmann::json::parse(jsonData);
  std::unordered_map<boost::uuids::uuid, Vertex, boost::hash<boost::uuids::uuid>> ans;
  for (auto& dataEntry : data)
  {
    auto uuid = gen(dataEntry["uuid"].get<std::string>());
    auto posX = dataEntry["x"].get<double>();
    auto posY = dataEntry["y"].get<double>();
    Vertex vertex(uuid, posX, posY);
    ans.try_emplace(uuid, vertex);
  }
  return ans;
}

std::unordered_map<boost::uuids::uuid, Edge, boost::hash<boost::uuids::uuid>> ParseEdges(std::string jsonData)
{
  //
}

void VerticesDownloadSucceeded(emscripten_fetch_t *fetch) {
  // The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
  auto parsedData = ParseVertices(std::string(fetch->data));
  workspace.load_vertices(parsedData);
  emscripten_fetch_close(fetch); // Free data associated with the fetch.
}

void VerticesDownloadFailed(emscripten_fetch_t *fetch) {
  emscripten_fetch_close(fetch); // Also free data on failure.
}

void EdgesDownloadSucceeded(emscripten_fetch_t *fetch) {
  emscripten_fetch_close(fetch);
}

void EdgesDownloadFailed(emscripten_fetch_t *fetch) {
  emscripten_fetch_close(fetch); // Also free data on failure.
}

void FetchVertices()
{
  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  attr.onsuccess = VerticesDownloadSucceeded;
  attr.onerror = VerticesDownloadFailed;
  emscripten_fetch(&attr, "test.json");
}

void FetchEdges()
{
  //
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
