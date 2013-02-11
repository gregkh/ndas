/*
 -------------------------------------------------------------------------
 Copyright (C) 2011, IOCELL Networks Corp. Plainsboro, NJ, USA.
 All rights reserved.

 LICENSE TERMS

 The free distribution and use of this software in both source and binary 
 form is allowed (with or without changes) provided that:

   1. distributions of this source code include the above copyright 
      notice, this list of conditions and the following disclaimer;

   2. distributions in binary form include the above copyright
      notice, this list of conditions and the following disclaimer
      in the documentation and/or other associated materials;

   3. the copyright holder's name is not used to endorse products 
      built using this software without specific written permission. 
      
 ALTERNATIVELY, provided that this notice is retained in full, this product
 may be distributed under the terms of the GNU General Public License (GPL v2),
 in which case the provisions of the GPL apply INSTEAD OF those given above.
 
 DISCLAIMER

 This software is provided 'as is' with no explcit or implied warranties
 in respect of any properties, including, but not limited to, correctness 
 and fitness for purpose.
 -------------------------------------------------------------------------
*/
#ifndef _NDASUSER_DEF_H_
#define _NDASUSER_DEF_H_

#include "../sal/sal.h" /* NDAS_SAL_API */

#define NDASUSER_API NDAS_CALL

// NDAS LU ( usually a disk ) length
#define NDAS_DISK_MODEL_NAME_LENGTH        40
#define NDAS_DISK_FMREV_LENGTH            8
#define NDAS_MAX_DEVICE_SERIAL_LENGTH        20

// NDAS device length
#define NDAS_MAX_NAME_LENGTH            128 // Maximum length of NDAS device name including null termination
#define NDAS_ID_LENGTH                    20    // Length of NDAS device ID excluding null termination
#define NDAS_KEY_LENGTH                    5    // Length of NDAS device key excluding null termination
#define NDAS_SERIAL_LENGTH                15   // Length of NDAS serial number excluding null termination
#define NDAS_SERIAL_EXTEND_LENGTH                NDAS_SERIAL_LENGTH   // Length of NDAS serial number in extended format excluding null termination
#define NDAS_SERIAL_SHORT_LENGTH                8   // Length of NDAS serial number in short format excluding null termination
#define NDAS_MAX_UNIT_DISK_PER_NDAS_DEVICE    2 // until next revision of library
#define NDAS_MAX_BIND_COUNT                8    // Upper count limit for aggregation and mirroring

#define NDAS_MAX_HOSTNAME_LENGTH        128 //  excluding null termination

#define NDAS_VERSION_UNKNOWN           -1
#define NDAS_VERSION_1_0                0
#define NDAS_VERSION_1_1                1
#define NDAS_VERSION_2_0                 2

#define NDAS_UNIT_TYPE_NOT_EXIST    0
#define NDAS_UNIT_TYPE_UNKNOWN        1
#define NDAS_UNIT_TYPE_HARD_DISK    2
#define NDAS_UNIT_TYPE_ATAPI        3

/* To do: change NDAS_DISK to NDAS_SLOT_TYPE_xxx */
#define NDAS_DISK_MODE_UNKNOWN          0
#define NDAS_DISK_MODE_SINGLE               1
#define NDAS_DISK_MODE_RAID0                2
#define NDAS_DISK_MODE_RAID1                3
#define NDAS_DISK_MODE_RAID4                4
#define NDAS_DISK_MODE_AGGREGATION  20
#define NDAS_DISK_MODE_MIRROR               21
#define NDAS_DISK_MODE_ATAPI                100
#define NDAS_DISK_MODE_MEDIAJUKE		104

#define NDAS_PROBE_REGISTERED        (1<<1)
#define NDAS_PROBE_UNREGISTERED        (1<<2)
#define NDAS_PROBE_ALL                (NDAS_PROBE_REGISTERED | NDAS_PROBE_UNREGISTERED)

#define NDAS_FIRST_SLOT_NR 1
#define NDAS_INVALID_SLOT ((NDAS_FIRST_SLOT_NR) -1)

#endif //_NDASUSER_DEF_H_
