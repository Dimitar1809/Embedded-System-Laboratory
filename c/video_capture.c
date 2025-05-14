#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <stdlib.h>

// Bus message handler: EOS and ERROR
static gboolean
bus_call(GstBus *bus,
         GstMessage *msg,
         gpointer data)
{
  GMainLoop *loop = (GMainLoop *)data;

  switch (GST_MESSAGE_TYPE(msg))
  {
  case GST_MESSAGE_EOS:
    g_print("End of stream\n");
    g_main_loop_quit(loop);
    break;
  case GST_MESSAGE_ERROR:
  {
    GError *error;
    gchar *debug;
    gst_message_parse_error(msg, &error, &debug);
    g_printerr("Error: %s\n", error->message);
    g_error_free(error);
    g_free(debug);
    g_main_loop_quit(loop);
    break;
  }
  default:
    break;
  }
  return TRUE;
}

// Stop on Enter key: send EOS
static gboolean
on_keyboard(GIOChannel *source,
            GIOCondition cond,
            gpointer data)
{
  GstElement *pipeline = (GstElement *)data;
  gst_element_send_event(pipeline, gst_event_new_eos());
  return FALSE; // remove this watch
}

// appsink callback
static GstFlowReturn
on_new_sample(GstAppSink *appsink, gpointer user_data)
{
  GstSample *sample = gst_app_sink_pull_sample(appsink);
  if (!sample)
    return GST_FLOW_ERROR; // EOS or error

  GstBuffer *buffer = gst_sample_get_buffer(sample);
  GstMapInfo info;
  gst_buffer_map(buffer, &info, GST_MAP_READ);

  // process raw YUV data at info.data, length = info.size
  gst_buffer_unmap(buffer, &info);
  gst_sample_unref(sample);
  return GST_FLOW_OK;
}

int main(int argc, char *argv[])
{
  GMainLoop *loop;
  GstElement *pipeline,
      *src, *capfilter, *dec, *tee,
      *queue_file, *queue_app,
      *file_sink, *app_sink;
  GstBus *bus;
  guint bus_watch_id;
  gchar filename[256], filepath[512];
  GstCaps *caps;

  gst_init(&argc, &argv);

  // Prompt user for output name
  g_print("Enter output file name (without extension): ");
  if (!fgets(filename, sizeof(filename), stdin))
  {
    g_printerr("Failed to read filename. Exiting.\n");
    return -1;
  }
  filename[strcspn(filename, "\n")] = '\0';

  // Check directory exists
  if (!g_file_test("video_stream", G_FILE_TEST_IS_DIR))
  {
    g_printerr("Directory 'video_stream' does not exist. Please create it.\n");
    return -1;
  }
  g_snprintf(filepath, sizeof(filepath), "video_stream/%s.yuv", filename);

  // Create elements
  pipeline = gst_pipeline_new("video-capture-pipeline");
  src = gst_element_factory_make("v4l2src", "source");
  capfilter = gst_element_factory_make("capsfilter", "caps");
  dec = gst_element_factory_make("jpegdec", "decoder");
  tee = gst_element_factory_make("tee", "tee");
  queue_file = gst_element_factory_make("queue", "queue_file");
  file_sink = gst_element_factory_make("filesink", "file_sink");
  queue_app = gst_element_factory_make("queue", "queue_app");
  app_sink = gst_element_factory_make("appsink", "app_sink");

  if (!pipeline || !src || !capfilter || !dec || !tee ||
      !queue_file || !file_sink || !queue_app || !app_sink)
  {
    g_printerr("Failed to create GStreamer elements.\n");
    return -1;
  }

  // Configure elements
  g_object_set(src, "device", "/dev/video0", NULL);

  caps = gst_caps_from_string(
      "image/jpeg,width=320,height=240,framerate=30/1");
  g_object_set(capfilter, "caps", caps, NULL);
  gst_caps_unref(caps);

  g_object_set(file_sink, "location", filepath, NULL);

  g_object_set(app_sink,
               "emit-signals", TRUE,
               "sync", FALSE,
               "max-buffers", 1,
               "drop", TRUE,
               NULL);

  // Build pipeline: add all elems
  gst_bin_add_many(GST_BIN(pipeline),
                   src, capfilter, dec, tee,
                   queue_file, file_sink,
                   queue_app, app_sink,
                   NULL);

  // Link capture → capsfilter → decoder → tee
  if (!gst_element_link_many(src, capfilter, dec, tee, NULL))
  {
    g_printerr("Failed to link capture/decoding chain\n");
    return -1;
  }
  // Link tee → file branch
  if (!gst_element_link_many(tee, queue_file, file_sink, NULL))
  {
    g_printerr("Failed to link file branch\n");
    return -1;
  }
  // Link tee → appsink branch
  if (!gst_element_link_many(tee, queue_app, app_sink, NULL))
  {
    g_printerr("Failed to link appsink branch\n");
    return -1;
  }

  // Connect appsink callback
  g_signal_connect(app_sink, "new-sample",
                   G_CALLBACK(on_new_sample), NULL);

  // Create main loop
  loop = g_main_loop_new(NULL, FALSE);

  // Bus watch for EOS/errors
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
  gst_object_unref(bus);

  // Keyboard watch for Enter → EOS
  GIOChannel *io_stdin = g_io_channel_unix_new(fileno(stdin));
  g_io_add_watch(io_stdin, G_IO_IN, on_keyboard, pipeline);

  // Start capturing
  g_print("Recording... Press Enter to stop.\n");
  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  g_main_loop_run(loop);

  // Cleanup
  g_print("Stopping recording...\n");
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  g_source_remove(bus_watch_id);
  g_main_loop_unref(loop);

  return 0;
}