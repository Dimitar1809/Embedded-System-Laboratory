#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>
#include <gio/gio.h>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdlib.h>

/* Simple HSV‐based green object detector: returns false if no green pixels, otherwise fills center. */
static bool
GreenObjectTracker(const cv::Mat &bgr_image, cv::Point &center)
{
    // Convert BGR → HSV
    cv::Mat hsv;
    cv::cvtColor(bgr_image, hsv, cv::COLOR_BGR2HSV);

    // Green range in HSV
    const int lower_h_ = 35, upper_h_ = 85;
    const int s_thresh_ = 50, v_thresh_ = 50;
    cv::Scalar lowerb(lower_h_, s_thresh_, v_thresh_);
    cv::Scalar upperb(upper_h_, 255, 255);

    // Threshold out everything except green
    cv::Mat mask;
    cv::inRange(hsv, lowerb, upperb, mask);

    // Morphological denoise
    cv::erode(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);
    cv::dilate(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);

    // Compute moments
    cv::Moments M = cv::moments(mask, true);
    if (M.m00 <= 0.0) {
        return false;  // no green detected
    }

    center.x = static_cast<int>(M.m10 / M.m00);
    center.y = static_cast<int>(M.m01 / M.m00);
    return true;
}

/* Called whenever the GStreamer bus posts an error or EOS. */
static gboolean
bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
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

/* Called when user hits “Enter” in the terminal: send an EOS event to the pipeline. */
static gboolean
on_keyboard(GIOChannel *source, GIOCondition cond, gpointer data)
{
    GstElement *pipeline = (GstElement *)data;
    gst_element_send_event(pipeline, gst_event_new_eos());
    return FALSE;  // remove this watch
}

/* appsink callback: here is where we pull each frame, wrap it in a cv::Mat, run green detection, then display it. */
static GstFlowReturn
on_new_sample(GstAppSink *appsink, gpointer user_data)
{
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    if (!sample)
        return GST_FLOW_ERROR;  // EOS or error

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    // We requested BGR from videoconvert → appsink #{ caps = "video/x-raw, format=BGR, width=320, height=240" }  
    // So map.data points to 320×240×3 bytes of BGR in row‐major order:
    int width  = 320;
    int height = 240;
    cv::Mat bgr_frame(height, width, CV_8UC3, (void *)map.data, cv::Mat::AUTO_STEP);

    // Run green‐ball detection:
    cv::Point center(-1, -1);
    bool found = GreenObjectTracker(bgr_frame, center);
    if (found) {
        printf("Green object found at (%d, %d)\n", center.x, center.y);
        // Draw a circle so we can see it:
        cv::circle(bgr_frame, center, 10, cv::Scalar(0, 0, 255), 2);
    } else {
        printf("No green object detected\n");
    }

    // “Dump” the image by showing in an OpenCV window:
    cv::imshow("Processed Frame", bgr_frame);
    cv::waitKey(1);  // allow window update

    // Cleanup
    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

int main(int argc, char *argv[])
{
    GMainLoop *loop;
    GstElement *pipeline, *src, *capfilter, *dec, *convert, *appsink;
    GstBus *bus;
    guint bus_watch_id;
    GstCaps *caps;

    gst_init(&argc, &argv);

    // 1) Create all elements
    pipeline  = gst_pipeline_new("video-capture-pipeline");
    src       = gst_element_factory_make("v4l2src",   "src");
    capfilter = gst_element_factory_make("capsfilter","caps");
    dec       = gst_element_factory_make("jpegdec",   "decoder");
    convert   = gst_element_factory_make("videoconvert","converter");
    appsink   = gst_element_factory_make("appsink",   "app_sink");

    if (!pipeline || !src || !capfilter || !dec || !convert || !appsink) {
        g_printerr("Failed to create one of the GStreamer elements.\n");
        return -1;
    }

    // 2) Configure the v4l2src and capsfilter so that we get 320×240@30fps JPEG,
    //    then decode to raw BGR via videoconvert → appsink:
    g_object_set(src, "device", "/dev/video0", NULL);

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
        return -1;
    }

    // 4) Connect appsink’s “new-sample” signal to our callback
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), NULL);

    // 5) Create a GLib main loop
    loop = g_main_loop_new(NULL, FALSE);

    // 6) Watch the bus for errors/EOS
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    // 7) Watch keyboard so that “Enter” → pipeline EOS
    GIOChannel *io_stdin = g_io_channel_unix_new(fileno(stdin));
    g_io_add_watch(io_stdin, G_IO_IN, on_keyboard, pipeline);

    // 8) Start playback
    g_print("Streaming from webcam... press [Enter] to stop.\n");
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_main_loop_run(loop);

    // 9) Clean up when EOS or Ctrl+C
    g_print("Stopping playback...\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);
    cv::destroyAllWindows();

    return 0;
}
