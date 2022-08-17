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

	//��ȡ��
	double getElapsedTimeInSec()
	{
		return getElapsedTimeInMicroSec()*0.000001;
	}

	//��ȡ����
	double getElapsedTimeInMilliSec()
	{
		return getElapsedTimeInMicroSec()*0.001;
	}

	//��ȡ΢��
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

