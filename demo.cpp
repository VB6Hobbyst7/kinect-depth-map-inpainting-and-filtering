// A demo for playback capabilities.
#include "playback.hpp"
#include "visualize.hpp"
#include "filter.hpp"
#include "our_fmm.hpp"
#include <opencv2/core/core.hpp>

#include <utility>

#define SIZEXY 7
#define SIZET  0
#define SIGMAXY 5
#define SIGMAT  1
#define SIGMAC  0
#define SIGMAD  500
#define ALPHA 0.5
#define BETA  .5

void help()
{
  printf("Demonstrates playback capabilities. To use, type:\n"
         "./demo\n"
         "    -n <node file name> : to use an ni node recording.\n"
         "    -v <rgb file name> <depth file name> : to use 2 video files.\n"
         "    -d : to use the kinect device.\n"
         "    -o <out_filename> : to save the output to the given video file. \n");
}

int main(int argc, char* argv[])
{
  KinectPlayback playback = KinectPlayback();
  if (argc < 2)
  {
    help();
    return -1;
  }

  const char* output_filename = 0;
  for (int i = 0; i < argc; i++) {
    if(argv[i][0] == '-')
    {
      switch (argv[i][1]) {
        case 'n':
          playback.init(argv[++i]);
          break;
        case 'v':
          playback.init(argv[++i], argv[++i]);
          break;
        case 'd':
          playback.init();
          break;
        case 'o':
          output_filename = argv[++i];
          break;
        default:
          help();
          return -1;
      }
    }
  }

  int prev_frame_num = 2;
  //Init filter
  BilinearFilter filter_bil = BilinearFilter(SIZEXY, SIZET, SIGMAXY, SIGMAT, SIGMAD, SIGMAC);
  for (int i = 0; i < SIZET; i++) {
    playback.update();
    Mat inpainted_depth;
    Mat invalid_mask = (playback.depth == 0);
    inpaint(playback.depth, invalid_mask, inpainted_depth, 5, (float)ALPHA);
    filter_bil.update(playback.rgb, inpainted_depth);
  }
  
  MedianFilter filter_med = MedianFilter(SIZEXY, BETA);

  printf("Filters initalized.\n");

  VideoWriter writer;
  if(output_filename)
  {
    writer.open(output_filename, CV_FOURCC('M','J','P','G'), playback.get_fps(),
                Size(playback.get_width(), playback.get_height()));
  }

  //MedianFilter filter = MedianFilter(7, .3);
  while (playback.update() && waitKey(1) != 27) {
    //Passing Previous frame buffer
    //printf("Inpainting...\n");
    Mat inpainted_depth;
    Mat invalid_mask = (playback.depth == 0);
    inpaint(playback.depth, invalid_mask, inpainted_depth, 5, (float)ALPHA);
    printf("Filtering Bilinear...\n");
    Mat filtered_bil_depth = filter_bil.update(playback.rgb, inpainted_depth);
    filtered_bil_depth = filter_bil.update(playback.rgb, filtered_bil_depth);
    printf("Filtering Median...\n");
    Mat filtered_med_depth = filter_med.update(playback.rgb, inpainted_depth);
    filtered_med_depth = filter_med.update(playback.rgb, filtered_med_depth);
    
    printf("Visualizing...\n");
    Mat out_img, inpainted_out_img, filtered_bil_img, filtered_med_img;

    visualize(playback.depth, out_img);
    visualize(inpainted_depth, inpainted_out_img);
    visualize(filtered_bil_depth, filtered_bil_img);
    visualize(filtered_med_depth, filtered_med_img);
    
    printf("Done...\n");
    imshow("rgb", playback.rgb);
    imshow("depth", out_img);
    imshow("inpaint", inpainted_out_img);
    imshow("filtered bilinear", filtered_bil_img);
    imshow("filtered median", filtered_med_img);
    waitKey(0);
    if (writer.isOpened()) {
      writer << filtered_bil_img;
    }
  }
  return 1;
}
