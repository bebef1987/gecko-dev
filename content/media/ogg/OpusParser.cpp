/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string.h>

#include "mozilla/DebugOnly.h"
#include <stdint.h>

#include "OpusParser.h"

#include "nsDebug.h"
#include "MediaDecoderReader.h"
#include "VideoUtils.h"
#include <algorithm>

#include "opus/opus.h"
extern "C" {
#include "opus/opus_multistream.h"
}

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaDecoderLog;
#define OPUS_LOG(type, msg) PR_LOG(gMediaDecoderLog, type, msg)
#else
#define OPUS_LOG(type, msg)
#endif

OpusParser::OpusParser():
  mRate(0),
  mNominalRate(0),
  mChannels(0),
  mPreSkip(0),
#ifdef MOZ_SAMPLE_TYPE_FLOAT32
  mGain(1.0f),
#else
  mGain_Q16(65536),
#endif
  mChannelMapping(0),
  mStreams(0),
  mCoupledStreams(0)
{ }

bool OpusParser::DecodeHeader(unsigned char* aData, size_t aLength)
{
    if (aLength < 19 || memcmp(aData, "OpusHead", 8)) {
      OPUS_LOG(PR_LOG_DEBUG, ("Invalid Opus file: unrecognized header"));
      return false;
    }

    mRate = 48000; // The Opus decoder runs at 48 kHz regardless.

    int version = aData[8];
    // Accept file format versions 0.x.
    if ((version & 0xf0) != 0) {
      OPUS_LOG(PR_LOG_DEBUG, ("Rejecting unknown Opus file version %d", version));
      return false;
    }

    mChannels = aData[9];
    if (mChannels<1) {
      OPUS_LOG(PR_LOG_DEBUG, ("Invalid Opus file: Number of channels %d", mChannels));
      return false;
    }

    mPreSkip = LEUint16(aData + 10);
    mNominalRate = LEUint32(aData + 12);
    double gain_dB = LEInt16(aData + 16) / 256.0;
#ifdef MOZ_SAMPLE_TYPE_FLOAT32
    mGain = static_cast<float>(pow(10,0.05*gain_dB));
#else
    mGain_Q16 = static_cast<int32_t>(std::min(65536*pow(10,0.05*gain_dB)+0.5,
                                            static_cast<double>(INT32_MAX)));
#endif
    mChannelMapping = aData[18];

    if (mChannelMapping == 0) {
      // Mapping family 0 only allows two channels
      if (mChannels>2) {
        OPUS_LOG(PR_LOG_DEBUG, ("Invalid Opus file: too many channels (%d) for"
                           " mapping family 0.", mChannels));
        return false;
      }
      mStreams = 1;
      mCoupledStreams = mChannels - 1;
      mMappingTable[0] = 0;
      mMappingTable[1] = 1;
    } else if (mChannelMapping == 1) {
      // Currently only up to 8 channels are defined for mapping family 1
      if (mChannels>8) {
        OPUS_LOG(PR_LOG_DEBUG, ("Invalid Opus file: too many channels (%d) for"
                           " mapping family 1.", mChannels));
        return false;
      }
      if (aLength>20+mChannels) {
        mStreams = aData[19];
        mCoupledStreams = aData[20];
        int i;
        for (i=0; i<mChannels; i++)
          mMappingTable[i] = aData[21+i];
      } else {
        OPUS_LOG(PR_LOG_DEBUG, ("Invalid Opus file: channel mapping %d,"
                           " but no channel mapping table", mChannelMapping));
        return false;
      }
    } else {
      OPUS_LOG(PR_LOG_DEBUG, ("Invalid Opus file: unsupported channel mapping "
                         "family %d", mChannelMapping));
      return false;
    }
    if (mStreams < 1) {
      OPUS_LOG(PR_LOG_DEBUG, ("Invalid Opus file: no streams"));
      return false;
    }
    if (mCoupledStreams > mStreams) {
      OPUS_LOG(PR_LOG_DEBUG, ("Invalid Opus file: more coupled streams (%d) than "
                         "total streams (%d)", mCoupledStreams, mStreams));
      return false;
    }

#ifdef DEBUG
    OPUS_LOG(PR_LOG_DEBUG, ("Opus stream header:"));
    OPUS_LOG(PR_LOG_DEBUG, (" channels: %d", mChannels));
    OPUS_LOG(PR_LOG_DEBUG, ("  preskip: %d", mPreSkip));
    OPUS_LOG(PR_LOG_DEBUG, (" original: %d Hz", mNominalRate));
    OPUS_LOG(PR_LOG_DEBUG, ("     gain: %.2f dB", gain_dB));
    OPUS_LOG(PR_LOG_DEBUG, ("Channel Mapping:"));
    OPUS_LOG(PR_LOG_DEBUG, ("   family: %d", mChannelMapping));
    OPUS_LOG(PR_LOG_DEBUG, ("  streams: %d", mStreams));
#endif
  return true;
}

bool OpusParser::DecodeTags(unsigned char* aData, size_t aLength)
{
  if (aLength < 16 || memcmp(aData, "OpusTags", 8))
    return false;

  // Copy out the raw comment lines, but only do basic validation
  // checks against the string packing: too little data, too many
  // comments, or comments that are too long. Rejecting these cases
  // helps reduce the propagation of broken files.
  // We do not ensure they are valid UTF-8 here, nor do we validate
  // the required ASCII_TAG=value format of the user comments.
  const unsigned char* buf = aData + 8;
  uint32_t bytes = aLength - 8;
  uint32_t len;
  // Read the vendor string.
  len = LEUint32(buf);
  buf += 4;
  bytes -= 4;
  if (len > bytes)
    return false;
  mVendorString = nsCString(reinterpret_cast<const char*>(buf), len);
  buf += len;
  bytes -= len;
  // Read the user comments.
  if (bytes < 4)
    return false;
  uint32_t ncomments = LEUint32(buf);
  buf += 4;
  bytes -= 4;
  // If there are so many comments even their length fields
  // won't fit in the packet, stop reading now.
  if (ncomments > (bytes>>2))
    return false;
  uint32_t i;
  for (i = 0; i < ncomments; i++) {
    if (bytes < 4)
      return false;
    len = LEUint32(buf);
    buf += 4;
    bytes -= 4;
    if (len > bytes)
      return false;
    mTags.AppendElement(nsCString(reinterpret_cast<const char*>(buf), len));
    buf += len;
    bytes -= len;
  }

#ifdef DEBUG
  OPUS_LOG(PR_LOG_DEBUG, ("Opus metadata header:"));
  OPUS_LOG(PR_LOG_DEBUG, ("  vendor: %s", mVendorString.get()));
  for (uint32_t i = 0; i < mTags.Length(); i++) {
    OPUS_LOG(PR_LOG_DEBUG, (" %s", mTags[i].get()));
  }
#endif
  return true;
}

} // namespace mozilla

