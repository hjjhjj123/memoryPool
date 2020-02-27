#include "dlmempool.h"
#include <unistd.h>
#include <stdio.h>
#include <cstring>
#include<stdlib.h>
static const char * PrintSize(unsigned long long sizeB)
{
	static char buffer[5][256] = {0};
	static int si = -1;
	si++;
	si %= 5;

	if(sizeB < 1024)
		sprintf(buffer[si], "%lld B", sizeB);
	else if(sizeB < 1024*1024)
		sprintf(buffer[si], "%.2f KB", sizeB/1024.0f);
	else if(sizeB < 1024*1024*1024)
		sprintf(buffer[si], "%.2f MB", sizeB/1024.0f/1024.0f);
	else
		sprintf(buffer[si], "%.2f GB", sizeB/1024.0f/1024.0f/1024.0f);

	return buffer[si];
}
void * DLMemPoolCoolingThread(void * p)
{
	if (p == NULL)
	{
		return NULL;
	}

	CDLMemPool * pool = (CDLMemPool *)p;

	int count = -1;
	while(pool->m_bExitThread == false)
	{
		sleep(1);
		bool bDel = pool->DoCoolingCheck();

		count++;
		if (count > 60)
		{
			count = 0;
			pool->PrintUsage();
		}
	}

	pthread_exit(p);
	return p;
}

CDLMemPool::CDLMemPool(size_t limit_size = (50*SIZE_1MB), int cooling_second = 300, const char * name = "com")
{
	strcpy(m_mp_name, name ? name:"unnamed");

	m_limit_size = limit_size;
	m_cooling_second = cooling_second;

	m_idle_area = NULL;
	m_busy_area = NULL;

	pthread_mutex_init(&m_sem_mutex, NULL);

	// ������ȴ�߳�
	m_bExitThread = false;
	pthread_create(&m_cooling_thread, NULL, DLMemPoolCoolingThread, this);
}

CDLMemPool::~CDLMemPool()
{
	m_bExitThread = true;
	pthread_join(m_cooling_thread, NULL);

	pthread_mutex_lock(&m_sem_mutex);
	// �ͷ�
	DlBlock * cur_block = m_idle_area;
	while (cur_block)
	{
		DlBlock * del_block = cur_block;
		cur_block = cur_block->next;
		free(del_block->meminfo.u64VirAddr);
		free(del_block);
		del_block = NULL;
	}
	
	// busy����ô�죿�Ȳ��ͷ�

	pthread_mutex_unlock(&m_sem_mutex);

	pthread_mutex_destroy(&m_sem_mutex);

}

bool CDLMemPool::DoCoolingCheck()
{
	time_t tNow = time(NULL);
	bool bDelOne = false;

	pthread_mutex_lock(&m_sem_mutex);
	
	DlBlock * cur_block = m_idle_area;
	DlBlock * pre_block = NULL;
	while (cur_block)
	{
		if (cur_block->tIdle > 0 && (tNow - cur_block->tIdle) > m_cooling_second)
		{
			// ��¼Ҫɾ���Ľڵ�
			DlBlock * del_block = cur_block;

			// ������һ���ڵ㣬 ��������ʹ��del_block
			cur_block = cur_block->next;

			// �ͷ��ڴ��mmz
			char born_time[32] = {0};
			char idle_time[32] = {0};
			/***printf("-[COOLING TO DIE] Born:%s LastIdle:%s Size:%s\t PhyAddr:%p VirAddr:%p",
				TimeFormatString(del_block->tBorn, born_time, sizeof(born_time), eYMDHMS1),
				TimeFormatString(del_block->tIdle, idle_time, sizeof(idle_time), eYMDHMS1),
				PrintSize(del_block->meminfo.u32Size),
				del_block->meminfo.u64PhyAddr, del_block->meminfo.u64VirAddr);***/
			//���ͷ�����洢�ռ�
			free(del_block->meminfo.u64VirAddr);

			// ������ȴ������ṹ��ɾ���ڵ�
			if (pre_block == NULL)
			{
				m_idle_area = del_block->next;// ɾ��ͷ���
			}
			else
			{
				pre_block->next = del_block->next;// ɾ���м�ڵ�
			}

			// �ͷŴ洢�ڵ���Դ
			free(del_block);
			del_block = NULL;

			bDelOne = true;
		}
		else
		{
			// ��¼ǰ�ڵ�
			pre_block = cur_block;
			cur_block = cur_block->next;
		}
	}

	pthread_mutex_unlock(&m_sem_mutex);

	return bDelOne;
}

void CDLMemPool::PrintUsage()
{
	pthread_mutex_lock(&m_sem_mutex);

	int busy_count = 0, idle_count;
	size_t busy_size = 0, idle_size = 0;
	get_list_info(m_busy_area, busy_count, busy_size);
	get_list_info(m_idle_area, idle_count, idle_size);

	if(1)
	{
		printf("======DYNAMIC LIMITED MEMORY POOL %s USEAGE======\n", m_mp_name);
		printf("�ڴ����ޣ�%s\n", PrintSize(m_limit_size));
		printf("��ǰ����:%d blocks, %s\n", busy_count+idle_count, PrintSize(busy_size+idle_size));
		printf("����ʹ��:%d blocks, %s\n", busy_count, PrintSize(busy_size));
		printf("����״̬:%d blocks, %s\n", idle_count, PrintSize(idle_size));
		printf("=================================================\n");
	}

	/***DlBlock * cur_block = m_idle_area;
	while (cur_block)
	{
		char born_time[32] = {0};
		char idle_time[32] = {0};
		printf("-[IDLE] Born:%s Idle:%s Size:%s\t  PhyAddr:%p VirAddr:%p", 
			TimeFormatString(cur_block->tBorn, born_time, sizeof(born_time), eYMDHMS1),
			TimeFormatString(cur_block->tIdle, idle_time, sizeof(idle_time), eYMDHMS1),
			PrintSize(cur_block->meminfo.u32Size),
			cur_block->meminfo.u64PhyAddr, cur_block->meminfo.u64VirAddr);
		cur_block = cur_block->next;
	}
	cur_block = m_busy_area;
	while (cur_block)
	{
		char born_time[32] = {0};
		//char idle_time[32] = {0};
		printf("-[BUSY] Born:%s Idle:%s Size:%s\t  PhyAddr:%p VirAddr:%p", 
			TimeFormatString(cur_block->tBorn, born_time, sizeof(born_time), eYMDHMS1),
			"-------------------",
			PrintSize(cur_block->meminfo.u32Size),
			cur_block->meminfo.u64PhyAddr, cur_block->meminfo.u64VirAddr);
		cur_block = cur_block->next;
	}***/
	
	pthread_mutex_unlock(&m_sem_mutex);
}

void * CDLMemPool::DlMalloc(size_t size)
{
	void * p = NULL;

	// �ȿ�����ȴ������û�д��ɵ�
	pthread_mutex_lock(&m_sem_mutex);

	DlBlock * cur_block = m_idle_area;
	DlBlock * pre_block = NULL;
	while (cur_block)
	{
		if (cur_block->meminfo.u32Size == size)
		{
			//OK,�����ð�
			p = (void *)(cur_block->meminfo.u64VirAddr);

			// �Ѹ��ڴ��ת�Ƶ�BUSY,��һ����IDLE��ɾ��
			if (pre_block == NULL)
			{
				m_idle_area = cur_block->next;// ɾ��ͷ���
			}
			else
			{
				pre_block->next = cur_block->next;// ɾ���м�ڵ�
			}

			// �Ѹ��ڴ��ת�Ƶ�BUSY,�ڶ�����ӵ�BUSYͷ��
			cur_block->next = m_busy_area;
			m_busy_area = cur_block;
			cur_block->tIdle = 0;

			break;
		}

		pre_block = cur_block;
		cur_block = cur_block->next;
	}

	pthread_mutex_unlock(&m_sem_mutex);

	if (p)
	{
		return p;
	}
	
	// �ж��Ƿ��ܹ�����
	pthread_mutex_lock(&m_sem_mutex);
	int busy_count = 0, idle_count;
	size_t busy_size = 0, idle_size = 0;
	get_list_info(m_busy_area, busy_count, busy_size);
	get_list_info(m_idle_area, idle_count, idle_size);
	pthread_mutex_unlock(&m_sem_mutex);

	if (busy_size + idle_size + size > m_limit_size)
	{
		//LOG_WARNS_FMT("�����ڴ�ش�С���ƣ��޷��������ڴ�飬 ����:%s ʹ��:%s ������:%s", 
		//	PrintSize(m_limit_size), PrintSize(busy_size+idle_size), PrintSize(size));

		return NULL;
	}

	// �������ڴ��ṹ��,��¼���ڴ����Ϣ
	cur_block = (DlBlock*)malloc(sizeof(DlBlock));
	if (cur_block == NULL)
	{
		printf("malloc DlBlock failed");
		return NULL;
	}
	cur_block->meminfo.u32Size = size;
	cur_block->meminfo.u64PhyAddr = NULL;
	cur_block->meminfo.u64VirAddr = NULL;
	cur_block->tBorn = time(NULL);
	cur_block->tIdle = 0;
	cur_block->next = NULL;

	//����洢�ռ�
	cur_block->meminfo.u64VirAddr = (void *)malloc(cur_block->meminfo.u32Size);
	if( cur_block->meminfo.u64VirAddr == NULL )
	{
		printf("malloc u64VirAddr failed");
		free(cur_block);
		return NULL;
	}
	// ���뵽
	p = (void *)(cur_block->meminfo.u64VirAddr);

	// �Ѹ��ڴ��ת�Ƶ�BUSY,
	pthread_mutex_lock(&m_sem_mutex);
	cur_block->next = m_busy_area;
	m_busy_area = cur_block;
	cur_block->tIdle = 0;
	pthread_mutex_unlock(&m_sem_mutex);
	
	return p;
}

int CDLMemPool::DlFree(void * p)
{
	int ret = -1;
	if (p == NULL)
	{
		return -1;
	}

	// �ȿ����ǲ���BUSY�еĽڵ�
	pthread_mutex_lock(&m_sem_mutex);

	DlBlock * cur_block = m_busy_area;
	DlBlock * pre_block = NULL;
	while (cur_block)
	{
		if ((void *)(cur_block->meminfo.u64VirAddr) == p)
		{
			// �Ѹ��ڴ��ת�Ƶ�IDLE,��һ����BUSY��ɾ��
			if (pre_block == NULL)
			{
				m_busy_area = cur_block->next;// ɾ��ͷ���
			}
			else
			{
				pre_block->next = cur_block->next;// ɾ���м�ڵ�
			}

			// �Ѹ��ڴ��ת�Ƶ�IDLE,�ڶ�����ӵ�IDLEͷ��
			cur_block->next = m_idle_area;
			m_idle_area = cur_block;
			cur_block->tIdle = time(NULL);

			ret = 0;
			break;
		}

		pre_block = cur_block;
		cur_block = cur_block->next;
	}

	pthread_mutex_unlock(&m_sem_mutex);

	return ret;
}

void CDLMemPool::get_list_info(DlBlock *list, int & block_count, size_t & total_size)
{
	block_count = 0;
	total_size = 0;
	DlBlock * cur_block = list;
	while (cur_block)
	{
		block_count++;
		total_size += cur_block->meminfo.u32Size;

		cur_block = cur_block->next;
	}
}
