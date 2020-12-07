#include "decoder.h"

GST_DEBUG_CATEGORY_STATIC(DECODER_NAME);
#define GST_CAT_DEFAULT DECODER_NAME

struct _GstAptXDecoder {
  GstAudioDecoder parent;
  struct aptx_context *ctx;
};

G_DEFINE_TYPE(GstAptXDecoder, gst_aptx_decoder, GST_TYPE_AUDIO_DECODER)

static void gst_aptx_decoder_init(GstAptXDecoder *self) {}

static GstStaticPadTemplate sink_factory =
    GST_STATIC_PAD_TEMPLATE(GST_AUDIO_DECODER_SINK_NAME, GST_PAD_SINK,
                            GST_PAD_ALWAYS, GST_STATIC_CAPS("audio/aptx"));

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE(
    GST_AUDIO_DECODER_SRC_NAME, GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS("audio/x-raw, "
                    "format = S24LE, "
                    "layout = interleaved"));

static GstFlowReturn handle_frame(GstAudioDecoder *dec, GstBuffer *buffer) {
  GstAptXDecoder *self = GST_APTX_DECODER(dec);

  GST_DEBUG("handle frame. memory=%d s=%" G_GSIZE_FORMAT,
            gst_buffer_n_memory(buffer), gst_buffer_get_size(buffer));

  g_assert_cmpint(gst_buffer_n_memory(buffer), ==, 1);

  for (size_t n = 0; n < gst_buffer_n_memory(buffer); n++) {
    GstMemory *input_mem = gst_buffer_peek_memory(buffer, n);
    GstMapInfo input_mapping;
    gboolean mapped = gst_memory_map(input_mem, &input_mapping, GST_MAP_READ);
    g_assert_true(mapped);

    // FIXME size
    GstBuffer *target = gst_audio_decoder_allocate_output_buffer(
        dec, gst_buffer_get_size(buffer) * 8);
    g_assert_nonnull(target);

    size_t written;

    GstMemory *mem = gst_buffer_peek_memory(target, 0);
    GstMapInfo mapping;
    mapped = gst_memory_map(mem, &mapping, GST_MAP_WRITE);
    g_assert_true(mapped);

    GST_DEBUG("decoding in.size=%" G_GSIZE_FORMAT
              " out.maxsize=%" G_GSIZE_FORMAT,
              input_mapping.size, mapping.maxsize);

    size_t processed =
        aptx_decode(self->ctx, input_mapping.data, input_mapping.size,
                    mapping.data, mapping.maxsize, &written);

    GST_DEBUG("decoded read=%" G_GSIZE_FORMAT " written=%" G_GSIZE_FORMAT,
              processed, written);

    // FIXME
    g_assert_cmpuint(processed, >, 0);
    mapping.size = written;
    gst_memory_unmap(mapping.memory, &mapping);

    GstCaps *src_caps =
        gst_caps_copy(gst_static_pad_template_get_caps(&src_factory));
    g_assert_nonnull(src_caps);
    gst_caps_set_simple(src_caps, "rate", G_TYPE_INT, 42100, NULL);
    gst_caps_set_simple(src_caps, "channels", G_TYPE_INT, 2, NULL);

    GstAudioInfo *info = gst_audio_info_new();
    g_assert_nonnull(info);
    gboolean converted = gst_audio_info_from_caps(info, src_caps);
    g_assert_true(converted);

    GstAudioMeta *meta =
        gst_buffer_add_audio_meta(target, info, written / 6, NULL);

    gst_audio_decoder_set_output_format(dec, info);

    if (processed != gst_buffer_get_size(buffer)) {
      // FIXME
      // GST_AUDIO_DECODER_ERROR(dec, 1,
    }

    return gst_audio_decoder_finish_frame(dec, target, 1);
  }
  g_assert_not_reached();
}

static gboolean start(GstAudioDecoder *dec) {
  GstAptXDecoder *self = GST_APTX_DECODER(dec);

  GST_DEBUG("start");

  self->ctx = aptx_init(0);
  return TRUE;
}

static gboolean stop(GstAudioDecoder *dec) {
  GstAptXDecoder *self = GST_APTX_DECODER(dec);

  GST_DEBUG("stop");

  aptx_finish(self->ctx);
  self->ctx = NULL;
  return TRUE;
}

static void gst_aptx_decoder_class_init(GstAptXDecoderClass *klass) {
  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

  GST_DEBUG_CATEGORY_INIT(DECODER_NAME, G_STRINGIFY(DECODER_NAME), 0,
                          "libopenaptx decoder");

  GstAudioDecoderClass *audio_decoder_class = GST_AUDIO_DECODER_CLASS(klass);

  gst_element_class_set_static_metadata(
      element_class, "libopenaptx decoder", "Codec/Decoder/Audio",
      "Audio decoder based on libopenaptx",
      "Thomas Weißschuh <thomas@weissschuh.net>");

  gst_element_class_add_pad_template(element_class,
                                     gst_static_pad_template_get(&src_factory));
  gst_element_class_add_pad_template(
      element_class, gst_static_pad_template_get(&sink_factory));

  audio_decoder_class->handle_frame = handle_frame;
  audio_decoder_class->start = start;
  audio_decoder_class->stop = stop;
}
