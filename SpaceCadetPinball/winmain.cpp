#include "pch.h"
#include "winmain.h"

#include "fullscrn.h"
#include "memory.h"
#include "midi.h"
#include "pinball.h"
#include "options.h"
#include "pb.h"
#include "Sound.h"
#include "resource.h"

#ifdef VITA
#ifndef NDEBUG
#include <debugnet.h>
#define DEBUG_IP "192.168.0.45"
#define DEBUG_PORT 18194
#endif
#include "vita_input.h"

static bool vita_imgui_enabled = false;
static bool needs_focus = false;

static inline void vita_set_imgui_enabled(bool enabled)
{
	if(enabled) needs_focus = true;
	vita_imgui_enabled = enabled;
}

static inline int vita_translate_joystick(int joystickButton)
{
	const VITA_BUTTONS asVb = (const VITA_BUTTONS)joystickButton;

	switch(asVb)
	{
		case LEFT_SHOULDER:
		    return options::Options.LeftFlipperKeyDft;
			break;
		case RIGHT_SHOULDER:
			return options::Options.RightFlipperKeyDft;
			break;
		case CROSS:
			return options::Options.PlungerKeyDft;
			break;
		case D_DOWN:
			return options::Options.BottomTableBumpKeyDft;
			break;
		case D_LEFT:
			return options::Options.LeftTableBumpKeyDft;
			break;
		case D_RIGHT:
			return options::Options.RightTableBumpKeyDft;
			break;
	}
	return 0;
}
#else
static inline int vita_translate_joystick(int joystickButton)
{
	return 0;
}
#endif

static ImFont *custom_font = nullptr;
static std::map<Mix_MIDI_Device, std::string> _MixerMidiDevices = 
{
	{MIDI_ADLMIDI, std::string("OPL3 Synth (ADLMIDI)")},
	/*
	{MIDI_Native, std::string("Native")},
	{MIDI_Timidity, std::string("Timidity")},
	*/
	{MIDI_OPNMIDI, std::string("OPN2 Synth (OPNMIDI)")},
	{MIDI_ANY, ""},
	{MIDI_KnownDevices, ""}
};

static std::map<Mix_ADLMIDI_Emulator, std::string> _MixerADLEmus =
{
	{ADLMIDI_OPL3_EMU_DEFAULT, "DEFAULT"},
	{ADLMIDI_OPL3_EMU_NUKED, "Nuked (HEAVY)"},
	{ADLMIDI_OPL3_EMU_NUKED_1_7_4, "Nuked 1.7.4 (HEAVY)"},
	{ADLMIDI_OPL3_EMU_DOSBOX, "DOSBox"},
	{ADLMIDI_OPL3_EMU_OPAL, "Opal"},
	{ADLMIDI_OPL3_EMU_JAVA, "Java"}
};

/**
 * Win95 style for ImGui.
 */
static inline void vita_setup_custom_imgui_style()
{
	ImGuiStyle& curStyle = ImGui::GetStyle();

	curStyle.MouseCursorScale = 0.f;
	curStyle.AntiAliasedLines = false;
	curStyle.AntiAliasedLinesUseTex = false;
	curStyle.AntiAliasedFill = false;

	curStyle.Colors[ImGuiCol_Separator] = ImColor(0, 0, 0, 255);
	curStyle.Colors[ImGuiCol_SeparatorActive] = ImColor(0, 0, 0, 255);
	curStyle.Colors[ImGuiCol_SeparatorHovered] = ImColor(0, 0, 0, 255);

	curStyle.Colors[ImGuiCol_Border] = ImColor(0,0,0,255);
	curStyle.Colors[ImGuiCol_MenuBarBg] = ImColor(192,192,192,255);
	
	curStyle.Colors[ImGuiCol_FrameBg] = ImColor(255, 0, 0, 255);

	curStyle.Colors[ImGuiCol_Button] = curStyle.Colors[ImGuiCol_MenuBarBg];
	curStyle.Colors[ImGuiCol_ButtonHovered] = ImColor(150,150,150,255);
	curStyle.Colors[ImGuiCol_ButtonActive] = ImColor(120, 120, 120, 255);

	curStyle.Colors[ImGuiCol_PopupBg] = curStyle.Colors[ImGuiCol_MenuBarBg];
	curStyle.Colors[ImGuiCol_Header] = curStyle.Colors[ImGuiCol_MenuBarBg];
	curStyle.Colors[ImGuiCol_HeaderHovered] = ImColor(34,34,255,255);
	curStyle.Colors[ImGuiCol_HeaderActive] = ImColor(105,105,105,255);

	curStyle.Colors[ImGuiCol_TitleBg] = ImColor(128, 128, 128, 255);
	curStyle.Colors[ImGuiCol_TitleBgActive] = ImColor(34,34,255,255);
	curStyle.Colors[ImGuiCol_TitleBgCollapsed] = ImColor(0, 128, 0, 255);

	curStyle.Colors[ImGuiCol_Text] = ImColor(0, 0, 0, 255);

	curStyle.Colors[ImGuiCol_ScrollbarBg] = ImColor(60, 60, 60, 255);
	curStyle.Colors[ImGuiCol_ScrollbarGrab] = ImColor(150, 150, 150, 255);
	curStyle.Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(130, 130, 130, 255);


	// curStyle.PopupRounding = 8.0f;
	curStyle.PopupBorderSize = 2.0f;
	curStyle.ChildBorderSize = 2.0f;
	curStyle.WindowBorderSize = 2.0f;
	curStyle.FrameBorderSize = 2.0f;
	curStyle.DisplayWindowPadding = ImVec2(4.0f, 4.0f);
	curStyle.ScrollbarRounding = 0.f;
	// curStyle.FrameRounding = 8.f;
	
	curStyle.ScaleAllSizes(2.f);

#ifndef VITA
	const std::string archivo_path = std::string(SDL_GetBasePath()) + std::string("W95FA.ttf");
#else
	const std::string archivo_path = std::string("app0:font.ttf");
#endif
	// ImFontConfig config;
	auto glyphRanges = winmain::ImIO->Fonts->GetGlyphRangesDefault();
	
	custom_font = winmain::ImIO->Fonts->AddFontFromFileTTF(archivo_path.c_str(), 24, nullptr, glyphRanges);
	winmain::ImIO->Fonts->Build();
	ImGuiSDL::CreateFontsTexture();
// #ifdef VITA
	winmain::ImIO->FontGlobalScale = 1.f;
// #endif
}


const double TargetFps = 60, TargetFrameTime = 1000 / TargetFps;

SDL_Window* winmain::MainWindow = nullptr;
SDL_Renderer* winmain::Renderer = nullptr;
ImGuiIO* winmain::ImIO = nullptr;

int winmain::return_value = 0;
int winmain::bQuit = 0;
int winmain::activated;
int winmain::DispFrameRate = 0;
int winmain::DispGRhistory = 0;
int winmain::single_step = 0;
int winmain::has_focus = 1;
int winmain::last_mouse_x;
int winmain::last_mouse_y;
int winmain::mouse_down;
int winmain::no_time_loss;

DWORD winmain::then;
DWORD winmain::now;
bool winmain::restart = false;

gdrv_bitmap8 winmain::gfr_display{};
std::string winmain::DatFileName;
bool winmain::ShowAboutDialog = false;
bool winmain::ShowImGuiDemo = false;
bool winmain::LaunchBallEnabled = true;
bool winmain::HighScoresEnabled = true;
bool winmain::DemoActive = false;
char* winmain::BasePath;

uint32_t timeGetTimeAlt()
{
	auto now = std::chrono::high_resolution_clock::now();
	auto duration = now.time_since_epoch();
	auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	return static_cast<uint32_t>(millis);
}

int winmain::WinMain(LPCSTR lpCmdLine)
{
	memory::init(memalloc_failure);

	// SDL init
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not initialize SDL2", SDL_GetError(), nullptr);
		return 1;
	}

#ifdef VITA
	SDL_setenv("VITA_DISABLE_TOUCH_BACK", "TRUE", 1);
	BasePath = SDL_GetPrefPath(nullptr, "SpaceCadetPinball");
#else
	BasePath = SDL_GetBasePath();
#endif
	pinball::quickFlag = strstr(lpCmdLine, "-quick") != nullptr;
	DatFileName = options::get_string("Pinball Data", pinball::get_rc_string(168, 0));

#if VITA
#ifndef NDEBUG
	std::string dataFullPath = BasePath + DatFileName;
	debugNetInit(DEBUG_IP, DEBUG_PORT, DEBUG);
	debugNetPrintf(INFO, "The data path is '%s'\n", dataFullPath.c_str());
#endif

	vita_init_joystick();
#endif


	/*Check for full tilt .dat file and switch to it automatically*/
	auto cadetFilePath = pinball::make_path_name("CADET.DAT");
	auto cadetDat = fopen(cadetFilePath.c_str(), "r");
	if (cadetDat)
	{
		fclose(cadetDat);
		DatFileName = "CADET.DAT";
		pb::FullTiltMode = true;
	}

	options::init_resolution();

	// SDL window
	SDL_Window* window = SDL_CreateWindow
	(
		pinball::get_rc_string(38, 0),
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		800, 556,
		SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE
	);
	MainWindow = window;
	if (!window)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not create window", SDL_GetError(), nullptr);
		return 1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer
	(
		window,
		-1,
		SDL_RENDERER_ACCELERATED
	);
	Renderer = renderer;
	if (!renderer)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not create renderer", SDL_GetError(), window);
		return 1;
	}

	// ImGui init
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiSDL::Initialize(renderer, 0, 0);
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO();
#ifdef VITA
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
#endif
	ImIO = &io;

	// ImGui_ImplSDL2_Init is private, we are not actually using ImGui OpenGl backend
	ImGui_ImplSDL2_InitForOpenGL(window, nullptr);

	

	auto prefPath = SDL_GetPrefPath(nullptr, "SpaceCadetPinball");
	auto iniPath = BasePath + std::string("imgui_pb.ini");
	std::cout << "Ini Path for imgui: " << iniPath << std::endl;
	io.IniFilename = iniPath.c_str();
	SDL_free(prefPath);

	// PB init from message handler
	{
		++memory::critical_allocation;

		options::init();
		auto voiceCount = options::get_int("Voices", 8);
		auto defaultMidiPlayer = options::Options.CurMidiBackend;
		if (Sound::Init(voiceCount, defaultMidiPlayer))
			options::Options.Sounds = 0;
		Sound::Activate();

		if (!pinball::quickFlag && !midi::music_init())
			options::Options.Music = 0;

		if (pb::init())
		{
			std::string msg = std::string("The .dat file is missing.\nPlace dat file: ") + BasePath + DatFileName;
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not load game data",
				msg.c_str(), window);
			return 1;
		}

		fullscrn::init();

		--memory::critical_allocation;
	}

	pb::reset_table();
	pb::firsttime_setup();

	if (strstr(lpCmdLine, "-fullscreen"))
	{
		options::Options.FullScreen = 1;
	}

	SDL_ShowWindow(window);
	fullscrn::set_screen_mode(options::Options.FullScreen);

	if (strstr(lpCmdLine, "-demo"))
		pb::toggle_demo();
	else
		pb::replay_level(0);

	DWORD updateCounter = 300u, frameCounter = 0, prevTime = 0u;
	then = timeGetTimeAlt();


	// io.Fonts->AddFontDefault();
	// io.Fonts->Build();
	vita_setup_custom_imgui_style();
	


	double sdlTimerResMs = 1000.0 / static_cast<double>(SDL_GetPerformanceFrequency());
	auto frameStart = static_cast<double>(SDL_GetPerformanceCounter());
	while (true)
	{
		if (!updateCounter)
		{
			updateCounter = 300;
			if (DispFrameRate)
			{
				auto curTime = timeGetTimeAlt();
				if (prevTime)
				{
					char buf[60];
					auto elapsedSec = static_cast<float>(curTime - prevTime) * 0.001f;
					snprintf(buf, sizeof buf, "Updates/sec = %02.02f Frames/sec = %02.02f ",
					          300.0f / elapsedSec, frameCounter / elapsedSec);
					SDL_SetWindowTitle(window, buf);
#if defined(VITA) && !defined(NDEBUG)
					debugNetPrintf(DEBUG, buf);
#endif
					frameCounter = 0;

					if (DispGRhistory)
					{
						if (!gfr_display.BmpBufPtr1)
						{
							auto plt = static_cast<ColorRgba*>(malloc(1024u));
							auto pltPtr = &plt[10];
							for (int i1 = 0, i2 = 0; i1 < 256 - 10; ++i1, i2 += 8)
							{
								unsigned char blue = i2, redGreen = i2;
								if (i2 > 255)
								{
									blue = 255;
									redGreen = i1;
								}

								auto clr = Rgba{redGreen, redGreen, blue, 0};
								*pltPtr++ = {*reinterpret_cast<uint32_t*>(&clr)};
							}
							gdrv::display_palette(plt);
							free(plt);
							gdrv::create_bitmap(&gfr_display, 400, 15);
						}

						gdrv::blit(&gfr_display, 0, 0, 0, 0, 300, 10);
						gdrv::fill_bitmap(&gfr_display, 300, 10, 0, 0, 0);
					}
				}
				prevTime = curTime;
			}
			else
			{
				prevTime = 0;
			}
		}

		if (!ProcessWindowMessages() || bQuit)
			break;

		if (has_focus)
		{
			if (mouse_down)
			{
				now = timeGetTimeAlt();
				if (now - then >= 2)
				{
					int x, y;
					SDL_GetMouseState(&x, &y);
					pb::ballset(last_mouse_x - x, y - last_mouse_y);
					SDL_WarpMouseInWindow(window, last_mouse_x, last_mouse_y);
				}
			}
			if (!single_step)
			{
				auto curTime = timeGetTimeAlt();
				now = curTime;
				if (no_time_loss)
				{
					then = curTime;
					no_time_loss = 0;
				}

				if (curTime == then)
				{
					SDL_Delay(8);
				}
				else if (pb::frame(curTime - then))
				{
					if (gfr_display.BmpBufPtr1)
					{
						auto deltaT = now - then + 10;
						auto fillChar = static_cast<char>(deltaT);
						if (deltaT > 236)
						{
							fillChar = -7;
						}
						gdrv::fill_bitmap(&gfr_display, 1, 10, 299 - updateCounter, 0, fillChar);
					}
					--updateCounter;
					then = now;
				}
			}

			auto frameEnd = static_cast<double>(SDL_GetPerformanceCounter());
			auto elapsedMs = (frameEnd - frameStart) * sdlTimerResMs;
			if (elapsedMs >= TargetFrameTime)
			{
				// Keep track of remainder, limited to one frame time.
				frameStart = frameEnd - std::min(elapsedMs - TargetFrameTime, TargetFrameTime) / sdlTimerResMs;

#ifdef VITA
				if(vita_imgui_enabled)
				{
#endif
					ImGui_ImplSDL2_NewFrame();
						

					ImGui::NewFrame();
					

					RenderUi();
#ifdef VITA
				}
#endif

				SDL_RenderClear(renderer);
				gdrv::BlitScreen();

#ifdef VITA
				if(vita_imgui_enabled)
				{
#endif
					ImGui::Render();
					ImGuiSDL::Render(ImGui::GetDrawData());
#ifdef VITA
				}
#endif

				SDL_RenderPresent(renderer);
				frameCounter++;
			}
		}
	}

	gdrv::destroy_bitmap(&gfr_display);
	options::uninit();
	midi::music_shutdown();
	pb::uninit();
	Sound::Close();
	gdrv::uninit();
	ImGuiSDL::DestroyFontsTexture();
	ImGuiSDL::Deinitialize();
	ImGui_ImplSDL2_Shutdown();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	ImGui::DestroyContext();
	SDL_Quit();

	if (restart)
	{
		// Todo: get rid of restart to change resolution.
		/*char restartPath[300]{};
		if (GetModuleFileNameA(nullptr, restartPath, 300))
		{
			STARTUPINFO si{};
			PROCESS_INFORMATION pi{};
			si.cb = sizeof si;
			if (CreateProcess(restartPath, nullptr, nullptr, nullptr,
			                  FALSE, 0, nullptr, nullptr, &si, &pi))
			{
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
		}*/
	}

	return return_value;
}

void winmain::RenderUi()
{
	if(custom_font != nullptr)
		ImGui::PushFont(custom_font);

	// No demo window in release to save space
#ifndef NDEBUG
	if (ShowImGuiDemo)
		ImGui::ShowDemoWindow();
#endif

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Game"))
		{
			if (ImGui::MenuItem("New Game", "F2"))
			{
				new_game();
			}
			if (ImGui::MenuItem("Launch Ball", nullptr, false, LaunchBallEnabled))
			{
				end_pause();
				pb::launch_ball();
			}

			if (ImGui::MenuItem("Pause/ Resume Game", "F3"))
			{
				pause();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("High Scores...", nullptr, false, HighScoresEnabled))
			{
				if (!single_step)
					pause();
				pb::high_scores();
			}
			if (ImGui::MenuItem("Demo", nullptr, DemoActive))
			{
				end_pause();
				pb::toggle_demo();
			}
			if (ImGui::MenuItem("Exit"))
			{
				SDL_Event event{SDL_QUIT};
				SDL_PushEvent(&event);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Options"))
		{
			if (ImGui::MenuItem("Full Screen", "F4", options::Options.FullScreen))
			{
				options::toggle(Menu1_Full_Screen);
			}
			if (ImGui::BeginMenu("Select Players"))
			{
				if (ImGui::MenuItem("1 Player", nullptr, options::Options.Players == 1))
				{
					options::toggle(Menu1_1Player);
					new_game();
				}
				if (ImGui::MenuItem("2 Players", nullptr, options::Options.Players == 2))
				{
					options::toggle(Menu1_2Players);
					new_game();
				}
				if (ImGui::MenuItem("3 Players", nullptr, options::Options.Players == 3))
				{
					options::toggle(Menu1_3Players);
					new_game();
				}
				if (ImGui::MenuItem("4 Players", nullptr, options::Options.Players == 4))
				{
					options::toggle(Menu1_4Players);
					new_game();
				}
				ImGui::EndMenu();
			}

			ImGui::Separator();

			if(ImGui::BeginMenu("MIDI Backend"))
			{
				int count = 0;
				for(auto kvp : _MixerMidiDevices)
				{
					if(kvp.second.length() > 0)
					{
						if(ImGui::MenuItem(kvp.second.c_str(), nullptr, options::Options.CurMidiBackend == (int)kvp.first, true))
						{
							printf("Changing MIDI Backend (%d -> %d)\n", options::Options.CurMidiBackend, (int)kvp.first);
							options::Options.CurMidiBackend = (int)kvp.first;
							options::set_int("CurMidiBackend", (int)kvp.first);
							Mix_SetMidiPlayer(options::Options.CurMidiBackend);
							midi::music_reboot();
						}
						count++;
					}
				}

				ImGui::EndMenu();
			}

			if(options::Options.CurMidiBackend == MIDI_ADLMIDI)
			{
				// PICK EMULATOR
				if(ImGui::BeginMenu("OPL3 Settings"))
				{
					for(auto kvp : _MixerADLEmus)
					{
						if(ImGui::MenuItem(kvp.second.c_str(), nullptr, options::Options.ADLEmu == (int)kvp.first, true))
						{
							printf("Changing ADL Emu (%d -> %d)\n", options::Options.ADLEmu, (int)kvp.first);
							options::Options.ADLEmu = (int)kvp.first;
							options::set_int("ADLEmu", (int)kvp.first);
							Mix_ADLMIDI_setEmulator(options::Options.ADLEmu);
							Mix_ADLMIDI_setChipsCount(1);
							midi::music_reboot();
						}
					}

					ImGui::EndMenu();
				}

				// PICK SOUND BANK
				if(ImGui::BeginMenu("OPL3 Banks"))
				{
					const int currentBankID = Mix_ADLMIDI_getBankID();
					int totalBankNames = Mix_ADLMIDI_getTotalBanks();
					auto *bankNameArray = Mix_ADLMIDI_getBankNames();
					int i = 0;
					for(i = 0; i < totalBankNames; i++)
					{
						if(ImGui::MenuItem(bankNameArray[i], nullptr, currentBankID == i, true))
						{
							options::Options.ADLBank = i;
							options::set_int("ADLBank", i);
							Mix_ADLMIDI_setBankID(i);
							Mix_ADLMIDI_setChipsCount(1);
							midi::music_reboot();
						}
					}
					

					ImGui::EndMenu();
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Sound", nullptr, options::Options.Sounds))
			{
				options::toggle(Menu1_Sounds);
			}
			if (ImGui::MenuItem("Music", nullptr, options::Options.Music))
			{
				options::toggle(Menu1_Music);
			}
			ImGui::Separator();

			if (ImGui::MenuItem("Player Controls...", "F8"))
			{
				if (!single_step)
					pause();
				options::keyboard();
			}
			if (ImGui::BeginMenu("Table Resolution"))
			{
				if (ImGui::MenuItem("Not implemented"))
				{
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Window"))
			{
				if (ImGui::MenuItem("Uniform Scaling", nullptr, options::Options.UniformScaling))
				{
					options::toggle(Menu1_WindowUniformScale);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
#ifndef NDEBUG
			if (ImGui::MenuItem("ImGui Demo", nullptr, ShowImGuiDemo))
			{
				ShowImGuiDemo ^= true;
			}
#endif
			if (ImGui::MenuItem("Help Topics", "F1"))
			{
				if (!single_step)
					pause();
				help_introduction();
			}
			ImGui::Separator();

			if (ImGui::MenuItem("About Pinball"))
			{
				if (!single_step)
					pause();
				ShowAboutDialog = true;
			}
			ImGui::EndMenu();
		}

		if(ImGui::IsWindowAppearing()
#ifdef VITA
			|| ImGui::GetFocusID() == 0
#endif
			)
		{
#ifdef VITA
			needs_focus = false;
#endif
			auto defaultMenu = ImGui::GetID("Launch Ball");
			auto parentMenu = ImGui::GetID("Game");
#if defined(VITA) && !defined(NDEBUG)
			debugNetPrintf(DEBUG, "FOCUSING %u & its child %u\n", parentMenu, defaultMenu);
#endif
			ImGui::OpenPopup(parentMenu);
			ImGui::SetFocusID(defaultMenu, ImGui::GetCurrentWindow());
			GImGui->NavId = defaultMenu;
		}

		ImGui::EndMainMenuBar();
	}

	a_dialog();
	high_score::RenderHighScoreDialog();

	if(custom_font != nullptr)
		ImGui::PopFont();
}

int winmain::event_handler(const SDL_Event* event)
{
	ImGui_ImplSDL2_ProcessEvent(event);

	if (ImIO->WantCaptureMouse)
	{
		if (mouse_down)
		{
			mouse_down = 0;
#ifndef VITA
			SDL_ShowCursor(SDL_ENABLE);
			SDL_SetWindowGrab(MainWindow, SDL_FALSE);
#endif
		}
		switch (event->type)
		{
		case SDL_FINGERMOTION:
#ifdef VITA
			debugNetPrintf(DEBUG, "HEY!!!! WE GOT A TOUCH EVENT!!!! pos: %.2f, %.2f; delta: %.2f, %.2f\n", event->tfinger.x, event->tfinger.y, event->tfinger.dx, event->tfinger.dy);
#endif
        break;
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEWHEEL:
			return 1;
		default: ;
		}
	}
	if (ImIO->WantCaptureKeyboard)
	{
		switch (event->type)
		{
	#ifdef VITA
		case SDL_TEXTEDITING:
		case SDL_TEXTINPUT:
			high_score::vita_done_input();
	#endif
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			return 1;
		default: ;
		}
	}
	if(ImIO->NavActive && ImIO->NavVisible)
	{
		switch(event -> type)
		{
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			return 1;
		default: ;
		}
	}

	switch (event->type)
	{
	case SDL_QUIT:
		end_pause();
		bQuit = 1;
		fullscrn::shutdown();
		return_value = 0;
		return 0;
#ifdef VITA
	case SDL_JOYBUTTONDOWN:
		pb::keydown(vita_translate_joystick(event->jbutton.button));
		break;
	case SDL_JOYBUTTONUP:
		if(((VITA_BUTTONS)event->jbutton.button) == START)
			vita_set_imgui_enabled(!vita_imgui_enabled);
		else
			pb::keyup(vita_translate_joystick(event->jbutton.button));
		break;
#endif
	case SDL_KEYUP:
		pb::keyup(event->key.keysym.sym);
		break;
	case SDL_KEYDOWN:
		if (!event->key.repeat)
			pb::keydown(event->key.keysym.sym);
		switch (event->key.keysym.sym)
		{
		case SDLK_ESCAPE:
			if (options::Options.FullScreen)
				options::toggle(Menu1_Full_Screen);
			SDL_MinimizeWindow(MainWindow);
			break;
		case SDLK_F1:
			help_introduction();
			break;
		case SDLK_F2:
			new_game();
			break;
		case SDLK_F3:
			pause();
			break;
		case SDLK_F4:
			options::toggle(Menu1_Full_Screen);
			break;
		case SDLK_F5:
			options::toggle(Menu1_Sounds);
			break;
		case SDLK_F6:
			options::toggle(Menu1_Music);
			break;
		case SDLK_F8:
			if (!single_step)
				pause();
			options::keyboard();
			break;
		default:
			break;
		}

		if (!pb::cheat_mode)
			break;

		switch (event->key.keysym.sym)
		{
		case SDLK_h:
			DispGRhistory = 1;
			break;
		case SDLK_y:
			SDL_SetWindowTitle(MainWindow, "Pinball");
			DispFrameRate = DispFrameRate == 0;
			break;
		case SDLK_F1:
			pb::frame(10);
			break;
		case SDLK_F10:
			single_step = single_step == 0;
			if (single_step == 0)
				no_time_loss = 1;
			break;
		default:
			break;
		}
		break;
#ifndef VITA
	case SDL_MOUSEBUTTONDOWN:
		switch (event->button.button)
		{
		case SDL_BUTTON_LEFT:
			if (pb::cheat_mode)
			{
				mouse_down = 1;
				last_mouse_x = event->button.x;
				last_mouse_y = event->button.y;
				SDL_ShowCursor(SDL_DISABLE);
				SDL_SetWindowGrab(MainWindow, SDL_TRUE);
			}
			else
				pb::keydown(options::Options.LeftFlipperKey);
			break;
		case SDL_BUTTON_RIGHT:
			if (!pb::cheat_mode)
				pb::keydown(options::Options.RightFlipperKey);
			break;
		case SDL_BUTTON_MIDDLE:
			pb::keydown(options::Options.PlungerKey);
			break;
		default:
			break;
		}
		break;
	case SDL_MOUSEBUTTONUP:
		switch (event->button.button)
		{
		case SDL_BUTTON_LEFT:
			if (mouse_down)
			{
				mouse_down = 0;
				SDL_ShowCursor(SDL_ENABLE);
				SDL_SetWindowGrab(MainWindow, SDL_FALSE);
			}
			if (!pb::cheat_mode)
				pb::keyup(options::Options.LeftFlipperKey);
			break;
		case SDL_BUTTON_RIGHT:
			if (!pb::cheat_mode)
				pb::keyup(options::Options.RightFlipperKey);
			break;
		case SDL_BUTTON_MIDDLE:
			pb::keyup(options::Options.PlungerKey);
			break;
		default:
			break;
		}
		break;
#endif
	case SDL_WINDOWEVENT:
		switch (event->window.event)
		{
		case SDL_WINDOWEVENT_FOCUS_GAINED:
		case SDL_WINDOWEVENT_TAKE_FOCUS:
		case SDL_WINDOWEVENT_EXPOSED:
		case SDL_WINDOWEVENT_SHOWN:
			activated = 1;
			Sound::Activate();
			if (options::Options.Music && !single_step)
				midi::play_pb_theme(0);
			no_time_loss = 1;
			has_focus = 1;
			gdrv::get_focus();
			pb::paint();
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
		case SDL_WINDOWEVENT_HIDDEN:
			activated = 0;
			fullscrn::activate(0);
			options::Options.FullScreen = 0;
			Sound::Deactivate();
			midi::music_stop();
		#ifndef VITA
			has_focus = 0;
		#endif
			gdrv::get_focus();
			pb::loose_focus();
			break;
		case SDL_WINDOWEVENT_SIZE_CHANGED:
		case SDL_WINDOWEVENT_RESIZED:
			fullscrn::window_size_changed();
			break;
		default: ;
		}
		break;
	default:
#if defined(VITA) && !defined(NDEBUG)
		if(event->type != 1536 && event->type != 1619 && event->type != 1616)
			debugNetPrintf(DEBUG, "Default Event Type: %d\n", event->type);
#endif
		break;
	}

	return 1;
}

int winmain::ProcessWindowMessages()
{
	SDL_Event event;
	if (has_focus && !single_step)
	{
		while (SDL_PollEvent(&event))
		{
			if (!event_handler(&event))
				return 0;
		}

		return 1;
	}
	
	SDL_WaitEvent(&event);
	return event_handler(&event);
}

void winmain::memalloc_failure()
{
	midi::music_stop();
	Sound::Close();
	gdrv::uninit();
	char* caption = pinball::get_rc_string(170, 0);
	char* text = pinball::get_rc_string(179, 0);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption, text, MainWindow);
	std::exit(1);
}

void winmain::a_dialog()
{
	if (ShowAboutDialog == true)
	{
		ShowAboutDialog = false;
		ImGui::OpenPopup("About");
	}

	bool unused_open = true;
	if (ImGui::BeginPopupModal("About", &unused_open, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("3D Pinball for Windows - Space Cadet");
		ImGui::TextUnformatted("Decompiled -> Ported to SDL by k4zmu2a");
#ifdef VITA
		ImGui::Separator();
		ImGui::TextUnformatted("Ported to Vita by Axiom (axiom@ignoresolutions.xyz)");
		ImGui::TextUnformatted("https://github.com/suicvne/SpaceCadetPinball_Vita/");
#endif
		ImGui::Separator();

		if (ImGui::Button("Ok"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void winmain::end_pause()
{
	if (single_step)
	{
		pb::pause_continue();
		no_time_loss = 1;
	}
}

void winmain::new_game()
{
	end_pause();
	pb::replay_level(0);
}

void winmain::pause()
{
	pb::pause_continue();
	no_time_loss = 1;
}

void winmain::help_introduction()
{
}

void winmain::Restart()
{
	restart = true;
	SDL_Event event{SDL_QUIT};
	SDL_PushEvent(&event);
}
