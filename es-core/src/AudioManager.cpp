#include "AudioManager.h"

#include "Log.h"
#include "Settings.h"
#include "Sound.h"
#include <SDL.h>
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include <unistd.h>

std::vector<std::shared_ptr<Sound>> AudioManager::sSoundVector;
AudioManager* AudioManager::sInstance = NULL;


AudioManager::AudioManager() : mInitialized(false), mCurrentMusic(nullptr), mMusicVolume(Settings::getInstance()->getInt("MusicVolume"))
{
	init();
}

AudioManager::~AudioManager()
{
	deinit();
}

AudioManager* AudioManager::getInstance()
{
	//check if an AudioManager instance is already created, if not create one
	if (sInstance == nullptr && Settings::getInstance()->getBool("EnableSounds")) {
		sInstance = new AudioManager();
	}

	return sInstance;
}

bool AudioManager::isInitialized()
{
	if(sInstance == nullptr)
		return false;

	return sInstance->mInitialized;
}


void AudioManager::init()
{
	if(mInitialized)
		return;

	if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
	{
		LOG(LogError) << "Error initializing SDL audio!\n" << SDL_GetError();
		return;
	}

	// Open the audio device and pause
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0)
		LOG(LogError) << "SDL AUDIO Error - Unable to open SDLMixer audio: " << SDL_GetError() << std::endl;
	else
	{
		LOG(LogInfo) << "SDL AUDIO Initialized";
		mInitialized = true;

		// Reload known sounds
		for(unsigned int i = 0; i < sSoundVector.size(); i++)
			sSoundVector[i]->init();
	}
}

void AudioManager::deinit()
{
	if(!mInitialized)
		return;

	mInitialized = false;

	//stop all playback
	stop();
	stopMusic();

	// Free known sounds from memory
	for(unsigned int i = 0; i < sSoundVector.size(); i++)
		sSoundVector[i]->deinit();

	Mix_HookMusicFinished(nullptr);
	Mix_HaltMusic();

	//completely tear down SDL audio. else SDL hogs audio resources and emulators might fail to start...
	Mix_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	sInstance = NULL;

	LOG(LogInfo) << "SDL AUDIO Deinitialized";
}

void AudioManager::registerSound(std::shared_ptr<Sound> & sound)
{
	getInstance();
	sSoundVector.push_back(sound);
}

void AudioManager::unregisterSound(std::shared_ptr<Sound> & sound)
{
	getInstance();
	for(unsigned int i = 0; i < sSoundVector.size(); i++)
	{
		if(sSoundVector.at(i) == sound)
		{
			sSoundVector[i]->stop();
			sSoundVector.erase(sSoundVector.cbegin() + i);
			return;
		}
	}
	LOG(LogError) << "AudioManager Error - tried to unregister a sound that wasn't registered!";
}

void AudioManager::play()
{
	getInstance();
}

void AudioManager::stop()
{
	// Stop playing all Sounds
	for (unsigned int i = 0; i < sSoundVector.size(); i++)
		if (sSoundVector.at(i)->isPlaying())
			sSoundVector[i]->stop();
}

void AudioManager::getMusicIn(const std::string &path, std::vector<std::string>& all_matching_files)
{	
	if(!Utils::FileSystem::isDirectory(path)){
		return;
	}

  auto dirContent = Utils::FileSystem::getDirContent(path);

  for(auto it = dirContent.cbegin(); it != dirContent.cend(); ++it)
	{	
		if (Utils::FileSystem::isDirectory(*it))
		{
			if (*it == "." || *it == "..")
				continue;
		}
		else
		{
			std::string extension = Utils::String::toLower(Utils::FileSystem::getExtension(*it));
			if (extension == ".mp3" || extension == ".ogg" || extension == ".flac"
				|| extension == ".wav" || extension == ".mod" || extension == ".xm"
				|| extension == ".stm" || extension == ".s3m" || extension == ".far"
				|| extension == ".it" || extension == ".669" || extension == ".mtm")
				all_matching_files.push_back(*it);
		}
	}		
}

void AudioManager::playRandomMusic(bool continueIfPlaying) 
{
	if(!Settings::getInstance()->getBool("EnableMusic"))
		return;
		
	std::vector<std::string> musics;

	// check in RetroPie music directory
	if(musics.empty())
		getMusicIn(Utils::FileSystem::getHomePath() + "/RetroPie/roms/music", musics);
  
	// check in system sound directory
	if(musics.empty())
		getMusicIn("/opt/retropie/music", musics);
  
	// check in .emulationstation/music directory
	if(musics.empty())
		getMusicIn(Utils::FileSystem::getHomePath() + "/.emulationstation/music", musics);

	if(musics.empty())
		return;

#if defined(WIN32)
	srand(time(NULL) % getpid());
#else
	srand(time(NULL) % getpid() + getppid());
#endif

	int randomIndex = rand() % musics.size();

	// continue playing ?
	if(mCurrentMusic != nullptr && continueIfPlaying)
		return;

	playMusic(musics.at(randomIndex));
}	

void AudioManager::playMusic(std::string path)
{
	if (!mInitialized)
		return;

	// free the previous music
	stopMusic(false);

	if(!Settings::getInstance()->getBool("EnableMusic"))
		return;

	// load a new music
	mCurrentMusic = Mix_LoadMUS(path.c_str());
	if (mCurrentMusic == NULL)
	{
		LOG(LogError) << Mix_GetError() << " for " << path;
		return;
	}

	if (Mix_FadeInMusic(mCurrentMusic, 1, 1000) == -1)
	{
		stopMusic();
		return;
	}

	Mix_HookMusicFinished(AudioManager::musicEnd_callback);
}

void AudioManager::musicEnd_callback()
{
  if(!Settings::getInstance()->getBool("EnableMusic"))
		return;

	AudioManager::getInstance()->playRandomMusic(false);
}

void AudioManager::stopMusic(bool fadeOut)
{
	if (mCurrentMusic == NULL)
		return;

	Mix_HookMusicFinished(nullptr);

	if (fadeOut)
	{
		// Fade-out is nicer !
		while (!Mix_FadeOutMusic(500) && Mix_PlayingMusic())
			SDL_Delay(100);
	}

	Mix_HaltMusic();
	Mix_FreeMusic(mCurrentMusic);
	mCurrentMusic = NULL;
}

int AudioManager::getMaxMusicVolume()
{
	int ret = (Settings::getInstance()->getInt("MusicVolume") * MIX_MAX_VOLUME) / 100;
	if (ret > MIX_MAX_VOLUME)
		return MIX_MAX_VOLUME;

	if (ret < 0)
		return 0;

	return ret;
}

void AudioManager::update(int deltaTime)
{
	if(sInstance == nullptr || !sInstance->mInitialized || !Settings::getInstance()->getBool("EnableMusic"))
		return;

	Mix_VolumeMusic((int)sInstance->mMusicVolume);
}