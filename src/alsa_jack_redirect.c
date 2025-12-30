#define _GNU_SOURCE
#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int jackshim_verbose = 0;
static int jackshim_disabled = 0;
static const char *jackshim_target = "default";

static int (*real_snd_pcm_open)(
	snd_pcm_t **pcm,
	const char *name,
	snd_pcm_stream_t stream,
	int mode) = NULL;

static int (*real_snd_pcm_open_lconf)(
	snd_pcm_t **pcm,
	const char *name,
	snd_pcm_stream_t stream,
	int mode,
	snd_config_t *lconf) = NULL;

static int (*real_snd_pcm_open_noupdate)(
	snd_pcm_t **pcm,
	const char *name,
	snd_pcm_stream_t stream,
	int mode) = NULL;

static void jackshim_log(const char *fmt, ...)
{
	if (!jackshim_verbose)
		return;
	va_list ap;
	va_start(ap, fmt);
	fputs("[jackshim] ", stderr);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
}

static int has_prefix(const char *s, const char *prefix)
{
	size_t n = strlen(prefix);
	return strncmp(s, prefix, n) == 0;
}

static int should_redirect(const char *name)
{
	if (!name)
		return 0;
	/* Do not interfere with explicit default or other virtual PCMs. */
	if (!strcmp(name, "default"))
		return 0;
	/* Core problematic PCMs used by Proton / winealsa. */
	if (!strcmp(name, "dmix") || !strcmp(name, "dsnoop"))
		return 1;
	if (has_prefix(name, "dmix:") || has_prefix(name, "dsnoop:"))
		return 1;
	/* Variants that wrap the above in plug. */
	if (has_prefix(name, "plug:dmix") || has_prefix(name, "plug:dsnoop"))
		return 1;
	/* Direct hardware opens skip JACK entirely and often fail here. */
	if (has_prefix(name, "hw:") || has_prefix(name, "plughw:"))
		return 1;
	return 0;
}

static const char *jackshim_map_name(const char *name)
{
	if (jackshim_disabled)
		return name;
	if (!name)
		return NULL;
	if (!should_redirect(name))
		return name;
	jackshim_log("redirecting \"%s\" -> \"%s\"", name, jackshim_target);
	return jackshim_target;
}

static void jackshim_init(void) __attribute__((constructor));

static void jackshim_init(void)
{
	const char *v;

	/* Check disable flag first so we can bail out early if requested. */
	v = getenv("JACKSHIM_DISABLE");
	if (v && *v && strcmp(v, "0") != 0)
		jackshim_disabled = 1;

	v = getenv("JACKSHIM_VERBOSE");
	if (v && *v && strcmp(v, "0") != 0)
		jackshim_verbose = 1;

	v = getenv("JACKSHIM_TARGET");
	if (v && *v)
		jackshim_target = v;
	else
		jackshim_target = "default";

	if (!real_snd_pcm_open)
		real_snd_pcm_open = dlsym(RTLD_NEXT, "snd_pcm_open");
	if (!real_snd_pcm_open_lconf)
		real_snd_pcm_open_lconf = dlsym(RTLD_NEXT, "snd_pcm_open_lconf");
	if (!real_snd_pcm_open_noupdate)
		real_snd_pcm_open_noupdate = dlsym(RTLD_NEXT, "snd_pcm_open_noupdate");

	if (!real_snd_pcm_open)
		jackshim_log("warning: failed to resolve real snd_pcm_open");
}

int snd_pcm_open(
	snd_pcm_t **pcm,
	const char *name,
	snd_pcm_stream_t stream,
	int mode)
{
	const char *use = jackshim_map_name(name);
	if (!real_snd_pcm_open)
		real_snd_pcm_open = dlsym(RTLD_NEXT, "snd_pcm_open");
	if (!real_snd_pcm_open)
		return -ENODEV;
	return real_snd_pcm_open(pcm, use, stream, mode);
}

int snd_pcm_open_lconf(
	snd_pcm_t **pcm,
	const char *name,
	snd_pcm_stream_t stream,
	int mode,
	snd_config_t *lconf)
{
	const char *use = jackshim_map_name(name);
	if (!real_snd_pcm_open_lconf)
		real_snd_pcm_open_lconf = dlsym(RTLD_NEXT, "snd_pcm_open_lconf");
	if (!real_snd_pcm_open_lconf)
		return -ENODEV;
	return real_snd_pcm_open_lconf(pcm, use, stream, mode, lconf);
}

int snd_pcm_open_noupdate(
	snd_pcm_t **pcm,
	const char *name,
	snd_pcm_stream_t stream,
	int mode)
{
	const char *use = jackshim_map_name(name);
	if (!real_snd_pcm_open_noupdate)
		real_snd_pcm_open_noupdate = dlsym(RTLD_NEXT, "snd_pcm_open_noupdate");
	if (!real_snd_pcm_open_noupdate)
		return -ENODEV;
	return real_snd_pcm_open_noupdate(pcm, use, stream, mode);
}

