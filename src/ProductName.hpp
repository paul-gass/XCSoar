// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
// Fork: OpenGlide Nav — live routing for glider pilots

#pragma once

/**
 * Product name configuration — OpenGlide Nav fork of XCSoar.
 *
 * This fork adds live routing, weather overlays and OGN thermals
 * to XCSoar's proven navigation core. The XCSoar engine is unchanged;
 * OpenGlide code lives in src/OpenGlide/.
 */

#ifndef PRODUCT_NAME
#define PRODUCT_NAME "OpenGlide Nav"
#endif

/* Lowercase version for file paths and Unix conventions */
#ifndef PRODUCT_NAME_LC
#define PRODUCT_NAME_LC "openglidenav"
#endif

/* ASCII version for help text and logging */
#define PRODUCT_NAME_A PRODUCT_NAME

/* Android package name (reverse domain notation) */
#ifndef ANDROID_PACKAGE
#define ANDROID_PACKAGE "org.openglide.nav"
#endif

/* HTTPS URLs for --help footers, credits, etc. */
#ifndef PRODUCT_WEB_SITE_URL
#define PRODUCT_WEB_SITE_URL "https://openglide.org/"
#endif

#ifndef PRODUCT_BUGS_URL
#define PRODUCT_BUGS_URL "https://github.com/paul-gass/open-glide/issues"
#endif

/* Data directory names by platform:
 * - Windows: %LOCALAPPDATA%\OpenGlide NavData
 * - macOS: ~/OpenGlide NavData
 * - iOS: ~/Documents/OpenGlide NavData
 * - Linux/Unix: ~/.openglidenav
 * - System-wide: /etc/openglidenav
 */
#ifndef PRODUCT_DATA_DIR
#define PRODUCT_DATA_DIR PRODUCT_NAME "Data"
#endif

#define PRODUCT_UNIX_SYSCONF_DIR "/etc/" PRODUCT_NAME_LC
#define PRODUCT_UNIX_HOME_DIR "." PRODUCT_NAME_LC
