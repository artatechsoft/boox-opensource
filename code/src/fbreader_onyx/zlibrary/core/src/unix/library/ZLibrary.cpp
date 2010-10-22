/*
 * Copyright (C) 2004-2009 Geometer Plus <contact@geometerplus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <locale.h>
#ifndef _WINDOWS
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#endif

#include <algorithm>

#include <ZLibrary.h>
#include <ZLStringUtil.h>

#include "ZLibraryImplementation.h"

const std::string ZLibrary::FileNameDelimiter("/");
const std::string ZLibrary::PathDelimiter(":");
const std::string ZLibrary::EndOfLine("\n");

void ZLibrary::initLocale() {
#ifndef _WINDOWS
	const char *locale = setlocale(LC_MESSAGES, ""); 
	if (locale != 0) {
		std::string sLocale = locale;
		const int dotIndex = sLocale.find('.');
		if (dotIndex != -1) {
			sLocale = sLocale.substr(0, dotIndex);
		}
		const int dashIndex = std::min(sLocale.find('_'), sLocale.find('-'));
		if (dashIndex == -1) {
			ourLanguage = sLocale;
		} else {
			ourLanguage = sLocale.substr(0, dashIndex);
			ourCountry = sLocale.substr(dashIndex + 1);
			if ((ourLanguage == "es") && (ourCountry != "ES")) {
				ourCountry = "LA";
			}
		}
	}
#endif
}

ZLibraryImplementation *ZLibraryImplementation::Instance = 0;

ZLibraryImplementation::ZLibraryImplementation() {
	Instance = this;
}

ZLibraryImplementation::~ZLibraryImplementation() {
}

bool ZLibrary::init(int &argc, char **&argv) {
#ifdef ZLSHARED
	const std::string pluginPath = std::string(LIBDIR) + "/zlibrary/ui";

	void *handle = 0;

	if ((argc > 2) && std::string("-zlui") == argv[1]) {
		handle = dlopen((pluginPath + "/zlui-" + argv[2] + ".so").c_str(), RTLD_NOW);
		argc -= 2;
		argv += 2;
	}

	if (handle == 0) {
		DIR *dir = opendir(pluginPath.c_str());
		if (dir == 0) {
			return false;
		}
		std::vector<std::string> names;
		const dirent *file;
		struct stat fileInfo;
		while ((file = readdir(dir)) != 0) {
			const std::string shortName = file->d_name;
			if ((shortName.substr(0, 5) != "zlui-") ||
					!ZLStringUtil::stringEndsWith(shortName, ".so")) {
				continue;
			}
			const std::string fullName = pluginPath + "/" + shortName;
			stat(fullName.c_str(), &fileInfo);
			if (!S_ISREG(fileInfo.st_mode)) {
				continue;
			}
			names.push_back(fullName);
		}
		closedir(dir);

		std::sort(names.begin(), names.end());
		for (std::vector<std::string>::const_iterator it = names.begin();
				 (it != names.end()) && (handle == 0); ++it) {
			handle = dlopen(it->c_str(), RTLD_NOW);
		}

		if (handle == 0) {
			return false;
		}
	}
	dlerror();

	void (*initLibrary)();
	*(void**)&initLibrary = dlsym(handle, "initLibrary");
	if (dlerror() != 0) {
		return false;
	}
#endif /* ZLSHARED */
	initLibrary();

	if (ZLibraryImplementation::Instance == 0) {
		return false;
	}

	ZLibraryImplementation::Instance->init(argc, argv);
	return true;
}

ZLPaintContext *ZLibrary::createContext() {
	return ZLibraryImplementation::Instance->createContext();
}

void ZLibrary::run(ZLApplication *application) {
	ZLibraryImplementation::Instance->run(application);
}