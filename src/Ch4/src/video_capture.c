#include <gst/gst.h>
#include <glib.h>
#include <gio/gio.h>
// standard libraries
#include <stdio.h>
#include <stdlib.h>

static gboolean
bus_call (GstBus     *bus,
          GstMessage *msg,
          gpointer    data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);
      g_free (debug);

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

// Callback for keyboard input (Enter key) to stop recording
static gboolean
on_keyboard (GIOChannel   *source,
             GIOCondition   cond,
             gpointer       data)
{
  GstElement *pipeline = (GstElement *) data;
  // Send EOS event to pipeline to finish processing
  gst_element_send_event (pipeline, gst_event_new_eos ());
  // Remove this callback
  return FALSE;
}

int
main (int   argc,
      char *argv[])
{
  GMainLoop *loop;
  GstElement *pipeline, *src, *enc, *capfilter, *dec, *sink;
  GstBus *bus;
  guint bus_watch_id;
  gchar filename[256];
  gchar filepath[512];
  GstCaps *caps;

  gst_init (&argc, &argv);

  // Prompt user for file name
  g_print ("Enter output file name (without extension): ");
  if (fgets (filename, sizeof (filename), stdin) == NULL) {
    g_printerr ("Failed to read filename. Exiting.\n");
    return -1;
  }
  // Remove trailing newline
  filename[strcspn (filename, "\n")] = '\0';

  // Check that output directory exists
  if (!g_file_test ("video_stream", G_FILE_TEST_IS_DIR)) {
    g_printerr ("Directory 'video_stream' does not exist. Please create it and try again.\n");
    return -1;
  }

  // Build full file path
  g_snprintf (filepath, sizeof (filepath), "video_stream/%s.yuv", filename);

  // Create elements
  pipeline = gst_pipeline_new ("video-capture-pipeline");
  src      = gst_element_factory_make ("v4l2src",        "source");
  enc      = gst_element_factory_make ("jpegenc",        "encoder");
  capfilter= gst_element_factory_make ("capsfilter",     "filter");
  dec      = gst_element_factory_make ("jpegdec",        "decoder");
  sink     = gst_element_factory_make ("filesink",       "sink");

  if (!pipeline || !src || !enc || !capfilter || !dec || !sink) {
    g_printerr ("Failed to create GStreamer elements.\n");
    return -1;
  }

  // Set element properties
  g_object_set (G_OBJECT (src), "device", "/dev/video0", NULL);
  caps = gst_caps_from_string ("image/jpeg,width=320,height=240,framerate=30/1");
  g_object_set (G_OBJECT (capfilter), "caps", caps, NULL);
  gst_caps_unref (caps);
  g_object_set (G_OBJECT (sink), "location", filepath, NULL);

  // Build the pipeline: src -> jpegenc -> capsfilter -> jpegdec -> filesink
  gst_bin_add_many (GST_BIN (pipeline), src, enc, capfilter, dec, sink, NULL);
  if (!gst_element_link_many (src, enc, capfilter, dec, sink, NULL)) {
    g_printerr ("Failed to link elements in the pipeline.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  // Create the main loop
  loop = g_main_loop_new (NULL, FALSE);

  // Add bus watch
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  // Add keyboard watch for Enter key to stop recording
  GIOChannel *io_stdin = g_io_channel_unix_new (fileno (stdin));
  g_io_add_watch (io_stdin, G_IO_IN, on_keyboard, pipeline);

  // Start streaming
  g_print ("Recording... Press Enter to stop.\n");
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_main_loop_run (loop);

  // Clean up
  g_print ("Stopping recording...\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}
