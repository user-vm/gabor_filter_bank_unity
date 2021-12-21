#include "wrinkles.h"
#include <cinttypes>

int HKS = 10; //half of kernel size

//Example values for Gabor filter parameters
//sigma = 5.0;
//lm = 1.0;
//th = 0;
//psi = 90;

/// <summary>
/// Creates the Gabor filter kernel.
/// from https://github.com/dominiklessel/opencv-gabor-filter/blob/master/gaborFilter.cpp
/// </summary>
/// <param name="sig">The sigma/standard deviation of the Gaussian envelope of the filter</param>
/// <param name="th">The angle theta if the Gabor filter (orientation of the normal to the parallel stripes of a Gabor function), in degrees</param>
/// <param name="lm">The wavelength lambda of the sinusoidal factor of the filter</param>
/// <param name="ps">The phase offset psi of the filter, in degrees</param>
/// <returns></returns>
cv::Mat mkKernel(double sig, double th, double lm, double ps)
{
    int ks = HKS * 2 + 1;
    double theta = th * CV_PI / 180;
    double psi = ps * CV_PI / 180;
    double del = 1.0 / HKS;
    double lmbd = lm;
    double sigma = sig / ks;
    double x_theta;
    double y_theta;
    cv::Mat kernel(ks, ks, CV_32F);
    for (int y = -HKS; y <= HKS; y++)
    {
        for (int x = -HKS; x <= HKS; x++)
        {
            x_theta = x * del * cos(theta) + y * del * sin(theta);
            y_theta = -x * del * sin(theta) + y * del * cos(theta);
            kernel.at<float>(HKS + y, HKS + x) = (float)exp(-0.5 * (pow(x_theta, 2) + pow(y_theta, 2)) / pow(sigma, 2)) * cos(2 * CV_PI * x_theta / lmbd + psi);
        }
    }
    return kernel;
}

/// <summary>
/// Sets alpha value of all pixels to the same value, for an ARGB image (CV_8UC4/1 byte per channel)
/// </summary>
/// <param name="img">The image to modify</param>
/// <param name="value">The value to set the alpha value to</param>
void setAlphaChannelARGB(cv::Mat img, uchar value){ //TODO: maybe use namespaces
    const int cols = img.cols;
    const int step = img.channels();
    const int rows = img.rows;
    for (int y = 0; y < rows; y++) {
        unsigned char* p_row = img.ptr(y); //gets pointer to the first byte to be changed in this row. First byte corresponds to the alpha channel of the first pixel of the row.
        unsigned char* row_end = p_row + cols * step;
        for (; p_row != row_end; p_row += step)
            *p_row = value;
    }
}



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
extern "C" void EXPORT_CS detectWrinkles(uchar* imageData, uchar* outputData, uchar* segmentationData, int32_t height, int32_t width, double sigma, double lambda, int32_t numAngles, double psi) {

    cv::Mat imageARGB = cv::Mat(height, width, CV_8UC4, imageData); //TODO: check if the format needs changing
    cv::Mat imageRGB = cv::Mat(height, width, CV_8UC3);
    cv::Mat imageGray = cv::Mat(height, width, CV_8U);
    cv::Mat imageGrayFloat = cv::Mat_<float>(height, width);

    cv::Mat outputGrayFloat = cv::Mat_<float>(height, width);

    cv::Mat cumulativeOutputGrayFloat = cv::Mat_<float>(height, width, 0.0);
    cv::Mat cumulativeOutputGray = cv::Mat_<float>(height, width, 0.0);
    cv::Mat cumulativeOutputARGB = cv::Mat(height, width, CV_8UC4, outputData);

    cv::Mat kernel;
    int th; //current Gabor filter angle

    int chARGBToRGB[] = { 1, 0, 2, 1, 3, 2 };
    cv::mixChannels(imageARGB, imageRGB, chARGBToRGB, 3);
    cv::cvtColor(imageRGB, imageGray, cv::COLOR_RGB2GRAY);
    imageGray.convertTo(imageGrayFloat, CV_32FC1);

    for (int i = 0; i < numAngles; i++) {

        kernel = mkKernel(sigma, i * 180 / numAngles, lambda, psi);
        cv::filter2D(imageGrayFloat, outputGrayFloat, CV_32F, kernel);
        cumulativeOutputGrayFloat = max(cumulativeOutputGrayFloat, outputGrayFloat);
    }

    cumulativeOutputGrayFloat.convertTo(cumulativeOutputGray, CV_8UC1);

    int chGrayToARGB[] = { 0, 1, 0, 2, 0, 3 };
    cv::mixChannels(cumulativeOutputGray, cumulativeOutputARGB, chGrayToARGB, 3);

    setAlphaChannelARGB(cumulativeOutputARGB, 255); //set A channel to maximum in cumulativeOutputARGB. Might not be necessary if it's already set that way from Unity initialization.
	
	//TODO: multiply by segmentation map. Not particularly useful unless support for segmentation maps is added on the Unity side.
}
