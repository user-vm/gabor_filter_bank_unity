using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Unity.Collections;
using Unity.Collections.LowLevel.Unsafe;
using UnityEngine;
using UnityEngine.UI;

public class GetCameraImage : MonoBehaviour
{
    private WebCamTexture webCamTexture;
    private string path;
    private RawImage imgDisplayforPhotoshop;
    
    private Texture2D snapTexture;
    private NativeArray<byte> snapCPUTexture;
    private unsafe byte* snapCPUTextureData;
    
    private Texture2D outputTexture;
    private NativeArray<byte> outputCPUTexture;
    private unsafe byte* outputCPUTextureData;

    private double sigma;
    private double lambda;
    private int numAngles;
    private double psi;

    public int maxAngles;
    private double maxSigma = 21; //needs to be equal to the kernel size, which is hardcoded on the CPP side (kernel size = HKS*2+1)

    public InputField sigmaInputField;
    public InputField lambdaInputField;
    public InputField numAnglesInputField;
    public InputField psiInputField;

    public Toggle showLastOutputToggle;
    public Toggle showLastInputToggle;
    public Toggle showWebcamToggle;

    public void SigmaValueChangeCheck(String sigmaInput)
    {
        if (double.TryParse(sigmaInput, out var sigmaValue))
        {
            sigma = Math.Min(Math.Max(0.0, sigmaValue), maxSigma);
        }
        sigmaInputField.text = sigma.ToString(); //replaces text input with last sigma value if the new one doesn't work, or constrains the new value to the limits
    }

    public void LambdaValueChangeCheck(String lambdaInput)
    {
        if (double.TryParse(lambdaInput, out var lambdaValue))
        {
            lambda = lambdaValue;
        }
        else
        {
            lambdaInputField.text = lambda.ToString();
        }
        
    }

    public void NumAnglesValueChangeCheck(String numAnglesInput)
    {
        if (int.TryParse(numAnglesInput, out var numAnglesValue))
        {
            numAngles = numAnglesValue;
            numAngles = Math.Min(Math.Max(1, numAngles), maxAngles); //enforce limits
        }

        //update input field
        numAnglesInputField.text = numAngles.ToString();
    }

    public void PsiValueChangeCheck(String psiInput)
    {
        if (double.TryParse(psiInput, out var psiValue))
        {
            psi = psiValue;
        }
        else
        {
            psiInputField.text = psi.ToString();
        }
    }

    public void ShowLastInput()
    {
        GetComponent<Renderer>().material.mainTexture = snapTexture;
    }
    
    public void ShowLastOutput()
    {
        GetComponent<Renderer>().material.mainTexture = outputTexture;
    }

    public void ShowWebcam()
    {
        GetComponent<Renderer>().material.mainTexture = webCamTexture;
    }

    // Start is called before the first frame update
    void Start()
    {
        //get webcam input and display on screen
        webCamTexture = new WebCamTexture();
        GetComponent<Renderer>().material.mainTexture = webCamTexture;
        webCamTexture.Play();
        
        //create these now to avoid errors if the toggles are changed before GetWrinklesTexture is called 
        snapTexture = new Texture2D(webCamTexture.width, webCamTexture.height, TextureFormat.ARGB32,
            false); //turn mipmapping off, or else the image data has a weird size
        outputTexture = new Texture2D(snapTexture.width, snapTexture.height, TextureFormat.ARGB32,
            false);

        //populate detectWrinkles-required parameters
        SigmaValueChangeCheck(sigmaInputField.text);
        LambdaValueChangeCheck(lambdaInputField.text);
        NumAnglesValueChangeCheck(numAnglesInputField.text);
        PsiValueChangeCheck(psiInputField.text);
    }

    /// <summary>
    /// Get a screenshot from the webcam and run the Gabor filter bank
    /// </summary>
    public void CaptureFrame()
    {
        if (snapTexture is null)
            snapTexture = new Texture2D(webCamTexture.width, webCamTexture.height, TextureFormat.ARGB32,
                false); //turn mipmapping off, or else the image data has a weird size
        snapTexture.SetPixels(webCamTexture.GetPixels());
        snapTexture.Apply();

        GetComponent<Renderer>().material.mainTexture = snapTexture;
        
        GetWrinklesTexture();
    }
    
    /// <summary>
    /// Get output of Gabor filter bank on the webcam screenshot
    /// </summary>
    private void GetWrinklesTexture()
    {
        if (outputTexture is null)
            outputTexture = new Texture2D(snapTexture.width, snapTexture.height, TextureFormat.ARGB32,
                false);
        // if implementing on a still image from file instead of the webcam input, a size check will need to be done for the input and output texture, and a new texture created if necessary
        
        //copy textures to CPU
        snapCPUTexture = snapTexture.GetRawTextureData<byte>();
        outputCPUTexture = outputTexture.GetRawTextureData<byte>();

        unsafe
        {
            //get pointers to raw data
            snapCPUTextureData = (byte*) snapCPUTexture.GetUnsafeReadOnlyPtr();
            outputCPUTextureData = (byte*) outputCPUTexture.GetUnsafePtr();

            //call detectWrinkles with this frame
            //the segmentationData can be used if using an example image instead of a webcam frame, which also has a skin segmentation map
            detectWrinkles(snapCPUTextureData, outputCPUTextureData, null, snapTexture.height, snapTexture.width, sigma, lambda, numAngles, psi);

            //assign output to texture
            outputTexture.LoadRawTextureData(outputCPUTexture);
            outputTexture.Apply();
        }
        
        //display output
        GetComponent<Renderer>().material.mainTexture = outputTexture;
        //select appropriate toggle
        showLastOutputToggle.isOn = true;
    }

    // Update is called once per frame
    private void Update()
    {
        
    }

    /// <summary>
    /// 
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
    [DllImport("wrinkles_plugin")]
    private static extern unsafe void detectWrinkles(byte* imageData, byte* outputData, byte* segmentationData,
        int height, int width, double sigma, double lambda, int numAngles, double psi);
}
