/**********************************************************************
*
* Copyright (C)2015-2022 Weinbery Corporation. All rights reserved.
*
* @file sharedmemory.cpp
* @brief 
* @details 
* @author wd
* @version 1.0.0
* @date 2021-10-09 22:41:27.478
*
**********************************************************************/
#include "sharedmemory.h"

SharedMemory::SharedMemory()
{
	size_ = 0;
	sync_type_ = kSync;
	lpBuffer_ = nullptr;
	read_mutex_ = nullptr;
	write_mutex_ = nullptr;
	read_event_ = nullptr;
	write_event_ = nullptr;
	shared_handle_ = nullptr;
}

SharedMemory::~SharedMemory()
{
	UnmapViewOfFile(lpBuffer_);
	//
	CloseHandle(read_mutex_);
	CloseHandle(write_mutex_);
	CloseHandle(read_event_);
	CloseHandle(write_event_);
	CloseHandle(shared_handle_);
}

int32_t SharedMemory::Create(std::wstring name, int32_t size, SyncType type)
{
	sync_type_ = type;
	size_ = size;
	base_name_ = name;
	//
	shared_handle_ = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, base_name_.c_str());
	if (shared_handle_ == nullptr)
	{
		shared_handle_ = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size_, base_name_.c_str());
	}
	if (shared_handle_ == NULL)
	{
		return -1;
	}
	//
	lpBuffer_ = ::MapViewOfFile(shared_handle_, FILE_MAP_ALL_ACCESS, 0,	0, 0);
	if (lpBuffer_ == NULL)
		return -1;
	//
	read_mutex_ = OpenMutex(MUTEX_ALL_ACCESS, FALSE, (base_name_ + L"-rm").c_str());
	if (read_mutex_ == NULL)
		read_mutex_ = CreateMutex(NULL, FALSE, (base_name_ + L"-rm").c_str());
	//
	write_mutex_ = OpenMutex(MUTEX_ALL_ACCESS, FALSE, (base_name_ + L"-wm").c_str());
	if (write_mutex_ == NULL)
		write_mutex_ = CreateMutex(NULL, FALSE, (base_name_ + L"-wm").c_str());
	//
	read_event_ = OpenEvent(EVENT_ALL_ACCESS, FALSE, (base_name_ + L"-re").c_str());
	if (read_event_ == NULL)
		read_event_ = CreateEvent(NULL, FALSE, FALSE, (base_name_ + L"-re").c_str());
	//
	write_event_ = OpenEvent(EVENT_ALL_ACCESS, FALSE, (base_name_ + L"-we").c_str());
	if (write_event_ == NULL)
		write_event_ = CreateEvent(NULL, FALSE, FALSE, (base_name_ + L"-we").c_str());
	//
	return 0;
}

int32_t SharedMemory::Read(void* buffer, int32_t size)
{
	if (size > size_)
		return -1;
	//同步
	if (sync_type_ == SharedMemory::kSync)
	{
		WaitForSingleObject(read_event_, INFINITE);		// 触发->可读
		//
		WaitForSingleObject(read_mutex_, INFINITE);		// 使用互斥体加锁
		memcpy(buffer, lpBuffer_, size);
		ReleaseMutex(read_mutex_);						// 释放锁
		//
		SetEvent(write_event_);							// 通知Write可写（同步）
	}
	else // 非同步
	{
		WaitForSingleObject(read_mutex_, INFINITE);		// 使用互斥体加锁
		memcpy(buffer, lpBuffer_, size);
		ReleaseMutex(read_mutex_);						// 释放锁
	}				
	//
	return size;
}

int32_t SharedMemory::Write(const void* buffer, int32_t size)
{
	if (size > size_)
		return -1;
	//
	WaitForSingleObject(write_mutex_, INFINITE);	// 使用互斥体加锁
	memcpy(lpBuffer_, buffer, size);
	ReleaseMutex(write_mutex_);						// 释放锁
	// 同步机制
	if (sync_type_ == SharedMemory::kSync)
	{
		SetEvent(read_event_);							// 通知Read可读（同步）
		//
		WaitForSingleObject(write_event_, INFINITE);	// 触发->可写
	}
	//
	return size;
}
