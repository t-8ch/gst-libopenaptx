#include "encoder.h"
#include "constants.h"

GST_DEBUG_CATEGORY_STATIC(ENCODER_NAME);
#define GST_CAT_DEFAULT ENCODER_NAME

struct _GstAptXEncoder {
  GstAudioEncoder parent;
  gboolean hd;
  struct aptx_context *ctx;
};

G_DEFINE_TYPE(GstAptXEncoder, gst_aptx_encoder, GST_TYPE_AUDIO_ENCODER)

static void gst_aptx_encoder_init(GstAptXEncoder *self) {}

static GstStaticPadTemplate sink_factory =
    GST_STATIC_PAD_TEMPLATE(GST_AUDIO_ENCODER_SINK_NAME, GST_PAD_SINK,
                            GST_PAD_ALWAYS, GST_STATIC_CAPS(RAW_CAPS));

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE(
    GST_AUDIO_ENCODER_SRC_NAME, GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS("audio/aptx; "
                    "audio/aptx-hd"));

static GstFlowReturn handle_frame(GstAudioEncoder *enc, GstBuffer *buffer) {
  GstAptXEncoder *self = GST_APTX_ENCODER(enc);

  g_assert_nonnull(self->ctx);

  gboolean success;

  size_t output_size;
  size_t sample_size;

  if (self->hd) {
    sample_size = 6;
  } else {
    sample_size = 4;
  }

  if (buffer != NULL) {
    size_t input_size = gst_buffer_get_size(buffer);
    GST_DEBUG("handle frame: memory=%d s=%" G_GSIZE_FORMAT,
              gst_buffer_n_memory(buffer), input_size);

    g_assert_cmpint(gst_buffer_n_memory(buffer), ==, 1);
    output_size = input_size / 24 * sample_size;
  } else {
    output_size = 92 * sample_size;
  }

  GstBuffer *target =
      gst_audio_encoder_allocate_output_buffer(enc, output_size);
  GstMemory *mem = gst_buffer_peek_memory(target, 0);
  GstMapInfo mapping;
  success = gst_memory_map(mem, &mapping, GST_MAP_WRITE);
  g_assert_true(success);

  if (buffer != NULL) {
    size_t encoded;
    size_t read;

    GstMemory *input_mem = gst_buffer_peek_memory(buffer, 0);
    GstMapInfo input_mapping;
    success = gst_memory_map(input_mem, &input_mapping, GST_MAP_READ);
    g_assert_true(success);
    read = aptx_encode(self->ctx, input_mapping.data, input_mapping.size,
                       mapping.data, mapping.maxsize, &encoded);
    gst_memory_resize(mapping.memory, 0, encoded);

    GST_DEBUG("encoded read:=%" G_GSIZE_FORMAT " written=%" G_GSIZE_FORMAT
              " output_size=%" G_GSIZE_FORMAT,
              read, encoded, gst_buffer_get_size(target));

    g_assert_cmpint(read, ==, gst_buffer_get_size(buffer));
    g_assert_cmpint(encoded, ==, output_size);

    gst_memory_unmap(input_mapping.memory, &input_mapping);
    gst_memory_unmap(mapping.memory, &mapping);
    return gst_audio_encoder_finish_frame(enc, target, 1);
  } else {
    size_t encoded;
    int r;

    r = aptx_encode_finish(self->ctx, mapping.data, mapping.maxsize, &encoded);
    g_assert_cmpint(r, !=, 0);
    gst_memory_resize(mapping.memory, 0, encoded);
    GST_DEBUG("finished: written=%" G_GSIZE_FORMAT, encoded);
    gst_memory_unmap(mapping.memory, &mapping);
    gst_audio_encoder_finish_frame(enc, target, 1);
    return GST_FLOW_EOS;
  }

  g_assert_not_reached();
}

static void reset_ctx(GstAptXEncoder *self) {
  if (self->ctx != NULL) {
    aptx_finish(self->ctx);
  }
  self->ctx = aptx_init(self->hd ? 1 : 0);
  g_assert_nonnull(self->ctx);
}

static gboolean set_format(GstAudioEncoder *enc, GstAudioInfo *info) {
  gboolean success;
  gboolean hd;

  GstAptXEncoder *self = GST_APTX_ENCODER(enc);

  hd = gst_pad_peer_query_accept_caps(
      enc->srcpad, gst_caps_new_empty_simple("audio/aptx-hd"));
  g_autoptr(GstCaps) caps =
      gst_caps_new_empty_simple(hd ? "audio/aptx-hd" : "audio/aptx");
  gst_caps_set_simple(caps, "rate", G_TYPE_INT, info->rate, NULL);
  success = gst_audio_encoder_set_output_format(enc, caps);
  g_assert_true(success);
  success = gst_audio_encoder_negotiate(enc);
  g_assert_true(success);
  self->hd = hd;
  reset_ctx(self);

  return TRUE;
}

static gboolean stop(GstAudioEncoder *enc) {
  GstAptXEncoder *self = GST_APTX_ENCODER(enc);

  aptx_finish(self->ctx);
  self->ctx = NULL;
  return TRUE;
}

static void flush(GstAudioEncoder *enc) {
  GstAptXEncoder *self = GST_APTX_ENCODER(enc);

  reset_ctx(self);
}

static void gst_aptx_encoder_class_init(GstAptXEncoderClass *klass) {
  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
  GstAudioEncoderClass *audio_encoder_class = GST_AUDIO_ENCODER_CLASS(klass);

  GST_DEBUG_CATEGORY_INIT(ENCODER_NAME, G_STRINGIFY(ENCODER_NAME), 0,
                          "libopenaptx encoder");

  gst_element_class_set_static_metadata(
      element_class, "libopenaptx encoder", "Codec/Encoder/Audio",
      "Audio encoder based on libopenaptx",
      "Thomas Weißschuh <thomas@weissschuh.net>");

  gst_element_class_add_pad_template(element_class,
                                     gst_static_pad_template_get(&src_factory));
  gst_element_class_add_pad_template(
      element_class, gst_static_pad_template_get(&sink_factory));
  audio_encoder_class->handle_frame = handle_frame;
  audio_encoder_class->set_format = set_format;
  audio_encoder_class->stop = stop;
  audio_encoder_class->flush = flush;
}
