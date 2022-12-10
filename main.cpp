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
#include <boost/functional/hash.hpp>
#include <iostream>
#include <vector>
#include <array>
#include <random>
#include <unordered_map>
#include <string>

class Pedestrian;
class Car;
class Vertex;
class Edge;

class Pedestrian
{
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
  boost::uuids::uuid uuid;
  double posX;
  double posY;
};

class Edge
{
  boost::uuids::uuid uuid;
  double posX;
  double posY;
};

class Workspace
{
  std::unordered_map<boost::uuids::uuid, Car, boost::hash<boost::uuids::uuid>> cars;
  std::unordered_map<boost::uuids::uuid, Pedestrian, boost::hash<boost::uuids::uuid>> pedestrians;
  std::unordered_map<boost::uuids::uuid, Vertex, boost::hash<boost::uuids::uuid>> vertices;
  std::unordered_map<boost::uuids::uuid, Edge, boost::hash<boost::uuids::uuid>> edges;
  std::size_t ticks;
  Workspace(std::unordered_map<boost::uuids::uuid, Vertex, boost::hash<boost::uuids::uuid>>& vertices,
            std::unordered_map<boost::uuids::uuid, Edge, boost::hash<boost::uuids::uuid>>& edges)
  {
    //
  }
  void load_vertices(std::unordered_map<boost::uuids::uuid, Vertex, boost::hash<boost::uuids::uuid>>& vertices)
  {
    this->vertices = vertices;
  }
  void load_edges(std::unordered_map<boost::uuids::uuid, Edge, boost::hash<boost::uuids::uuid>>& edges)
  {
    this->edges = edges;
  }
  void tick()
  {
    ticks++;
  }
};

void verticesDownloadSucceeded(emscripten_fetch_t *fetch) {
  // The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
  std::cout << fetch->data << " " << std::string(fetch->data) << "\n";
  emscripten_fetch_close(fetch); // Free data associated with the fetch.
}

void verticesDownloadFailed(emscripten_fetch_t *fetch) {
  emscripten_fetch_close(fetch); // Also free data on failure.
}

void edgesDownloadFailed(emscripten_fetch_t *fetch) {
  emscripten_fetch_close(fetch);
}

void edgesDownloadFailed(emscripten_fetch_t *fetch) {
  emscripten_fetch_close(fetch); // Also free data on failure.
}

void FetchVertices()
{
  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  attr.onsuccess = verticesDownloadSucceeded;
  attr.onerror = verticesDownloadFailed;
  emscripten_fetch(&attr, "test.dat");
}

void FetchEdges()
{
  //
}

std::unordered_map<boost::uuids::uuid, Vertex, boost::hash<boost::uuids::uuid>> ParseVertices(std::string jsonData)
{
  jsonData
}

std::unordered_map<boost::uuids::uuid, Edge, boost::hash<boost::uuids::uuid>> ParseEdges(std::string jsonData)
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
    emscripten::val window = emscripten::val::global("window");
    emscripten::val document = emscripten::val::global("document");
    auto canvas = document.call<emscripten::val>("getElementById", emscripten::val("canvas"));
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
