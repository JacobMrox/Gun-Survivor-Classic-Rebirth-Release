> This project is provided as-is, free of charge and without warranty. Development is done in my own time, and while I do my best to provide updates and fixes, I can’t treat this as a full-time job.

**Want to see this project reach the next level?**
I’m currently navigating some rough seas and could use a little wind in my sails. If you’ve enjoyed my work so far, a [small donation goes](https://www.paypal.com/paypalme/JACOBQAROONI) a long way in keeping the lights on and the updates coming. Thanks for having my back!

# Why

Biohazard: Gun Survivor (known in China as 惡靈古堡：生存遊戲 and internationally Resident Evil: Survivor) was released on PC exclusively in China and Taiwan, and that version has long been infamous for its instability, bugs, and poor technical implementation. On modern systems, the game is extremely difficult—if not impossible—to run properly due to outdated APIs, broken dependencies, and compatibility issues.

Inspired by [Classic Rebirth](https://classicrebirth.com/) by Gemini-Loboto3. This project exists to modernize and preserve the PC version of Gun Survivor by replacing legacy systems with modern, stable alternatives. The goal is not only to make the game playable on current hardware and operating systems, but also to improve its overall usability, performance, and configurability—without altering the original experience.

In short, this project aims to:

* Restore playability on modern Windows systems
* Replace obsolete or broken APIs
* Fix long-standing bugs and technical issues
* Add quality-of-life improvements and configuration options
* Lay the groundwork for further enhancements and modding

# Project Status

<img width="496" height="517" alt="image" src="https://github.com/user-attachments/assets/3ba34689-a19e-474c-98e7-34a9b0903d25" />

## V 1.0 (Current Public Build)
* ✅ ~~Remove CD check~~ DONE 
* ✅ ~~60 FPS Lock to fix door transitions~~ DONE 
* ✅ ~~Add ability to play in window mode~~ DONE 
* ✅ ~~Show window border in windowed mode~~ DONE 
* ✅ ~~Add ability to lock mouse in window~~ DONE 
* ✅ ~~Fix window title typo [GUN SURUIVOR] -> RESIDENT EVIL: SURVIVOR PC~~ DONE 
* ✅ ~~Fix mouse crosshair alignment (replace crosshair)~~ DONE 
* ✅ ~~Add custom resolution option~~ DONE
* ✅ ~~Add option to enable/disable log printing~~ DONE
* ✅ ~~Add option to skip movies/videos~~ DONE - Set MovieSkip to 1 to skip entirely always, or press SPACE/RETURN/ENTER to skip manually whenever you want!
* ✅ ~~Add a proper in-game options menu backed by d3d8.ini~~ DONE

## V 1.1
* ✅ ~~Add option to show/disable config at boot~~ DONE - when disabled users must hold SHIFT or edit config.ini to show config menu
* ✅ ~~Show icon in Window~~ DONE
* Fix shadow/blood pool under enemies/npcs (In progress...).
* Fix movie player by replacing DirectShow with ffmpeg (In progress...).
* ✅ ~~Add crosshair skins + ability to select custom one.~~ DONE
* Remove old log printing shipped with the original game (debug.txt).
* Remove the need for the game to have GBIO.CFG in the directory to launch.
* Add shooting screen flash (gun con flash simulation) effect.
* Add widescreen option (to prevent stretched ratio).
* Fix Zombie footstep sound.
* Fix NPC/enemy lighting/shader.
* Fix enemy AI/Animations.
* Add Xinput and Raw Input support, in addition to Dinput.
* Mod Support

# Installation

1) Click <> Code → Download ZIP
2) Extract all files into your game installation directory, replacing any files when prompted.
3) The game requires every included file to run correctly, including GBIO.CFG (a dummy file the original game expects at startup).
4) This requirement will be automated by the DLL in the next version
5) This project is provided free, as-is, without warranty, and will continue improving over time

# F.A.Q.

***1) Will you translate this game?***

An English patch already exists for this title, created by Reikaz. You can find it here:
https://www.legendsworld.net/webroot/phpBB3/viewtopic.php?p=86314

Because a translation is already available, I have no plans to recreate that work from scratch. My focus is on restoring and improving the PC experience.

***2) The patch didn’t work / I don’t like it / I want to complain***

If the patch does not work, please double-check that all included files were copied correctly into the game directory. The vast majority of reported issues are caused by incomplete installation rather than a problem with the project itself.

This project is provided free of charge and as-is, without warranty. It is a personal development effort, not a commercial product or full-time service. Constructive feedback is always welcome. Harassment, insults, or false accusations will be ignored.

<img width="1350" height="324" alt="image" src="https://github.com/user-attachments/assets/3f2d6bea-2945-4833-93e6-f6d5abac4b8a" />

A previous example of abusive behavior was removed by YouTube moderation for violating their policies. I will not engage with that kind of conduct.

If you encounter a genuine bug, please provide clear steps to reproduce it, your game version, and your system details. That helps far more than frustration does.

***3) When will feature X be finished?***

Development is done in my own time. There is no fixed schedule.

The project is currently playable again on modern systems, which was the primary goal. Additional features will be implemented when time and circumstances allow.

Support and encouragement genuinely help keep the project moving forward.

# Media

[![Watch the video](https://img.youtube.com/vi/u___fNYwvH0/maxresdefault.jpg)](https://youtu.be/u___fNYwvH0)

<img width="1920" height="1032" alt="image" src="https://github.com/user-attachments/assets/ff9dddc9-73c1-4b38-9ece-df09b8459819" />
<img width="1920" height="1032" alt="image" src="https://github.com/user-attachments/assets/ab899a4f-0aec-4f0d-95aa-dddbb1cae2f7" />
<img width="1920" height="1032" alt="image" src="https://github.com/user-attachments/assets/b2207f70-ce1c-4de8-8d1a-f6f3e3854d15" />
