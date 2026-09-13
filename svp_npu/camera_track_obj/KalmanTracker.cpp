///////////////////////////////////////////////////////////////////////////////
// KalmanTracker.cpp: KalmanTracker Class Implementation Declaration

#include "KalmanTracker.h"

#include <iostream>

int KalmanTracker::kf_count = 0;


// initialize Kalman filter
void KalmanTracker::init_kf(StateType stateMat)
{
	int stateNum = 7;
	int measureNum = 4;
	kf = KalmanFilter(stateNum, measureNum, 0);

	measurement = Mat::zeros(measureNum, 1, CV_32F);

	kf.transitionMatrix = (Mat_<float>(stateNum, stateNum) <<
		1, 0, 0, 0, 0.5, 0, 0,
		0, 1, 0, 0, 0, 0.5, 0,
		0, 0, 1, 0, 0, 0, 0.5,
		0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 1);

	setIdentity(kf.measurementMatrix);
	setIdentity(kf.processNoiseCov, Scalar::all(1e-2));
	setIdentity(kf.measurementNoiseCov, Scalar::all(1e-1));
	setIdentity(kf.errorCovPost, Scalar::all(1));
	
	// initialize state vector with bounding box in [cx,cy,s,r] style
	kf.statePost.at<float>(0, 0) = stateMat.x + stateMat.width / 2;
	kf.statePost.at<float>(1, 0) = stateMat.y + stateMat.height / 2;
	kf.statePost.at<float>(2, 0) = stateMat.area();
	kf.statePost.at<float>(3, 0) = stateMat.width / stateMat.height;

	//std::cout << "kf.measurementMatrix is " << kf.measurementMatrix << endl;
	//std::cout << "kf.processNoiseCov is " << kf.processNoiseCov << endl;
	//std::cout << "kf.measurementNoiseCov is " << kf.measurementNoiseCov << endl;
	//std::cout << "kf.errorCovPost is " << kf.errorCovPost << endl;
	//std::cout << "kf.statePost is " << kf.statePost << endl;
	//std::cout << "kf.statePre is " << kf.statePre << endl;

}


// Predict the estimated bounding box.
StateType KalmanTracker::predict()
{
	// predict
	Mat p = kf.predict();
    //std::cout << "p is " << p << endl;
	m_age += 1;

	if (m_time_since_update > 0)
		m_hit_streak = 0;
	m_time_since_update += 1;

	StateType predictBox = get_rect_xysr(p.at<float>(0, 0), p.at<float>(1, 0), p.at<float>(2, 0), p.at<float>(3, 0));

	//预测结果历史记录
	m_history.push_back(predictBox);
	return m_history.back();
}


// Update the state vector with observed bounding box. flag false 只是为了修正窗口大小
void KalmanTracker::update(StateType stateMat, int detectID, bool flag)
{
	if (flag)
	{
		m_time_since_update = 0;
		m_history.clear();
		m_hits += 1;
		m_hit_streak += 1;
		//车辆检测结果历史记录,最多记录 real_history_length个 ==》 3个
		m_real_history.push_back(stateMat);
		if (m_real_history.size() > real_history_length)
			m_real_history.pop_front();
	}
	
	is_move = true;
	// measurement
	measurement.at<float>(0, 0) = stateMat.x + stateMat.width / 2;
	measurement.at<float>(1, 0) = stateMat.y + stateMat.height / 2;
	measurement.at<float>(2, 0) = stateMat.area();
	measurement.at<float>(3, 0) = stateMat.width / stateMat.height;

	this->detectID = detectID;
	// update
	kf.correct(measurement);
}


// Return the current state vector
StateType KalmanTracker::get_state(float &vx, float &vy)
{
	Mat s = kf.statePost;
	vx = s.at<float>(4, 0);
	vy = s.at<float>(5, 0);
	return get_rect_xysr(s.at<float>(0, 0), s.at<float>(1, 0), s.at<float>(2, 0), s.at<float>(3, 0));
}


// Convert bounding box from [cx,cy,s,r] to [x,y,w,h] style.
StateType KalmanTracker::get_rect_xysr(float cx, float cy, float s, float r)
{
	float w, h, x, y;
	if (s * r > 0)
	{
		w = sqrt(s * r);
		h = s / w;
		x = (cx - w / 2);
		y = (cy - h / 2);
	}
	else
	{
		x = cx-10;
		y = cy-10;
    w = 0;
    h = 0;
	}

	if (x < 0 && cx > 0)
		x = 0;
	if (y < 0 && cy > 0)
		y = 0;

	return StateType(x, y, w, h);
}

void KalmanTracker::set_real_car_flag()
{
	is_real_car = true;
}

bool KalmanTracker::get_real_car_flag()
{
	return is_real_car;
}
std::vector<StateType> KalmanTracker::get_history()
{
	return m_history;
}

std::list<StateType> KalmanTracker::get_real_history()
{
	return m_real_history;
}

void KalmanTracker::set_move(bool flag)
{
	is_move = flag;
}


bool KalmanTracker::get_move()
{
	return is_move;
}
/*
// --------------------------------------------------------------------
// Kalman Filter Demonstrating, a 2-d ball demo
// --------------------------------------------------------------------

const int winHeight = 600;
const int winWidth = 800;

Point mousePosition = Point(winWidth >> 1, winHeight >> 1);

// mouse event callback
void mouseEvent(int event, int x, int y, int flags, void *param)
{
	if (event == CV_EVENT_MOUSEMOVE) {
		mousePosition = Point(x, y);
	}
}

void TestKF();

void main()
{
	TestKF();
}


void TestKF()
{
	int stateNum = 4;
	int measureNum = 2;
	KalmanFilter kf = KalmanFilter(stateNum, measureNum, 0);

	// initialization
	Mat processNoise(stateNum, 1, CV_32F);
	Mat measurement = Mat::zeros(measureNum, 1, CV_32F);

	kf.transitionMatrix = *(Mat_<float>(stateNum, stateNum) <<
		1, 0, 1, 0,
		0, 1, 0, 1,
		0, 0, 1, 0,
		0, 0, 0, 1);

	setIdentity(kf.measurementMatrix);
	setIdentity(kf.processNoiseCov, Scalar::all(1e-2));
	setIdentity(kf.measurementNoiseCov, Scalar::all(1e-1));
	setIdentity(kf.errorCovPost, Scalar::all(1));

	randn(kf.statePost, Scalar::all(0), Scalar::all(winHeight));

	namedWindow("Kalman");
	setMouseCallback("Kalman", mouseEvent);
	Mat img(winHeight, winWidth, CV_8UC3);

	while (1)
	{
		// predict
		Mat prediction = kf.predict();
		Point predictPt = Point(prediction.at<float>(0, 0), prediction.at<float>(1, 0));

		// generate measurement
		Point statePt = mousePosition;
		measurement.at<float>(0, 0) = statePt.x;
		measurement.at<float>(1, 0) = statePt.y;

		// update
		kf.correct(measurement);

		// visualization
		img.setTo(Scalar(255, 255, 255));
		circle(img, predictPt, 8, CV_RGB(0, 255, 0), -1); // predicted point as green
		circle(img, statePt, 8, CV_RGB(255, 0, 0), -1); // current position as red

		imshow("Kalman", img);
		char code = (char)waitKey(100);
		if (code == 27 || code == 'q' || code == 'Q')
			break;
	}
	destroyWindow("Kalman");
}
*/
