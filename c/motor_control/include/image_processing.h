#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

int image_processing_start(void);
int image_processing_stop(void);
int image_processing_is_running(void);
int has_new_frame(void);
int get_ball_position(int *x, int *y);


#endif // IMAGE_PROCESSING_H