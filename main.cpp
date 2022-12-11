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
#include <chrono>
#include <ctime>
#include "json.hpp"

class Pedestrian;
class Car;
class Vertex;
class Way;

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
  Way* wayPointer;
  double position;
  double velocity;
  double acceleration;
  double jerk;
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
  std::vector<Way*> wayPointerVector;
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
  void add_way(Way& way)
  {
    wayPointerVector.emplace_back(&way);
  }
  [[nodiscard]] const std::vector<Way*>& get_way_pointer_vector() const
  {
    return wayPointerVector;
  }
};

class Way
{
private:
  boost::uuids::uuid uuid;
  std::vector<Vertex*> vertexPointerVector;
public:
  Way(boost::uuids::uuid uuid, std::vector<Vertex*>& vertexPointerVector)
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
  std::vector<Vertex*> vertexPointerVector;
  std::vector<Way*> wayPointerVector;
  double maxVertexX;
  double maxVertexY;
  double minVertexX;
  double minVertexY;
  std::unordered_map<boost::uuids::uuid, Vertex, boost::hash<boost::uuids::uuid>> vertexMap;
  std::unordered_map<boost::uuids::uuid, Way, boost::hash<boost::uuids::uuid>> ways;
  std::optional<nlohmann::json> cachedWayData;
  std::size_t ticks;
  long int numCars;
  bool verticesLoaded;
  bool waysLoaded;
public:
  Workspace()
  {
    ticks = 0;
    numCars = 1000;
    verticesLoaded = false;
    waysLoaded = false;
  }
protected:
  double lat_lon_to_x_y()
  {
    // TODO: make this account for the curvature of the earth better
  }
  double random_double()
  {
    static std::random_device rd;
    static std::mt19937 rng(rd());
    static std::uniform_real_distribution<double> dist(0, 1);
    return dist(rng);
  }
  template <typename T>
  T& random_vector_element(std::vector<T*> vector)
  {
    return *(vector.at((std::size_t) (random_double() * vector.size())));
  }
  Way& random_way()
  {
    return random_vector_element(wayPointerVector);
  }
  boost::uuids::uuid generate_uuid() {
    // this is not thread safe probably
    static boost::uuids::random_generator uuidGenerator;
    return uuidGenerator();
  }
  void parse_ways_helper(nlohmann::json jsonData, bool clearCachedWayData) {
    static boost::uuids::string_generator gen;
    ways.clear();
    for (auto& dataEntry : jsonData)
    {
      auto uuid = gen(dataEntry["uuid"].get<std::string>());
      auto nodeUuidList = dataEntry["nodeUuidList"];
      std::vector<Vertex*> currentVertexPointerVector;
      Way way(uuid, currentVertexPointerVector);
      ways.try_emplace(uuid, way);
      Way& currentWay = ways.at(uuid);
      for (auto& nodeUuidJson : nodeUuidList)
      {
        auto nodeUuid = gen(nodeUuidJson.get<std::string>());
        Vertex& currentVertex = vertexMap.at(nodeUuid);
        currentWay.add_vertex(currentVertex);
        currentVertex.add_way(currentWay);
        wayPointerVector.emplace_back(&currentWay);
      }
    }
    if (clearCachedWayData)
    {
      cachedWayData.reset();
    }
    waysLoaded = true;
  }
public:
  void parse_ways(emscripten_fetch_t *fetch)
  {
    auto truncatedFetchedData = std::string(fetch->data, fetch->data + fetch->numBytes);
    auto data = nlohmann::json::parse(truncatedFetchedData);
    emscripten_fetch_close(fetch); // Free data associated with the fetch.
    if (verticesLoaded) {
      parse_ways_helper(data, false);
    } else {
      cachedWayData = data;
    }
  }
  void parse_vertices(emscripten_fetch_t *fetch)
  {
    static boost::uuids::string_generator gen;
    auto truncatedFetchedData = std::string(fetch->data, fetch->data + fetch->numBytes);
    auto data = nlohmann::json::parse(truncatedFetchedData);
    emscripten_fetch_close(fetch); // Free data associated with the fetch.
    vertexMap.clear();
    auto first = data.begin().value();
    auto firstX = first["x"].get<double>();
    auto firstY = first["y"].get<double>();
    minVertexX = firstX;
    maxVertexX = firstX;
    minVertexY = firstY;
    maxVertexY = firstY;
    for (auto& dataEntry : data)
    {
      auto uuid = gen(dataEntry["uuid"].get<std::string>());
      auto posX = dataEntry["x"].get<double>();
      auto posY = dataEntry["y"].get<double>();
      if (posX < minVertexX)
      {
        minVertexX = posX;
      }
      if (posY < minVertexY)
      {
        minVertexY = posY;
      }
      if (posX > maxVertexX)
      {
        maxVertexX = posX;
      }
      if (posY > maxVertexY)
      {
        maxVertexY = posY;
      }
      Vertex vertex(uuid, posX, posY);
      vertexMap.try_emplace(uuid, vertex);
      vertexPointerVector.emplace_back(&vertexMap.at(uuid));
    }
    verticesLoaded = true;
    if (cachedWayData.has_value())
    {
      parse_ways_helper(cachedWayData.value(), true);
    }
  }
  bool initialize_workspace()
  {
    if (!verticesLoaded or !waysLoaded)
    {
      return false;
    }
    cars.clear();
    for (long int i = 0; i < numCars; i++)
    {
      // TODO: make a weighted list of roads based on its length and type
      auto& randomWay = random_way();
      auto& randomWayVertexPointerVector = randomWay.get_vertex_pointer_vector();
      double randomPos = random_double() * (randomWayVertexPointerVector.size() - 1);
      Car currentCar;
      currentCar.uuid = generate_uuid();
      currentCar.wayPointer = &randomWay;
      currentCar.position = randomPos;
      currentCar.velocity = 0.05;
      currentCar.maxSpeed = 60;
      currentCar.maxAcceleration = 8;
      currentCar.jerk = 15;
      currentCar.maxJerk = 25;
      cars.try_emplace(currentCar.uuid, currentCar);
    }
    return true;
  }
  double pos_x(double lon, double canvasWidth) const
  {
    return (lon - minVertexX) / (maxVertexX - minVertexX) * canvasWidth;
  }
  double pos_y(double lat, double canvasHeight) const
  {
    return canvasHeight - (lat - minVertexY) / (maxVertexY - minVertexY) * canvasHeight;
  }
  void render_to_canvas(emscripten::val canvas) const
  {
    auto canvasWidth = canvas["width"].as<double>();
    auto canvasHeight = canvas["height"].as<double>();
    auto ctx = canvas.call<emscripten::val>("getContext", emscripten::val("2d"));
    ctx.call<void>("clearRect", emscripten::val(0), emscripten::val(0), canvas["width"], canvas["height"]);
    /*
    for (const auto& [uuid, vertex] : vertexMap)
    {
      ctx.call<void>("beginPath");
      ctx.call<void>("arc", pos_x(vertex.get_x(), canvasWidth), pos_y(vertex.get_y(), canvasHeight), 10, 0, 2 * std::numbers::pi);
      ctx.call<void>("stroke");
    }
    */
    ctx.call<void>("beginPath");
    for (const auto& [uuid, way] : ways)
    {
      auto currentVertexPointerVector = way.get_vertex_pointer_vector();
      ctx.call<void>("moveTo", pos_x(currentVertexPointerVector.at(0)->get_x(), canvasWidth), pos_y(currentVertexPointerVector.at(0)->get_y(), canvasHeight));
      for (const auto& vertexPointer : currentVertexPointerVector)
      {
        ctx.call<void>("lineTo", pos_x(vertexPointer->get_x(), canvasWidth), pos_y(vertexPointer->get_y(), canvasHeight));
      }
    }
    ctx.call<void>("stroke");
    for (const auto& [uuid, car] : cars)
    {
      ctx.call<void>("beginPath");
      auto position = car.position;
      auto& wayVertexPointerVector = car.wayPointer->get_vertex_pointer_vector();
      std::size_t floor = std::floor(position);
      auto decimal = position - floor;
      std::size_t ceil = std::ceil(position);
      auto& vertex0 = *(wayVertexPointerVector.at(floor));
      auto& vertex1 = *(wayVertexPointerVector.at(ceil));
      auto posX = (1 - decimal) * vertex0.get_x() + decimal * vertex1.get_x();
      auto posY = (1 - decimal) * vertex0.get_y() + decimal * vertex1.get_y();
      posX = pos_x(posX, canvasWidth);
      posY = pos_y(posY, canvasHeight);
      ctx.call<void>("fillRect", posX, posY, 3, 3);
      ctx.call<void>("stroke");
    }
  }
  void tick()
  {
    for (auto& [uuid, car] : cars)
    {
      car.position += car.velocity;
      auto max = car.wayPointer->get_vertex_pointer_vector().size() - 1;
      bool isOverflowing = false;
      bool isUnderflowing = false;
      do
      {
        if (isOverflowing)
        {
          car.position = 2 * max - car.position;
          car.velocity *= -1;
        }
        if (isUnderflowing)
        {
          car.position = -car.position;
          car.velocity *= -1;
        }
        isOverflowing = car.position > max;
        isUnderflowing = car.position < 0;
      }
      while (isOverflowing or isUnderflowing);
    }
    ticks++;
  }
};

Workspace workspace;

void FetchVertices();
void FetchWays();

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

void WayDownloadHandler(emscripten_fetch_t *fetch, bool wasSuccessful)
{
  if (wasSuccessful) {
    workspace.parse_ways(fetch);
  } else {
    emscripten_fetch_close(fetch);
    // TODO: this is bad and will probably spam the user if they don't have internet
    FetchWays();
  }
}

void VerticesDownloadSucceeded(emscripten_fetch_t *fetch) {
  // The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
  VertexDownloadHandler(fetch, true);
}

void VerticesDownloadFailed(emscripten_fetch_t *fetch) {
  VertexDownloadHandler(fetch, false);
}

void WaysDownloadSucceeded(emscripten_fetch_t *fetch) {
  WayDownloadHandler(fetch, true);
}

void WaysDownloadFailed(emscripten_fetch_t *fetch) {
  WayDownloadHandler(fetch, false);
}

void FetchVertices()
{
  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  attr.onsuccess = VerticesDownloadSucceeded;
  attr.onerror = VerticesDownloadFailed;
  emscripten_fetch(&attr, "nodes.json");
}

void FetchWays()
{
  // must be called AFTER FetchVertices
  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  attr.onsuccess = WaysDownloadSucceeded;
  attr.onerror = WaysDownloadFailed;
  emscripten_fetch(&attr, "ways.json");
}

void RenderCanvas(double DOMHighResTimeStamp)
{
  static std::size_t lastUpdateTime = -1;
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
  auto now = std::chrono::system_clock::now();
  auto unixMilis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
  if (lastUpdateTime % 200 != unixMilis % 200)
  {
    workspace.tick();
  }
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
    FetchWays();
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
