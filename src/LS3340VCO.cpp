#include "LabSeven.hpp"

#include "LabSeven_3340_VCO.h"
#include <time.h>

#include <fstream>
using namespace std;

//TODO:
//fine tune vco parameters to match my own synth

struct LS3340VCO : Module
{
    enum ParamIds
    {
		PARAM_MOD,
		PARAM_RANGE,
		PARAM_PULSEWIDTH,
		PARAM_PWMSOURCE,
		PARAM_VOLSQUARE,
		PARAM_VOLSAW,
    PARAM_VOLTRIANGLE,
		PARAM_VOLSUBOSC,
		PARAM_SUBOSCRATIO,
		PARAM_VOLNOISE,
		NUM_PARAMS
	};
    enum InputIds
    {
		IN_PITCH,
		IN_MOD,
		IN_RANGE,
		IN_LFO,
		IN_ENV,
		IN_SUBOSCSELECT,
		NUM_INPUTS
	};
    enum OutputIds
    {
		OUT_SQUARE,
		OUT_SAW,
		OUT_SUB,
		OUT_TRIANGLE,
		OUT_MIX,
		OUT_NOISE,
		NUM_OUTPUTS
	};
    enum LightIds
    {
		NUM_LIGHTS
	};

    //VCO instance and VCO output frame
    LabSeven::LS3340::TLS3340VCO vco;
    LabSeven::LS3340::TLS3340VCOFrame nextFrame;

    // Panel Theme
    int Theme = 0;

    LS3340VCO() {
      config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
      configParam(LS3340VCO::PARAM_MOD, 0.0f, 1.0f, 0.0f, "Mod");
      configSwitch(LS3340VCO::PARAM_RANGE, 0.0, 3.0, 1.0, "Range", {"16'", "8'", "4'", "2'"});
      configParam(LS3340VCO::PARAM_PULSEWIDTH, 0.0f, 0.5f, 0.0f, "Pulse width");
      configSwitch(LS3340VCO::PARAM_PWMSOURCE, 0.0f, 2.0f, 1.0f, "PWM source", {"Env", "Man", "LFO"});
      configParam(LS3340VCO::PARAM_VOLSQUARE, 0.0f, 1.0f, 0.0f, "Square volume");
      configParam(LS3340VCO::PARAM_VOLSAW, 0.0f, 1.0f, 0.0f, "Saw volume");
      configParam(LS3340VCO::PARAM_VOLTRIANGLE, 0.0f, 1.0f, 0.0f, "Triangle volume");
      configParam(LS3340VCO::PARAM_VOLSUBOSC, 0.0f, 1.0f, 0.0f, "Sub osc volume");
      configParam(LS3340VCO::PARAM_SUBOSCRATIO, 0.0f, 2.0f, 2.0f, "Sub osc ratio");
      configParam(LS3340VCO::PARAM_VOLNOISE, 0.0f, 1.0f, 0.0f, "Noise volume");
      configInput(IN_PITCH, "Pitch");
      configInput(IN_MOD, "Mod");
      configInput(IN_RANGE, "Range");
      configInput(IN_LFO, "LFO");
      configInput(IN_ENV, "Envelope");
      configInput(IN_SUBOSCSELECT, "Sub osc select");
      configOutput(OUT_SQUARE, "Square");
      configOutput(OUT_SAW, "Saw");
      configOutput(OUT_SUB, "Sub osc");
      configOutput(OUT_TRIANGLE, "Triangle");
      configOutput(OUT_MIX, "Mix");
      configOutput(OUT_NOISE, "Noise");
      srand(time(0));
    }
    
    void process(const ProcessArgs& args) override;

    float sampleTimeCurrent = 0.0;
    float sampleRateCurrent = 0.0;

    double pitch,maxPitch,rangeFactor;
	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};


void LS3340VCO::process(const ProcessArgs& args)
{
    //update external sample rate if neccessary
    if (sampleTimeCurrent != args.sampleTime)
    {
        sampleTimeCurrent = args.sampleTime;
        sampleRateCurrent = 1.0/sampleTimeCurrent;
        vco.setSamplerateExternal(sampleRateCurrent);

		maxPitch = sampleRateCurrent*0.45;
    	if (maxPitch > 40000) maxPitch = 40000; //high value so that suboscillator can go up to 10kHz
    }

	//get pitch and pitch mod input
    pitch = inputs[IN_PITCH].getVoltage();
    pitch +=  pow(2,2.25*0.2*inputs[IN_MOD].getVoltage() * params[PARAM_MOD].getValue());

    //set rangeFactor
	rangeFactor = params[PARAM_RANGE].getValue();
	switch ((int)rangeFactor)
	{
		case 0: rangeFactor = 0.5; break;
		case 1: rangeFactor = 1.0; break;
		case 2: rangeFactor = 2.0; break;
		case 3: rangeFactor = 4.0; break;
		default: rangeFactor = 1.0;
	}

    //range modulation
    if (inputs[IN_RANGE].active)
    {
        int rangeSelect = (int)round(inputs[IN_RANGE].getVoltage()*0.6);
        switch (rangeSelect)
        {
            case -3: rangeFactor /= 8.0; break;
            case -2: rangeFactor /= 4.0; break;
            case -1: rangeFactor /= 2.0; break;
            case  0: break; //no change
            case  1: rangeFactor *= 2.0; break;
            case  2: rangeFactor *= 4.0; break;
            case  3: rangeFactor *= 8.0; break;
        }
        if (rangeFactor > 16.0) rangeFactor = 16.0;
    }

    //set pitch
    //TODO: Clean up this paragraph!!!
	pitch = 261.626f * pow(2.0, pitch) * rangeFactor;
    pitch = clamp(pitch, 0.01f, maxPitch);
    //simulate the jitter observed in the hardware synth
    //use values > 0.02 for dirtier sound
    pitch *= 1.0+0.02*((double) rand() / (RAND_MAX)-0.5);
    vco.setFrequency(pitch);


	//update suboscillator
    switch((int)inputs[IN_SUBOSCSELECT].getVoltage())
    {
        case 1: vco.setSuboscillatorMode(0); break;
        case 2: vco.setSuboscillatorMode(1); break;
        case 3: vco.setSuboscillatorMode(2); break;
        default: vco.setSuboscillatorMode(2 - (unsigned short)params[PARAM_SUBOSCRATIO].getValue());
    }

	//pulse width modulation
    switch ((int)params[PARAM_PWMSOURCE].getValue())
	{
        //LFO PWM requires values between -0.4 and 0.4; SH does PWM between 10% and 90% pulse width
        case 2:  vco.setPwmCoefficient(-2.0*params[PARAM_PULSEWIDTH].getValue()*0.4*0.2*inputs[IN_LFO].getVoltage()); break; //bipolar, -5V - +5V
        case 1:  vco.setPwmCoefficient(-0.8*params[PARAM_PULSEWIDTH].getValue()); break;
        case 0:  vco.setPwmCoefficient(-2.0*params[PARAM_PULSEWIDTH].getValue()*0.4*0.1*inputs[IN_ENV].getVoltage()); break; //unipolar, 0V - +10v
        default: vco.setPwmCoefficient(-0.8*params[PARAM_PULSEWIDTH].getValue());
	}

    //get next frame and put it out
    double scaling = 8.0;

    //TODO: PROPER (FREQUENCY DEPENDENT) AMPLITUDE SCALING FOR SAW AND TRIANGLE
    //TODO: PWM FOR TRIANGLE

    //calculate next frame

    if (this->sampleRateCurrent != 192000)
    {
        //TODO: Add a 'standard/high' quality switch to GUI
        //and choose interpolation method accordingly
        if (true)
        {
            vco.getNextFrameAtExternalSampleRateSinc(&nextFrame);
        }
        else
        {
            //currently next neighbour interpolation, not used!
            //TODO: Add quality switch (low/medium/high) to select
            //nn (has its own sound), dsp::cubic or sinc interpolation
            vco.getNextFrameAtExternalSampleRateCubic(&nextFrame);
        }
    }
    else //no interpolation required if internal sample rate == external sample rate == 192kHz
    {
        vco.getNextBlock(&nextFrame,1);
    }

    //TODO: Activate/deactivate interpolation if outs are not active
    outputs[OUT_SQUARE].setVoltage(scaling   * nextFrame.square);
    outputs[OUT_SAW].setVoltage(scaling      * nextFrame.sawtooth);
    outputs[OUT_SUB].setVoltage(scaling      * nextFrame.subosc);
    outputs[OUT_TRIANGLE].setVoltage(scaling * nextFrame.triangle);
    outputs[OUT_NOISE].setVoltage(6.0* nextFrame.noise);
    outputs[OUT_MIX].setVoltage(0.4*(outputs[OUT_SQUARE].getVoltage()   * params[PARAM_VOLSQUARE].getValue() +
                                     outputs[OUT_SAW].getVoltage()      * params[PARAM_VOLSAW].getValue() +
                                     outputs[OUT_SUB].getVoltage()      * params[PARAM_VOLSUBOSC].getValue() +
                                     outputs[OUT_TRIANGLE].getVoltage() * params[PARAM_VOLTRIANGLE].getValue() +
                                     outputs[OUT_NOISE].getVoltage()    * params[PARAM_VOLNOISE].getValue()));
}

struct LS3340VCOClassicMenu : MenuItem {
	LS3340VCO *ls3340vco;
	void onAction(const event::Action &e) override {
		ls3340vco->Theme = 0;
	}
	void step() override {
		rightText = (ls3340vco->Theme == 0) ? "✔" : "";
		MenuItem::step();
	}
};

struct LS3340VCOBlueMenu : MenuItem {
	LS3340VCO *ls3340vco;
	void onAction(const event::Action &e) override {
		ls3340vco->Theme = 1;
	}
	void step() override {
		rightText = (ls3340vco->Theme == 1) ? "✔" : "";
		MenuItem::step();
	}
};

struct LS3340VCOWidget : ModuleWidget {
      SvgPanel *panelClassic;
      SvgPanel *panelBlue;

      void appendContextMenu(Menu *menu) override {
        LS3340VCO *ls3340vco = dynamic_cast<LS3340VCO*>(module);
        assert(ls3340vco);
        menu->addChild(construct<MenuEntry>());
        menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Theme"));
        menu->addChild(construct<LS3340VCOClassicMenu>(&LS3340VCOClassicMenu::text, "Classic (default)", &LS3340VCOClassicMenu::ls3340vco, ls3340vco));
        menu->addChild(construct<LS3340VCOBlueMenu>(&LS3340VCOBlueMenu::text, "Blue", &LS3340VCOBlueMenu::ls3340vco, ls3340vco));
      }

      void step() override {
      	if (module) {
      		LS3340VCO *ls3340vco = dynamic_cast<LS3340VCO*>(module);
      		assert(ls3340vco);
      		panelClassic->visible = (ls3340vco->Theme == 0);
      		panelBlue->visible = (ls3340vco->Theme == 1);
      	}
      	ModuleWidget::step();
      }

    	LS3340VCOWidget(LS3340VCO *module) {
        setModule(module);
        box.size = Vec(17 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

		//BACKGROUND
        // Classic Theme
        panelClassic = new SvgPanel();
        panelClassic->box.size = box.size;
        panelClassic->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/LabSeven_3340_Classic_Skins/LabSeven_3340_VCO.svg")));
        panelClassic->visible = true;
        addChild(panelClassic);
        // Blue Theme
        panelBlue = new SvgPanel();
        panelBlue->box.size = box.size;
        panelBlue->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/LabSeven_3340_Standard_Skins_blue/LabSeven_3340_VCO.svg")));
        panelBlue->visible = false;
        addChild(panelBlue);

		//SCREWS
    		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
    		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		//INPUTS
        addInput(createInput<LabSeven_Port>(Vec(114, 380-24-231+2), module, LS3340VCO::IN_PITCH));
        addInput(createInput<LabSeven_Port>(Vec( 75, 380-24-231+2), module, LS3340VCO::IN_MOD));
        addInput(createInput<LabSeven_Port>(Vec(114, 380-24-276+2), module, LS3340VCO::IN_RANGE));
        addInput(createInput<LabSeven_Port>(Vec(219, 380-24-284+2), module, LS3340VCO::IN_LFO));
        addInput(createInput<LabSeven_Port>(Vec(219, 380-24-214+2), module, LS3340VCO::IN_ENV));
        addInput(createInput<LabSeven_Port>(Vec(153, 380-24- 32+2), module, LS3340VCO::IN_SUBOSCSELECT));

        //VCO SECTION
        addParam(createParam<LabSeven_3340_FaderRedLargeRed>(Vec(28-4, 380-24-272), module, LS3340VCO::PARAM_MOD));
        addParam(createParam<LabSeven_3340_KnobLargeRange>(Vec(69, 380-36-266), module, LS3340VCO::PARAM_RANGE));
        addParam(createParam<LabSeven_3340_FaderRedLargeRed>(Vec(164-4, 380-24-272), module, LS3340VCO::PARAM_PULSEWIDTH));
        addParam(createParam<LabSeven_3340_FaderRedLargeYellow3Stage>(Vec(201-4, 380-40-234), module, LS3340VCO::PARAM_PWMSOURCE));

        //SOURCE MIXER SECTION
        addParam(createParam<LabSeven_3340_FaderRedLargeGreen>(Vec( 28-4, 380-20-4-125), module, LS3340VCO::PARAM_VOLSQUARE));
        addParam(createParam<LabSeven_3340_FaderRedLargeGreen>(Vec( 59-4, 380-20-4-125), module, LS3340VCO::PARAM_VOLSAW));
        addParam(createParam<LabSeven_3340_FaderRedLargeGreen>(Vec( 90-4, 380-20-4-125), module, LS3340VCO::PARAM_VOLTRIANGLE));
        addParam(createParam<LabSeven_3340_FaderRedLargeGreen>(Vec(121-4, 380-20-4-125), module, LS3340VCO::PARAM_VOLSUBOSC));
        addParam(createParam<LabSeven_3340_FaderRedLargeYellow3Stage>(Vec(158-4-1, 380-40-88), module, LS3340VCO::PARAM_SUBOSCRATIO));
        addParam(createParam<LabSeven_3340_FaderRedLargeGreen>(Vec(213-4, 380-20-4-125), module, LS3340VCO::PARAM_VOLNOISE));

		//OUTPUTS
        addOutput(createOutput<LabSeven_Port>(Vec( 24, 380-30-24), module, LS3340VCO::OUT_SQUARE));
        addOutput(createOutput<LabSeven_Port>(Vec( 55, 380-30-24), module, LS3340VCO::OUT_SAW));
        addOutput(createOutput<LabSeven_Port>(Vec(117, 380-30-24), module, LS3340VCO::OUT_SUB));
        addOutput(createOutput<LabSeven_Port>(Vec( 86, 380-30-24), module, LS3340VCO::OUT_TRIANGLE));
        addOutput(createOutput<LabSeven_Port>(Vec(181, 380-30-24), module, LS3340VCO::OUT_MIX));
        addOutput(createOutput<LabSeven_Port>(Vec(208, 380-30-24), module, LS3340VCO::OUT_NOISE));
     }
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelLS3340VCO = createModel<LS3340VCO, LS3340VCOWidget>("LS3340VCO");
