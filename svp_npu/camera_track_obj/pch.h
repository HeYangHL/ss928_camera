// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include<vector>

using namespace std;

#endif //PCH_H



typedef struct RetureInfo
{
	int trackID;
	int detectID;
	int width;
	int height;
	int x;  //左上角坐标
	int y;  //左上角坐标
  int type; //类型
}RetureInfo;


struct TrackParam
{
	int max_false_age; //误检的车辆存在的最大周期限制
	int max_lost_age; //长期丢失的车辆存在的最大周期限制
	int min_hits;
	double iouThreshold;
	TrackParam()
	{
		max_false_age = 1; //误检的车辆存在的最大周期限制
		max_lost_age = 2; //长期丢失的车辆存在的最大周期限制
		min_hits = 2;
		iouThreshold = 0.3;
	}
};


//初始化接口，返回handle
void* InitPara(TrackParam param);

//跟踪接口，传递handle和carsPoint
//vecor<float>中间包含6的倍数个车辆位置 ，每个车辆位置由detectID,左上角x,y,边框宽度w 和高度h、类型等六个浮点数数组成，如果有N辆车，则float数量为N*6 个。
//输出是跟踪的车辆信息
vector<RetureInfo>  GetTrackInfos(void* handle, vector<float> carsPoint);

//释放接口
void  Release(void* handle);