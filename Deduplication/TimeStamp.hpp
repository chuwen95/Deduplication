#ifndef CellTimestamp_hpp
#define CellTimestamp_hpp

#include <chrono>

class CellTimestamp
{
public:
	CellTimestamp()
	{
	}
	~CellTimestamp()
	{

	}

	void update()
	{
		m_begin = std::chrono::high_resolution_clock::now();
	}

	//获取秒
	double getElapsedTimeInSec()
	{
		return getElapsedTimeInMicroSec()*0.000001;
	}

	//获取毫秒
	double getElapsedTimeInMilliSec()
	{
		return getElapsedTimeInMicroSec()*0.001;
	}

	//获取微秒
#ifdef _WIN32
	__int64 getElapsedTimeInMicroSec()
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_begin).count();
	}
#else
	__int64_t getElapsedTimeInMicroSec()
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_begin).count();
	}
#endif


private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_begin{ std::chrono::high_resolution_clock::now() };
};

#endif // CellTimestamp_hpp

