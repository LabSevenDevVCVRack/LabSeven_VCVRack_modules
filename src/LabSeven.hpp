#include "rack.hpp"

#pragma once

using namespace rack;

// Forward-declare the Plugin, defined in Template.cpp
extern Plugin *pluginInstance;

// Forward-declare each Model, defined in each module source file
extern Model *modelLS3340VCO;

//GUI elements
struct LabSeven_Port : SvgPort
{
	LabSeven_Port()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/LabSeven_Port.svg")));
	}
};

struct LabSeven_3340_FaderLarge : SvgSlider
{
    LabSeven_3340_FaderLarge()
	{
		Vec margin = Vec(4, 4);
		maxHandlePos = Vec(0, -20).plus(margin);
		minHandlePos = Vec(0, 40).plus(margin);
        background->svg = APP->window->loadSvg(asset::plugin(pluginInstance,"res/LabSeven_3340_SlidePot.svg"));
		background->wrap();
		background->box.pos = margin;
		box.size = background->box.size.plus(margin.mult(2));
	}
};

struct LabSeven_3340_FaderRedLargeRed : LabSeven_3340_FaderLarge
{
    LabSeven_3340_FaderRedLargeRed()
	{
        handle->svg = APP->window->loadSvg(asset::plugin(pluginInstance,"res/LabSeven_3340_SlidePotHandleRed.svg"));
		handle->wrap();
	}
};

struct LabSeven_3340_FaderRedLargeGreen : LabSeven_3340_FaderLarge
{
    LabSeven_3340_FaderRedLargeGreen()
	{
        handle->svg = APP->window->loadSvg(asset::plugin(pluginInstance,"res/LabSeven_3340_SlidePotHandleGreen.svg"));
		handle->wrap();
	}
};

struct LabSeven_3340_FaderRedLargeYellow3Stage : SvgSlider
{
    LabSeven_3340_FaderRedLargeYellow3Stage()
	{
		Vec margin = Vec(4, 4);
		maxHandlePos = Vec(0, -20).plus(margin);
		minHandlePos = Vec(0,  -5).plus(margin);

        background->svg = APP->window->loadSvg(asset::plugin(pluginInstance,"res/LabSeven_3340_SlidePot3Stage.svg"));
		background->wrap();
		background->box.pos = margin;

		box.size = background->box.size.plus(margin.mult(2));
        handle->svg = APP->window->loadSvg(asset::plugin(pluginInstance,"res/LabSeven_3340_SlidePotHandleYellow.svg"));
		handle->wrap();
		snap=true;
	}
};


struct LabSeven_3340_KnobLarge : SvgKnob
{
    LabSeven_3340_KnobLarge()
    {
		box.size = Vec(36, 36);
		minAngle = -0.3*M_PI;
		maxAngle = 0.3*M_PI;
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/LabSeven_3340_KnobLarge.svg")));
		snap=true;
	}
};

struct LabSeven_3340_KnobLargeRange : LabSeven_3340_KnobLarge
{
    LabSeven_3340_KnobLargeRange()
    {
		minAngle = -0.25*M_PI;
		maxAngle = 0.27*M_PI;
	}
};

/*
struct LabSeven_KnobLargeBlue : SVGKnob {
    LabSeven_KnobLargeBlue() {
		box.size = Vec(50, 50);
		minAngle = -0.75*M_PI;
		maxAngle = 0.75*M_PI;
        setSVG(APP->window->loadSvg(asset::plugin(pluginInstance, "res/LabSeven_KnobLargeBlue.svg")));
	}
};
*/
