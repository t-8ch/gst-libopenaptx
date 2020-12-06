#pragma once

#include <gst/audio/gstaudioencoder.h>
#include <openaptx.h>

G_BEGIN_DECLS

#define ENCODER_NAME openaptx_enc

#define GST_APTX_TYPE_ENCODER gst_aptx_encoder_get_type ()
G_DECLARE_FINAL_TYPE (GstAptXEncoder, gst_aptx_encoder, GST_APTX, ENCODER, GstAudioEncoder)

GstAptXEncoder *gst_aptx_encoder_new(void);

G_END_DECLS
