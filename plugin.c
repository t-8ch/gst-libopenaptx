#include <gst/gst.h>

#include "config.h"
#include "decoder.h"
#include "encoder.h"

#include <stdio.h>

static gboolean plugin_init(GstPlugin *plugin) {
  gst_element_register(plugin, G_STRINGIFY(DECODER_NAME), GST_RANK_NONE,
                       GST_APTX_TYPE_DECODER);
  gst_element_register(plugin, G_STRINGIFY(ENCODER_NAME), GST_RANK_NONE,
                       GST_APTX_TYPE_ENCODER);
  return TRUE;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, openaptx,
                  PACKAGE " Plugin", plugin_init, VERSION, LICENSE, PACKAGE,
                  ORIGIN)
