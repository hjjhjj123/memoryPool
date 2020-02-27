/************************************************************************
����ʵ�ֵ���һ�ֹ涨���޵Ķ�̬�ڴ�أ��ڴ���е��ڴ��SIZE���ȡ��ڴ�س�ʼʱû���ڴ�飬�趨�ܴ�С���ޡ������ڴ��ʱ���ȴ���ȴ�����룬��ȴ��û��ʱ��ϵͳ���룬����ﵽ���޽�����ʧ�ܣ�����ɹ��Ժ���뵽�ڴ�ء��ͷŵ��ڴ���뵽��ȴ���У���ȴ���ڵ��ڴ棬�ڱ���㹻���Ժ��ͷŸ�ϵͳ��
�ļ� �� dlmempool.h
���� �� OpenSource
�汾 �� V1.0
���� �� 2020-02-27
/************************************************************************/
#ifndef __DYNAMIC_LIMITED_MEMORY_POOL_H__
#define __DYNAMIC_LIMITED_MEMORY_POOL_H__
#include <time.h>
#include <stdio.h>
#include <pthread.h>
typedef struct _HI_RUNTIME_MEM_S
{
	long long u32Size;
	void * u64PhyAddr;
	void * u64VirAddr;
}HI_RUNTIME_MEM_S;
typedef struct _DlBlock
{
	HI_RUNTIME_MEM_S meminfo;
	time_t tBorn;
	time_t tIdle;
	struct _DlBlock * next;
}DlBlock;

#define SIZE_1KB	(1024)
#define SIZE_1MB	(1048576)		//1024*1024
#define SIZE_1GB	(1073741824)	//1024*1024*1024
class CDLMemPool
{
public:
	CDLMemPool(size_t limit_size, int cooling_second, const char * name);
	~CDLMemPool();

	bool m_bExitThread;
	pthread_t m_cooling_thread;

	// �����ȴ���Ƿ�黹�ڴ��ϵͳ
	bool DoCoolingCheck();

	// ��ӡ�ڴ��ʹ�����
	void PrintUsage();

	// ���ڴ�ط����ڴ�
	void * DlMalloc(size_t size);

	// �ͷ��ڴ浽�ڴ��
	int DlFree(void * p);
private:
	void get_list_info(DlBlock *list, int & block_count, size_t & total_size);

private:
	char   m_mp_name[128];
	size_t m_limit_size;

	int m_cooling_second;
	DlBlock * m_idle_area;	// ��ȴ��
	DlBlock * m_busy_area;	// �ѷ���

	pthread_mutex_t m_sem_mutex;
};

#endif