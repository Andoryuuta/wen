# Wen Modding Framework (work-in-progress)
`wen` is a work-in-progress C++ modding framework for Monster Hunter: World.

The goal of this project is to provide:
1. A unified loader and plugin callback system for loading plugins specific stages:
    * Pre-MSVC CRT Init (e.g. run code before any static initializers have run)
    * Post-MSVC CRT Init (after static initializers, before WinMain)
    * WinMain
2. Zero-cost\* game callbacks
    * Collection of game function hooks, which have zero cost unless a plugin is registered to use it. 
    * Game tick
    * Scene load
    * EM action
    * Player action
    * Input controller
    * etc.
3. A shared set of binary-compatible class headers
    * Auto-generated based on client DTI debug data, refined manually.


If these three goals are met, we can provide a framework/api for creating in-memory C++ mods without the end developer ever needing to specify any function address or class offset manually. If a new hook, class member, or function is required, it would be added to `wen` and available to everyone who might want to use it in the future.

## Ideal plugin API (not real)
Ideally, the plugin API will look something like this:

**(Example code is purely regarding the structure -- all the fields and classes are fictional and convoluted)**
```cpp
#include <wen/core.h>

using namespace wen;
using namespace wen::mhw;

class FishTrackerPlugin: WenPlugin
{
    FishTrackerPlugin()
    {
        // By requiring each plugin to register the callbacks it uses,
        // we only have to hook game functions that at least one mod needs.
        // (and hence have very little overhead when supporting many many callbacks)
        this->register_callback(WEN_CALLBACK_MAIN);
        this->register_callback(WEN_CALLBACK_TICK);

        this->prevTickFishingState = sFishing::FISHING_STATE::INACTIVE;
    }

    virtual OnMain() override
    {
        Log("FishTrackerPlugin running!");
    }

    virtual OnTick() override
    {
        // Check if the fishing state has changed.
        auto& fishingObj = sFishing::instance();
        if (this->prevTickFishingState == sFishing::FISHING_STATE::ACTIVE &&
            fishingObj->mState == sFishing::FISHING_STATE::INACTIVE)
        {
            // We have just finished fishing.
            if (fishingObj->mIsBigFish)
            {
                sChat::instance()->Print("Wow! A big fish!", -1, -1, false);
            }
        }

        // Update our tracked fishing state used to determine state transitions.
        this->prevTickFishingState = fishingObj->mIsFishing;
    }

    sFishing::FISHING_STATE prevTickFishingState;
}

WEN_REGISTER_PLUGIN(FishTracker);
WEN_PLUGIN_VERSION("1.0");
WEN_PLUGIN_AUTHOR("Andoryuuta");
WEN_PLUGIN_SITE("github.com/andoryuuta/wen/examples/fishtracker");

```