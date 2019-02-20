// This is copyrighted software. More information is at the end of this file.
#include "Aulib/AudioResamplerSrc.h"

#include <cstring>
#include <samplerate.h>
#include "aulib_debug.h"

namespace Aulib {

/// \private
struct AudioResamplerSRC_priv final {
    explicit AudioResamplerSRC_priv(AudioResamplerSRC::Quality quality)
        : fQuality(quality)
    { }

    std::unique_ptr<SRC_STATE, decltype(&src_delete)> fResampler{nullptr, &src_delete};
    SRC_DATA fData{};
    AudioResamplerSRC::Quality fQuality;
};

} // namespace Aulib


Aulib::AudioResamplerSRC::AudioResamplerSRC(Quality quality)
    : d(std::make_unique<AudioResamplerSRC_priv>(quality))
{ }


Aulib::AudioResamplerSRC::~AudioResamplerSRC() = default;


Aulib::AudioResamplerSRC::Quality
Aulib::AudioResamplerSRC::quality() const noexcept
{
    return d->fQuality;
}


void
Aulib::AudioResamplerSRC::doResampling(float dst[], const float src[], int& dstLen, int& srcLen)
{
    if (not d->fResampler) {
        dstLen = srcLen = 0;
        return;
    }

    d->fData.data_in = src;
    d->fData.data_out = dst;
    int channels = currentChannels();
    d->fData.input_frames = srcLen / channels;
    d->fData.output_frames = dstLen / channels;
    d->fData.end_of_input = 0;

    src_process(d->fResampler.get(), &d->fData);

    dstLen = d->fData.output_frames_gen * channels;
    srcLen = d->fData.input_frames_used * channels;
}


int
Aulib::AudioResamplerSRC::adjustForOutputSpec(int dstRate, int srcRate, int channels)
{
    int err;
    d->fData.src_ratio = static_cast<double>(dstRate) / srcRate;

    int src_q;
    switch (d->fQuality) {
    case Quality::Linear:        src_q = SRC_LINEAR; break;
    case Quality::ZeroOrderHold: src_q = SRC_ZERO_ORDER_HOLD; break;
    case Quality::SincFastest:   src_q = SRC_SINC_FASTEST; break;
    case Quality::SincMedium:    src_q = SRC_SINC_MEDIUM_QUALITY; break;
    case Quality::SincBest:      src_q = SRC_SINC_BEST_QUALITY; break;
    }

    d->fResampler.reset(src_new(src_q, channels, &err));
    if (not d->fResampler) {
        return -1;
    }
    return 0;
}


/*

Copyright (C) 2014, 2015, 2016, 2017, 2018 Nikos Chantziaras.

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
