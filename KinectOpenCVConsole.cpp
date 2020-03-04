#include <iostream>
#include <ctime>
#include <windows.h>
#include <strsafe.h>
#include <vector>

#include <thread> // для работы с потоками 
#include <chrono> // для работы со временем

//подключение библиотеки для работы с кинектом
#include "NuiApi.h"


//подключение библиотеки opencv
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

//#include <opencv2/core/cuda.hpp>

using namespace std;

HANDLE m_pColorStreamHandle;
HANDLE m_hNextColorFrameEvent;

HANDLE m_pDepthStreamHandle;
HANDLE m_hNextDepthFrameEvent;
BYTE* m_depthRGBX;

NUI_IMAGE_FRAME imageFrame;
INuiSensor* m_pNuiSensor;

cv::Mat imgThresholded;

int iHRed, iHGreen, iHBlue, iLRed, iLGreen, iLBlue = 0;

int Rmin = 0;
int Rmax = 256;

int Gmin = 0;
int Gmax = 256;

int Bmin = 0;
int Bmax = 256;

int RGBmax = 256;


// functions
HRESULT CreateFirstConnected();

void ProcessColor();
void ProcessDepth();
void createSettings(bool on);

void frameToGray(cv::Mat originalFrame, cv::Mat outGrayFrame, bool showFrame);

// fps
int frameCounter = 0;
int tick = 0;
int fps;
std::time_t timeBegin = std::time(0);

vector<vector<cv::Point>> contours;

cv::Mat imTest(480, 640, CV_8UC4);

HRESULT hr2;
int main()
{
	HRESULT hr = CreateFirstConnected();
	 
	if (S_OK == hr) {
		m_depthRGBX = new BYTE[640 * 480 * CV_8UC4];

		createSettings(true);

		cv::namedWindow("testEditImage");
		cv::namedWindow("BlurImage");
		//cv::createTrackbar("Low", "testEditImage", &Low, 255); //Value (0 - 255)
		//cv::createTrackbar("High", "testEditImage", &High, 255);
		//создание окна opencv
		cv::namedWindow("KinectImage");
		cv::namedWindow("KinectDepth");
		while (cv::waitKey(33) != 27) {
			std::thread th(ProcessColor);
			std::thread th2(ProcessDepth);
			th.detach();
			th2.detach();
		}
	}
	return 0;
}

HRESULT CreateFirstConnected()
{
	INuiColorCameraSettings* settings;
	INuiSensor* pNuiSensor;
	HRESULT hr;

	int iSensorCount = 0;
	hr = NuiGetSensorCount(&iSensorCount);
	
	if (FAILED(hr))
	{
		return hr;
	}

	// Look at each Kinect sensor
	for (int i = 0; i < iSensorCount; ++i)
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
			m_pNuiSensor = pNuiSensor;
			break;
		}

		// This sensor wasn't OK, so release it since we're not using it
		pNuiSensor->Release();
	}
	

	if (NULL != m_pNuiSensor)
	{
		
		// Initialize the Kinect and specify that we'll be using color
		hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH);
		if (SUCCEEDED(hr))
		{
			// Create an event that will be signaled when color data is available
			m_hNextColorFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	
			// Open a color image stream to receive color frames
			hr = m_pNuiSensor->NuiImageStreamOpen(
				NUI_IMAGE_TYPE_COLOR,
				NUI_IMAGE_RESOLUTION_640x480,
				0,
				2,
				m_hNextColorFrameEvent,
				&m_pColorStreamHandle);

			// Create an event that will be signaled when depth data is available
			//m_hNextDepthFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

			// Open a depth image stream to receive depth frames
			hr = m_pNuiSensor->NuiImageStreamOpen(
				NUI_IMAGE_TYPE_DEPTH,
				NUI_IMAGE_RESOLUTION_640x480,
				0,
				2,
				m_hNextDepthFrameEvent,
				&m_pDepthStreamHandle);
		}


	}


	if (NULL == m_pNuiSensor || FAILED(hr))
	{
		std::cout << "No ready Kinect found!" << std::endl;
		return E_FAIL;
	}

	return hr;
}

void ProcessColor()
{
	HRESULT hr;
	NUI_IMAGE_FRAME imageFrame;

	// Attempt to get the color frame
	hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_pColorStreamHandle, 0, &imageFrame);
	if (FAILED(hr))
	{
		return;
	}

	INuiFrameTexture* pTexture = imageFrame.pFrameTexture;
	NUI_LOCKED_RECT LockedRect;

	// Lock the frame data so the Kinect knows not to modify it while we're reading it
	pTexture->LockRect(0, &LockedRect, NULL, 0);

		// Преобразование буффера данных с кинекта в Mat
		cv::Mat texture = cv::Mat(480, 640, CV_8UC4, LockedRect.pBits);
		//gpuMatFrame.upload(texture);
		// счётчик кадров
		frameCounter++;
		// новое время кадра
		std::time_t timeNow = std::time(0) - timeBegin;

		if (timeNow - tick >= 1)
		{
			tick++;
			fps = frameCounter;
			frameCounter = 0;
		}

		if (fps > 25) {
			cv::putText(texture, cv::format("FPS=%d", fps), cv::Point(30, 30), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0));
		}
		else if(fps <= 25 && fps > 15){
			cv::putText(texture, cv::format("FPS=%d", fps), cv::Point(30, 30), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 255));
		}
		else {
			cv::putText(texture, cv::format("FPS=%d", fps), cv::Point(30, 30), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255));
		}


		cv::Mat blurBin(480, 640, CV_8UC4);

		cv::inRange(texture, cv::Scalar(iHRed, iHGreen, iHBlue), cv::Scalar(iLRed, iLGreen, iLBlue), imTest);

		cv::GaussianBlur(imTest, blurBin, cv::Size(3,3), 0);

		cv::findContours(blurBin, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
		//cv::drawContours(texture, contours, 0, cv::Scalar(255, 0, 255), 2);

		//cout << contours.size() << endl;
		cv::imshow("KinectImage", texture);
		cv::imshow("testEditImage", imTest);
		cv::imshow("BlurImage", blurBin);
		


	
	// We're done with the texture so unlock it
	pTexture->UnlockRect(0);

	// Release the frame
	m_pNuiSensor->NuiImageStreamReleaseFrame(m_pColorStreamHandle, &imageFrame);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

cv::Point2f pos;

void ProcessDepth() {
	HRESULT hr;
	NUI_IMAGE_FRAME imageFrame;

	// Попытка получить кадр глубины
	hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_pDepthStreamHandle, 0, &imageFrame);
	if (FAILED(hr))
	{
		return;
	}

	BOOL nearMode;
	INuiFrameTexture* pTexture;

	// Получить глубину пиксельной текстуры изображения
	hr = m_pNuiSensor->NuiImageFrameGetDepthImagePixelFrameTexture(
		m_pDepthStreamHandle, &imageFrame, &nearMode, &pTexture);
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
		// Получить минимальную и максимальную достоверную глубину для текущего кадра
		int minDepth = (nearMode ? NUI_IMAGE_DEPTH_MINIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MINIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
		int maxDepth = (nearMode ? NUI_IMAGE_DEPTH_MAXIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MAXIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;

		BYTE* rgbrun = m_depthRGBX;
		const NUI_DEPTH_IMAGE_PIXEL* pBufferRun = reinterpret_cast<const NUI_DEPTH_IMAGE_PIXEL*>(LockedRect.pBits);

		// конечным пикселем является начало + ширина * высота - 1
		const NUI_DEPTH_IMAGE_PIXEL* pBufferEnd = pBufferRun + (640 * 480);

		int i = 0;
		while (pBufferRun < pBufferEnd)
		{
			i++;
			// отбрасываем часть глубины, которая содержит только индекс игрока
			USHORT depth = pBufferRun->depth;
			USHORT depth2 = depth;
			
			
			// Чтобы преобразовать в байт, мы отбрасываем наиболее значимые
			// а не наименее значимые биты.
			// Мы сохраняем детали, хотя интенсивность будет «оборачиваться».
			// Значения за пределами надежного диапазона глубины отображаются на 0 (черный).

			// Примечание. Использование условных выражений в этом цикле может снизить производительность.
			// Рассмотрим вместо этого использование таблицы поиска при написании производственного кода.

			BYTE intensity = static_cast<BYTE>(depth >= minDepth && depth <= maxDepth ? depth % 256 : 0);


			/*if (i > 153270 && i < 153290) {

				*(rgbrun++) = 255;

				// Выписать зеленый байт
				*(rgbrun++) = 0;

				// Выписать красный байт
				*(rgbrun++) = 0;
				if (i == 153280) {
					cout << (int)depth2 << endl;
				}
			}
			else {*/

				/*// Выписать синий байт
				*(rgbrun++) = intensity;

				// Выписать зеленый байт
				*(rgbrun++) = intensity;

				// Выписать красный байт
				*(rgbrun++) = intensity;
				*/

			int data = depth2 * 768 / maxDepth;	

				*(rgbrun++) = data;

				*(rgbrun++) = data;

				*(rgbrun++) = data;

			//}

			// Мы выводим BGR, последний байт в 32 битах не используется, поэтому пропустите его
			// Если бы мы выводили BGRA, мы бы написали здесь альфа.
			++rgbrun;

			// Увеличиваем наш индекс в буфер глубины Kinect
			++pBufferRun;

			
		}
		//cout << i << endl;
		// Преобразование глубины в Mat

		
		

		cv::Mat texture = cv::Mat(480, 640, CV_8UC4, m_depthRGBX);
		cv::drawContours(texture, contours, 0, cv::Scalar(255, 0, 255), 2);
		if (contours.size() != 0) {
			float radius;
			
			//cv::minEnclosingCircle(contours[0], pos, radius);
			//cout << "pos x = " << pos.x << " pos y = " << pos.y << " radius = " << radius << endl;
		}
		cv::imshow("KinectDepth", texture);
	}

	// Мы закончили с текстурой, поэтому разблокируйте ее
	pTexture->UnlockRect(0);
	// Освободить текстуру
	pTexture->Release();

ReleaseFrame:
	// Освободить кадр
	m_pNuiSensor->NuiImageStreamReleaseFrame(m_pDepthStreamHandle, &imageFrame);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void createSettings(bool on = false) {
	if (on == true) {
		cv::namedWindow("Settings");
		cv::resizeWindow("Settings", 640, 300);

		cv::createTrackbar("RedH", "Settings", &iHRed, 255); //Hue (0 - 179)
		cv::createTrackbar("RedL", "Settings", &iLRed, 255);

		cv::createTrackbar("GreenH", "Settings", &iHGreen, 255); //Saturation (0 - 255)
		cv::createTrackbar("GreenL", "Settings", &iLGreen, 255);

		cv::createTrackbar("BlueH", "Settings", &iHBlue, 255); //Value (0 - 255)
		cv::createTrackbar("BlueL", "Settings", &iLBlue, 255);
	}
}

void frameToGray(cv::Mat originalFrame, cv::Mat outGrayFrame, bool showFrame = false) {
	//cv::cvtColor(originalFrame, outGrayFrame, cv::COLOR_BGR2GRAY);
	cv::cvtColor(originalFrame, outGrayFrame, cv::COLOR_BGR2HSV);
	if (showFrame) {
		cv::namedWindow("Gray");
		cv::imshow("Gray", outGrayFrame);
	}
}


