///////////////////////////////////////////////////////////////////////////////
// KalmanTracker.h: KalmanTracker Class Declaration

#ifndef KALMAN_H
#define KALMAN_H 2

#include "opencv2/video/tracking.hpp"
#include "opencv2/highgui/highgui.hpp"
#include<list>
using namespace std;
using namespace cv;

#define StateType Rect_<float>


// This class represents the internel state of individual tracked objects observed as bounding box.
class KalmanTracker
{
public:
	KalmanTracker()
	{
		init_kf(StateType());
		m_time_since_update = 0;
		m_hits = 0;
		m_hit_streak = 0;
		m_age = 0;
		m_id = kf_count;
		is_real_car = false;
		real_history_length = 3;
		is_move = true;
		//kf_count++;
	}
	KalmanTracker(StateType initRect,int detectID,int type)
	{
		init_kf(initRect);
		m_time_since_update = 0;
		m_hits = 0;
		m_hit_streak = 0;
		m_age = 0;
		m_id = kf_count;
		kf_count++;
		is_real_car = false;
		is_move = true;
		real_history_length = 20;
		this->detectID = detectID;
    this->type = type;
	}

	~KalmanTracker()
	{
		m_history.clear();
	}

	StateType predict();
	void update(StateType stateMat, int detectID, bool flag = true);
	
	StateType get_state(float& vx, float& vy);
	StateType get_rect_xysr(float cx, float cy, float s, float r);
	void set_real_car_flag();
	bool get_real_car_flag();

	void set_move(bool flag);
	bool get_move();
	static int kf_count;
	std::vector<StateType> get_history();
	std::list<StateType> get_real_history();
	int m_time_since_update;
	int m_hits;
	int m_hit_streak;
	int m_age;
	int m_id;
	int real_history_length;
	int detectID;
  int type;
	void init_kf(StateType stateMat);
private:
	

	cv::KalmanFilter kf;
	cv::Mat measurement;
	bool is_real_car;
	bool is_move;
	std::vector<StateType> m_history;
	std::list<StateType> m_real_history;
};




#endif