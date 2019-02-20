/*
  SDL_audiolib - An audio decoding, resampling and mixing library
  Copyright (C) 2014  Nikos Chantziaras

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/
#ifndef AULIB_VERSION_H
#define AULIB_VERSION_H

/*!
 * This macro expands to a string literal containing the version of SDL_audiolib.
 */
#define AULIB_VERSION_STR "0.0.0"

/*!
 * This macro expands to a numeric value of the form 0xMMNNPP (MM = major, NN = minor, PP = patch)
 * that represents SDL_audiolib's version number. For example, when using version 1.2.3, the
 * AULIB_VERSION macro will expand to 0x010203.
 *
 * You can use this for easy compile-time version comparisons, like:
 *
 *     #if AULIB_VERSION >= 0x010402
 *         // This version of SDL_audiolib is at least 1.4.2.
 *     #endif
 */
#define AULIB_VERSION 0x000000

/*!
 * Turns the major, minor and patch numbers of a version into an integer of the form 0xMMNNPP
 * (MM = major, NN = minor, PP = patch).
 *
 * You can use this for easy compile-time version comparisons, like:
 *
 *     #if (AULIB_VERSION >= AULIB_VERSION_CHECK(1, 4, 2))
 *         // This version of SDL_audiolib is at least 1.4.2.
 *     #endif
 */
#define AULIB_VERSION_CHECK(major, minor, patch) ((major<<16)|(minor<<8)|(patch))

#endif
