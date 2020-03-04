#include "stdafx.h"
#include "kinectProcess.h"
#include "kinectOpenCV.h"
//#include <opencv2/core/cuda.hpp>

kinect devices;
kinectOpenCV imageEdit;

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


void createSettings(bool on);

// fps
int frameCounter = 0;
int tick = 0;
int fps;
std::time_t timeBegin = std::time(0);

vector<vector<cv::Point>> contours;

cv::Mat imTest(480, 640, CV_8UC4);

HRESULT hr2;

byte image;

int main()
{
	vector<byte> image;
	vector<byte> depth;
	devices.connectKinect();

	devices.getColorData(0, &image);
	devices.getDepthData(0, &depth);

	return 0;
}

/*
void ProcessDepth() {
	HRESULT hr;
	NUI_IMAGE_FRAME imageFrame;
	HANDLE m_pDepthStreamHandle;
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
*/

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

/*			int data = depth2 * 768 / maxDepth;	

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
*/

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


