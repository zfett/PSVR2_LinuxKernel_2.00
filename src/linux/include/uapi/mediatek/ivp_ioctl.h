/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: JB Tsai <jb.tsai@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/**
 * @file ivp_ioctl.h
 * Header of ivp_ioctl.c
 */

#ifndef __IVP_IOCTL_H__
#define __IVP_IOCTL_H__

/*---------------------------------------------------------------------------*/
/*  IOCTL Command                                                            */
/*---------------------------------------------------------------------------*/

/** @ingroup IP_group_ivp_external_def
 * @brief IVP ivoct definition
 * @{
 */
#define IVP_MAGICNO			'v'
#define IVP_IOCTL_GET_CORE_NUM		_IOWR(IVP_MAGICNO,  0, u32)
#define IVP_IOCTL_GET_CORE_INFO		_IOWR(IVP_MAGICNO,  1,\
					struct ivp_fw_info)
#define IVP_IOCTL_REQ_ENQUE		_IOWR(IVP_MAGICNO,  2,\
					struct ivp_cmd_enque)
#define IVP_IOCTL_REQ_DEQUE		_IOWR(IVP_MAGICNO,  3,\
					struct ivp_cmd_deque)
#define IVP_IOCTL_FLUSH_QUE		_IOW(IVP_MAGICNO,   4,\
					struct ivp_flush_que)
#define IVP_IOCTL_ALLOC_BUF		_IOWR(IVP_MAGICNO,  5,\
					struct ivp_alloc_buf)
#define IVP_IOCTL_MMAP_BUF		_IOWR(IVP_MAGICNO,  6,\
					struct ivp_mmap_buf)
#define IVP_IOCTL_EXPORT_BUF_TO_FD	_IOWR(IVP_MAGICNO,  7,\
					struct ivp_export_buf_to_fd)
#define IVP_IOCTL_IMPORT_FD_TO_BUF	_IOWR(IVP_MAGICNO,  8,\
					struct ivp_import_fd_to_buf)
#define IVP_IOCTL_FREE_BUF		_IOWR(IVP_MAGICNO,  9,\
					struct ivp_free_buf)
#define IVP_IOCTL_ENQUE_DMA_REQ		_IOWR(IVP_MAGICNO, 10,\
					struct ivp_dma_memcpy_buf)
#define IVP_IOCTL_DEQUE_DMA_REQ		_IOWR(IVP_MAGICNO, 11,\
					struct ivp_cmd_deque)
#define IVP_IOCTL_SYNC_CACHE		_IOWR(IVP_MAGICNO, 12,\
					struct ivp_sync_cache_buf)
#define IVP_IOCTL_KSERVICE_REQ		_IOW(IVP_MAGICNO, 13,\
					struct ivp_kservice_req)
#define IVP_IOCTL_KSERVICE_REQ_PERPARE	_IOWR(IVP_MAGICNO, 14,\
					struct ivp_cmd_enque)
#define IVP_IOCTL_KSERVICE_REQ_UNPERPARE _IOWR(IVP_MAGICNO, 15,\
					 struct ivp_cmd_deque)

#define IVP_IOCTL_KSERVICE_AUTO_TEST    _IOWR(IVP_MAGICNO, 16,\
					u32)

#define MTK_IVP_LOAD_FROM_KERNEL
#if defined(MTK_IVP_LOAD_FROM_KERNEL)
#define MAX_ALGO_NAME_SIZE 64
#endif


/**
 * @}
 */

/** @ingroup IP_group_ivp_external_enum
 * @brief Define the internal status of the request command. \n
 * value is from 0 to 5.
 */
enum ivp_req_status {
	/** 0: IVP_REQ_STATUS_ENQUEUE */
	IVP_REQ_STATUS_ENQUEUE,
	/** 1: IVP_REQ_STATUS_RUN */
	IVP_REQ_STATUS_RUN,
	/** 2: IVP_REQ_STATUS_DEQUEUE */
	IVP_REQ_STATUS_DEQUEUE,
	/** 3: IVP_REQ_STATUS_TIMEOUT */
	IVP_REQ_STATUS_TIMEOUT,
	/** 4: IVP_REQ_STATUS_INVALID */
	IVP_REQ_STATUS_INVALID,
	/** 5: IVP_REQ_STATUS_FLUSH */
	IVP_REQ_STATUS_FLUSH,
};

/** @ingroup IP_group_ivp_external_enum
 * @brief Define the command type for controlling IVP core.
 * value is from 0 to 7.
 */
enum ivp_command_type {
	/** 0: IVP_PROC_LOAD_ALGO */
	IVP_PROC_LOAD_ALGO,
	/** 1: IVP_PROC_EXEC_ALGO */
	IVP_PROC_EXEC_ALGO,
	/** 2: IVP_PROC_RESET */
	IVP_PROC_RESET,
	/** 3: IVP_PROC_UNLOAD_ALGO */
	IVP_PROC_UNLOAD_ALGO,
	/** 4: IVP_PROC_GET_TRAN_CONN */
	IVP_PROC_GET_TRAN_CONN,
	/** 5: IVP_PROC_SET_TRAN_INFO */
	IVP_PROC_SET_TRAN_INFO,
	/** 6: IVP_PROC_FREE_TRAN_CONN */
	IVP_PROC_FREE_TRAN_CONN,
	/** 7: IVP_PROC_MAX */
	IVP_PROC_MAX,
};

/** @ingroup IP_group_ivp_external_enum
 * @brief Define the IVP request transfer target type
 * value is from 0 to 2.
 */
enum ivp_transfer_target_type {
	/** 0: IVP_TRAN_TO_NULL */
	IVP_TRAN_TO_NULL,
	/** 1: IVP_TRAN_TO_MUXER */
	IVP_TRAN_TO_MUXER,
	/** 2: IVP_TRAN_TO_MAX */
	IVP_TRAN_TO_MAX,
};

/** @ingroup IP_group_ivp_external_enum
 * @brief Define the sync direction for buffer
 * value is from 0 to 1.
 */
enum ivp_sync_cache_direction {
	IVP_SYNC_TO_DEVICE,
	IVP_SYNC_TO_CPU,
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to specify the core and \n
 * get the IVP firmware information.
 */
struct ivp_fw_info {
	/** An input parameter to specify which core is being concerned. */
	u32 core;
	/** An output parameter to return \n
	 * major version number of the firmware.
	 */
	u32 ver_major;
	/** An output parameter to return \n
	 * minor version number of the firmware.
	 */
	u32 ver_minor;
	/** This is an output parameter to return the status of the IVP core. */
	u32 state;
};

/** @ingroup IP_group_ivp_external_struct
 * @brief structure for IVP header data
 */
struct ivp_header_data {
	/** IVP herader data */
	u32 data[1024];
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to specify the transfer information. \n
 * There is the transfer basic information.
 */
struct ivp_tran_info {
	/** Transport stream PID. */
	u32 ts_pid;
	/** Presentation timestamp. */
	u64 pts;
	/** Vision timestamp. */
	u64 vision_timestamp;
	/** The header buffer. */
	struct ivp_header_data *header_data;
	/** The header buffer size. */
	u32 header_size;
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to specify the load command. \n
 * This struct must be align struct ivp_cmd_enque. \n
 * This struct is designed to be used easily by user space applications.
 */
struct ivp_cmd_load {
	/** An output parameter to return the load command handle. */
	s32 cmd_handle;
	/** This is IVP_PROC_LOAD_ALGO. */
	u32 cmd_type;
	/** An input parameter to specify which core will be controlled. */
	u32 core;
	/** An input parameter to specify working duration in milisecond. */
	u32 duration;
	/** algo handle */
	s32 algo_hnd;
	/** An input to specify the algo flag. */
	u32 algo_flag;
	/** An input to specify the algo buffer address. */
	u32 algo_addr;
	/** An input to specify the algo buffer size. */
	u32 algo_size;
#if defined(MTK_IVP_LOAD_FROM_KERNEL)
	/** An input to specify the algo name. */
	char algo_name[MAX_ALGO_NAME_SIZE];
#endif
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to specify the exec command. \n
 * This struct must be align struct ivp_cmd_enque. \n
 * This struct is designed to be used easily by user space applications.
 */
struct ivp_cmd_exec {
	/** An output parameter to return the exec command handle. */
	s32 cmd_handle;
	/** This is IVP_PROC_EXEC_ALGO. */
	u32 cmd_type;
	/** An input parameter to specify which core will be controlled. */
	u32 core;
	/** An input parameter to specify working duration in milisecond. */
	u32 duration;
	/** An input to specify which algo will be executed. */
	s32 algo_hnd;
	/** No use. */
	u32 reserved;
	/** An input to specify the algo argument buffer address. */
	u32 argu_addr;
	/** An input to specify the algo argument buffer size. */
	u32 argu_size;
#if defined(MTK_IVP_LOAD_FROM_KERNEL)
	/** An input to specify the algo name. */
	char algo_name[MAX_ALGO_NAME_SIZE];
#endif
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to specify the reset command. \n
 * This struct must be align struct ivp_cmd_enque. \n
 * This struct is designed to be used easily by user space applications.
 */
struct ivp_cmd_reset {
	/** An output parameter to return the reset command handle. */
	s32 cmd_handle;
	/** This is IVP_PROC_RESET. */
	u32 cmd_type;
	/** An input parameter to specify which core will be controlled. */
	u32 core;
	/** An input parameter to specify working duration in milisecond. */
	u32 duration;
	/** No use. */
	s32 reserved0;
	/** An input parameter to specify the flag. */
	u32 flag;
	/** No use. */
	u32 reserved1;
	/** No use. */
	u32 reserved2;
#if defined(MTK_IVP_LOAD_FROM_KERNEL)
	/** An input to specify the algo name. */
	char algo_name[MAX_ALGO_NAME_SIZE];
#endif
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to specify the unload command. \n
 * This struct must be aligning struct ivp_cmd_enque. \n
 * This struct is designed to be used easily by user space applications.
 */
struct ivp_cmd_unload {
	/** An output parameter to return the unload command handle. */
	s32 cmd_handle;
	/** This is IVP_PROC_UNLOAD_ALGO. */
	u32 cmd_type;
	/** An input parameter to specify which core will be controlled. */
	u32 core;
	/** An input parameter to specify working duration in milisecond. */
	u32 duration;
	/** An input to specify which algo will be unload. */
	s32 algo_hnd;
	/** No use. */
	u32 reserved0;
	/** No use. */
	u32 reserved1;
	/** No use. */
	u32 reserved2;
#if defined(MTK_IVP_LOAD_FROM_KERNEL)
	/** An input to specify the algo name. */
	char algo_name[MAX_ALGO_NAME_SIZE];
#endif
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to specify \n
 * the get transfer connect handle command. \n
 * This struct must be align struct ivp_cmd_transfer_enque. \n
 * This struct is designed to be used easily by user space applications.
 */
struct ivp_cmd_get_tran_conn {
	/** An output parameter to return \n
	 * the get transfer connect command handle.
	 */
	s32 cmd_handle;
	/** This is IVP_PROC_GET_TRAN_CONN. */
	u32 cmd_type;
	/** An input parameter to specify which core will be controlled. */
	u32 core;
	/** No use. */
	s32 reserved0;
	/** An input parameter to specify the flag. */
	u32 target;
	/** No use. */
	u32 resource;
	/** No use. */
	u32 reserved1;
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to specify \n
 * the set transfer basic information with transfer connection handle. \n
 * This struct must be align struct ivp_cmd_transfer_enque. \n
 * This struct is designed to be used easily by user space applications.
 */
struct ivp_cmd_set_tran_info {
	/** An output parameter to return \n
	 * the set transfer basic information command handle.
	 */
	s32 cmd_handle;
	/** This is IVP_PROC_SET_TRAN_CONN. */
	u32 cmd_type;
	/** An input parameter to specify which core will be controlled. */
	u32 core;
	/** The transfer information memory. */
	s32 info_base;
	/** The transfer information size. */
	u32 info_size;
	/** The transfer connection handle. */
	u32 tran_hnd;
	/** No use. */
	u32 reserved0;
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to specify \n
 * the free transfer connect handle command. \n
 * This struct must be align struct ivp_cmd_transfer_enque. \n
 * This struct is designed to be used easily by user space applications.
 */
struct ivp_cmd_free_tran_conn {
	/** An output parameter to return \n
	 * the free transfer connect command handle.
	 */
	s32 cmd_handle;
	/** This is IVP_PROC_FREE_TRAN_CONN. */
	u32 cmd_type;
	/** An input parameter to specify which core will be controlled. */
	u32 core;
	/** No use. */
	s32 reserved0;
	/** No use. */
	u32 reserved1;
	/** The transfer connection handle. */
	u32 tran_hnd;
	/** No use. */
	u32 reserved2;
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to transfer request command to IVP kernel driver. \n
 * This is the unified interface for a variety of commands.
 */
struct ivp_cmd_enque {
	/** An output parameter to return the request command handle. */
	s32 cmd_handle;
	/** An input parameter to specify the request command type. */
	u32 cmd_type;
	/** An input parameter to specify which core will be controlled. */
	u32 core;
	/** An input parameter to specify working duration in milisecond. */
	u32 duration;
	/** An input parameter to specify which algo will be set. */
	s32 algo_hnd;
	/** An input parameter to specify some argument value. */
	u32 argv;
	/** An input parameter to specify the buffer address. */
	u32 addr_handle;
	/** An input parameter to specify the buffer size. */
	u32 size;
#if defined(MTK_IVP_LOAD_FROM_KERNEL)
	/** An input parameter to specify the algo name. */
	char algo_name[MAX_ALGO_NAME_SIZE];
#endif
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to query request status.
 */
struct ivp_cmd_deque {
	/** An input parameter to specify the request command handle. */
	s32 cmd_handle;
	/** An input parameter to specify which core will be controlled. */
	u32 core;
	/** An output parameter to return the request result of the IVP. */
	s32 cmd_result;
	/** An output parameter to return the request status. */
	u32 cmd_status;
	/** An output parameter to return the cycle of the request enque. */
	u32 submit_cyc;
	/** An output parameter to return the cycle of the request start. */
	u32 execute_start_cyc;
	/** An output parameter to return the cycle of the request end. */
	u32 execute_end_cyc;
	/** An output parameter to return the cpu time of the request enque. */
	s64 submit_nsec;
	/** An output parameter to return the cpu time of the request start. */
	s64 execute_start_nsec;
	/** An output parameter to return the cpu time of the request end. */
	s64 execute_end_nsec;
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to transfer \n
 * the transfer-request command to IVP kernel driver. \n
 * This is the unified interface for a variety of commands.
 */
struct ivp_cmd_transfer_enque {
	/** An output parameter to return the request command handle. */
	s32 cmd_handle;
	/** An input parameter to specify the request command type. */
	u32 cmd_type;
	/** An input parameter to specify which core will be controlled. */
	u32 core;
	/** The transfer information buffer. */
	s32 info_base;
	/** The transfer information buffer size. */
	u32 info_size;
	/** The transfer connection handle. */
	u32 tran_hnd;
	/** No use. */
	u32 reserved0;
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to flush queue list.
 */
struct ivp_flush_que {
	/** This is an input parameter to specify the core number. */
	u32 core;
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to specify the buffer size and create a buffer.
 */
struct ivp_alloc_buf {
	/** An output parameter to return the buffer handle. */
	s32 handle;
	/** An input parameter to specify the buffer size.  */
	u32 req_size;
	/** An output parameter to return the buffer iova address. */
	u32 iova_addr;
	/** An output parameter to return the buffer iova size. */
	u32 iova_size;
	/** An input parameter to specify if using cache. */
	u32 is_cached;
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to specify the buffer handle and \n
 * get the offset value for mmap() function.
 */
struct ivp_mmap_buf {
	/** An input parameter to specify the buffer handle. */
	s32 handle;
	/** An input parameter to specify the buffer size. */
	u32 size;
	/** An output parameter to return \n
	 * the offset value for mmap() function.
	 */
	u32 offset;
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to exports the specified buffer and \n
 * return dma fd for the other users.
 */
struct ivp_export_buf_to_fd {
	/** An input parameter to specify the buffer handle. */
	s32 handle;
	/** An output parameter to return the dma fd. */
	u32 dma_fd;
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to import the specified dma fd to \n
 * create a new buffer.
 */
struct ivp_import_fd_to_buf {
	/** An input parameter to specify dma fd. */
	u32 dma_fd;
	/** An output parameter to return the buffer handle. */
	s32 handle;
	/** An output parameter to specify the buffer size. */
	u32 size;
	/** An output parameter to specify the buffer iova address. */
	u32 iova_addr;
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to free the specified buffer.
 */
struct ivp_free_buf {
	/** An input parameter to specify the buffer handle. */
	s32 handle;
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to sync the specified buffer.
 */
struct ivp_sync_cache_buf {
	s32 handle;
	u32 direction;
};

/** @ingroup IP_group_ivp_external_enum
 * @brief Define the memory type for DMA. \n
 * value is from 0 to 1.
 */
enum ivp_dma_memtype {
	/** 0: MEM_SYSTEM */
	MEM_SYSTEM,
	/** 1: MEM_INTERNAL */
	MEM_INTERNAL,
};

/** @ingroup IP_group_ivp_external_struct
 * @brief Use this struct to copy the specified memory.
 */
struct ivp_dma_memcpy_buf {
	/** An output parameter to return the buffer handle. */
	s32 handle;
	/** An input parameter to specify the source handle. */
	s32 src_buf_handle;
	/** An input parameter to specify the destination handle. */
	s32 dst_buf_handle;
	/** An input parameter to specify the source memory type. */
	enum ivp_dma_memtype src_memtype;
	/** An input parameter to specify the destination memory type. */
	enum ivp_dma_memtype dst_memtype;

	/** An input parameter to specify the source buffer offset. */
	u64 src_buf_offset;
	/** An input parameter to specify the destination buffer offset. */
	u64 dst_buf_offset;
	/** An input parameter to specify which core will be controlled. */
	u32 core;
	/** An input parameter to specify the memory size. */
	u32 size;
};

struct ivp_kservice_req {
	u32 core;
	u32 lock;
};

#endif /* __IVP_IOCTL_H__ */
