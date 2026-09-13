// pch.cpp: 与预编译标头对应的源文件

#include "pch.h"
#include "opencv2/opencv.hpp"
#include "Hungarian.h"
#include "KalmanTracker.h"

#include "opencv2/video/tracking.hpp"

using namespace cv;

// 当使用预编译的头时，需要使用此源文件，编译才能成功。

const float MIN_VALUE = 1;
typedef struct TrackingBox
{
	int frame;
	int trackID;
	int detectID;
	Rect_<float> box;
	int type;
} TrackingBox;

class CarTracking
{
public:
	vector<KalmanTracker> trackers;
	TrackParam param;
	CarTracking()
	{
		KalmanTracker::kf_count = 0;
	}
};

// Computes IOU between two bounding boxes
double GetIOU(Rect_<float> bb_test, Rect_<float> bb_gt)
{
	float in = (bb_test & bb_gt).area();
	float un = bb_test.area() + bb_gt.area() - in;

	if (un < DBL_EPSILON)
		return 0;

	return (double)(in / un);
}

void *InitPara(TrackParam param)
{
	cout << "init start" << endl;
	CarTracking *p_car = new CarTracking();
	p_car->param = param;
	cout << "init end" << endl;
	return p_car;
}

void Release(void *p)
{
	CarTracking *p_car = (CarTracking *)(p);
	delete p_car;
}

bool TrakingCarsID(vector<TrackingBox> carsList, vector<TrackingBox> &cars, CarTracking *p_car);
static bool boxCompare(TrackingBox &b1, TrackingBox &b2)
{
	return b1.box.area() < b2.box.area();
}
vector<RetureInfo> GetTrackInfos(void *p, vector<float> carsPoint)
{

	CarTracking *p_car = (CarTracking *)(p);
	vector<TrackingBox> carsRect, carsRectFilter;
	int image_width, image_height;

	for (size_t j = 0; j < carsPoint.size(); j = j + 6)
	{
		TrackingBox car;
		car.detectID = int(carsPoint[j]);
		car.box.x = carsPoint[j + 1];
		car.box.y = carsPoint[j + 2];
		car.box.width = carsPoint[j + 3];
		car.box.height = carsPoint[j + 4];
		car.type = int(carsPoint[j + 5]);
		if (car.box.width < MIN_VALUE || car.box.height < MIN_VALUE)
			continue;
		carsRect.push_back(car);
	}

	// 过滤大框包围小框的情况 首先按照框的大小排序，然后和所有的比较 在框内的被抑制
	sort(carsRect.begin(), carsRect.end(), boxCompare);
	// printf("---> carsRect size : %d\n", carsRect.size());
	for (int i = 0; i < carsRect.size(); i++)
	{
		bool flag = true;
		/*Rect_<float> box1 = carsRect[i].box;
		for (int j = i + 1; j < carsRect.size(); j++)
		{
			Rect_<float> box2 = carsRect[j].box;
			if (int(box1.x) >= int(box2.x) && int(box1.y) >= int(box2.y) && int(box1.x + box1.width) <= int(box2.x + box2.width) && int(box1.y + box1.height) <= int(box2.y + box2.height))
			{
				flag = false;
				break;
			}
		}*/
		if (flag)
			carsRectFilter.push_back(carsRect[i]);
	}

	vector<TrackingBox> cars_track;
	// printf("--->carsRectFilter size : %d\n", carsRectFilter.size());
	TrakingCarsID(carsRectFilter, cars_track, p_car);
	// printf("--->cars_track size : %d\n", cars_track.size());
	vector<RetureInfo> returnInfos;
	// printf("--->1 returnInfos size : %d\n", returnInfos.size());
	for (int i = 0; i < cars_track.size(); i++)
	{
		TrackingBox car = cars_track[i];

		RetureInfo returnInfo;
		returnInfo.trackID = car.trackID;
		returnInfo.width = car.box.width;
		returnInfo.height = car.box.height;
		returnInfo.x = car.box.x;
		returnInfo.y = car.box.y;
		returnInfo.detectID = car.detectID;
		returnInfo.type = car.type;
		returnInfos.push_back(returnInfo);
	};
	// printf("--->2 returnInfos size : %d\n", returnInfos.size());
	return returnInfos;
}
bool TrakingCarsID(vector<TrackingBox> carsList, vector<TrackingBox> &cars, CarTracking *p_car)
{

	vector<KalmanTracker> &trackers = p_car->trackers;
	TrackParam param = p_car->param;

	vector<Rect_<float>> predictedBoxes;
	vector<vector<double>> iouMatrix;
	vector<int> assignment;
	set<int> unmatchedDetections;
	set<int> unmatchedTrajectories;
	set<int> allItems;
	set<int> matchedItems;
	vector<cv::Point> matchedPairs;

	unsigned int trkNum = 0;
	unsigned int detNum = 0;

	if (trackers.size() == 0) // the first frame met
	{
		// initialize kalman trackers using first detections.
		for (unsigned int i = 0; i < carsList.size(); i++)
		{

			KalmanTracker trk = KalmanTracker(carsList[i].box, carsList[i].detectID, carsList[i].type);
			trackers.push_back(trk);
		}
		// output the first frame detections

		for (auto it = trackers.begin(); it != trackers.end(); it++)
		{
			TrackingBox res;
			float vx, vy;
			res.box = (*it).get_state(vx, vy);
			res.trackID = (*it).m_id;
			res.detectID = (*it).detectID;
			res.type = (*it).type;
			if (res.box.area() > MIN_VALUE)
				cars.push_back(res);
		}
		return true;
	}

	predictedBoxes.clear();

	for (auto it = trackers.begin(); it != trackers.end();)
	{
		Rect_<float> pBox;
		if ((*it).get_move())
			pBox = (*it).predict();
		else
		{
			float vx, vy;
			pBox = (*it).get_state(vx, vy);
		}
		if (pBox.x >= 0 && pBox.y >= 0 && pBox.area() > 0)
		{
			predictedBoxes.push_back(pBox);
			it++;
		}
		else
		{
			// 驶出监控区域 才能 去除，否则保留
			it = trackers.erase(it);
		}
	}

	// 3.2. associate detections to tracked object (both represented as bounding boxes)
	// dets : detFrameData[fi]
	trkNum = predictedBoxes.size();
	if (trkNum == 0)
		return true;
	detNum = carsList.size();

	cout << "trkNum is " << trkNum << "  detNum is " << detNum << endl;
	iouMatrix.clear();
	iouMatrix.resize(trkNum, vector<double>(detNum, 0));

	for (unsigned int i = 0; i < trkNum; i++) // compute iou matrix as a distance matrix
	{
		for (unsigned int j = 0; j < detNum; j++)
		{
			// use 1-iou because the hungarian algorithm computes a minimum-cost assignment.
			iouMatrix[i][j] = 1 - GetIOU(predictedBoxes[i], carsList[j].box);
		}
	}

	HungarianAlgorithm HungAlgo;
	assignment.clear();
	HungAlgo.Solve(iouMatrix, assignment);
	unmatchedTrajectories.clear();
	unmatchedDetections.clear();
	allItems.clear();
	matchedItems.clear();
	printf("预测框数量：%d, 检测框数量%d\n", trkNum, detNum);
	// 处理未匹配项
	if (detNum > trkNum) //	there are unmatched detections
	{
		for (unsigned int n = 0; n < detNum; n++)
			allItems.insert(n);

		for (unsigned int i = 0; i < trkNum; ++i)
			matchedItems.insert(assignment[i]);

		set_difference(allItems.begin(), allItems.end(),
					   matchedItems.begin(), matchedItems.end(),
					   insert_iterator<set<int>>(unmatchedDetections, unmatchedDetections.begin()));
	}
	else if (detNum < trkNum) // there are unmatched trajectory/predictions
	{
		for (unsigned int i = 0; i < trkNum; ++i)
			if (assignment[i] == -1) // unassigned label will be set as -1 in the assignment algorithm
				unmatchedTrajectories.insert(i);
	}
	// 通过交并比值过滤
	//  filter out matched with low IOU
	matchedPairs.clear();
	for (unsigned int i = 0; i < trkNum; ++i)
	{
		printf("--->1\n");
		if (assignment[i] == -1) // pass over invalid values
		{
			printf("索引%d未找到匹配的检测框\n", i);
			continue;
		}
		printf("--->2\n");
		if (1 - iouMatrix[i][assignment[i]] < param.iouThreshold)
		{
			printf("--->3\n");
			unmatchedTrajectories.insert(i);
			unmatchedDetections.insert(assignment[i]);
		}
		else
		{
			printf("检测到匹配项，添加到容器\n");
			matchedPairs.push_back(cv::Point(i, assignment[i]));
		}
		printf("--->4\n");
	}
	// 3.3. updating trackers
	cout << "matchedPairs size is " << matchedPairs.size() << endl;
	// update matched trackers with assigned detections.
	// each prediction is corresponding to a tracker
	int detIdx, trkIdx;
	for (unsigned int i = 0; i < matchedPairs.size(); i++)
	{
		printf("更新匹配项，校准卡尔曼滤波器\n");
		trkIdx = matchedPairs[i].x;
		detIdx = matchedPairs[i].y;
		printf("trackers[trkIdx].m_time_since_update : %d, trkIdx : %d, detIdx : %d\n", trackers[trkIdx].m_time_since_update, trkIdx, detIdx);
		trackers[trkIdx].update(carsList[detIdx].box, carsList[detIdx].detectID);
		printf("trackers[trkIdx].m_time_since_update : %d\n", trackers[trkIdx].m_time_since_update);
	}

	// create and initialise new trackers for unmatched detections
	for (auto umd : unmatchedDetections)
	{
		// 限制坐标范围
		Rect_<float> ubox = carsList[umd].box;
		{
			printf("为新的目标创建追踪器\n");
			KalmanTracker tracker = KalmanTracker(carsList[umd].box, carsList[umd].detectID, carsList[umd].type);
			trackers.push_back(tracker);
			cout << "tracker.trackID is  " << tracker.m_id << endl;
			cout << "tracker.detectID is  " << tracker.detectID << endl;
		}
	}

	cout << "unmatchedDetections size is " << unmatchedDetections.size() << endl;
	printf("===>trackers size : %d\n", trackers.size());
	for (auto it = trackers.begin(); it != trackers.end();)
	{
		printf("===>m_time_since_update : %d, param.max_lost_age : %d\n", (*it).m_time_since_update, param.max_lost_age);
		// remove dead tracklet 一种是很长时间没找到 去掉跟踪目标，一种是目标驶出监控区域
		if ((*it).m_time_since_update >= param.max_lost_age)
		{
			it = trackers.erase(it);
			continue;
		}
		if ((*it).m_time_since_update != 0)
		{
			it++;
			continue;
		}
		
		// 驶出监控区域 删除掉它 不显示
		// cout << "========tracker.trackID is  " << (*it).m_id << endl;
		// cout << "========tracker.detectID is  " << (*it).detectID << endl;
		// if ((*it).m_time_since_update < 1)
		//{
		TrackingBox res;
		float vx, vy;
		res.box = (*it).get_state(vx, vy);
		res.trackID = (*it).m_id;
		res.detectID = (*it).detectID;
		res.type = (*it).type;

		if ((*it).m_hit_streak >= param.min_hits) // 只要有过连续多帧出现 说明是一个运动的车辆
		{
			(*it).set_real_car_flag();
		}
		if ((*it).get_real_car_flag())
			cars.push_back(res);
		//}
		it++;
	}
	return true;
}
