﻿#include <SFML/Graphics.hpp>
//! For mixing native OpenGL context with SFML -> https://www.sfml-dev.org/tutorials/2.5/window-opengl.php:
//#include <SFML/OpenGL.hpp>

#include <cmath>
#include <memory> // shared_ptr
#include <vector>
#include <iostream> // cerr
using namespace std;


//----------------------------------------------------------------------------
class Engine_SFML;
class Render_SFML // "View"
{
public:
	static const auto VIEW_WIDTH  = 800;
	static const auto VIEW_HEIGHT = 600;

	static const auto ALPHA_ACTIVE = 255;
	static const auto ALPHA_INACTIVE = 127;

// Input params
	uint8_t p_alpha;

// Internals:
//!!...not quite yet -- just allow access:
public:
	vector< shared_ptr<sf::Drawable> >      shapes_to_draw; // ::Shape would be way too restritive here
	//!!misnomer alert below! should be sg. like "body_images" (as they are not just any Transformables!!! -- these are linked to physical bodies!):
	vector< shared_ptr<sf::Transformable> > shapes_to_change; // ::Shape would be way too restritive here

// Ops
	void render_next_frame(const Engine_SFML& game);

// Housekeeping
	Render_SFML()
	      :	p_alpha(ALPHA_ACTIVE)
	{
	}
};


//----------------------------------------------------------------------------
class World_SFML // "Model"
{
public:
struct Physics
{
//	...
};

public:
struct Body
//! Inner class of World, as it depends on the physics (e.g. constants).
{
	float r{0};
	sf::Vector2f p{0, 0};
	sf::Vector2f v{0, 0};

	float density{World_SFML::DENSITY_ROCK / 2};
	float color{0};

	// computed:
	float mass{0};

	//!!not yet possible with designater init:
	//!!Body() : mass(powf(r, 3) * density) {}
	//!!So... (see add_body()):
		void precalc() { mass = powf(r, 3) * density; }
};

// Physics constants -- !!MOVE TO Physics! --:
	//! `const` can't do non-integral statics! :-/
	static constexpr float G = 6.673e-11; //!! No point keeping this real and all the others stretched, 
	               //!! const unless a real orbital simulation is the goal (which isn't)!...
	static constexpr float DENSITY_ROCK = 2000; // kg/m3
	static constexpr float FRICTION = 0.3;

protected:
// Internal state:
	float dt; // inter-frame increment of the world model time
	sf::Vector2f v = {0, 0};

public: // Just allow access for now...:
	vector< shared_ptr<Body> > bodies;

public:
// Input params

// Ops
	auto add_body(Body&& obj)
	{
		obj.precalc();
		bodies.push_back(make_shared<Body>(obj));
	}

	void recalc_for_next_frame(const Engine_SFML& game); // ++world

// Housekeeping
	World_SFML()
	{
	}

	sf::Clock clock;
};


//----------------------------------------------------------------------------
class Engine_SFML // "Controller"
{
friend class Render_SFML;

// Config
public:
	//! See also: World physics! The specific values here depend on the laws there,
	//! so replacing the physics may very well invalidate these! :-o
	//! The depencendies should be formalized e.g. via using virtual units
	//! provided by the physics there!
	static constexpr float CFG_GLOBE_RADIUS = 50000000; // m
	static constexpr float CFG_V_NUDGE = 12000000; // m/s

	static constexpr float CFG_DEFAULT_SCALE = 0.000001; //! This one also depends very much on the physics!

	static constexpr float CFG_PAN_STEP = 10; // "SFML defaul pixel" :) (Not quite sure yet how it does coordinates...)

// Internals... -- not quite yet; just allow access for now:
public:
	World_SFML world;
	Render_SFML renderer;

protected:
	float _SCALE = CFG_DEFAULT_SCALE;
	float _OFFSET_X, _OFFSET_Y;

	sf::RenderWindow& window;

public:
// Ops
	auto move_up()    { world.bodies[0]->v.y -= CFG_V_NUDGE; }
	auto move_down()  { world.bodies[0]->v.y += CFG_V_NUDGE; }
	auto move_left()  { world.bodies[0]->v.x -= CFG_V_NUDGE; }
	auto move_right() { world.bodies[0]->v.x += CFG_V_NUDGE; }

	auto pan_up()     { _OFFSET_Y -= CFG_PAN_STEP; }
	auto pan_down()   { _OFFSET_Y += CFG_PAN_STEP; }
	auto pan_left()   { _OFFSET_X -= CFG_PAN_STEP; }
	auto pan_right()  { _OFFSET_X += CFG_PAN_STEP; }
	auto pan_reset()  { _OFFSET_X = _OFFSET_Y = 0; }
	auto pan_center_body(auto body_id) {
		const auto& body = world.bodies[body_id];
		_OFFSET_X = - body->p.x * _SCALE;
		_OFFSET_Y = - body->p.y * _SCALE;
	}
	auto _pan_adjust_after_zoom() {
		//!!??
	}

	auto zoom_in()  { auto factor = 1.25; _SCALE *= factor;
		_resize_objects(factor);
		_pan_adjust_after_zoom();
	}
	auto zoom_out () { auto factor = 0.80; _SCALE *= factor;
		_resize_objects(factor);
		_pan_adjust_after_zoom();
	}

	void _resize_objects(float factor)
	{
		_transform_objects([factor](sf::Transformable& shape) {
				shape.setScale(shape.getScale() * factor);
		});
	}

	void _transform_objects(const auto& op) // c++20 auto lambda ref (but why the `const` required by MSVC?); https://stackoverflow.com/a/67718838/1479945
	// op = [](Transformable& shape);
	{
		//! Only generic functions here -- Transformable is abstract!
		for (auto& shape : renderer.shapes_to_change) {
			auto& trshape = dynamic_cast<sf::Transformable&>(*shape);
			op(trshape);
		}
	}

	auto updates_for_next_frame()
	// Should be idempotent -- which doesn't matter normally, but testing could reveal bugs if it isn't!
	{
		world.recalc_for_next_frame(*this);
		renderer.render_next_frame(*this);
	}

	auto draw()
	{
	        window.clear();
		for (const auto& entity : renderer.shapes_to_draw) {
		        window.draw(*entity);
		}
	        window.display();
	}

	auto add_body(World_SFML::Body&& obj)
	{
		world.add_body(std::forward<decltype(obj)>(obj));

		// For rendering...

		//! Not all the Drawables are also Transformables! (E.g. vertex arrays.)
		// (But our little ugly circles are, for now.)
		auto shape = make_shared<sf::CircleShape>(obj.r * _SCALE);
		renderer.shapes_to_draw.push_back(shape);
		renderer.shapes_to_change.push_back(shape); // "to transform"
	}

	auto _setup()
	{
		//!! Well, we're gonna know these objects by name (index) for now, see recalc():
		// globe:
		add_body({ .r = CFG_GLOBE_RADIUS,    .p = {0,0}, .v = {0,0}, .density = world.DENSITY_ROCK, .color = 10});
		// moons:
		add_body({ .r = CFG_GLOBE_RADIUS/10, .p = {CFG_GLOBE_RADIUS * 2, 0}, .v = {0, -CFG_GLOBE_RADIUS * 2}, .color = 100});
		add_body({ .r = CFG_GLOBE_RADIUS/7,  .p = {-CFG_GLOBE_RADIUS * 1.6f, +CFG_GLOBE_RADIUS * 1.2f}, .v = {-CFG_GLOBE_RADIUS*1.8, -CFG_GLOBE_RADIUS*1.5}, .color = 160});
	}

// Housekeeping
	Engine_SFML(sf::RenderWindow& _window)
	      : window(_window)
	{
		_setup();
	}
};


//============================================================================
int main()
{
	sf::RenderWindow window(sf::VideoMode({Render_SFML::VIEW_WIDTH, Render_SFML::VIEW_HEIGHT}),
		"SFML (OpenGL) Test Drive"); //!, sf::Style::Fullscreen);
//!!??	For SFML + OpenGL mixed mode (https://www.sfml-dev.org/tutorials/2.5/window-opengl.php):
//!!??	glEnable(GL_TEXTURE_2D); //!!?? why is this needed, if SFML already draws into an OpenGL canvas?!
//!!??	--> https://en.sfml-dev.org/forums/index.php?topic=11967.0

	Engine_SFML engine(window);

	while (window.isOpen()) {
		for (sf::Event event; window.pollEvent(event);) {
			switch (event.type)
			{
			case sf::Event::KeyPressed:
				switch (event.key.code) {
				case sf::Keyboard::Escape:
					window.close(); break;

				case sf::Keyboard::Up:
					if (event.key.shift) engine.pan_up();
					else                 engine.move_up();
					break;
				case sf::Keyboard::Down:
					if (event.key.shift) engine.pan_down();
					else                 engine.move_down();
					break;
				case sf::Keyboard::Left:
					if (event.key.shift) engine.pan_left();
					else                 engine.move_left();
					break;
				case sf::Keyboard::Right:
					if (event.key.shift) engine.pan_right();
					else                 engine.move_right();
					break;
				}
				break;

			case sf::Event::MouseWheelScrolled:
	//				cerr << event.mouseWheelScroll.delta << endl;
				engine.renderer.p_alpha += (uint8_t)event.mouseWheelScroll.delta * 4; // seems to always be 1 or -1
				break;

			case sf::Event::TextEntered:
				if (event.text.unicode > 128) break; // non-ASCII!
				switch (static_cast<char>(event.text.unicode)) {
				case '+': engine.zoom_in(); break;
				case '-': engine.zoom_out(); break;
				case 'o': engine.pan_reset(); break;
				case 'h': engine.pan_center_body(0); break;
				}
				break;

			case sf::Event::LostFocus:
				engine.renderer.p_alpha = Render_SFML::ALPHA_INACTIVE;
				break;

			case sf::Event::GainedFocus:
				engine.renderer.p_alpha = Render_SFML::ALPHA_ACTIVE;
				break;

			case sf::Event::Closed:
				window.close();
				break;
			}
		}

		engine.updates_for_next_frame();
		engine.draw();
	}

	return 0;
}

//============================================================================
void World_SFML::recalc_for_next_frame(const Engine_SFML& game) // ++world
// Should be idempotent -- which doesn't matter normally, but testing could reveal bugs if it isn't!
{
	dt = clock.getElapsedTime().asSeconds();
	clock.restart();

	for (size_t i = 0; i < bodies.size(); ++i)
	{
		auto& body = bodies[i];

		// Gravity - only apply to the moon(s), ignore the moon's effect on the globe!
		if (i >= 1) {
			auto& globe = bodies[0];
			float distance = sqrt(pow(globe->p.x - body->p.x, 2) + pow(globe->p.y - body->p.y, 2));
			if (distance < globe->r) distance = globe->r; //!... avoid 0 -> infinity
			float g = G * globe->mass / (distance * distance);
			sf::Vector2f gvect((globe->p.x - body->p.x) * g, (globe->p.y - body->p.y) * g);
//!!should be:	sf::Vector2f gvect((globe->p.x - body->p.x) / distance * g, (globe->p.y - body->p.y) / distance * g);
			sf::Vector2f dv = gvect * dt;
			body->v += dv;
			sf::Vector2f ds(body->v.x * dt, body->v.y * dt);
			body->p += ds;

cerr << " - gravity: dist = "<<distance << ", g = "<<g << ", gv = ("<<body->v.x<<","<<body->v.y<<"), " << " dx = "<<ds.x << ", dy = "<<ds.y << endl;
		}

		// Friction:
		sf::Vector2f friction_decel(-body->v.x * FRICTION, -body->v.y * FRICTION);
		sf::Vector2f dv = friction_decel * (dt);
		body->v += dv;
		sf::Vector2f ds(body->v.x * dt, body->v.y * dt);
		body->p += ds;
cerr << "v = ("<<body->v.x<<","<<body->v.y<<"), " << " dx = "<<ds.x << ", dy = "<<ds.y << ", dt = "<<dt << endl;
	}
}


void Render_SFML::render_next_frame(const Engine_SFML& game)
// Should be idempotent -- which doesn't matter normally, but testing could reveal bugs if it isn't!
{
	// The indexes match the corresponding physical bodies!
	for (size_t i = 0; i < shapes_to_change.size(); ++i)
	{
		auto& body = game.world.bodies[i];

		//!!Sigh, this will break as soon as not just circles would be there...
		auto shape = dynamic_pointer_cast<sf::Shape>(shapes_to_change[i]);
		shape->setFillColor(sf::Color(70 + body->color, 12, 50 - body->color, p_alpha));

		auto& trshape = dynamic_cast<sf::Transformable&>(*shape);

		trshape.setPosition(sf::Vector2f(
			VIEW_WIDTH/2  + (body->p.x - body->r) * game._SCALE + game._OFFSET_X,
			VIEW_HEIGHT/2 + (body->p.y - body->r) * game._SCALE + game._OFFSET_Y));
	}
}
