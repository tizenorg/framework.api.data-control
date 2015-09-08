/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dlog.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <unistd.h>

#include <sys/smack.h>
#include <sys/types.h>
#include <fcntl.h>
#include "data_control_internal.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "CAPI_APPFW_DATA_CONTROL"

#define _LOGE(fmt, arg...) LOGE(fmt,##arg)
#define _LOGD(fmt, arg...) LOGD(fmt, ##arg)

static const char *data_control_error_to_string(data_control_error_e error)
{
	switch (error) {
		case DATA_CONTROL_ERROR_NONE:
			return "NONE";
		case DATA_CONTROL_ERROR_INVALID_PARAMETER:
			return "INVALID_PARAMETER";
		case DATA_CONTROL_ERROR_OUT_OF_MEMORY:
			return "OUT_OF_MEMORY";
		case DATA_CONTROL_ERROR_IO_ERROR:
			return "IO_ERROR";
		case DATA_CONTROL_ERROR_PERMISSION_DENIED:
			return "PERMISSION_DENIED";
		case DATA_CONTROL_ERROR_MAX_EXCEEDED:
			return "Too long argument";
		default:
			return "UNKNOWN";
	}
}

int convert_to_tizen_error(datacontrol_error_e error)
{
	switch (error) {
		case DATACONTROL_ERROR_NONE:
			return DATA_CONTROL_ERROR_NONE;
		case DATACONTROL_ERROR_INVALID_PARAMETER:
			return DATA_CONTROL_ERROR_INVALID_PARAMETER;
		case DATACONTROL_ERROR_OUT_OF_MEMORY:
			return DATA_CONTROL_ERROR_OUT_OF_MEMORY;
		case DATACONTROL_ERROR_IO_ERROR:
			return DATA_CONTROL_ERROR_IO_ERROR;
		case DATACONTROL_ERROR_PERMISSION_DENIED:
			return DATA_CONTROL_ERROR_PERMISSION_DENIED;
		case DATACONTROL_ERROR_MAX_EXCEEDED:
			return DATA_CONTROL_ERROR_MAX_EXCEEDED;
		default:
			return error;
	}
}

int data_control_error(data_control_error_e error,
		const char *function, const char *description)
{
	if (description) {
		_LOGE("[%s] %s(0x%08x) : %s", function,
				data_control_error_to_string(error), error,
				description);
	} else {
		_LOGE("[%s] %s(0x%08x)", function,
				data_control_error_to_string(error), error);
	}

	return error;
}


int check_privilege(privilege_type type)
{
	int fd = 0;
	int ret = 0;
	char subject_label[SMACK_LABEL_LEN + 1] = "";

	fd = open("/proc/self/attr/current", O_RDONLY);
	if (fd < 0) {
		_LOGE("open [%d] failed!", errno);
		return DATA_CONTROL_ERROR_IO_ERROR;
	}

	ret = read(fd, subject_label, SMACK_LABEL_LEN);
	if (ret < 0) {
		_LOGE("read [%d] failed!", errno);
		close(fd);
		return DATA_CONTROL_ERROR_IO_ERROR;
	}
	close(fd);

	_LOGD("subject_label : %s", subject_label);
	if (type == PRIVILEGE_DATA_SHARING) {
		ret = smack_have_access(subject_label,
				"security-server::api-open-for-privileged", "rw");
		if (ret == 1) {
			_LOGD("permission allowed");
			return DATA_CONTROL_ERROR_NONE;
		} else if (ret == -1) {
			_LOGE("smack_have_access() fail");
			return DATA_CONTROL_ERROR_IO_ERROR;
		} else if (ret == 0) {
			_LOGD("permission denied");
			return DATA_CONTROL_ERROR_PERMISSION_DENIED;
		}
	} else if (type == PRIVILEGE_APP_MANAGER_LAUNCH) {
		ret = smack_have_access(subject_label, "aul::launch", "x");
		if (ret == 1) {
			_LOGD("permission allowed");
			return DATA_CONTROL_ERROR_NONE;
		} else if (ret == -1) {
			_LOGE("smack_have_access() fail");
			return DATA_CONTROL_ERROR_IO_ERROR;
		} else if (ret == 0) {
			_LOGD("permission denied");
			return DATA_CONTROL_ERROR_PERMISSION_DENIED;
		}

		ret = smack_have_access(subject_label, "app-svc::db", "rwxa");
		if (ret == 1) {
			_LOGD("permission allowed");
			return DATA_CONTROL_ERROR_NONE;
		} else if (ret == -1) {
			_LOGE("smack_have_access() fail");
			return DATA_CONTROL_ERROR_IO_ERROR;
		} else if (ret == 0) {
			_LOGD("permission denied");
			return DATA_CONTROL_ERROR_PERMISSION_DENIED;
		}
	}

	return DATA_CONTROL_ERROR_IO_ERROR;
}
