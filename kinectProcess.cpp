#include "stdafx.h"
#include "kinectProcess.h"

HRESULT kinect::connectKinect() {
	INuiSensor* pNuiSensor;
	HRESULT hr;

	int iSensorCount = 0;
	hr = NuiGetSensorCount(&iSensorCount);

	sensors.resize(iSensorCount);
	ColorStreamHandle.resize(iSensorCount);
	ColorFrameEvent.resize(iSensorCount);
	DepthStreamHandle.resize(iSensorCount);
	DepthFrameEvent.resize(iSensorCount);


	if (FAILED(hr))
	{
		return hr;
	}

	// Look at each Kinect sensor
	for (int i = 0; i < iSensorCount; i++)
	{

		// Create the sensor so we can check status, if we can't create it, move on to the next
		hr = NuiCreateSensorByIndex(i, &pNuiSensor);

		if (FAILED(hr))
		{
			continue;
		}

		// Get the status of the sensor, and if connected, then we can initialize it
		hr = pNuiSensor->NuiStatus();
		if (S_OK == hr)
		{
			sensors[i] = pNuiSensor;
			//break;
		}

		// This sensor wasn't OK, so release it since we're not using it
		pNuiSensor->Release();

		if (NULL != sensors[i])
		{

			// Initialize the Kinect and specify that we'll be using color
			hr = sensors[i]->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH);
			if (SUCCEEDED(hr))
			{
				// Create an event that will be signaled when color data is available
				ColorFrameEvent[i] = CreateEvent(NULL, TRUE, FALSE, NULL);

				// Open a color image stream to receive color frames
				hr = sensors[i]->NuiImageStreamOpen(
					NUI_IMAGE_TYPE_COLOR,
					NUI_IMAGE_RESOLUTION_640x480,
					0,
					2,
					ColorFrameEvent[i],
					&ColorStreamHandle[i]);

				// Create an event that will be signaled when depth data is available
				DepthFrameEvent[i] = CreateEvent(NULL, TRUE, FALSE, NULL);

				// Open a depth image stream to receive depth frames
				hr = sensors[i]->NuiImageStreamOpen(
					NUI_IMAGE_TYPE_DEPTH,
					NUI_IMAGE_RESOLUTION_640x480,
					0,
					2,
					DepthFrameEvent[i],
					&DepthStreamHandle[i]);
			}


		}

		if (NULL == sensors[i] || FAILED(hr))
		{
			cout << "kinect id:" << i << " not Ready!" << endl;
			return E_FAIL;
		}
	}

	return hr;
}

/////////////////////////////////////////////////////// Color

void kinect::getColorData(int idKinect, vector<byte> *ColorBytes) 
{
	HRESULT hr;
	NUI_IMAGE_FRAME imageFrame;

	// Attempt to get the color frame
	hr = sensors[idKinect]->NuiImageStreamGetNextFrame(ColorStreamHandle[idKinect], 0, &imageFrame);
	if (FAILED(hr))
	{
		return;
	}

	INuiFrameTexture* pTexture = imageFrame.pFrameTexture;
	NUI_LOCKED_RECT LockedRect;

	// Lock the frame data so the Kinect knows not to modify it while we're reading it
	pTexture->LockRect(0, &LockedRect, NULL, 0);
	ColorBytes->resize(LockedRect.size);
	memcpy(ColorBytes->data(), LockedRect.pBits, LockedRect.size);
	// We're done with the texture so unlock it
	pTexture->UnlockRect(0);

	// Release the frame
	sensors[idKinect]->NuiImageStreamReleaseFrame(ColorStreamHandle[idKinect], &imageFrame);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

/////////////////////////////////////////////////////// Depth

void kinect::getDepthData(int idKinect, vector<byte>* DepthBytes)
{
	HRESULT hr;
	NUI_IMAGE_FRAME imageFrame;
	HANDLE m_pDepthStreamHandle;
	// Попытка получить кадр глубины
	hr = sensors[idKinect]->NuiImageStreamGetNextFrame(DepthStreamHandle[idKinect], 0, &imageFrame);
	if (FAILED(hr))
	{
		return;
	}

	BOOL nearMode;
	INuiFrameTexture* pTexture;

	// Получить глубину пиксельной текстуры изображения
	hr = sensors[idKinect]->NuiImageFrameGetDepthImagePixelFrameTexture(
		DepthStreamHandle[idKinect], &imageFrame, &nearMode, &pTexture);
	if (FAILED(hr))
	{
		goto ReleaseFrame;
	}

	NUI_LOCKED_RECT LockedRect;

	// Блокируем данные фрейма, чтобы Kinect знал, что не нужно изменять его во время чтения
	pTexture->LockRect(0, &LockedRect, NULL, 0);

	// Убедитесь, что мы получили действительные данные
	if (LockedRect.Pitch != 0)
	{
		DepthBytes->resize(LockedRect.size);
		memcpy(DepthBytes->data(), LockedRect.pBits, LockedRect.size);
	}

	// Мы закончили с текстурой, поэтому разблокируйте ее
	pTexture->UnlockRect(0);
	// Освободить текстуру
	pTexture->Release();

ReleaseFrame:
	// Освободить кадр
	sensors[idKinect]->NuiImageStreamReleaseFrame(DepthStreamHandle[idKinect], &imageFrame);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
