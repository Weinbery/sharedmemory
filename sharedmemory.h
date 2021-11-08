/**********************************************************************
*
* Copyright (C)2015-2022 Weinbery Corporation. All rights reserved.
*
* @file sharedmemory.h
* @brief 
* @details 
* @author wd
* @version 1.0.0
* @date 2021-10-09 22:41:27.478
*
**********************************************************************/
#pragma once

#include <stdint.h>
#include <string>
#include <Windows.h>

class SharedMemory
{
public:
	SharedMemory();
	~SharedMemory();

	enum SyncType {
		kSync = 0,
		kAsync = 1,
	};

	int32_t Create(std::wstring name, int32_t size, SyncType type = kSync);

	int32_t Read(void* buffer, int32_t size);

	int32_t Write(const void* buffer, int32_t size);

private:
	int32_t size_;
	LPVOID lpBuffer_;
	//
	HANDLE read_mutex_;
	HANDLE write_mutex_;
	HANDLE read_event_;
	HANDLE write_event_;
	HANDLE shared_handle_;
	//
	SyncType sync_type_;
	std::wstring base_name_;
};
