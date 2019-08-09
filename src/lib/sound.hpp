/*
 * sound.hpp
 *
 *  Created on: Aug 9, 2019
 *      Author: elchaschab
 */

#ifndef SRC_SLIDE_SOUND_HPP_
#define SRC_SLIDE_SOUND_HPP_

#include <SDL/SDL_mixer.h>
#include <vector>
#include <iostream>
#include <string>

class Sound {
private:
	std::vector<Mix_Chunk*> chunks;
public:
	Sound() {
		//Initialize SDL_mixer
		if (Mix_OpenAudio(44100, AUDIO_U8, 1, 512) == -1) {
			std::cerr << "Error opening audio device" << std::endl;
			throw std::exception();
		}
	}

	~Sound() {
		for (auto& c : chunks)
			Mix_FreeChunk(c);

		Mix_CloseAudio();

	}

	//Loads the wav file and returns it's index
	size_t load(const std::string& filename) {
		Mix_Chunk* effect = Mix_LoadWAV(filename.c_str());
		if (effect == NULL)
			throw std::exception();
		chunks.push_back(effect);
		return chunks.size() - 1;
	}

	void play(size_t idx) {
		if (Mix_Playing(1) != 0)
			return;
		if ( Mix_PlayChannel( -1, chunks[idx], 0 ) == -1) {
			throw std::exception();
		}
	}
};

#endif /* SRC_SLIDE_SOUND_HPP_ */
