#define SAVE_COMPRESSED

#ifdef USE_BACKEND_SFML
#   include "Backend/SFML.hpp"
#else
#   error Only the SFML backend is supported currently. (Define USE_BACKEND_SFML in your build procedure!)
#endif

#include "SimApp.hpp"

#include <string>
	using std::string, std::to_string;
	using std::stoul, std::stof;
	using namespace std::string_literals;
#include <string_view>
	using std::string_view;
//!!#include "sz/Args.hh"
#include "extern/Args.hpp" //!! move to sz or absorb directly by Szim
#include "sz/fs.hh"
	using sz::dirname;
#include <fstream>
	using std::ofstream, std::ifstream, std::ios;
#ifdef SAVE_COMPRESSED
#   include "extern/zstd/zstd.h"
#   include <sstream>
    using std::ostringstream, std::stringstream;
#   include <string_view>
    using std::string_view;
#   include <memory>
    using std::make_unique_for_overwrite;
#   include <cstddef>
    using std::byte; //!! It's fucked up in C++ tho: a byte[] buffer can't be used for file IO... Excellent.
#endif
#include <format>
	using std::format;
#include <iostream>
	using std::cerr, std::endl;
//#include <stdexcept>
//	using std::runtime_error;


using namespace Szim;
//============================================================================
//----------------------------------------------------------------------------
SimApp::SimApp(int argc, char** argv) :
	// Bootstrap the backend... (Could be done via the config in the future!)
#ifdef USE_BACKEND_SFML
	backend{Backend_SFML::create()}
#else
//!!	backend{Backend_...::create()}
#   error Only the SFML backend is supported currently! (Define USE_BACKEND_SFML)
#endif	


{
	//!!
	//!! Cfg...
	//!!

	/*!! Normalize the config. process:
	[ ] pass it all in one go to the game
	[ ] preferably just the config file name!
		[ ] SimApp should have the cfg loader already;
			[ ] calling down to the real app impl. (OON)
			    for any non-generic (i.e.: unknown) option (-> inih, too)
	!!*/

	Args args(argc, argv, {
	// Long options with 1 param. don't need to be defined:
	//	{"moons", 1}, // number of moons to start with
	// Short ones do, unfortunately (they're predicates by default, and don't have '=' to override):
		{"C", 1},
	});

	// 0. Check the cmdline to pick up a custom-config arg...
	string cfgfile = args("cfg");
	if (cfgfile.empty()) cfgfile = args("C"); // OK if still empty, we'll fall back to the default.
	else if (!args("C").empty())
		cerr << "- WARNING: Both -C and --cfg have been specified; ignoring \"-C " << args("C") << "\"!\n";
	if (cfgfile.empty()) cfgfile = DEFAULT_CFG_FILE;

	// Relative paths will be rooted to the dir of 'cfgfile' by default,
	// i.e. unless it's specifically set in the config
	//!!Move to unilang:
	cfg.select(cfgfile);
	//!!auto basename = fs::path(cfgfile).filename().string();

	// 1. Preset hardcoded baseline defaults...
	//!!?? Or just set them all in .value_or() (see below)?

	// 2. Load cfg. to override those...
	//!! This assignment is silly... cfg. should know how to set its own props! :)
	cfg.asset_dir       = cfg.get("fs-layout/asset_dir", sz::getcwd() + "/asset/"); //!! Trailing / still required!
	cfg.iteration_limit = cfg.get("sim/loopcap", -1);
	cfg.fixed_dt        = cfg.get("sim/timing/fixed_dt", 0.f);

	// 3. Process cmdline args to override again...
//!! See also main.cpp, currently! And if main goes into Szim (making it essentially a framework, not a lib, BTW...),
//!! then it's TBD where to actually take care of the cmdline. -- NOTE: There's likely gonna be an app
//!! composition layout, where the client retains its own main()!
	if   (args["loopcap"]) {
		try { cfg.iteration_limit = stoul(args("loopcap")); } catch(...) {
			cerr << "- WRNING: --loopcap ignored! "<<args("loopcap")<<" must be a valid positive integer.\n";
		}
	} if (args["fixed_dt"]) {
		try { cfg.fixed_dt = stof(args("fixed_dt")); } catch(...) {
			cerr << "- WRNING: --fixed_dt ignored! "<<args("fixed_dt")<<" must be a valid floating-pont number.\n";
		}
	}

	//!! 4. Fixup...
	cfg.fixed_dt_enabled = cfg.fixed_dt != 0.f;

	//!! 5. Apply the config...
	iterations.max(cfg.iteration_limit);

	if (cfg.fixed_dt_enabled) {
		time.dt_last = cfg.fixed_dt; // Otherwise no one might ever init this...
	}

cerr <<	"DBG> current dir: " << sz::getcwd() << '\n';
cerr <<	"DBG> cfg.iteration_limit: " << cfg.asset_dir << '\n';
cerr <<	"DBG> cfg.asset_dir: " << cfg.asset_dir << '\n';
cerr <<	"DBG> cfg.iteration_limit: " << cfg.iteration_limit << '\n';
cerr <<	"DBG> cfg.fixed_dt_enabled: " << cfg.fixed_dt_enabled << '\n';
cerr <<	"DBG> cfg.fixed_dt: " << cfg.fixed_dt << '\n';
}


//----------------------------------------------------------------------------
void SimApp::pause(bool newstate)
{
	time.paused = newstate;
	pause_hook(newstate);
}

//----------------------------------------------------------------------------
//!! Updating is not yet (inherently) thread-safe!
//!! These sould be atomic/blocking/mutex-protected/...!
//!! (Delegating that to all the various World::mutators, and imposing deps.
//!! on threading (etc.) mechanics there feels like a terrible idea!)
//!! While update() and load() are called currently from a locked section of the
//!! event loop anyway, it's a crime to rely on just that!
      Model::World& SimApp::world()       { return _world; }
const Model::World& SimApp::world() const { return _world; }
const Model::World& SimApp::const_world() { return _world; }
void SimApp::set_world(Model::World const& w) { _world = w; }

//----------------------------------------------------------------------------
bool SimApp::save_snapshot(unsigned slot_id) // starting from 1, not 0!
{
	//!!A kinda alluring abstraction would be SimApp not really having its own state
	//!!(worth saving, beside the model world), leaving all that to descendants...
	//!!But I suspect it's unfounded; at least I can't see the higher principle it
	//!!could be derived from... What I do see, OTOH, is the hassle in the App class
	//!!chain to actually deal with saving/loading all the meta/supplementary state...
/*
	using namespace MEMDB;
	assert(slot_id > 0 && slot_id <= MAX_WORLD_SNAPSHOTS); //!!should become a runtime "filename OK" check

	auto slot = slot_id - 1; //! internally they are 0-based tho...
	decltype(saved_slots) slot_bit = 1 << slot;
	if (saved_slots & slot_bit) {
		cerr << "- WARNING: Overwriting previously saved state at slot #" << slot_id << "!...\n";
	}

	world_snapshots[slot] = world(); // :)
	saved_slots |= slot_bit;
*/
	Model::World snapshot = world();

	string fname = snapshot_filename(slot_id);
	string OVERALL_FAIL = "ERROR: Couldn't save snapshot to file \""; OVERALL_FAIL += fname + "\"\n";

	//!! Note: perror("") may just print "No error" even if the stream in failure mode! :-/

#ifdef SAVE_COMPRESSED
	ofstream file(fname, ios::binary);
	if (!file || file.bad()) {
		cerr << OVERALL_FAIL;
		perror("");
		return false;
	}

	ostringstream out(ios::binary); //!!??Why did it fail with plain stringstream?!?!?!
	//!!Redesign this proc. so that such customizations can be handled by a descendant's save_...() override:
	//!!out << "BUILD_ID = " << ::BUILD_ID << endl;
	if (!snapshot.save(out)) {
		cerr << OVERALL_FAIL;
		perror("");
		return false;
	}

	//file << out.view();

	// Compress (the whole blob in one go... <- !!IMPROVE)
	auto data_size = out.tellp(); // or out.view().size()
	auto cbuf_size = ZSTD_compressBound(data_size);
	auto cbuf = make_unique_for_overwrite<char[]>(cbuf_size);
	auto cfile_size = ZSTD_compress(cbuf.get(), cbuf_size, out.view().data(), data_size, 9);

	if (!file.write(cbuf.get(), cfile_size) || file.bad()) {
		cerr << OVERALL_FAIL;
		perror("");
		return false;
	}

#else // !SAVE_COMPRESSED
	ofstream out(fname, ios::binary);
	if (!out || out.bad()) {
		cerr << OVERALL_FAIL;
		perror("");
		return false;
	}
	//!!Redesign this proc. so that such customizations can be handled by a descendant's save_...() override:
	//!!out << "BUILD_ID = " << ::BUILD_ID << endl;
	if (!snapshot.save(out)) {
		cerr << OVERALL_FAIL;
		return false;
	}
#endif // SAVE_COMPRESSED

	assert(out && !out.bad());

	cerr << "World state saved to slot #" << slot_id << ".\n";
	return true;
}

//----------------------------------------------------------------------------
bool SimApp::load_snapshot(unsigned slot_id) // starting from 1, not 0!
{
/*
	using namespace MEMDB;
	assert(slot_id > 0 && slot_id <= MAX_WORLD_SNAPSHOTS); //!!should become a runtime "filename OK" check

	auto slot = slot_id - 1; //! internally they are 0-based tho...
	decltype(saved_slots) slot_bit = 1 << slot;
	if (! (saved_slots & slot_bit)) {
		cerr << "- WARNING: No saved state at slot #" << slot_id << " yet!\n";
		return false;
	}
	set_world(world_snapshots[slot]);
	cerr << "World state loaded from slot " << slot_id << ".\n";
*/
	Model::World snapshot;

	string fname = snapshot_filename(slot_id);
	string OVERALL_FAIL = "ERROR: Couldn't load snapshot from file \""; OVERALL_FAIL += fname + "\"\n";

#ifdef SAVE_COMPRESSED
	ifstream file(fname, ios::binary);
	if (!file || file.bad()) {
		cerr << OVERALL_FAIL;
		perror("");
		return false;
	}

	stringstream in(ios::in|ios::out|ios::binary);
	in << file.rdbuf();
	if (!in || in.bad()) {
		cerr << OVERALL_FAIL;
		perror("");
		return false;
	}

	// Decompress (the whole blob in one go... <- !!IMPROVE)
	auto cbuf_size = in.view().size();
	auto cbuf = in.view().data();
	auto data_size = ZSTD_getFrameContentSize(cbuf, cbuf_size);
	auto data = make_unique_for_overwrite<char[]>(data_size);
	auto dsize = ZSTD_decompress(data.get(), data_size, cbuf, cbuf_size);
	assert(dsize == data_size);

	//!!Only in c++26: in.str(string_view((char*)data.get(), data_size)); // or: reset, then: in.write(data.get(), data_size);
	in.seekp(0, in.beg); // out
	in.write(data.get(), data_size);
	in.seekg(0, in.beg); // in

	if (!in || in.bad()) {
		cerr << OVERALL_FAIL;
		perror("");
		return false;
	}

	if (!Model::World::load(in, &snapshot)) {
		cerr << OVERALL_FAIL;
		return false;
	}

	if (!in || in.bad()) {
		cerr << OVERALL_FAIL;
		perror("");
		return false;
	}

#else // !SAVE_COMPRESSED
	ifstream in(fname, ios::binary);
	if (!in || in.bad()) {
		cerr << OVERALL_FAIL;
		perror("");
		return false;
	}

	//!!Redesign this proc. so that such customizations can be handled by a descendant's save_...() override:
	//!!in >> BUILD_ID...

	if (!Model::World::load(in, &snapshot)) {
		cerr << OVERALL_FAIL;
		return false;
	}
#endif // SAVE_COMPRESSED

	assert(in && !in.bad());

	set_world(snapshot);

	cerr << "World state loaded from slot #" << slot_id << ".\n";
	return true;
}

//----------------------------------------------------------------------------
bool SimApp::collide_hook(Model::World* w, Model::World::Body* obj1, Model::World::Body* obj2, float distance)
{w, obj1, obj2, distance;
	//!!?? body->interact(other_body) and then also, per Newton, other_body->interact(body)?!
	//!!...body->p -= ds...;
	return false;
}

bool SimApp::touch_hook(Model::World* w, Model::World::Body* obj1, Model::World::Body* obj2)
{w, obj1, obj2;
	return false;
}

// High-level, abstract (not as in "generic", but "app-level") hook for n-body interactions:
//!!The model should also pass the physical property/condition ("event type") that made it think these may interact!
//!!A self-documenting alternative would be calling a matching function for each known such event,
//!!but that might be prohibitively expensive in that tight loop, especiall if most callbacks would
//!!just do nothing.
//!!NOTE: This will anyway change to the objects themselves being notified (not the game "superclass")!
void SimApp::interaction_hook(Model::World* w, Model::World::Event event, Model::World::Body* obj1, Model::World::Body* obj2, ...)
{w, event, obj1, obj2;
	//!!?? body->interact(other_body) and then also, per Newton, other_body->interact(body)?!
}
