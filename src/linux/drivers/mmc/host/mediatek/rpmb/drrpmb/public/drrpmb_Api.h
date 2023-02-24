/*
 * Copyright (c) 2013 TRUSTONIC LIMITED
 * All rights reserved
 *
 * The present software is the confidential and proprietary information of
 * TRUSTONIC LIMITED. You shall not disclose the present software and shall
 * use it only in accordance with the terms of the license agreement you
 * entered into with TRUSTONIC LIMITED. This software may be subject to
 * export or import laws in certain countries.
 */

/**
 * @file drrpmb_Api.h
 * The drrpmb_Api.h file contains DCI command definitions and data structures.
 */

#ifndef __DRRPMBAPI_H__
#define __DRRPMBAPI_H__

#include "dci.h"


/*
 * Command ID's
 */
/** @ingroup type_group_linux_eMMC_def
 * @brief eMMC Flush definations
 * @{
 */
/** DCI CMD for read data */
#define DCI_RPMB_CMD_READ_DATA      1
/** DCI CMD for get wcnt */
#define DCI_RPMB_CMD_GET_WCNT       2
/** DCI CMD for write data */
#define DCI_RPMB_CMD_WRITE_DATA     3
/**
 * @}
 */

/*... add more command ids when needed */

/**
 * command message.
 *
 * @param len Length of the data to process.
 * @param data Data to be processed
 */
struct cmd_t {
	struct dciCommandHeader_t  header;     /**< Command header */
	uint32_t            len;        /**< Length of data to process */
};

/**
 * Response structure
 */
struct rsp_t {
	struct dciResponseHeader_t header;     /**< Response header */
	uint32_t            len;
};


/*
 * Alternative access flow to improve performance. (this is customization)
 */
#define RPMB_MULTI_BLOCK_ACCESS 1

#if RPMB_MULTI_BLOCK_ACCESS
#define MAX_RPMB_TRANSFER_BLK (16)
#define MAX_RPMB_REQUEST_SIZE (512*MAX_RPMB_TRANSFER_BLK)
#else
#define MAX_RPMB_TRANSFER_BLK (1)
#define MAX_RPMB_REQUEST_SIZE (512*MAX_RPMB_TRANSFER_BLK)
#endif

struct rpmb_req_t {
	uint8_t frame[MAX_RPMB_REQUEST_SIZE];
	uint32_t frameLen;
	uint16_t type;
	uint16_t addr;
	uint16_t blks;
	uint16_t result;
};

/*
 * DCI message data.
 */
struct dciMessage_t {
	union {
		struct cmd_t     command;
		struct rsp_t     response;
	};

	struct rpmb_req_t    request;

};

/*
 * Driver UUID. Update accordingly after reserving UUID
 */
#define RPMB_UUID { { 7, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }

#endif
