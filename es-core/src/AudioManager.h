#pragma once
#ifndef ES_CORE_AUDIO_MANAGER_H
#define ES_CORE_AUDIO_MANAGER_H

#include <SDL_audio.h>
#include <memory>
#include <vector>
#include "SDL_mixer.h"
#include <string>
#include <iostream>

class Sound;

class AudioManager
{
private:
	AudioManager();

	static std::vector<std::shared_ptr<Sound>> sSoundVector;
	static AudioManager* sInstance;

	bool mInitialized;

public:
	static AudioManager* getInstance();
	static bool isInitialized();

	void init();
	void deinit();

	void registerSound(std::shared_ptr<Sound> & sound);
	void unregisterSound(std::shared_ptr<Sound> & sound);

	void play();
	void stop();

	virtual ~AudioManager();
};

#endif // ES_CORE_AUDIO_MANAGER_H
