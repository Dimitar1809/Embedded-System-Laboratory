#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>
#include <gio/gio.h>
#include <opencv2/opencv.hpp>
#include <iostream>


bool GreenObjectTracker(const cv::Mat &bgr_image, cv::Point &center) {

    // Convert BGR image to HSV
    cv::Mat hsv;
    cv::cvtColor(bgr_image, hsv, cv::COLOR_BGR2HSV);

    // Parameters for green color detection
    const int lower_h_ = 35; // Lower hue for green
    const int upper_h_ = 85; // Upper hue for green
    const int s_thresh_ = 50; // Minimum saturation threshold
    const int v_thresh_ = 50; // Minimum value threshold
    // Define HSV range for green detection
    cv::Scalar lower_bound(lower_h_, s_thresh_, v_thresh_);
    cv::Scalar upper_bound(upper_h_, 255, 255);

    // Threshold to get only green pixels
    cv::Mat mask;
    cv::inRange(hsv, lower_bound, upper_bound, mask);

    // Morphological operations to reduce noise
    cv::erode(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);
    cv::dilate(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);

    // Compute moments to find the center
    cv::Moments M = cv::moments(mask, true);
    if (M.m00 == 0) {
        return false; // No green object detected
    }
    center.x = static_cast<int>(M.m10 / M.m00);
    center.y = static_cast<int>(M.m01 / M.m00);
    return true;
}


// Gstreamer bus callback
static gboolean
bus_call(GstBus *bus, GstMessage *msg, gpointer data)
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
on_keyboard(GIOChannel *source, GIOCondition cond, gpointer data) {
  GstElement *pipeline = (GstElement *)data;
  gst_element_send_event(pipeline, gst_event_new_eos());
  return FALSE; // remove this watch
}

static GstFlowReturn
on_new_sample(GstElement *appsink_element, gpointer user_data) {

    // Pull the sample from appsink
    GstAppSink *appsink = GST_APP_SINK(appsink_element);
    if (!appsink) {
        return GST_FLOW_ERROR;
    }

    // Pull the sample from the appsink
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    if (!sample) {
        // EOS or error
        return GST_FLOW_ERROR;
    }
    
    // Extract the buffer from the sample
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    // We asked for decoder to produce raw YUV (I420 or NV12, etc).
    // For simplicity, assume v4l2src→jpegdec outputs **BGR** already.
    // If it were YUV, we'd convert using cv::cvtColor—but here jpegdec gives BGR by default.
    // In most Linux setups, jpegdec → caps filter “video/x-raw, format=BGR” is automatic.
    // So we build an OpenCV Mat header around info.data, which is BGR packed:
    // width=320, height=240, 3 channels, uchar
    int width = 320, height = 240; // Set your width and height
    cv::Mat bgr_frame(height, width, CV_8UC3, (void *)map.data, cv::Mat::AUTO_STEP);

    // Run the green object tracker
    cv::Point center(-1, -1); // Initialize center point
    bool found = GreenObjectTracker(bgr_frame, center);
    if (found) {
        // Draw a circle at the center of the detected green object
        cv::circle(bgr_frame, center, 10, cv::Scalar(0, 0, 255), 2);
        std::cout << "Green object found at: " << center.x << ", " << center.y << std::endl;
    } else {
        std::cout << "No green object detected" << std::endl;
    }

    // Display the processed frame
    cv::imshow("Processed Frame", bgr_frame);
    cv::waitKey(1); // Wait for a short time to allow the window to update

    // Cleanup
    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}


// Main funcion to build the GStreamer pipeline and start processing
int main(int argc, char *argv[]) {

    GMainLoop *loop;
    GstElement *pipeline, *src, *capfilter, *dec, *tee, *queue_app, *app_sink;
    GstBus *bus;
    guint bus_watch_id;
    GstCaps *caps;

    gst_init(&argc, &argv);

    // Create the GStreamer elements
    pipeline  = gst_pipeline_new("video-capture-pipeline");
    src       = gst_element_factory_make("v4l2src", "source");
    capfilter = gst_element_factory_make("capsfilter", "caps");
    dec       = gst_element_factory_make("jpegdec", "decoder");
    tee       = gst_element_factory_make("tee", "tee");
    queue_app = gst_element_factory_make("queue", "queue_app");
    app_sink  = gst_element_factory_make("appsink", "app_sink");
    if (!pipeline || !src || !capfilter || !dec || !tee || !queue_app || !app_sink)
    {
        g_printerr("Failed to create GStreamer elements.\n");
        return -1;
    }

    // Configure v4l2src → "/dev/video0"
    g_object_set(src, "device", "/dev/video0", NULL);
    // Set caps to "image/jpeg, width=320, height=240, framerate=30/1"
    caps = gst_caps_from_string("image/jpeg,width=320,height=240,framerate=30/1");
    g_object_set(capfilter, "caps", caps, NULL);
    gst_caps_unref(caps);

    // Configure appsink to emit signals, no sync, drop old buffers
    g_object_set(app_sink, "emit-signals", TRUE,
                 "sync", FALSE,
                 "drop", TRUE,
                 "max-buffers", 1,
                 NULL);

    // Add elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), src, capfilter, dec, tee, queue_app, app_sink, NULL);

    // Link the elements
    if (!gst_element_link_many(src, capfilter, dec, tee, NULL))
    {
        g_printerr("Failed to link source→decoder→tee\n");
        return -1;
    }
    if (!gst_element_link_many(tee, queue_app, app_sink, NULL)) {
        g_printerr("Failed to link tee→queue_app→app_sink\n");
        return -1;
    }  

    // Connect the appsink callback
    g_signal_connect(app_sink, "new-sample", G_CALLBACK(on_new_sample), NULL);

    // Create a Glib main loop
    loop = g_main_loop_new(NULL, FALSE);

    // Watch the bus for messages
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    // 11) Watch stdin → on Enter, send EOS
    GIOChannel *io_stdin = g_io_channel_unix_new(fileno(stdin));
    g_io_add_watch(io_stdin, G_IO_IN, on_keyboard, pipeline);

    // Start the playback
    g_print("Streaming from webcam... press [Enter] to stop.\n");
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_main_loop_run(loop);

    // Cleanup
    g_print("Stopping playback...\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);
    cv::destroyAllWindows();
    return 0;
}
