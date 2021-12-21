#ifndef WRINKLES_PLUGIN_LIBRARY_H
#define WRINKLES_PLUGIN_LIBRARY_H

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <math.h>

#if defined(_WIN32)
#define EXPORT_CS __declspec(dllexport)
#else
#define EXPORT_CS 
#endif

/// <summary>
/// Creates the Gabor filter kernel.
/// from https://github.com/dominiklessel/opencv-gabor-filter/blob/master/gaborFilter.cpp
/// </summary>
/// <param name="sig">The sigma/standard deviation of the Gaussian envelope of the filter</param>
/// <param name="th">The angle theta if the Gabor filter (orientation of the normal to the parallel stripes of a Gabor function), in degrees</param>
/// <param name="lm">The wavelength lambda of the sinusoidal factor of the filter</param>
/// <param name="ps">The phase offset psi of the filter, in degrees</param>
/// <returns></returns>
cv::Mat mkKernel(double sig, double th, double lm, double ps);

/// <summary>
/// Sets alpha value of all pixels to the same value, for an ARGB image (CV_8UC4/1 byte per channel)
/// </summary>
/// <param name="img">The image to modify</param>
/// <param name="value">The value to set the alpha value to</param>
void setAlphaChannelARGB(cv::Mat img, uchar value);

/// <summary>
/// Calculates the output of a Gabor filter bank with equally spaced filter angles, with a given image as input. If a segmentation mask is provided corresponding to the skin of the face of a user, this can be used as a wrinkle detector.
/// </summary>
/// <param name="imageData">Input ARGB image bytes (height*width*4 bytes)</param>
/// <param name="outputData">Output ARGB image bytes (height*width*4 bytes)</param>
/// <param name="segmentationData">Single-channel 8-bit mask (CV_8U), which is multiplied with the output of the Gabor filter bank</param>
/// <param name="height">Height of the input image, output and segmentation data (they should all match)</param>
/// <param name="width">Width of the input image, output and segmentation data (they should all match)</param>
/// <param name="sigma">The sigma/standard deviation of the Gaussian envelope of the filter. Should be smaller than the filter size (here 21)</param>
/// <param name="lambda">The wavelength lambda of the sinusoidal factor of the filter. Should normally be between 0.5 and 1.5</param>
/// <param name="numAngles">Number of angles in Gabor filter bank (resulting theta angles are equally spaced and equal to i*180/numAngles for 0<=i<numAngles)</param>
/// <param name="psi">The phase offset psi of the filter, in degrees.</param>
extern "C" void EXPORT_CS detectWrinkles(uchar * imageData, uchar * outputData, uchar * segmentationData, int32_t height, int32_t width, double sigma, double lambda, int32_t numAngles, double psi);

#endif //WRINKLES_PLUGIN_LIBRARY_H
