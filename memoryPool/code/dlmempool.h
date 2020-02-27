/************************************************************************
本类实现的是一种规定上限的动态内存池，内存池中的内存块SIZE不等。内存池初始时没有内存块，设定总大小上限。申请内存块时首先从冷却池申请，冷却池没有时从系统申请，如果达到上限将申请失败，申请成功以后加入到内存池。释放的内存放入到冷却池中，冷却池内的内存，在变得足够冷以后释放给系统。
文件 ： dlmempool.h
作者 ： OpenSource
版本 ： V1.0
日期 ： 2020-02-27
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

	// 检查冷却池是否归还内存给系统
	bool DoCoolingCheck();

	// 打印内存池使用情况
	void PrintUsage();

	// 从内存池分配内存
	void * DlMalloc(size_t size);

	// 释放内存到内存池
	int DlFree(void * p);
private:
	void get_list_info(DlBlock *list, int & block_count, size_t & total_size);

private:
	char   m_mp_name[128];
	size_t m_limit_size;

	int m_cooling_second;
	DlBlock * m_idle_area;	// 冷却池
	DlBlock * m_busy_area;	// 已分配

	pthread_mutex_t m_sem_mutex;
};

#endif