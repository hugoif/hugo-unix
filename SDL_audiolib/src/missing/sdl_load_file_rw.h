// This is copyrighted software. More information is at the end of this file.
#pragma once
#include "aulib_export.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDL_RWops SDL_RWops;
#define SDL_LoadFile_RW SDL_LoadFile_RW_missing

AULIB_NO_EXPORT void* SDL_LoadFile_RW_missing(SDL_RWops* src, size_t* datasize, int freesrc);

#ifdef __cplusplus
}
#endif

/*

Copyright (C) 2014, 2015, 2016, 2017, 2018, 2019 Nikos Chantziaras.

This file is part of SDL_audiolib.

SDL_audiolib is free software: you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version.

SDL_audiolib is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details.

You should have received a copy of the GNU Lesser General Public License
along with SDL_audiolib. If not, see <http://www.gnu.org/licenses/>.

*/
