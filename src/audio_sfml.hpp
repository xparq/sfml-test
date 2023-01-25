#ifndef __AUDIO_SFML_
#define __AUDIO_SFML_

class Audio_Stub
{
public:
	virtual size_t add_sound(const char* filename)  { filename; return 0; }
	virtual void   play_sound(size_t ndx)  { ndx; }
	virtual bool   play_music(const char* filename) { filename; return false; }
	virtual void   toggle_music()  {}
};

#ifndef DISABLE_AUDIO // If disabled, only the stub class will be available.

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/Music.hpp>

#include <vector>

class Audio_SFML : public Audio_Stub
{
	struct SndBuf_NoCopy_Wrapper_thanksfornothing_std_vector : public sf::SoundBuffer {
		   SndBuf_NoCopy_Wrapper_thanksfornothing_std_vector(int) {}
		   SndBuf_NoCopy_Wrapper_thanksfornothing_std_vector() {}
		   SndBuf_NoCopy_Wrapper_thanksfornothing_std_vector(const SndBuf_NoCopy_Wrapper_thanksfornothing_std_vector&)
			   { /* cerr << "SFML SndBuf wrapper BEING COPIED!\n"; */ }
	};
	std::vector<SndBuf_NoCopy_Wrapper_thanksfornothing_std_vector> sounds;

public:
	size_t add_sound(const char* filename) override;
	void play_sound(size_t ndx) override;
	bool play_music(const char* filename) override;
	void toggle_music() override;

	Audio_SFML()
	{
		// Add an empty element so if add() returns 0 for errors it could still
		// be used as an index, pointing to a safe & silent spot.
		sounds.resize(1); //! Remember the implicit copy ctor call here (no matter what)! :-o (-> also: add_sound!)
	}
private:
	sf::Sound _sound; //!! only this one single player object yet!
	sf::Music _music; //!! only this one single player object yet!
};

#endif // DISABLE_AUDIO

#endif // __AUDIO_SFML_