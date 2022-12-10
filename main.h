//
// Created on 12/10/2022.
//

#ifndef ONLINE_TRAFFIC_SIM_MAIN_H
#define ONLINE_TRAFFIC_SIM_MAIN_H

#include <emscripten/val.h>
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>

class main {

};

void InitializeCanvas(emscripten::val canvas)
{
    emscripten::val window = emscripten::val::global("window");
    auto parentPanel = canvas["parentElement"];
    canvas.set("width", panel["offsetWidth"]);
    canvas.set("height", panel["offsetHeight"]);
    emscripten::val ctx = canvas.call<emscripten::val>("getContext", emscripten::val("2d"));
    ctx.set("textAlign", emscripten::val("center"));
    ctx.set("textBaseline", emscripten::val("middle"));
    ctx.set("font", emscripten::val("20px Arial"));
}

int main()
{
    emscripten::val window = emscripten::val::global("window");
    emscripten::val document = emscripten::val::global("document");
    window.call<void>("addEventListener", emscripten::val("resize"), emscripten::val::module_property("InitializeCanvases"));
    /*
    document.call<void>("addEventListener", emscripten::val("mousedown"), emscripten::val::module_property("InteractWithCanvas"));
    document.call<void>("addEventListener", emscripten::val("mouseup"), emscripten::val::module_property("InteractWithCanvas"));
    document.call<void>("addEventListener", emscripten::val("mousemove"), emscripten::val::module_property("InteractWithCanvas"));
    */
    // initialize width and height of the canvas
    InitializeCanvases(emscripten::val::null());

    // retrieve all settings from localStorage and set the appropriate boxes to "checked" and put the appropriate data into preview
    InitializeAllSettings();

    return 0;
}

EMSCRIPTEN_BINDINGS(bindings)\
{\
  emscripten::function("InitializeCanvas", InitializeCanvas);\
  emscripten::function("RenderCanvas", RenderCanvas);\
};

#endif //ONLINE_TRAFFIC_SIM_MAIN_H
