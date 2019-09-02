#ifndef SRC_SLIDE_SOUND_HPP_
#define SRC_SLIDE_SOUND_HPP_

#include <SDL/SDL_mixer.h>
#include <vector>
#include <iostream>
#include <string>

class Sound {
private:
	std::vector<Mix_Chunk*> chunks_;
	Mix_Music* music_ = NULL;
public:
	Sound() {
		//Initialize SDL_mixer
		if (Mix_OpenAudio(44100, AUDIO_U16, 1, 512) == -1) {
			std::cerr << "Error opening audio device" << std::endl;
			throw std::exception();
		}

		music_ = Mix_LoadMUS("music.wav");

		if (music_ == NULL) {
			throw new std::exception();
		}
	}

	~Sound() {
		Mix_FreeMusic(music_);
		for (auto& c : chunks_)
			Mix_FreeChunk(c);

		Mix_CloseAudio();

	}

	void playMusic() {
		Mix_PlayMusic( music_, -1 );
	}

	void pauseMusic() {
		Mix_PauseMusic();
	}

	//Loads the wav file and returns it's index
	size_t loadFx(const std::string& filename) {
		Mix_Chunk* effect = Mix_LoadWAV(filename.c_str());
		if (effect == NULL)
			throw std::exception();
		chunks_.push_back(effect);
		return chunks_.size() - 1;
	}

	void play(size_t idx) {
		if ( Mix_PlayChannel( -1, chunks_[idx], 0 ) == -1) {
			//throw std::exception();
		}
	}
};

#endif /* SRC_SLIDE_SOUND_HPP_ */
