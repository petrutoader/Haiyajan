/**
 * A simple and fast Libretro frontend.
 * Copyright (C) 2020  Mahyar Koshkouei
 *
 * This is free software, and you are welcome to redistribute it under the terms
 * of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * See the LICENSE file for more details.
 */

#include <SDL.h>
#include <stdlib.h>

#ifdef _WIN32
#include <stdio.h>
#endif

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include <optparse.h>

#include <font.h>
#include <input.h>
#include <load.h>
#include <play.h>
#include <rec.h>
#include <timer.h>
#include <util.h>
#include <sig.h>

#define PROG_NAME       "Haiyajan"
#define MAX_TITLE_LEN   56

#ifndef GIT_VERSION
#define GIT_VERSION     "NONE"
#endif

static void prerun_checks(void)
{
	SDL_version compiled;
	SDL_version linked;

	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);

	if(compiled.major != linked.major)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM,
				"The major version of SDL2 loaded (%d) does "
				"not match the version from which Haiyajan was "
				"compiled with (%d). "
				"Please recompile Haiyajan and try again.",
				linked.major, compiled.major);
		exit(EXIT_FAILURE);
	}

	if(SDL_VERSIONNUM(compiled.major, compiled.minor, compiled.patch) !=
	   SDL_VERSIONNUM(linked.major, linked.minor, linked.patch))
	{
		SDL_LogWarn(
			SDL_LOG_CATEGORY_SYSTEM,
			"The version of SDL2 loaded (%d.%d.%d) is different to "
			"the version that Haiyajan was compiled with "
			"(%d.%d.%d).",
			linked.major, linked.minor, linked.patch,
			compiled.major, compiled.minor, compiled.patch);
	}
}

static void print_info(void)
{
	struct features_s {
		SDL_bool (*get_cpu_feat)(void);
		const char *const feat_name;
	};
	const struct features_s cpu_features[] = {
		{SDL_Has3DNow,   "3DNow"},
		{SDL_HasAVX,     "AVX"},
		{SDL_HasAVX2,    "AVX2"},
		{SDL_HasAltiVec, "VMX"},
		{SDL_HasMMX,     "MMX"},
		{SDL_HasRDTSC,   "RDTSC"},
		{SDL_HasSSE,     "SSE"},
		{SDL_HasSSE2,    "SSE2"},
		{SDL_HasSSE3,    "SSE3"},
		{SDL_HasSSE41,   "SSE41"},
		{SDL_HasSSE42,   "SSE42"}
	};
	char str_feat[128] = "\0";

	for(size_t i = 0; i < SDL_arraysize(cpu_features); i++)
	{
		if(cpu_features[i].get_cpu_feat() == SDL_FALSE)
			continue;

		SDL_strlcat(str_feat, cpu_features[i].feat_name,
			    SDL_arraysize(str_feat));
		SDL_strlcat(str_feat, " ", SDL_arraysize(str_feat));
	}

	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
		    "%s platform, %d core CPU, featuring %s\n",
		    SDL_GetPlatform(), SDL_GetCPUCount(),
		    str_feat[0] == '\0' ? "no additional instructions"
					: str_feat);
}

static void print_help(void)
{
	const int num_drivers = SDL_GetNumVideoDrivers();
	const int num_rends = SDL_GetNumRenderDrivers();
	const int num_audio = SDL_GetNumAudioDrivers();

	fprintf(stderr, "\n"
			"Usage: haiyajan [OPTIONS] -L CORE FILE\n"
			"  -h, --help      Show this help message.\n"
			"      --version   Print version information.\n"
			"  -L, --libretro  Path to libretro core.\n"
			"  -I, --info      Print statistics onscreen.\n"
			"  -b, --benchmark Benchmark and print average frames"
			" per second.\n"
			"  -v, --verbose   Print verbose log messages.\n"
			"  -V, --video     Video driver to use\n");

	for(int index = 0; index < num_drivers; index++)
	{
		fprintf(stderr, "%s%s", index != 0 ?
					", " : "\nAvailable video drivers: ",
			SDL_GetVideoDriver(index));
	}

	for(int index = 0; index < num_rends; index++)
	{
		SDL_RendererInfo info;
		SDL_GetRenderDriverInfo(index, &info);
		fprintf(stderr, "%s%s", index != 0 ?
					", " : "\nAvailable render drivers: ",
			info.name);
	}

	for(int index = 0; index < num_audio; index++)
	{
		fprintf(stderr, "%s%s", index != 0 ?
					", " : "\nAvailable audio drivers: ",
			SDL_GetAudioDriver(index));
	}

	fprintf(stderr, "\nThe following environment variables may be used to "
			"select a specific driver:\n"
			"  SDL_VIDEODRIVER\n"
			"  SDL_RENDER_DRIVER\n"
			"  SDL_AUDIODRIVER\n");
}

static void free_settings(struct core_ctx_s *ctx)
{
	if(ctx->file_core != NULL)
	{
		SDL_free(ctx->file_core);
		ctx->file_core = NULL;
	}

	if(ctx->file_content != NULL)
	{
		SDL_free(ctx->file_content);
		ctx->file_content = NULL;
	}
}

static void apply_settings(char **argv, struct core_ctx_s *ctx)
{
	const struct optparse_long longopts[] = {
			{"libretro",  'L', OPTPARSE_REQUIRED},
			{"info",      'I', OPTPARSE_NONE},
			{"verbose",   'v', OPTPARSE_NONE},
			{"video",     'V', OPTPARSE_REQUIRED},
			{"version",   1,   OPTPARSE_NONE},
			{"benchmark", 'b', OPTPARSE_OPTIONAL},
			{"help",      'h', OPTPARSE_NONE},
			{0}
		};
	int option;
	struct optparse options;
	char *rem_arg;
	uint_fast8_t video_init = 0;

	optparse_init(&options, argv);

	while((option = optparse_long(&options, longopts, NULL)) != -1)
	{
		switch(option)
		{
		case 'L':
			ctx->file_core = SDL_strdup(options.optarg);
			break;

		case 'I':
			ctx->stngs.vid_info = 1;
			break;

		case 'v':
			SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
			break;

		case 'V':
			if(video_init)
			{
				SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO,
					    "Previously initialised video driver "
					    "%s will be replaced with %s",
					    SDL_GetCurrentVideoDriver(),
					    options.optarg);
				SDL_VideoQuit();
				video_init = 0;
			}

			if(SDL_VideoInit(options.optarg) != 0)
			{
				SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
					    "Unable to initialise specified "
					    "video driver: %s", SDL_GetError());
			}
			else
			{
				SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO,
					    "%s was successfully initialised",
					    options.optarg);
				video_init = 1;
			}

			break;

		case 1:
			/* Version information has already been printed. */
			exit(EXIT_SUCCESS);

		case 'b':
			ctx->stngs.benchmark = 1;
			if(options.optarg != 0)
				ctx->stngs.benchmark_dur =
					SDL_atoi(options.optarg);

			if(ctx->stngs.benchmark_dur == 0)
				ctx->stngs.benchmark_dur = 20;

			SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO,
				    "Haiyajan will exit after performing a "
				    "benchmark for %d seconds",
				    ctx->stngs.benchmark_dur);
			break;

		case 'h':
			print_help();
			exit(EXIT_SUCCESS);

		case '?':
			SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s",
					options.errmsg);
			goto err;

		default:
			break;
		}
	}

	/* Print remaining arguments. */
	rem_arg = optparse_arg(&options);

	if(rem_arg != NULL)
		ctx->file_content = SDL_strdup(rem_arg);
	else
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"The path to the content file was not given");
		goto err;
	}

	if(ctx->file_core == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"The path to a libretro core was not given");
		goto err;
	}

	/* Initialise default video driver if not done so already. */
	if(video_init == 0 && SDL_VideoInit(NULL) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"Unable to initialise a video driver: %s",
				SDL_GetError());
		goto err;
	}

	return;

err:
	print_help();
	exit(EXIT_FAILURE);
}

static SDL_atomic_t screenshot_timeout;

void take_screenshot(struct core_ctx_s *const ctx)
{
	SDL_Surface *surf;

	/* Screenshots limited to 1 every second. Should not be used for
	 * recording game play as a video, as this function is too slow for
	 * that. */
	if(SDL_AtomicGet(&screenshot_timeout) != 0)
		return;

	SDL_AtomicSet(&screenshot_timeout, 1);
	set_atomic_timeout(1024, &screenshot_timeout, 0, "Enable Screenshot");

	surf = util_tex_to_surf(ctx->disp_rend, ctx->core_tex,
				&ctx->game_frame_res, ctx->env.flip);
	if(surf == NULL)
		goto err;

	rec_single_img(surf, ctx->core_log_name);

out:
	return;

err:
	SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
		    "Could not take screen capture: %s", SDL_GetError());
	goto out;
}

#if ENABLE_VIDEO_RECORDING == 1

void cap_frame(rec *vid, SDL_Renderer *rend, SDL_Texture *tex,
	       const SDL_Rect *src, SDL_RendererFlip flip)
{
	SDL_Surface *surf = util_tex_to_surf(rend, tex, src, flip);

	if(surf == NULL)
		return;

	rec_enc_video(vid, surf);
}

#endif

static void run(struct core_ctx_s *ctx)
{
	font_ctx *font;
	SDL_Event ev;
	Uint32 ticks_before, ticks_next, delta_ticks;
	struct timer_ctx_s tim;
	int tim_cmd;
	float fps = 0.0F;
	Uint32 fps_beg;
	const uint_fast8_t fps_calc_frame_dur = 64;
	uint_fast8_t fps_curr_frame_dur = fps_calc_frame_dur;
	Uint32 benchmark_beg;
	const Uint8 frame_skip_max = 4;
	Uint8 frame_skip_count = 4;

	input_init(&ctx->inp);
	ctx->fn.retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);

	font = FontStartup(ctx->disp_rend);
	if(font == NULL)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
			    "Unable to start font drawer: %s", SDL_GetError());
		/* Disable font drawing on error. */
		ctx->stngs.vid_info = 0;
	}

	{
		Uint32 lim = SDL_GetTicks();
		while(ctx->env.status_bits.valid_frame == 0)
		{
			play_frame(ctx);

			/* If the first frame isn't drawn within two seconds,
			 * break and begin showing screen anyway. */
			if(SDL_GetTicks() - lim > 2000U)
				break;

		}
	}

	tim_cmd = timer_init(&tim, ctx->av_info.timing.fps);
	ticks_before = fps_beg = benchmark_beg = SDL_GetTicks();

	while(ctx->env.status_bits.shutdown == 0)
	{
		static Uint32 frames = 0;

		while(SDL_PollEvent(&ev) != 0)
		{
			if(ev.type == SDL_QUIT)
				goto out;
			else if(INPUT_EVENT_CHK(ev.type))
				input_handle_event(&ctx->inp, &ev);
			else if(ev.type == ctx->inp.input_cmd_event &&
				!ctx->stngs.benchmark)
			{
				switch(ev.user.code)
				{
				case INPUT_EVENT_TOGGLE_INFO:
					ctx->stngs.vid_info =
						!ctx->stngs.vid_info;
					SDL_SetRenderDrawBlendMode(
						ctx->disp_rend,
						ctx->stngs.vid_info
						? SDL_BLENDMODE_BLEND
						: SDL_BLENDMODE_NONE);
					if(!ctx->stngs.vid_info)
						break;

					/* Reset FPS counter. */
					fps_beg = SDL_GetTicks();
					fps_curr_frame_dur = fps_calc_frame_dur;
					break;

				case INPUT_EVENT_TOGGLE_FULLSCREEN:
					ctx->stngs.fullscreen =
						!ctx->stngs.fullscreen;
					if(ctx->stngs.fullscreen)
						SDL_SetWindowFullscreen(
							ctx->win,
							SDL_WINDOW_FULLSCREEN_DESKTOP);
					else
						SDL_SetWindowFullscreen(
							ctx->win,
							0);
					break;

				case INPUT_EVENT_TAKE_SCREENSHOT:
					take_screenshot(ctx);
					break;

#if ENABLE_VIDEO_RECORDING == 1
				case INPUT_EVENT_RECORD_VIDEO_TOGGLE:
				{
					if(ctx->vid == NULL &&
					   ctx->env.status_bits.valid_frame)
					{
						char vidfile[64];
						gen_filename(vidfile,
							     ctx->core_log_name,
							     "h264");

						/* FIXME: add double to Sint32 sample compensation should the sample rate not be an integer. */
						ctx->vid = rec_init(vidfile,
								    ctx->game_frame_res.w,
								    ctx->game_frame_res.h,
								    ctx->av_info.timing.fps,
								    SDL_ceil(
									    ctx->av_info.timing.sample_rate));
						if(ctx->vid == NULL)
						{
							SDL_LogWarn(
								SDL_LOG_CATEGORY_VIDEO,
								"Unable to initialise libx264: %s",
								SDL_GetError());
							break;
						}

						SDL_LogInfo(
							SDL_LOG_CATEGORY_VIDEO,
							"Video recording started");
					}
					else if(ctx->vid != NULL)
					{
						rec_end(&ctx->vid);
					}
					break;
				}
#endif
				}
			}
			else if(ev.type == tim.timer_event)
			{
				switch(ev.user.code)
				{
				case TIMER_SPEED_UP_AGGRESSIVELY:
#if ENABLE_VIDEO_RECORDING == 1
					if(ctx->vid != NULL)
					{
						rec_speedup(ctx->vid);
						rec_speedup(ctx->vid);
						rec_speedup(ctx->vid);
						rec_speedup(ctx->vid);
					}
#endif

					break;

				case TIMER_SPEED_UP:
#if ENABLE_VIDEO_RECORDING == 1
					if(ctx->vid != NULL)
						rec_speedup(ctx->vid);
#endif
					break;

				case TIMER_OKAY:
				default:
#if ENABLE_VIDEO_RECORDING == 1
					if(ctx->vid != NULL)
						rec_relax(ctx->vid);
#endif
					break;
				}
			}
		}

		/* If in benchmark mode, run as fast as possible. */
		if(ctx->stngs.benchmark)
			tim_cmd = 0;

		frames++;

#if ENABLE_VIDEO_RECORDING == 1
		if(tim_cmd < 0 && ctx->vid == NULL)
#else
			if(tim_cmd < 0)
#endif
		{
			/* Disable video for the skipped frame to improve
			 * performance. But only when we're not recording a
			 * video. */
			ctx->env.status_bits.video_disabled = 1;

		}
		else if(tim_cmd > 0)
			SDL_Delay(tim_cmd);

		timer_profile_start(&tim);
		SDL_SetRenderDrawColor(ctx->disp_rend, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderClear(ctx->disp_rend);
		play_frame(ctx);
		SDL_RenderCopyEx(ctx->disp_rend, ctx->core_tex,
				 &ctx->game_frame_res, NULL, 0.0, NULL,
				 ctx->env.flip);

		if(ctx->stngs.vid_info)
		{
			static char busy_str[10] = {'\0'};
			static char fps_str[10] = {'\0'};
			static char acc_str[10] = {'\0'};
			static char aud_str[10] = {'\0'};
			static char frames_str[10] = {'\0'};
			const SDL_Rect font_bg =
				{
					.w = 10 * FONT_CHAR_WIDTH,
					.h = 5 * (FONT_CHAR_HEIGHT + 1),
					.x = 0,
					.y = 0
				};
			SDL_Rect dim = {.w = 1, .h = 1, .x = 0, .y = 0};
			Uint32 ticks_busy = SDL_GetTicks();
			Uint32 busy_diff = ticks_busy - ticks_before;

			/* Only update every five frames to make values
			 * readable. */
			if(fps_curr_frame_dur % 5 == 0)
			{
				SDL_snprintf(busy_str, 10, "%6u ms", busy_diff);
				SDL_snprintf(acc_str, 10, "%6.2f ms",
					     tim.timer_accumulator);
				SDL_snprintf(aud_str, 10, "%6u",
					     (unsigned)(
						     SDL_GetQueuedAudioSize(
							     ctx->audio_dev)
						     /
						     sizeof(Uint16)
						     / 2U));
				SDL_snprintf(frames_str, 10, "%6u", frames);
			}

			/* Update only after FPS has been recalculated. */
			if(fps_curr_frame_dur == fps_calc_frame_dur)
				SDL_snprintf(fps_str, 10, "%6.2f Hz", fps);

			SDL_SetRenderDrawColor(ctx->disp_rend,
					       0x00, 0x00, 0x00, 0x40);
			SDL_RenderFillRect(ctx->disp_rend, &font_bg);

			SDL_SetRenderDrawColor(ctx->disp_rend,
					       0xFF, 0xFF, 0xFF, 0xFF);
			FontPrintToRenderer(font, busy_str, &dim);

			dim.y += FONT_CHAR_HEIGHT + 1;
			FontPrintToRenderer(font, fps_str, &dim);

			dim.y += FONT_CHAR_HEIGHT + 1;
			FontPrintToRenderer(font, acc_str, &dim);

			dim.y += FONT_CHAR_HEIGHT + 1;
			FontPrintToRenderer(font, aud_str, &dim);

			dim.y += FONT_CHAR_HEIGHT + 1;
			FontPrintToRenderer(font, frames_str, &dim);

			SDL_SetRenderDrawColor(ctx->disp_rend,
					       0x00, 0x00, 0x00, 0x00);
		}

#if ENABLE_VIDEO_RECORDING == 1
		while(ctx->vid != NULL)
		{
			SDL_Rect loc =
				{
					.x = 0,
					.y = FONT_CHAR_WIDTH * 2,
					.w = 2,
					.h = 2
				};
			struct {
				Sint64 (*get_size)(rec *);

				char str[16];
			} sz_map[2] =
				{
					{rec_video_size, ""},
					{rec_audio_size, ""}
				};

			SDL_RenderGetLogicalSize(ctx->disp_rend, &loc.x, NULL);
			if(loc.x < 320)
				break;

			cap_frame(ctx->vid, ctx->disp_rend, ctx->core_tex,
				  &ctx->game_frame_res, ctx->env.flip);

			SDL_SetRenderDrawColor(ctx->disp_rend, UINT8_MAX, 0, 0,
					       SDL_ALPHA_OPAQUE);

			loc.x -= FONT_CHAR_WIDTH * loc.h * 5;
			FontPrintToRenderer(font, " REC", &loc);

			/* Print output file sizes so far. */
			loc.y += FONT_CHAR_HEIGHT * 2;
			loc.w = 1;
			loc.h = 1;

			for(uint_fast8_t i = 0;
			    i < (uint_fast8_t)SDL_arraysize(sz_map); i++)
			{
				/* Technically MiB and GiB. */
				const char prefix[5][3] =
					{
						" B", "KB", "MB", "GB", "TB"
					};
				Uint64 sz = sz_map[i].get_size(ctx->vid);
				Uint8 p = 0;

				while(sz > 1 * 1024)
				{
					sz >>= (Uint8)10;
					p++;
				}

				SDL_snprintf(sz_map[i].str,
					     SDL_arraysize(sz_map[0].str),
					     "%5" PRIu64 " %.2s",
					     sz, prefix[p]);
			}

			FontPrintToRenderer(font, sz_map[0].str, &loc);
			loc.y += FONT_CHAR_HEIGHT;
			FontPrintToRenderer(font, sz_map[1].str, &loc);

			SDL_SetRenderDrawColor(ctx->disp_rend,
					       0x00, 0x00, 0x00, 0x00);
			break;
		}
#endif

		/* If the user took a screen capture within the last second,
		 * respond in the interface. */
		if(SDL_AtomicGet(&screenshot_timeout) != 0)
		{
			SDL_Rect loc =
				{
					.x = 0,
					.y = FONT_CHAR_WIDTH * 2,
					.w = 2,
					.h = 2
				};
			SDL_SetRenderDrawColor(ctx->disp_rend, UINT8_MAX,
					       UINT8_MAX, 0, SDL_ALPHA_OPAQUE);
			SDL_RenderGetLogicalSize(ctx->disp_rend, &loc.x, NULL);
			loc.x -= FONT_CHAR_WIDTH * loc.h * 8;
			FontPrintToRenderer(font, "CAP", &loc);
			SDL_SetRenderDrawColor(ctx->disp_rend,
					       0x00, 0x00, 0x00, 0x00);
		}

		timer_profile_end(&tim);

		/* Only draw to screen if we're not falling behind. */
		if(tim_cmd >= 0 || frame_skip_count == 0)
		{
			SDL_RenderPresent(ctx->disp_rend);
			frame_skip_count = frame_skip_max;
		}
		else
			frame_skip_count--;

		/* Reset video_disabled for next frame. */
		ctx->env.status_bits.video_disabled = 0;

		ticks_next = SDL_GetTicks();
		delta_ticks = ticks_next - ticks_before;
		tim_cmd = timer_get_delay(&tim, delta_ticks);
		ticks_before = ticks_next;

		if(ctx->stngs.vid_info)
			fps_curr_frame_dur--;

		if(ctx->stngs.vid_info && fps_curr_frame_dur == 0)
		{
			Uint32 fps_delta;
			Uint32 fps_end = SDL_GetTicks();
			fps_delta = fps_end - fps_beg;
			fps = 1000.0F / ((float)fps_delta
					 / (float)fps_calc_frame_dur);
			fps_curr_frame_dur = fps_calc_frame_dur;
			fps_beg = fps_end;
		}

		while(ctx->stngs.benchmark)
		{
			uint_fast32_t elapsed;
			float bench_fps;
			static Uint32 bench_frames = 0;

			bench_frames++;
			elapsed = SDL_GetTicks() - benchmark_beg;
			if(elapsed < ctx->stngs.benchmark_dur * 1000)
				break;

			bench_fps = (float)bench_frames /
				    ((float)elapsed / 1000.0F);
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
				    "Benchmark: %.2f FPS", bench_fps);
			goto out;
		}
	}

out:
	util_exit_all();
#if ENABLE_VIDEO_RECORDING == 1
	rec_end(&ctx->vid);
#endif
	FontExit(font);
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	struct core_ctx_s ctx = {0};

	/* Ignore argc being unused warning. */
	(void)argc;

#ifdef _WIN32
	/* Windows (MinGW) does not unbuffer stderr by default. */
	setbuf(stderr, NULL);
#endif

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);

	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
		    "%s Libretro Interface -- %d.%d (GIT %s)\n", PROG_NAME,
		    REL_VERSION_MAJOR, REL_VERSION_MINOR, GIT_VERSION);

	init_sig(&ctx);
	print_info();
	prerun_checks();

#if SDL_VERSION_ATLEAST(2, 0, 13)
	SDL_SetHint(SDL_HINT_AUDIO_DEVICE_APP_NAME, PROG_NAME);
#endif

	if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER
		    | SDL_INIT_TIMER) != 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
				"SDL initialisation failed: %s",
				SDL_GetError());
		exit(EXIT_FAILURE);
	}

	apply_settings(argv, &ctx);

	ctx.win = SDL_CreateWindow(PROG_NAME, SDL_WINDOWPOS_UNDEFINED,
				   SDL_WINDOWPOS_UNDEFINED, 320, 240,
				   SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if(ctx.win == NULL)
		goto err;

	{
		Uint32 flags =
			SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
		if(!ctx.stngs.benchmark)
			flags |= SDL_RENDERER_PRESENTVSYNC;

		ctx.disp_rend = SDL_CreateRenderer(ctx.win, -1, flags);
	}

	if(ctx.disp_rend == NULL)
		goto err;

#if 1
	//SDL_SetHint(SDL_HINT_RENDER_OPENGL_SHADERS, "0");
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
			    SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif

	/* Allows for transparency on information display. */
	if(ctx.stngs.vid_info)
		SDL_SetRenderDrawBlendMode(ctx.disp_rend, SDL_BLENDMODE_BLEND);

	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
		       "Created window and renderer");

	if(load_libretro_core(ctx.file_core, &ctx))
		goto err;

	/* TODO:
	 * - Check that input file is supported by core
	 */
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
		    "Libretro core \"%.32s\" loaded successfully.",
		    ctx.sys_info.library_name);

#if SDL_VERSION_ATLEAST(2, 0, 13)
	SDL_SetHint(SDL_HINT_AUDIO_DEVICE_STREAM_NAME, ctx.sys_info.library_name);
#endif

	{
		char title[MAX_TITLE_LEN];
		SDL_snprintf(title, MAX_TITLE_LEN, "%s: %s", PROG_NAME,
			     ctx.sys_info.library_name);
		SDL_SetWindowTitle(ctx.win, title);
	}

	play_init_cb(&ctx);

	if(load_libretro_file(&ctx) != 0)
		goto err;

	if(play_init_av(&ctx) != 0)
		goto err;

	SDL_SetWindowMinimumSize(ctx.win, ctx.game_max_res.w,
				 ctx.game_max_res.h);
	SDL_SetWindowSize(ctx.win, ctx.game_max_res.w, ctx.game_max_res.h);
	SDL_RenderSetLogicalSize(ctx.disp_rend, ctx.game_max_res.w,
				 ctx.game_max_res.h);

	run(&ctx);

	ret = EXIT_SUCCESS;

out:

	if(ctx.env.status_bits.game_loaded)
		unload_libretro_file(&ctx);

	if(ctx.env.status_bits.core_init)
	{
		unload_libretro_core(&ctx);
		play_deinit_cb(&ctx);
	}

	SDL_DestroyRenderer(ctx.disp_rend);
	SDL_DestroyWindow(ctx.win);
	SDL_VideoQuit();
	SDL_Quit();
	free_settings(&ctx);

	if(ret == EXIT_SUCCESS)
	{
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
			    "Exiting gracefully.");
	}

	exit(ret);

err:
	SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
			"Exiting due to an error. %s", SDL_GetError());

	goto out;
}
