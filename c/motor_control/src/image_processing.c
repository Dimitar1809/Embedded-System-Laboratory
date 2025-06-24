#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "image_processing.h"

#if defined(DE10)
	#define VIDEO_DEVICE_PATH "/dev/video7"  
#elif defined(RPI4)
	#define VIDEO_DEVICE_PATH "/dev/video0"
#endif

static pthread_t image_thread;
static GMainLoop *main_loop = NULL;

// Global variables for ball tracking data
static int threshold = 25;  // Threshold for green detection, can be adjusted
static volatile int ball_x = -1;
static volatile int ball_y = -1;
static volatile int ball_detected = 0;
static volatile int new_frame = 0;
static pthread_mutex_t ball_data_mutex = PTHREAD_MUTEX_INITIALIZER;

static int frame_count = 0;
static GstClockTime max_duration = GST_CLOCK_TIME_NONE;
static GstClockTime total_duration = 0;

// Simple green ball detection
static int detect_green_ball(unsigned char *bgr_data, int width, int height, int *x, int *y)
{
    int center_x = 0, center_y = 0;
    int pixel_count = 0;
    
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int idx = (row * width + col) * 3;
            unsigned char b = bgr_data[idx];
            unsigned char g = bgr_data[idx + 1];
            unsigned char r = bgr_data[idx + 2];

            // Simple green detection
            if (g > r + threshold && g > b + threshold) {
                center_x += col;
                center_y += row;
                pixel_count++;
            }
        }
    }
    if (pixel_count > 50) {
        *x = center_x / pixel_count;
        *y = center_y / pixel_count;
        return 1;
    }
    
    return 0;
}

// Thread save access to ball position data
int has_new_frame()
{
    pthread_mutex_lock(&ball_data_mutex);
    int result = new_frame;
    pthread_mutex_unlock(&ball_data_mutex);
    return result;
}

int get_ball_position(int *x, int *y)
{
    pthread_mutex_lock(&ball_data_mutex);
    if (ball_detected && new_frame) {
        *x = ball_x;
        *y = ball_y;
        new_frame = 0;  // Mark as read
        pthread_mutex_unlock(&ball_data_mutex);
        return 1;  // Ball found
    }
    pthread_mutex_unlock(&ball_data_mutex);
    return 0;  // No ball
}



/* Called whenever the GStreamer bus posts an error or EOS. */
static gboolean
bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    (void)bus;  // Unused parameter
    GMainLoop *loop = (GMainLoop *)data;
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR: {
            GError *error;
            gchar *debug;
            gst_message_parse_error(msg, &error, &debug);
            g_printerr("GStreamer Error: %s\n", error->message);
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

static GstFlowReturn
on_new_sample(GstAppSink *appsink)
{
    GstClockTime start_time = gst_util_get_timestamp();
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    if (!sample)
        return GST_FLOW_ERROR;  // EOS or error

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    // Process the image
    int detected_x = -1, detected_y = -1;
    int detected = detect_green_ball(map.data, 320, 240, &detected_x, &detected_y);
    
    // Update global variables with thread safety
    pthread_mutex_lock(&ball_data_mutex);
    ball_x = detected_x;
    ball_y = detected_y;
    ball_detected = detected;
    new_frame = 1;
    pthread_mutex_unlock(&ball_data_mutex);

    

    // Cleanup
    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);

    GstClockTime end_time = gst_util_get_timestamp();
    GstClockTime duration = end_time - start_time;

    // Track maximum and accumulate total
    if (max_duration == GST_CLOCK_TIME_NONE || duration > max_duration) {
        max_duration = duration;
    }
    total_duration += duration;
    frame_count++;
    
    // Print stats every 6 frames
    if (frame_count % 6 == 0) {
        GstClockTime avg_duration = total_duration / 6;
        double max_ms = (double)max_duration / GST_MSECOND;
        double avg_ms = (double)avg_duration / GST_MSECOND;
        printf(" Time: %.3fs", (double)end_time / GST_SECOND);
        printf(" [Image] Max: %.1fms, Avg: %.1fms", 
               max_ms, avg_ms);
        

        // Reset for next batch
        max_duration = GST_CLOCK_TIME_NONE;
        total_duration = 0;
    }
    return GST_FLOW_OK;
}




static void* image_processing_thread()
{
    GstElement *pipeline, *src, *capfilter, *dec, *convert, *appsink;
    GstBus *bus;
    guint bus_watch_id;
    GstCaps *caps;

    gst_init(NULL, NULL);

    // 1) Create all elements
    pipeline  = gst_pipeline_new("video-capture-pipeline");
    src       = gst_element_factory_make("v4l2src",   "src");
    capfilter = gst_element_factory_make("capsfilter","caps");
    dec       = gst_element_factory_make("jpegdec",   "decoder");
    convert   = gst_element_factory_make("videoconvert","converter");
    appsink   = gst_element_factory_make("appsink",   "app_sink");

    if (!pipeline || !src || !capfilter || !dec || !convert || !appsink) {
        g_printerr("Failed to create one of the GStreamer elements.\n");
        return NULL;
    }

    // 2) Configure the v4l2src and capsfilter so that we get 320×240@30fps JPEG,
    //    then decode to raw BGR via videoconvert → appsink:
    g_object_set(src, "device", VIDEO_DEVICE_PATH, NULL);

    caps = gst_caps_from_string("image/jpeg,width=320,height=240,framerate=30/1");
    g_object_set(capfilter, "caps", caps, NULL);
    gst_caps_unref(caps);

    // Tell appsink we want raw BGR frames at exactly 320×240:
    caps = gst_caps_from_string(
        "video/x-raw, "
        "format=BGR, "
        "width=320, "
        "height=240"
    );
    g_object_set(appsink,
                 "caps", caps,
                 "emit-signals", TRUE,
                 "sync", FALSE,
                 "drop", TRUE,
                 "max-buffers", 1,
                 NULL);
    gst_caps_unref(caps);

    // 3) Build the pipeline:
    //    src → capfilter → jpegdec → videoconvert → appsink
    gst_bin_add_many(GST_BIN(pipeline),
                     src, capfilter, dec, convert, appsink, NULL);

    if (!gst_element_link_many(src, capfilter, dec, convert, appsink, NULL)) {
        g_printerr("Failed to link src→capfilter→dec→convert→appsink\n");
        return NULL;
    }

    // 4) Connect appsink’s “new-sample” signal to our callback
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), NULL);

    // 5) Create a GLib main loop
    main_loop = g_main_loop_new(NULL, FALSE);

    // 6) Watch the bus for errors/EOS
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, main_loop);
    gst_object_unref(bus);

    // 8) Start playback
    g_print("Streaming from webcam... press [Enter] to stop.\n");
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_main_loop_run(main_loop);

    // 9) Clean up when EOS or Ctrl+C
    g_print("Stopping playback...\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_source_remove(bus_watch_id);
    g_main_loop_unref(main_loop);

    return NULL;
}

// Start the image processing thread
int image_processing_start()
{
    if (pthread_create(&image_thread, NULL, image_processing_thread, NULL) != 0) {
        g_printerr("Failed to create image processing thread\n");
        return -1;
    }
    
    // Give the thread a moment to start
    usleep(100000); // 100ms
    return 0;
}

// Stop the image processing thread
int image_processing_stop()
{
    // Signal the main loop to quit
    if (main_loop) {
        g_main_loop_quit(main_loop);
    }
    
    // Wait for thread to finish
    pthread_join(image_thread, NULL);
    
    return 0;
}
