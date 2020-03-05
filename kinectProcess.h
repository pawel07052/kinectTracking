#pragma once


class kinect {

	vector<INuiSensor*> sensors;

	vector<HANDLE> ColorStreamHandle;
	vector<HANDLE> ColorFrameEvent;

	vector<HANDLE> DepthStreamHandle;
	vector<HANDLE> DepthFrameEvent;

	

public:
	HRESULT connectKinect();

	void getColorData(int idKinect, vector<byte> *ColorBytes);
	void getDepthData(int idKinect, vector<byte>* DepthBytes);

};