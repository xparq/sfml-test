﻿#include "OON.hpp"

#include "sfw/GUI.hpp" //!! Used to be in OON_sfml only, but since scroll_locked() requires it...
                       //!! (And sooner or later it must be usable unrestricted anyway!
#include "UI/hud.hpp"  //!! <-- And also this would be integrated there, too, eventually.
                       //!! And we're already using keystate() here, too, shamelessly! ;) )

#include "sz/sign.hh"

#include <cstdlib>
	using std::rand; // and the RAND_MAX macro!
#include <cmath>
	using std::pow;
#include <iostream>
	using std::cerr, std::endl;
#include <cassert>

using namespace Szim;
using namespace Model;
using namespace Math;
using namespace UI;
using namespace std;


//----------------------------------------------------------------------------
#ifdef DEBUG
static std::string hud_test_callback_string() { return "this is a std::string"; }
static const char* hud_test_callback_ccptr()  { return "this is a const char*"; }
#endif


//----------------------------------------------------------------------------
void OON::show_cmdline_help(const Args& args, const char* banner)
{
	banner = "\"Out of Nothing\" - experimental simulation toy\n";

	SimApp::show_cmdline_help(args, banner);

	cout << R"(
  -C cfgfile
          Load config. from 'cfgfile'. If not found, abort.
	  If omitted, ./default.cfg is tried, and if that doesn't exist,
	  internal hardcoded defaultw will be used as a fallback.

  ...lots more to be documented here, sorry!
)";
}


//============================================================================
OON::OON(int argc, char** argv) :
	SimApp(argc, argv),
	appcfg(cfg, args) //!! appcfg(SimApp::syscfg)
{}

//----------------------------------------------------------------------------
void OON::init() // override
{
	//!! Currently, the data watcher HUD setup depends directly on the player objects
	//!! that have just been created above, so the UI init CAN'T happen before that...:
	//_setup_UI();

	const auto& w = const_world();
		//!! "Some" model world has been implicitly created by SimApp for now... :-o

	// Add the "Player Superglobe" first
	//!!
	//!! THIS MUST COME BEFORE CALLING add_random_bodies_near(player)! :-o
	//!!
	[[maybe_unused]] auto player_entity_index
		= add_player({.r = w.CFG_GLOBE_RADIUS, .density = Physics::DENSITY_OF_EARTH, .p = {0,0}, .v = {0,0}, .color = 0xffff20,
		              .mass = appcfg.get("sim/player_globe_mass", 50 * Physics::MASS_OF_EARTH)});
	assert(entity_count() > player_entity_index);
	assert(player_entity_ndx() == player_entity_index);

	focused_entity_ndx = player_entity_ndx();

	_setup_UI();

	//!! MOVE THE SESSION LOGIC TO SimApp:
	// Restore or start session (currently just loading a snapshot...) if requested (i.e. --session=name)...
	if (args["session"]) { // If empty, a new unnamed session will be started; see also at done()!
		session.open(args("session")); // So, a previously autosaved unnamed session state will NOT be loaded implicitly!
	} else {
		//!! Here should be a "load default initial world state" thing...
		//!!
		//!! session.create(); // call back to the app to build default state
		//!!
		//!! Or even just this would be fine:
		//!! load_snapshot(DEFAULT);
		//!!
		//!! - But preferably also being able to load from some text format
		//!!   (TOML etc.), to finally replace this sad little hardcoding here:
		//!!
		// Add 2 "moons" with fixed parameters (mainly for testing):
		add_body({.r = w.CFG_GLOBE_RADIUS/10, .p = {w.CFG_GLOBE_RADIUS * 2, 0}, .v = {0, -w.CFG_GLOBE_RADIUS * 2},
					.color = 0xff2020, .mass = 3e24f});
		add_body({.r = w.CFG_GLOBE_RADIUS/7,  .p = {-w.CFG_GLOBE_RADIUS * 1.6f, +w.CFG_GLOBE_RADIUS * 1.2f}, .v = {-w.CFG_GLOBE_RADIUS*1.8, -w.CFG_GLOBE_RADIUS*1.5},
					.color = 0x3060ff, .mass = 3e24f});
	}

	// App-level cmdline options (overrides)...
	// Note: the (!!actually: "some"...!!) system-/engine-level options have been processed/applied already!
	try { // <- Absolutely required, as sto...() are very throw-happy.
		// Doing the ones that can't fail first, so an excpt. won't skip them:
		if (args["interact"]) {
			interact_all();
		}; if (args["bodies"]) {
			auto n = stoi(args("bodies")) - 2; // 2 have already been created above
			add_random_bodies_near(player_entity_ndx(), n < 0 ? 0 : n); //! Dodge a possible overflow of n
		}; if (args["friction"]) {
			float f = stof(args("friction"));
			world().FRICTION = f;
		}; if (!args("zoom").empty()) { //!! Should be done by SimApp, and then the audio init
			float factor = stof(args("zoom"));
			if (factor) zoom_reset(factor);
		}
	} catch(...) {
		cerr << __FUNCTION__ ": ERROR processing/applying some cmdline args!\n";
		request_exit(-1);
		return;
	}

	// Audio...
	//!!
	//!! Later, with more mature session mgmt., music loading etc. should NOT
	//!! happen here, ignoring all that!
	//!!
	// Note: --snd=off has been taken care of by the engine already, so it's
	// safe to start playing here no matter what; it will not unmute.
	clack_sound = backend.audio.add_sound(string(cfg.asset_dir + "sound/clack.wav").c_str());
	backend.audio.play_music(cfg.background_music.c_str());
	//backend.audio.play_music(sz::prefix_if_rel(asset_dir, "music/extra sonic layer.ogg"));
}


//----------------------------------------------------------------------------
void OON::done() // override
{
//	cerr << __FUNCTION__ << ": Put any 'onExit' tasks (like saving the last state) here!...\n";

	//!! MOVE THE SESSION LOGIC TO SimApp:
	// Let the session-manager auto-save the current session (unless disabled with --session-no-autosave; see SimApp::init()!)
	if (args["session"]) { // If empty and no --session-save-as, it will be saved as "UNNAMED.autosave" or sg. like that.
		session.close();
	}
}

//----------------------------------------------------------------------------
#ifndef DISABLE_HUD
void OON::toggle_huds()  { _show_huds = !_show_huds; }
bool OON::huds_active()  { return _show_huds; }
void OON::toggle_help()  { ui_gebi(HelpPanel).active(!ui_gebi(HelpPanel).active()); }
#endif

//----------------------------------------------------------------------------
void OON::_setup_UI()
{
	using namespace sfw;
	// The SFW GUI is used as a translucent overlay, so an alpha-enabled bgColor
	// must be applied. The clearBackground option must be left at its default (true):
	//Theme::clearBackground = false;
	Theme::click.textColor = sfw::Color("#ee9"); //!!("input".textColor!) YUCK!! Also "click" for LABELS?!?!
	auto gui_main_hbox = gui.add(new HBox);

	// Misc UI controls...
	auto left_vbox = gui_main_hbox->add(new VBox);
		auto	gui_form = left_vbox->add(new Form, "Params");
			gui_form->add("Show HUDs", new CheckBox([&](auto*){ this->toggle_huds(); }, huds_active()));
			gui.recall("Show HUDs")->setTooltip("Press [?] to toggle the Help panel");
		auto	volrect = left_vbox->add(new Form, "VolForm");
			volrect->add("Volume", new Slider({/*.orientation = Vertical*/}, 70), "volume slider")
			->setCallback([&](auto* w){backend.audio.volume(w->get());})
			->update(75); // %
		auto	audio_onoff = left_vbox->add(new Form, "AudioOnOffForm");
			audio_onoff->add("Audio: ", new CheckBox([&](auto*){backend.audio.toggle_audio();}, backend.audio.enabled));
			audio_onoff->add(" - FX: ", new CheckBox([&](auto*){backend.audio.toggle_sounds();}, backend.audio.fx_enabled));

	// View...
	gui_main_hbox->add(new Label(" ")); // Just a vert. spacer
	auto	view_form = gui_main_hbox->add(new Form);
		view_form->add("Pan override", new CheckBox); // Will be polled by the control loop!
		view_form->add(" - pan locked", new CheckBox)->disable(); // Will be updated by the ctrl. loop!

	// Physics props...
	gui_main_hbox->add(new Label(" ")); // Just a vert. spacer
	auto	phys_form = gui_main_hbox->add(new Form);
		auto g_select = new OptionsBox<World::GravityMode>();
			g_select->add("Normal", World::GravityMode::Normal);
			g_select->add("Skewed", World::GravityMode::Skewed);
			g_select->add("Off",    World::GravityMode::Off);
			//g_select->set(World::GravityMode::Off);
			g_select->setCallback([&](auto* w){ this->world().gravity_mode = w->get(); });
		phys_form->add("Gravity mode", g_select);
		phys_form->add(" - bias", new sfw::Slider({.range={-3.0, 3.0}, .step=0, .orientation=sfw::Horizontal}, 80))
			->setCallback([&](auto* w){ this->world().gravity = Model::Physics::G * pow(10.f, w->get()); })
			->set(0);
		phys_form->add("Fixed model Δt", new CheckBox(
			[&](auto*){ this->toggle_fixed_model_dt(); }, cfg.fixed_model_dt_enabled));

	// Save/load...
	gui_main_hbox->add(new Label(" ")); // just a vert. spacer
	auto	saveload_form = gui_main_hbox->add(new Form);
		saveload_form->add("File", new TextBox);
		auto	saveload_buttons = saveload_form->add("", new HBox);
			saveload_buttons->add(new Button("Save"))
				->setTextColor(sf::Color::Black)->setColor(sfw::Color("#f002"))
				->setCallback([&]{
					if (auto* fname_widget = (TextBox*)gui.recall("File"); fname_widget) {
						auto fname = fname_widget->get();
						this->save_snapshot(fname.empty() ? "UNTITLED.save" : fname.c_str());
					}
				});
			saveload_buttons->add(new Button("Load"))
				->setTextColor(sf::Color::Black)->setColor(sfw::Color("#0f02"))
				->setCallback([&]{
					if (auto* fname_widget = (TextBox*)gui.recall("File"); fname_widget) {
						auto fname = fname_widget->get();
						this->load_snapshot(fname.empty() ? "UNTITLED.save" : fname.c_str());
					}
				});

	// Only position after built, so it has its dimensions:
	gui.setPosition(4, cfg.WINDOW_HEIGHT - gui.getSize().y - 4);
		//!! For that 4 above: sfw is still too lame for styling margins/padding... :-/
		//!! Not even this would do anything, actually: ->setPosition({100, -200});

#ifndef DISABLE_HUD
	//!!?? Why do all these member pointers just work, also without so much as a warning,
	//!!?? in this generic pointer passing context?!
	//!!
	//!! "Evenfurthermore": why do all these insane `this` captures apparently survive
	//!! all the obj recreation shenanigans (they *are* recreated, right??...) after
	//!! a World reload?!?!?!
	//!!

	//! This still needs to be a double nested lambda: the outer wrapper "converts" the
	//! signature to match the string-returning nullary functions the HUD stuff supports.
	auto ftos = [](auto* ptr_x) { return [ptr_x]() { static constexpr size_t LEN = 15;
			char buf[LEN + 1]; auto [ptr, ec] = std::to_chars(buf, buf+LEN, *ptr_x);
			return string(ec != std::errc() ? "???" : (*ptr = 0, buf));
		};
	};

	//------------------------------------------------------------------------
	// Timing
	ui_gebi(TimingStats)
		<< "FPS: " << [&](){ return to_string(1 / (float)this->avg_frame_delay); }
		<< "\nlast frame Δt: " << [&](){ return to_string(this->time.last_frame_delay * 1000.0f) + " ms"; }
		<< "\nmodel Δt: " << [&](){ return to_string(this->time.last_model_Δt * 1000.0f) + " ms"; }
		<<            " " << [&](){ return cfg.fixed_model_dt_enabled ? "(fixed)" : ""; }
		<< "\ncycle: " << [&](){ return to_string(iterations); }
		<< "\nReal elapsed time: " << &time.real_session_time
	//!!??WTF does this not compile? (It makes no sense as the gauge won't update, but regardless!):
	//!!??  << vformat("frame dt: {} ms", time.last_frame_delay)
		<< "\nTime reversed: " << &time.reversed
		<< "\nTime scale: " << ftos(&this->time.scale)
		<< "\nModel timing stats:"
//		<< "\n    updates: " << &time.model_Δt_stats.samples
		<< "\n    total t: " << &time.model_Δt_stats.total
		<< "\n  Δt:"
		<< "\n    last: " << &time.model_Δt_stats.last
		<< "\n    min abs: " << &time.model_Δt_stats.umin
		<< "\n    max abs: " << &time.model_Δt_stats.umax
		<< "\n    min: " << &time.model_Δt_stats.min
		<< "\n    max: " << &time.model_Δt_stats.max
		<< "\n    avg.: " << [&]{ return to_string(this->time.model_Δt_stats.average());}
	;
//cerr << timing_hud;

	//------------------------------------------------------------------------
	// World
	ui_gebi(WorldData)
		<< "# of objs.: " << [&](){ return to_string(this->entity_count()); }
		<< "\nBody interactions: " << &this->const_world()._interact_all
		<< "\nGravity mode: " << [&](){ return to_string((unsigned)this->const_world().gravity_mode); }
		<< "\n  - strength: " << &this->const_world().gravity
		<< "\nDrag: " << ftos(&this->const_world().FRICTION)
		<< "\n"
	;

	//------------------------------------------------------------------------
	// View
	ui_gebi(ViewData)
		<< "CAMERA: "
		<< "\n  X: " << &view.offset.x << ", Y: " << &view.offset.y
		<< "\n  ZOOM: " << &view.scale
	;

//	???_hud << "\nPress ? for help...";

	//------------------------------------------------------------------------
	// "Object Observer"

	//! Setting up player data watches below requires that the player
	//! entity have actually been created already!
	//! If not, an assertion will fail, but only in the DEBUG build!
	//! So... Just checking it run-time, too, as I have made this mistake
	//! too many times now... :)
  if ( !(player_entity_ndx() < entity_count()) ) {
	cerr << "- INTERNAL ERROR: UI/PlayerHUD init before player entity init!\n";
  } else {

	ui_gebi(ObjectData)
		<< [&]{	if (this->focused_entity_ndx == ~0u) return "<NOTHING>"s;
			if (this->focused_entity_ndx >= this->entity_count()) return "INVALID ENTITY #"s + to_string(this->focused_entity_ndx);
			if (this->focused_entity_ndx == player_entity_ndx()) return "Player #"s + to_string(player_entity_ndx());
			else return "Object #"s + to_string(this->focused_entity_ndx); }
		<< "\n"
//		<< "\n  R: " << ftos(&this->player_entity().r) //!!#29: &(world().CFG_GLOBE_RADIUS) // OK now, probably since c365c899
		<< "\n  lifetime: " << [&]{ if (this->focused_entity_ndx >= this->entity_count()) return ""s;
		                     else return this->entity(this->focused_entity_ndx).lifetime == World::Body::Unlimited ?
		                                 "(infinite)" : to_string(this->entity(this->focused_entity_ndx).lifetime); }
		<< "\n  R: " << [&]{ if (this->focused_entity_ndx >= this->entity_count()) return ""s;
		                     else return to_string(this->entity(this->focused_entity_ndx).r); }
		<< "\n  T: " << [&]{ if (this->focused_entity_ndx >= this->entity_count()) return ""s;
		                     else return to_string(this->entity(this->focused_entity_ndx).T); }
//		<< "\n  M: " << ftos(&this->player_entity().mass)
		<< "\n  M: " << [&]{ if (this->focused_entity_ndx >= this->entity_count()) return ""s;
		                     else { auto M = this->entity(this->focused_entity_ndx).mass / 6e24f;
		                            return ftos(&M)(); } } << " x Earth"
//		<< "\n  x: " << ftos(&this->player_entity().p.x)
//		<<   ", y: " << ftos(&this->player_entity().p.y)
//		<< "\n  vx: " << ftos(&this->player_entity().v.x)
//		<<   ", vy: " << ftos(&this->player_entity().v.y)
		<< "\n  x: " << [&]{ if (this->focused_entity_ndx >= this->entity_count()) return ""s;
		                     else return (ftos(&this->entity(this->focused_entity_ndx).p.x))(); }
		<<   ", y: " << [&]{ if (this->focused_entity_ndx >= this->entity_count()) return ""s;
		                     else return (ftos(&this->entity(this->focused_entity_ndx).p.y))(); }
		<< "\n  vx: " << [&]{ if (this->focused_entity_ndx >= this->entity_count()) return ""s;
		                     else return (ftos(&this->entity(this->focused_entity_ndx).v.x))(); }
		<<   ", vy: " << [&]{ if (this->focused_entity_ndx >= this->entity_count()) return ""s;
		                     else return (ftos(&this->entity(this->focused_entity_ndx).v.y))(); }
	;
  }

	//------------------------------------------------------------------------
	// Debug
#ifdef DEBUG
	auto& debug_hud = ui_gebi(Debug);

//!!Should be Rejected compile-time (with a static_assert):
//!! - well, rejected indeed, but only "fortunately", and NOT by my static_assert!... :-/
//!!	debug_hud << "DBG>" << string("debug");
	static const auto const_debug = "CONST STRING "s; debug_hud << "DBG>" << &const_debug;
//!!shouldn't compile:	debug_hud << "DBG>" << const_debug;
	static       auto debug = "STR "s;                debug_hud << "DBG>" << &debug;
//!!shouldn't compile:	debug_hud << "DBG>" << debug;
	debug_hud << "\nHŐTŰRŐ lótúró [αβ°C]" << "\n";
	debug_hud << "\n"
		<< "test fn->string: " << hud_test_callback_string << "\n"
		<< "test fn->ccptr: " << hud_test_callback_ccptr << "\n"
		<< "test λ->ccptr: " << []{ return "autoconv to string?..."; } << "\n"
//!!NOPE:	<< "test λ->int: " << []{ return 0xebba; } << "\n"
//!!NOPE:	<< "test λ->float: " << []{ return 12.345; } << "\n"
		// By-value data that are not closures:
		<< "test val. string: " << "temp"s << "\n"
		<< "test val. int: " << 12345 << "\n"
		<< "test val. float: " << 12.345f << "\n"
		<< "test val. double: " << 1e300 << "\n"
//!!NOT YET:	<< "test val. bool: " << true << "\n"
	<< "\n";

/*	debug_hud
		<< "\n"
		<< "\nSHIFT" << (bool*)&_kbd_state[SHIFT]);
		<< "\nLSHIFT" << (bool*)&_kbd_state[LSHIFT]);
		<< "\nRSHIFT" << (bool*)&_kbd_state[RSHIFT]);
		<< "\nCAPS LOCK" << (bool*)&_kbd_state[CAPS_LOCK]);
		<< "\nSCROLL LOCK" << (bool*)&_kbd_state[SCROLL_LOCK]);
		<< "\nNUM LOCK" << (bool*)&_kbd_state[NUM_LOCK]);
	;
*/
#endif

	//------------------------------------------------------------------------
	// Help
	auto& help_hud = ui_gebi(HelpPanel);
	help_hud
		<< "------------- Actions:\n"
		<< "← → ↑ ↓       Thrust\n"
		<< "SPACE         \"Chemtrail\" sprinkle\n"
		<< "INS           Spawn object(s), +CTRL: 10, +SHIFT: 100\n"
		<< "DEL           Remove object(s), +CTRL: 10, +SHIFT: 100\n"
		<< "------------- God Mode - Metaphysics:\n"
		<< "TAB           Toggle object interactions\n"
		<< "G             Gravity mode\n"
		<< "F             Decrease friction, +SHIFT: increase\n"
        //	<< "C             chg. collision mode: pass/stick/bounce\n"
		<< "------------- God Mode - Time Control:\n"
		<< "PAUSE, H      Halt time (model time only, sorry)\n"
		<< "ENTER         Step 1 time slice forward\n"
		<< "BACKSPACE     Step 1 time slice backward\n"
		<< "R             Reverse time (not 100% exact even with fix Δt!)\n" // #376...
		<< "T             Time speedup (half resol.), +SHIFT: slowdn.\n"
		<< "X             Toggle fixed Δt for model updates\n"
		<< "------------- View:\n"
		<< "A W S D       Pan\n"
		<< "MOUSE WHEEL,\n"
		<< "NUMPAD +/-    Zoom\n"
		<< "SHIFT         Auto-scroll to follow player (or other obj.)\n"
		<< "SCROLL LOCK   Toggle locked auto-scroll\n"
		<< "LEFT MOUSE B. Set POI: obj. to follow, or just zoom center\n"
		<< "              +SHIFT: bring player to mouse and/or follow obj.\n"
		<< "HOME          Home in on (center) the player\n"
		<< "CTRL+HOME     Reset view (to Home position & default zoom)\n"
		<< "CTRL+ALT      Leave temp. trails (to trace trajectories)\n"
		<< "F11           Toggle fullscreen\n"
		<< "F12           Toggle HUD overlays\n"
		<< "------------- Admin:\n"
		<< "F1-F8         Quicksave (overwrites!), +SHIFT: qickload\n"
		<< "M             Mute/unmute music, N: sound fx\n"
		<< "SHIFT+M       Mute/unmute all audio\n"
		<< "SHIFT+P       Performance (FPS) throttling on/off\n"
		<< "\n"
		<< "ESC           Quit\n"
		<< "\n"
		<< "Command-line options: " << args.exename() << " /?"
	;
//cerr << help_hud;

	help_hud.active(cfg.get("show_help_on_start", true));
#endif
}

//----------------------------------------------------------------------------
unsigned OON::add_player(World::Body&& obj) //override
{
	// These are the player modelling differences:
	obj.add_thrusters();
	obj.superpower.gravity_immunity = appcfg.get("sim/player_antigravity", true);
	obj.superpower.free_color = true;
	obj/*.superpower*/.lifetime = World::Body::Unlimited; //!!?? Should this be a superpower instead?

	return (unsigned) //!! Blatant narrowing conv., hoping entity_count() will never overflow `unsigned`...
		add_body(std::forward<World::Body>(obj));
}

void OON::remove_player(unsigned)
{
}



//----------------------------------------------------------------------------
bool OON::poll_and_process_controls()
{
	bool action = false;

	// Thruster plumes
	if (_ctrl_update_thrusters()) {
		action = true;
		exhaust_burst(player_entity_ndx(), 5); // 5 per thruster by default, unless reconfigured
	}

	// "Chemtrail" release
	if (keystate(SPACE)) {
		action = true;
		chemtrail_burst(player_entity_ndx(), appcfg.chemtrail_burst_particles);
	}

	return action;
}

//----------------------------------------------------------------------------
void OON::up_thruster_start()    { entity(player_entity_ndx()).thrust_up.thrust_level(appcfg.player_thrust_force); }
void OON::down_thruster_start()  { entity(player_entity_ndx()).thrust_down.thrust_level(appcfg.player_thrust_force); }
void OON::left_thruster_start()  { entity(player_entity_ndx()).thrust_left.thrust_level(appcfg.player_thrust_force); }
void OON::right_thruster_start() { entity(player_entity_ndx()).thrust_right.thrust_level(appcfg.player_thrust_force); }
void OON::up_thruster_stop()     { entity(player_entity_ndx()).thrust_up.thrust_level(0); }
void OON::down_thruster_stop()   { entity(player_entity_ndx()).thrust_down.thrust_level(0); }
void OON::left_thruster_stop()   { entity(player_entity_ndx()).thrust_left.thrust_level(0); }
void OON::right_thruster_stop()  { entity(player_entity_ndx()).thrust_right.thrust_level(0); }

bool OON::_ctrl_update_thrusters()
{
	auto drv = false;
	if (keystate(UP))    { drv = true;    up_thruster_start(); } else    up_thruster_stop();
	if (keystate(DOWN))  { drv = true;  down_thruster_start(); } else  down_thruster_stop();
	if (keystate(LEFT))  { drv = true;  left_thruster_start(); } else  left_thruster_stop();
	if (keystate(RIGHT)) { drv = true; right_thruster_start(); } else right_thruster_stop();
	return drv;
}

//----------------------------------------------------------------------------
void OON::pan_reset()
{
	pan_step_x = pan_step_y = 0;

	view.offset = {0, 0};

	// Since the player entity may have moved out of view, stop focusing on it:
	//!!
	//!!?? What is the rule for Scroll Lock in this case?
	//!!The key should be turned off!...
	//!!
	focused_entity_ndx = ~0u; //!!... Whoa! :-o See updates_for_next_frame()!
}

void OON::pan(Vector2f delta) { view.pan_x(delta.x); view.pan_y(delta.y); }
void OON::pan_x(float delta)  { view.pan_x(delta); }
void OON::pan_y(float delta)  { view.pan_y(delta); }

void OON::center_to_entity(size_t id)
{
	view.offset = entity(id).p * view.scale;
	view.focus_offset = {0, 0};
}

void OON::center_to_player(unsigned player_id)
{
	assert(player_id == 1);
	focused_entity_ndx = player_entity_ndx(player_id);
	center_to_entity(focused_entity_ndx);
}


void OON::follow_entity(size_t id)
{
	auto vpos = view.world_to_view_coord(entity(id).p);
	view.offset += vpos - view.focus_offset;
}

void OON::follow_player(unsigned player_id)
{
	assert(player_id == 1);
	auto new_player_vpos = view.world_to_view_coord(player_entity(player_id).p);
	view.offset += new_player_vpos - view.focus_offset;
}

bool OON::scroll_locked()
{
	return keystate(SCROLL_LOCKED) || keystate(SHIFT)
		|| ((sfw::CheckBox*)gui.recall("Pan override"))->get();
}

void OON::zoom_reset(float factor/* = 0*/)
{
	if (factor) view.cfg.base_scale *= factor;
	zoom(view.cfg.base_scale / view.scale);
}

void OON::zoom(float factor)
{
	//!!pre_zoom_hook(factor);
	view.zoom(factor);
	resize_shapes(factor);
}
// These can't call view.zoom_in/out directly, because we need to trigger our zoom_hook!...
void OON::zoom_in (float amount) { zoom(1.f + amount); }
void OON::zoom_out(float amount) { zoom(1.f / (1.f + amount)); }


/*!!
void OON::zoom(float factor)
{
//auto viewpos = view.world_to_view_coord(player_entity().p);
//cerr << "- focus vs player diff: " << (viewpos - view.focus_offset).x << ", " << (viewpos - view.focus_offset).y << '\n';

//!!pre_zoom_hook(factor);
	// Compensate for zoom displacement when the player object is not centered
	auto v = view.world_to_view_coord(view.offset);
	pan((view.focus_offset - v) * view.zoom/factor);
//	auto viewpos = view.focus_offset + view.offset;
//	pan(viewpos - viewpos/factor);

//	auto vpos = view.world_to_view_coord(view.offset);
//	pan(view.focus_offset/factor);

	view.zoom *= factor;

	resize_shapes(factor);
}

//----------------------------------------------------------------------------
void OON_sfml::_adjust_pan_after_zoom(float factor)
{
	// If the new zoom level would put the player object out of view, reposition the view so that
	// it would keep being visible; also roughly at the same view-offset as before!

	auto visible_R = player_entity().r * view.zoom; //!! Not a terribly robust method to get that size...

	if (abs(vpos.x) > cfg.VIEWPORT_WIDTH/2  - visible_R ||
	    abs(vpos.y) > cfg.VIEWPORT_HEIGHT/2 - visible_R)
	{
cerr << "R-viewsize: " << view.zoom * plm->r
	 << " abs(vpos.x): " << abs(vpos.x) << ", "
     << " abs(vpos.u): " << abs(vpos.y) << endl;

		pan_to_player(offset);
		pan_to_entity(player_entity_ndx(), vpos * CFG_ZOOM_CHANGE_RATIO); // keep the on-screen pos!
//		zoom_out(); //!! Shouldn't be an infinite zoom loop (even if moving way too fast, I think)
	}
}
!!*/


//----------------------------------------------------------------------------
bool OON::view_control(float mousewheel_delta) //!!override
{
	auto action = false;
	action |= pan_control();
	action |= zoom_control(mousewheel_delta);
	return action;
}

//----------------------------------------------------------------------------
bool OON::pan_control() //!!override
{
	//
	// Calibrated for 30FPS; normalized below (#306)
	//
	// (In theory, it could've been normalized by adjusting the sampling rate, rather than
	// each of the values below, but I suspect that the uneven reactions could be noticeable.
	// Also, since I've set things for 30 PFS, skip-frame compensation can't help with <30 FPS!...)
	//
	//AUTO_CONST CFG_PAN_INITIAL_STEP = 5; // pixel
	AUTO_CONST CFG_PAN_AUTOCHANGE_STEP = 1; // +/- pixel

	auto fps_factor = (float)avg_frame_delay * 30.f; // Adjust relative to the 30 FPS calibration reference
		//! Note: this weird cast is required to avoid an "operator ambiguous" error!

	//auto CFG_PAN_INITIAL_STEP_fps = CFG_PAN_INITIAL_STEP * fps_factor;
	auto CFG_PAN_AUTOCHANGE_STEP_fps = CFG_PAN_AUTOCHANGE_STEP * fps_factor;
//!!??auto CFG_PAN_AUTOCHANGE_STEP_fps = CFG_PAN_AUTOCHANGE_STEP;

	auto action = false;
	if (keystate(W)) { action = true; pan_step_y -= CFG_PAN_AUTOCHANGE_STEP_fps; } // = !pan_step_y ? -CFG_PAN_INITIAL_STEP_fps : pan_step_y - CFG_PAN_AUTOCHANGE_STEP_fps; } // approach 2
	if (keystate(S)) { action = true; pan_step_y += CFG_PAN_AUTOCHANGE_STEP_fps; } // = !pan_step_y ?  CFG_PAN_INITIAL_STEP_fps : pan_step_y + CFG_PAN_AUTOCHANGE_STEP_fps; }
	if (keystate(A)) { action = true; pan_step_x -= CFG_PAN_AUTOCHANGE_STEP_fps; } // = -CFG_PAN_INITIAL_STEP_fps; } // approach 1
	if (keystate(D)) { action = true; pan_step_x += CFG_PAN_AUTOCHANGE_STEP_fps; } // =  CFG_PAN_INITIAL_STEP_fps; }
	if (!action) {
		if (pan_step_x) pan_step_x -= sz::sign(pan_step_x) * CFG_PAN_AUTOCHANGE_STEP_fps;
		if (pan_step_y) pan_step_y -= sz::sign(pan_step_y) * CFG_PAN_AUTOCHANGE_STEP_fps;
		if (abs(pan_step_x) < CFG_PAN_AUTOCHANGE_STEP_fps) pan_step_x = 0;
		if (abs(pan_step_y) < CFG_PAN_AUTOCHANGE_STEP_fps) pan_step_y = 0;
	}
	if (scroll_locked()) { // Shift, Scroll Lock etc.
		if (pan_step_x) view.focus_offset.x -= pan_step_x * fps_factor;
		if (pan_step_y) view.focus_offset.y -= pan_step_y * fps_factor;
	} else {
		if (pan_step_x) pan_x(pan_step_x * fps_factor);
		if (pan_step_y) pan_y(pan_step_y * fps_factor);
	}

	return action;
}

//----------------------------------------------------------------------------
bool OON::zoom_control(float mousewheel_delta) //!!override
{
	// Note: the mouse-wheel case needs no calibration, as it's triggered
	// directly by the mouse events, independently of frame rate!
	// See more about FPS norm. at pan_control()!
	AUTO_CONST CFG_ZOOM_CHANGE_RATIO = 0.08f; // 8%
//	AUTO_CONST CFG_ZOOM_CHANGE_MOUSEWHEEL_RATIO = 0.2f; // 20%
	static float CFG_ZOOM_CHANGE_MOUSEWHEEL_RATIO = appcfg.get("controls/zoom_speed_factor_mousewheel", 0.13f); // 13%

	AUTO_CONST CFG_ZOOM_AUTOCHANGE_STEP = 0.01f; // +/- ratio delta

	auto fps_factor = (float)avg_frame_delay * 30.f; // Adjust relative to the 30 FPS calibration reference
		//! Note: this weird cast is required to avoid an "operator ambiguous" error!

	auto CFG_ZOOM_CHANGE_RATIO_fps = CFG_ZOOM_CHANGE_RATIO * fps_factor;
	auto CFG_ZOOM_AUTOCHANGE_STEP_fps = CFG_ZOOM_AUTOCHANGE_STEP * fps_factor;

	auto action = false;
	// Mouse-wheel zoom?
	if (mousewheel_delta) { action = true; zoom_step = mousewheel_delta * CFG_ZOOM_CHANGE_MOUSEWHEEL_RATIO; }
	// Keyboard zoom?
	else if (keystate(NUMPAD_PLUS)) { action = true; zoom_step += zoom_step == 0 ?
	                                                        CFG_ZOOM_CHANGE_RATIO_fps : CFG_ZOOM_AUTOCHANGE_STEP_fps; }
	else if (keystate(NUMPAD_MINUS)){ action = true; zoom_step -= zoom_step == 0 ?
	                                                        CFG_ZOOM_CHANGE_RATIO_fps : CFG_ZOOM_AUTOCHANGE_STEP_fps; }
	if (!action) {
		if (zoom_step) zoom_step -= sz::sign(zoom_step) * CFG_ZOOM_AUTOCHANGE_STEP_fps;
		if (abs(zoom_step) < CFG_ZOOM_AUTOCHANGE_STEP_fps) zoom_step = 0;
	}
	if (zoom_step) {
		if (zoom_step > 0) zoom_in(  zoom_step * fps_factor);
		else               zoom_out(-zoom_step * fps_factor);
	}

	return action;
}


//----------------------------------------------------------------------------
//!!Move chores like this to the Szim API!
void OON::toggle_muting() { backend.audio.toggle_audio(); }
void OON::toggle_music() { backend.audio.toggle_music(); }
void OON::toggle_sound_fx() { backend.audio.toggle_sounds(); }


//----------------------------------------------------------------------------
void OON::interaction_hook(Model::World* w, Model::World::Event event, Model::World::Body* obj1, Model::World::Body* obj2, ...)
{w, event, obj1, obj2;
//	if (!obj1->is_player())
//		obj1->color += 0x3363c3;
}

//----------------------------------------------------------------------------
bool OON::touch_hook(World* w, World::Body* obj1, World::Body* obj2)
{w;
	if (obj1->is_player() || obj2->is_player()) {
		backend.audio.play_sound(clack_sound);
	}

	obj1->T += 100;
	obj2->T += 100;

	obj1->recalc();
	obj2->recalc();

	return false; //!!Not yet used!
}



//----------------------------------------------------------------------------
void OON::add_random_bodies_near(size_t base_ndx, size_t n)
{
	while (n--) add_random_body_near(base_ndx);
}

//----------------------------------------------------------------------------
void OON::remove_random_bodies(size_t n/* = -1*/)
{
	if (n == (unsigned)-1) n = entity_count();
	while (n--) remove_random_body();
}


//----------------------------------------------------------------------------
size_t OON::add_body(World::Body&& obj) //virtual
// Add new entity (moved) from a template (temporary) obj.
{
	return world().add_body(std::forward<decltype(obj)>(obj));
}

//----------------------------------------------------------------------------
size_t OON::add_random_body_near(size_t base_ndx)
//!! This is still a version of (mass-ignoring) spawn()!...
//!! Callers may not know, but this depends on the properties of the player body!
//!! See also spawn() (that calls this), which is at least is explicit about it!
{
	//!! These should be either static, or actually depend on dynamic state...
	const auto& cw = const_world();
//	auto constexpr r_min = cw.CFG_GLOBE_RADIUS / 9;
//	auto constexpr r_max = cw.CFG_GLOBE_RADIUS * 3;
	auto constexpr p_range = cw.CFG_GLOBE_RADIUS * 30;
	auto constexpr v_range = cw.CFG_GLOBE_RADIUS * 10; //!!Stop depending on GLOBE_RADIUS so directly/cryptically!

	const auto& base = const_entity(base_ndx);
	auto M_min = base.mass / 9;
	auto M_max = base.mass * 3;

//cerr << "Adding new object #" << cw.bodies.size() + 1 << "...\n";
	return add_body({
		.p = { (rand() * p_range) / RAND_MAX - p_range/2 + base.p.x,
		       (rand() * p_range) / RAND_MAX - p_range/2 + base.p.y },
		.v = { (rand() * v_range) / RAND_MAX - v_range/2 + base.v.x * 0.05f,
		       (rand() * v_range) / RAND_MAX - v_range/2 + base.v.y * 0.05f },
		.color = 0xffffff & ((uint32_t) rand() * rand()),
		.mass = M_min + (M_max - M_min) * float(rand())/RAND_MAX,
	});
}

//----------------------------------------------------------------------------
void OON::remove_body(size_t ndx) //virtual
{
	world().remove_body(ndx);

	//-------------------------
	// Adjust references...
	//--------------

	// Focus obj.:
	if (focused_entity_ndx != ~0u) {
		if (focused_entity_ndx > ndx) {
//cerr << "- NOTE: Index of the followed object has changed due to object removal.\n";
			--focused_entity_ndx;
		} else if (focused_entity_ndx == ndx) {
cerr << "- WARNING: The followed object has ceased to exist...\n";
			focused_entity_ndx = ~0u; //!! Don't just fall back to the player!
		}                                 //!! That'd be too subtle/unexpected/unwanted.
	}

	assert(focused_entity_ndx == ~0u || focused_entity_ndx < entity_count());
}

//----------------------------------------------------------------------------
void OON::remove_random_body()
{
	auto entities = entity_count();
	if (entities <= 1) { // Leave the player "superglobe", so not just checking for empty()!
//cerr << "No more \"free\" items to delete.\n";
		return;
	}

	auto ndx = 1/*leave the globe!*/ + (size_t) (rand() * (float(entities-1)) / (RAND_MAX + 1));
//cerr << "Deleting object #" << ndx << "...\n";
	assert(ndx < entities); // Note: entity indexes are 0-based
	assert(ndx > 0);        // Note: 0 is the player globe
	remove_body(ndx);
}

//----------------------------------------------------------------------------
void OON::spawn(size_t parent_ndx, size_t n)
//!! Should not ignore mass!...
//!!??Should gradually become a method of the object itself?
{
if (parent_ndx != player_entity_ndx()) cerr << "- INTERANL: Non-player object #"<<parent_ndx<<" is spawning...\n";

	const auto& parent = const_entity(parent_ndx); // #41: Support inheritance
	for (size_t i = 0; i < n; ++i) {
		auto ndx = add_random_body_near(player_entity_ndx());
		auto& newborn = entity(ndx);
		newborn.lifetime = World::Body::Unlimited;
		newborn.T = parent.T; // #155: Inherit temperature
		newborn.v = parent.v; // 1e5e8be3: Inherit speed
	}
}


//----------------------------------------------------------------------------
//!! Move to SimApp!
void OON::_emit_particles(const EmitterConfig& ecfg, size_t emitter_ndx, size_t n)
{
	auto& emitter = entity(emitter_ndx); // Not const: will deplete!

//if (!ecfg.create_mass) cerr <<"DBG> emitter.mass BEFORE burst: "<< emitter.mass <<'\n';

	float p_range = emitter.r * ecfg.velocity_divergence;
	float v_range = Model::World::CFG_GLOBE_RADIUS * ecfg.velocity_divergence; //!! ...by magic, right? :-/

	float emitter_old_r = emitter.r;

	for (int i = 0; i++ < n;) {
		auto particle_mass = ecfg.particle_mass_min + (ecfg.particle_mass_max - ecfg.particle_mass_min) * float(rand())/RAND_MAX;

		if (!ecfg.create_mass && emitter.mass < particle_mass) {
//cerr << "- Not enough mass to emit particle!\n";
			continue;
		}
//cerr <<"DBG> density: "<< ecfg.particle_density <<'\n';
//cerr <<"DBG>   ==?  : "<< Model::Physics::DENSITY_ROCK * 0.0000000123f <<'\n';

		auto pndx = add_body({
			.lifetime = ecfg.particle_lifetime,
			.density = ecfg.particle_density,
			//!!...Jesus, those "hamfixted" pseudo Δt "factors" here! :-o :)
			.p = { (rand() * p_range) / RAND_MAX - p_range/2 + emitter.p.x - emitter.v.x * ecfg.offset_factor,
			       (rand() * p_range) / RAND_MAX - p_range/2 + emitter.p.y - emitter.v.y * ecfg.offset_factor },
			.v = { (rand() * v_range) / RAND_MAX - v_range/2 + emitter.v.x * ecfg.v_factor + ecfg.eject_velocity.x,
			       (rand() * v_range) / RAND_MAX - v_range/2 + emitter.v.y * ecfg.v_factor + ecfg.eject_velocity.y },
			.color = ecfg.color,
			.mass = particle_mass,
		});
//cerr <<"DBG> particle.r: "<< entity(pndx).r <<'\n';

//cerr <<"DBG> emitter v:  "<< emitter.v.x <<", "<< emitter.v.y <<'\n';
//cerr <<"     - eject Δv: "<< ecfg.eject_velocity.x <<", "<< ecfg.eject_velocity.y <<'\n';
//cerr <<"     - part. v:  "<< entity(pndx).v.x <<", "<< entity(entity_count()-1).v.y <<'\n';

		if (!ecfg.create_mass) {
			emitter.mass -= particle_mass;
//cerr <<"DBG> Decreasing emitter.mass by: "<< particle_mass <<'\n';
		}
	}

	if (!ecfg.create_mass) {
		assert(emitter.mass >= 0);
//cerr <<"DBG> emitter.r before recalc: "<< emitter.r <<'\n';
		emitter.recalc();
//cerr <<"DBG> emitter.r after recalc: "<< emitter.r <<'\n';
		resize_shape(emitter_ndx, emitter.r/emitter_old_r);
//cerr <<"DBG> emitter.mass AFTER burst: "<< emitter.mass <<'\n';
	}
}


//----------------------------------------------------------------------------
//!! An exhaust jet should be created for each thruster!
void OON::exhaust_burst(size_t base_ndx/* = 0*/, /*Math::Vector2f thrust_vector,*/ size_t n/* = ...*/)
{
	static size_t   add_particles = appcfg.get("sim/exhaust_particles_add", 0);
	static float    exhaust_density = Model::Physics::DENSITY_ROCK * appcfg.get("sim/exhaust_density_ratio", 0.001f);
	static uint32_t exhaust_color = appcfg.get("sim/exhaust_color", 0xaaaaaa);
	static float r_min = Model::World::CFG_GLOBE_RADIUS * appcfg.get("sim/exhaust_particle_min_size_ratio", 0.02f);
	static float r_max = Model::World::CFG_GLOBE_RADIUS * appcfg.get("sim/exhaust_particle_max_size_ratio", 0.1f);

//cerr <<"DBG> cfg.exhaust_density_ratio: "<< appcfg.get("sim/exhaust_density_ratio", 0.001f) <<'\n';
//cerr <<"DBG> -> exhaust_density: "<< exhaust_density <<'\n';

	static EmitterConfig thrust_exhaust_emitter =
	{
		.eject_velocity = {0, 0},
		.v_factor = appcfg.exhaust_v_factor, //!! Should just be calculated instead!,
		.offset_factor = 0, //!!appcfg.exhaust_offset_factor, //!! Should just be calculated instead!
		.particle_lifetime = appcfg.exhaust_lifetime,
		.create_mass = appcfg.get("sim/exhaust_creates_mass", true),
		.particle_density = exhaust_density,
		.position_divergence = 5.f, // Relative to emitter radius
		.velocity_divergence = 1.f, //!! Just an exp. "randomness factor" for now!...
		.particle_mass_min = Model::Physics::mass_from_radius_and_density(r_min, Model::Physics::DENSITY_OF_EARTH), //!! WAS: exhaust_density
		.particle_mass_max = Model::Physics::mass_from_radius_and_density(r_max, Model::Physics::DENSITY_OF_EARTH), //!! WAS: exhaust_density
		.color = exhaust_color,
	};

	auto& base = entity(base_ndx); // Not const: will deplete!

// This accidentally creates a lovely rainbow color pattern in the plumes!... :-o
	constexpr const float color_spread = 0x111111f;
	thrust_exhaust_emitter.color = (uint32_t) exhaust_color + color_spread - 2 * color_spread * float(rand())/RAND_MAX;

	if (base.thrust_up.thrust_level()) {
		thrust_exhaust_emitter.eject_velocity = {0, 4e9f};
		_emit_particles(thrust_exhaust_emitter, base_ndx, add_particles ? add_particles : n);
	}
	if (base.thrust_down.thrust_level()) {
		thrust_exhaust_emitter.eject_velocity = {0, -4e9f};
		_emit_particles(thrust_exhaust_emitter, base_ndx, add_particles ? add_particles : n);
	}
	if (base.thrust_left.thrust_level()) {
		thrust_exhaust_emitter.eject_velocity = {4e9f, 0};
		_emit_particles(thrust_exhaust_emitter, base_ndx, add_particles ? add_particles : n);
	}
	if (base.thrust_right.thrust_level()) {
		thrust_exhaust_emitter.eject_velocity = {-4e9f, 0};
		_emit_particles(thrust_exhaust_emitter, base_ndx, add_particles ? add_particles : n);
	}
}

//----------------------------------------------------------------------------
void OON::chemtrail_burst(size_t emitter_ndx/* = 0*/, size_t n/* = ...*/)
{
	static float chemtrail_v_factor      = appcfg.get("sim/chemtrail_v_factor", 0.1f);
	static float chemtrail_offset_factor = appcfg.get("sim/chemtrail_offset_factor", 0.2f);
	static float chemtrail_lifetime      = appcfg.get("sim/chemtrail_lifetime", Model::World::Body::Unlimited);
	static bool  chemtrail_creates_mass  = appcfg.get("sim/chemtrail_creates_mass", true);
	static float chemtrail_density       = Model::Physics::DENSITY_ROCK * appcfg.get("sim/chemtrail_density_ratio", 0.001f);
	static float chemtrail_divergence    = appcfg.get("sim/chemtrail_divergence", 1.f);
	static float r_min = Model::World::CFG_GLOBE_RADIUS * appcfg.get("sim/chemtrail_particle_min_size_ratio", 0.02f);
	static float r_max = Model::World::CFG_GLOBE_RADIUS * appcfg.get("sim/chemtrail_particle_max_size_ratio", 0.1f);
	static float M_min = Model::Physics::mass_from_radius_and_density(r_min, chemtrail_density);
	static float M_max = Model::Physics::mass_from_radius_and_density(r_max, chemtrail_density);

	auto& emitter = entity(emitter_ndx); // Not const: will deplete!
	float p_range = emitter.r * 5;
	float v_range = Model::World::CFG_GLOBE_RADIUS * chemtrail_divergence; //!! ...by magic, right? :-/

	float emitter_old_r = emitter.r;

	for (int i = 0; i++ < n;) {
		auto particle_mass = M_min + (M_max - M_min) * float(rand())/RAND_MAX;
		if (!chemtrail_creates_mass && emitter.mass < particle_mass) {
//cerr << "- Not enough mass to emit particle...\n";
			continue;
		}

		add_body({
			.lifetime = chemtrail_lifetime,
			.density = chemtrail_density,
			//!!...Jesus, those "hamfixted" pseudo Δts here! :-o :)
			.p = { (rand() * p_range) / RAND_MAX - p_range/2 + emitter.p.x - emitter.v.x * chemtrail_offset_factor,
			       (rand() * p_range) / RAND_MAX - p_range/2 + emitter.p.y - emitter.v.y * chemtrail_offset_factor },
			.v = { (rand() * v_range) / RAND_MAX - v_range/2 + emitter.v.x * chemtrail_v_factor,
			       (rand() * v_range) / RAND_MAX - v_range/2 + emitter.v.y * chemtrail_v_factor },
			.color = (uint32_t) (float)0xffffff * rand(),
			.mass = particle_mass,
		});

		if (!chemtrail_creates_mass) emitter.mass -= particle_mass;
//cerr <<"emitter.mass -= emitter_mass_loss: "<< emitter.mass <<" -= "<< particle_mass <<'\n';
	}

	assert(emitter.mass >= 0);
	emitter.recalc();
	resize_shape(emitter_ndx, emitter.r / emitter_old_r);
}
