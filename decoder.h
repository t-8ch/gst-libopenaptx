#pragma once

#include <gst/audio/gstaudiodecoder.h>
#include <openaptx.h>

G_BEGIN_DECLS

#define DECODER_NAME openaptx_dec

#define GST_APTX_TYPE_DECODER gst_aptx_decoder_get_type()
G_DECLARE_FINAL_TYPE(GstAptXDecoder, gst_aptx_decoder, GST_APTX, DECODER,
                     GstAudioDecoder)

GstAptXDecoder *gst_aptx_decoder_new(void);

G_END_DECLS
