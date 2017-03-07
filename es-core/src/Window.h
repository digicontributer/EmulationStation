#pragma once

#include "GuiComponent.h"
#include <vector>
#include "resources/Font.h"
#include "InputManager.h"

class FileData;
class HelpComponent;
class ImageComponent;

class Window
{
public:
	class ScreenSaver {		
	public:		
		virtual void startScreenSaver() = 0;		
		virtual void stopScreenSaver() = 0;		
		virtual void renderScreenSaver() = 0;		
		virtual bool allowSleep() = 0;		
		virtual void update(int deltaTime) = 0;		
		virtual bool isScreenSaverActive() = 0;
		virtual std::string getSystemName() = 0;
		virtual std::string getGameName() = 0;
		virtual FileData* getCurrentGame() = 0;
		virtual void launchGame() = 0;
	};
 
	Window();
	~Window();

	void pushGui(GuiComponent* gui);
	void removeGui(GuiComponent* gui);
	GuiComponent* peekGui();

	void textInput(const char* text);
	void input(InputConfig* config, Input input);
	void update(int deltaTime);
	void render();

	bool init(unsigned int width = 0, unsigned int height = 0);
	void deinit();

	void normalizeNextUpdate();

	inline bool isSleeping() const { return mSleeping; }
	bool getAllowSleep();
	void setAllowSleep(bool sleep);
	
	void renderLoadingScreen();

	void renderHelpPromptsEarly(); // used to render HelpPrompts before a fade
	void setHelpPrompts(const std::vector<HelpPrompt>& prompts, const HelpStyle& style);
	
	void setScreenSaver(ScreenSaver* screenSaver) { mScreenSaver = screenSaver; }

	void startScreenSaver();
	void cancelScreenSaver();
	void renderScreenSaver();

private:
	void onSleep();
	void onWake();

	// Returns true if at least one component on the stack is processing
	bool isProcessing();
	//void renderScreenSaver();
	//void startScreenSaver();
	//void cancelScreenSaver();
 
	HelpComponent* mHelp;
	ImageComponent* mBackgroundOverlay;
	ScreenSaver*	mScreenSaver;
	bool			mRenderScreenSaver;

	std::vector<GuiComponent*> mGuiStack;

	std::vector< std::shared_ptr<Font> > mDefaultFonts;

	int mFrameTimeElapsed;
	int mFrameCountElapsed;
	int mAverageDeltaTime;

	std::unique_ptr<TextCache> mFrameDataText;

	bool mNormalizeNextUpdate;

	bool mAllowSleep;
	bool mSleeping;
	unsigned int mTimeSinceLastInput;

	bool mRenderedHelpPrompts;
};
