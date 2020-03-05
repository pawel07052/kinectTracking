#pragma once

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

using namespace std; // использование пространства std
using namespace cv; // использование пространства opencv